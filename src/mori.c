/* MORI engine: own DSP, written from scratch by L'Cronx (inspired by, not derived from, any other product).
 * Chain: DRY + OVERLAY -> routing matrix -> DELAY / TEXTURE (order switch) -> mix -> REVERB -> FILTER -> MASTER FX -> MASTER.
 * 44.1 kHz, interleaved int16 stereo, up to 128 frames per call. No allocation in process(). */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params.h"
#include "mori_keys.h"
#include "engine.h"

#define SR 44100.0f
#define PI 3.14159265f
#define DLY_LEN (44100 * 4 + 8)        /* 4 s per side */
#define TEX_LEN (44100 * 8)            /* 8 s texture memory */
#define OVL_LEN (44100 * 20)           /* overlay loop up to 20 s */
#define NGR 16

typedef struct { float z1, z2; } svf_t;
typedef struct { float a; float z; } lp1_t;

typedef struct {
    float v[K_COUNT];
    float bpm;
    /* delay */
    float *dl[2]; int dw; float dtime[2]; float dfb_lp[2], dfb_hp[2];
    /* texture */
    float *tx[2]; int tw; float head_pos[8]; float head_lp[2], head_hp[2];
    struct { int on; float pos, inc, age, len, pan; } gr[NGR];
    uint32_t rng; float gr_acc; float bc_hold[2]; int bc_cnt; float rm_ph;
    float txfb[2];
    /* overlay */
    float *ov[2]; int ow; float ovl_prev;
    /* filter */
    svf_t f1[2], f2[2];
    /* reverb */
    float *rb[8]; int rbl[8], rbw[8]; float rb_lp[8]; float rb_mod[8];
    float rb_pre[2]; float rb_wet[2];
    float *sh; int shw; float shph;
    /* master fx */
    float mf_hp[2], mf_lp[2]; float dcx[2], dcy[2];
} mori_t;

static float frand(mori_t *m) { m->rng = m->rng * 1664525u + 1013904223u; return (m->rng >> 8) * (1.0f / 16777216.0f); }
static float clampf(float x, float a, float b) { return x < a ? a : x > b ? b : x; }
static float lerp_buf(const float *b, int len, float pos) {
    while (pos < 0) pos += len;
    int i = (int)pos; float f = pos - i;
    int i0 = i % len, i1 = (i + 1) % len;
    return b[i0] + (b[i1] - b[i0]) * f;
}

static const float DIVB[22] = { 8, 4, 2, 1, .5f, .25f, .125f, .0625f,
    6, 3, 1.5f, .75f, .375f, .1875f, .09375f,
    8.f / 3, 4.f / 3, 2.f / 3, 1.f / 3, 1.f / 6, 1.f / 12, 1.f / 24 };

static void *eng_create(const char *dir) {
    (void)dir;
    mori_t *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    for (int i = 0; i < K_COUNT; i++) m->v[i] = KDEF[i];
    m->bpm = 120; m->rng = 12345u;
    for (int c = 0; c < 2; c++) {
        m->dl[c] = calloc(DLY_LEN, sizeof(float)); m->tx[c] = calloc(TEX_LEN, sizeof(float)); m->ov[c] = calloc(OVL_LEN, sizeof(float));
    }
    static const int base[8] = { 1117, 1303, 1523, 1747, 1949, 2161, 2377, 2593 };
    for (int i = 0; i < 8; i++) { m->rbl[i] = base[i] * 4; m->rb[i] = calloc(m->rbl[i] + 4, sizeof(float)); }
    m->sh = calloc(8192, sizeof(float));
    for (int i = 0; i < 8; i++) m->head_pos[i] = i * (TEX_LEN / 8.0f);
    return m;
}
static void eng_destroy(void *p) {
    mori_t *m = p; if (!m) return;
    for (int c = 0; c < 2; c++) { free(m->dl[c]); free(m->tx[c]); free(m->ov[c]); }
    for (int i = 0; i < 8; i++) free(m->rb[i]);
    free(m->sh); free(m);
}
static void eng_midi(void *p, const uint8_t *msg, int len) { (void)p; (void)msg; (void)len; }

static void eng_set(void *p, const char *key, const char *val) {
    mori_t *m = p; float x = (float)atof(val);
    if (!strcmp(key, "lfo_bpm")) { if (x > 20 && x < 400) m->bpm = x; return; }
    for (int i = 0; i < K_COUNT; i++) if (!strcmp(key, KEYS[i])) {
        m->v[i] = x;
        if (i == K_OV_CLEAR && x > 0.5f) { for (int c = 0; c < 2; c++) memset(m->ov[c], 0, OVL_LEN * sizeof(float)); }
        return;
    }
}
static int eng_get(void *p, const char *key, char *buf, int n) {
    mori_t *m = p;
    for (int i = 0; i < K_COUNT; i++) if (!strcmp(key, KEYS[i])) { snprintf(buf, n, "%g", m->v[i]); return (int)strlen(buf); }
    return 0;
}

/* state variable filter, one pole pair, mode 0 LP, 1 HP, 2 BP */
static float svf(svf_t *s, float x, float g, float k, int mode) {
    float hp = (x - (k + g) * s->z1 - s->z2) / (1 + g * (k + g));
    float bp = g * hp + s->z1; s->z1 = g * hp + bp;
    float lp = g * bp + s->z2; s->z2 = g * bp + lp;
    return mode == 0 ? lp : mode == 1 ? hp : bp;
}
static float svf_g(float fc) { return tanf(PI * clampf(fc, 15.f, 18000.f) / SR); }
static float cut_to_hz(float t) { return 20.f * powf(1000.f, t); }   /* t 0..1 -> 20 Hz..20 kHz */

static void reverb_params(mori_t *m, float *decay, float *size, float *damp) {
    static const float TD[5] = { .86f, .93f, .78f, .84f, .97f };
    static const float TS[5] = { 1.f, 1.6f, .55f, .8f, 2.2f };
    int t = (int)m->v[K_RV_TYPE]; if (t < 0) t = 0; if (t > 4) t = 4;
    *decay = TD[t] + (m->v[K_RV_DIM] / 100.f - .5f) * .12f;
    *size = TS[t] * (.5f + m->v[K_RV_DIM] / 100.f);
    *damp = .15f + .8f * m->v[K_RV_DAMP] / 100.f;
    if (*decay > .985f) *decay = .985f;
}

static void eng_process(void *p, const int16_t *in, int16_t *out, int frames) {
    mori_t *m = p; float *v = m->v;
    float lvl_dry = v[K_DRY] / 100.f, lvl_ovl = v[K_OVL] / 100.f, lvl_dly = v[K_DLY] / 100.f, lvl_txt = v[K_TXT] / 100.f;
    float master = v[K_MASTER] / 100.f; master = master * master * 1.4f;
    int order = (int)v[K_ORDER];
    /* delay times in samples */
    float spb = 60.f / m->bpm * SR;
    float tl = clampf(DIVB[(int)v[K_DL_L] % 22] * spb, 64, DLY_LEN - 16);
    float tr = clampf(DIVB[(int)v[K_DL_R] % 22] * spb * (1.f + v[K_DL_OFF] / 8.f), 64, DLY_LEN - 16);
    float tgt[2] = { tl, tr };
    float glide = 1.f / (SR * (0.02f + v[K_DL_TAPE] / 100.f * 0.8f));
    float fb = v[K_DL_FB] / 100.f * .93f;
    float fbf = v[K_DL_FLTFB] / 100.f;
    float fbg_lp = 1.f - expf(-2 * PI * (fbf < 0 ? 200.f * powf(40.f, 1.f + fbf) : 18000.f) / SR);
    float fbg_hp = 1.f - expf(-2 * PI * (fbf > 0 ? 20.f * powf(150.f, fbf) : 10.f) / SR);
    /* texture */
    int heads = (int)clampf(v[K_TX_HEADS], 2, 8), mode = (int)v[K_TX_MODE];
    int txlen = (int)clampf(v[K_TX_SIZE] * SR, 4410, TEX_LEN);
    static const float MIXSP[8] = { 1, .5f, 2, .25f, 1, 4, .5f, 2 };
    float txf = v[K_TX_FLT] / 100.f;
    float tx_hp_g = 1.f - expf(-2 * PI * (txf > 0 ? 40.f * powf(100.f, txf) : 10.f) / SR);
    float tx_lp_g = 1.f - expf(-2 * PI * (txf < 0 ? 300.f * powf(60.f, 1.f + txf) : 18000.f) / SR);
    float fxa = v[K_TX_FXA] / 100.f; int fxt = (int)v[K_TX_FX];
    float boost = 1.f + v[K_TX_BOOST] / 100.f * 2.f;
    int blocked = v[K_TX_BLOCK] > .5f;
    /* overlay */
    int ovlen = (int)clampf(v[K_OV_SIZE] * spb, 4410, OVL_LEN); int ovmode = (int)v[K_OV_REC];
    /* filter */
    float fc = v[K_FLT_CUT] / 100.f; int fon = fabsf(fc) > .02f || (int)v[K_FLT_TYPE] == 1;
    int fbp = (int)v[K_FLT_TYPE] == 1; int f24 = (int)v[K_FLT_SLOPE] == 1;
    float fk = 2.f - 1.9f * v[K_FLT_RES] / 100.f;
    float fg_lp = svf_g(cut_to_hz(fc < 0 ? 1.f + fc : 1.f)), fg_hp = svf_g(cut_to_hz(fc > 0 ? fc : 0.f));
    if (fbp) { float t = (fc + 1.f) * .5f; fg_lp = fg_hp = svf_g(cut_to_hz(t)); }
    /* reverb */
    float rdec, rsize, rdamp; reverb_params(m, &rdec, &rsize, &rdamp);
    float ramt = v[K_RV_AMT] / 100.f, shim = v[K_RV_SHIM] / 100.f;
    float pre_g = 1.f - expf(-2 * PI * (30.f + 600.f * v[K_RV_PRE] / 100.f) / SR); int pre_on = v[K_RV_PRE] > 1;
    /* master fx */
    int mft = (int)v[K_MF_TYPE]; float mfa = v[K_MF_AMT] / 100.f;
    float mf_g = 1.f - expf(-2 * PI * 3000.f / SR);
    float rev_rt[8];
    for (int i = 0; i < 8; i++) rev_rt[i] = clampf(m->rbl[i] * rsize / 4.f * 1.f, 64, (float)m->rbl[i] - 4);

    for (int n = 0; n < frames; n++) {
        float x[2] = { in[2 * n] / 32768.f, in[2 * n + 1] / 32768.f };
        float ovo[2], dlo[2] = { 0, 0 }, txo[2] = { 0, 0 };
        /* overlay loop: read, then write by mode */
        for (int c = 0; c < 2; c++) {
            float o = m->ov[c][m->ow];
            ovo[c] = o;
            if (ovmode == 1) m->ov[c][m->ow] = o * .995f + x[c] * .7f;
            else if (ovmode == 2) m->ov[c][m->ow] = x[c];
            else if (ovmode == 3) m->ov[c][m->ow] = 0;
        }
        if (++m->ow >= ovlen) m->ow = 0;
        float dly_in[2], tex_in[2];
        for (int pass = 0; pass < 2; pass++) {
            int do_delay = (order == 0) ? pass == 0 : pass == 1;
            if (do_delay) {
                for (int c = 0; c < 2; c++) {
                    dly_in[c] = x[c] * (v[K_R_DD] > .5f) + ovo[c] * (v[K_R_OD] > .5f) + (order == 1 ? txo[c] * (v[K_R_TD] > .5f) : 0);
                    if (m->dtime[c] < 1.f || (int)v[K_DL_MODE] == 1) m->dtime[c] = tgt[c];
                    else m->dtime[c] += (tgt[c] - m->dtime[c]) * glide;
                    float rd = (float)m->dw - m->dtime[c]; if (rd < 0) rd += DLY_LEN;
                    float d = lerp_buf(m->dl[c], DLY_LEN, rd);
                    dlo[c] = d;
                    float f = d;
                    m->dfb_lp[c] += fbg_lp * (f - m->dfb_lp[c]); f = m->dfb_lp[c];
                    m->dfb_hp[c] += fbg_hp * (f - m->dfb_hp[c]); f -= m->dfb_hp[c];
                    float w = dly_in[c] + tanhf(f * fb);
                    m->dl[c][m->dw] = w;
                }
                m->dw = (m->dw + 1) % DLY_LEN;
            } else {
                for (int c = 0; c < 2; c++)
                    tex_in[c] = x[c] * (v[K_R_DT] > .5f) + ovo[c] * (v[K_R_OT] > .5f) + (order == 0 ? dlo[c] * (v[K_R_LDT] > .5f) : 0);
                /* write */
                if (!blocked) for (int c = 0; c < 2; c++) m->tx[c][m->tw] = tex_in[c] + m->txfb[c] * v[K_FR_FB] / 100.f * .8f;
                int wpos = m->tw;
                if ((int)v[K_TX_ENG] == 0) {
                    float g = 1.f / sqrtf((float)heads) * 1.6f;
                    for (int h = 0; h < heads; h++) {
                        float sp;
                        switch (mode) {
                        case 1: sp = MIXSP[h]; break;
                        case 2: sp = -MIXSP[h]; break;
                        case 3: sp = (h & 1) ? -MIXSP[h] : MIXSP[h]; break;
                        case 4: sp = (h & 1) ? .5f : .25f; break;
                        case 5: sp = (h & 1) ? 4.f : 2.f; break;
                        default: sp = MIXSP[h]; if (heads == 8 || (h % 3) == 2) sp = (h & 1) ? -sp : sp; break;
                        }
                        m->head_pos[h] += sp - 1.f;       /* relative to the write head */
                        while (m->head_pos[h] < 0) m->head_pos[h] += txlen;
                        while (m->head_pos[h] >= txlen) m->head_pos[h] -= txlen;
                        float rp = (float)wpos - m->head_pos[h]; while (rp < 0) rp += txlen;
                        for (int c = 0; c < 2; c++) {
                            float pan = ((h + c) & 1) ? 1.f : .45f;
                            txo[c] += lerp_buf(m->tx[c], txlen, rp) * g * pan;
                        }
                    }
                } else {
                    /* fragment: spawn grains */
                    float dens = v[K_FR_DENS] / 100.f;
                    float gsz = v[K_FR_SIZE] * SR / 1000.f;
                    m->gr_acc += dens * 40.f / SR * (500.f / (v[K_FR_SIZE] + 20.f));
                    if (m->gr_acc >= 1.f) {
                        m->gr_acc -= 1.f;
                        for (int g = 0; g < NGR; g++) if (!m->gr[g].on) {
                            float st = v[K_FR_PITCH] + (frand(m) * 2 - 1) * v[K_FR_PRAND];
                            m->gr[g].on = 1; m->gr[g].age = 0; m->gr[g].len = gsz;
                            m->gr[g].inc = powf(2.f, st / 12.f) * (frand(m) < v[K_FR_REV] / 100.f ? -1.f : 1.f);
                            float off = frand(m) * v[K_FR_SPREAD] / 100.f * txlen;
                            m->gr[g].pos = (float)wpos - off - gsz * .5f; m->gr[g].pan = frand(m);
                            break;
                        }
                    }
                    for (int g = 0; g < NGR; g++) if (m->gr[g].on) {
                        float ph = m->gr[g].age / m->gr[g].len;
                        float w = .5f - .5f * cosf(2 * PI * ph);
                        float rp = m->gr[g].pos; while (rp < 0) rp += txlen; while (rp >= txlen) rp -= txlen;
                        txo[0] += lerp_buf(m->tx[0], txlen, rp) * w * (1.f - m->gr[g].pan * .6f) * .5f;
                        txo[1] += lerp_buf(m->tx[1], txlen, rp) * w * (.4f + m->gr[g].pan * .6f) * .5f;
                        m->gr[g].pos += m->gr[g].inc; m->gr[g].age += 1;
                        if (m->gr[g].age >= m->gr[g].len) m->gr[g].on = 0;
                    }
                }
                if (++m->tw >= txlen) m->tw = 0;
                /* texture filter + fx + boost */
                for (int c = 0; c < 2; c++) {
                    float s = txo[c];
                    m->head_lp[c] += tx_lp_g * (s - m->head_lp[c]); s = m->head_lp[c];
                    m->head_hp[c] += tx_hp_g * (s - m->head_hp[c]); s -= m->head_hp[c];
                    if (fxt == 1) s = (1.f - fxa) * s + fxa * sinf(s * (1.f + 5.f * fxa));
                    else if (fxt == 2) { float q = powf(2.f, 14.f - 11.f * fxa); s = (1.f - fxa * .8f) * s + fxa * .8f * floorf(s * q + .5f) / q; }
                    else if (fxt == 3) { s = (1.f - fxa) * s + fxa * s * sinf(m->rm_ph); }
                    txo[c] = s * boost; m->txfb[c] = txo[c];
                }
                m->rm_ph += 2 * PI * 150.f / SR; if (m->rm_ph > 2 * PI) m->rm_ph -= 2 * PI;
            }
        }
        /* mix and reverb send */
        float mix[2], rsend[2];
        for (int c = 0; c < 2; c++) {
            mix[c] = x[c] * lvl_dry * (v[K_R_DO] > .5f) + ovo[c] * lvl_ovl * (v[K_R_OO] > .5f) + dlo[c] * lvl_dly * (v[K_R_LDO] > .5f) + txo[c] * lvl_txt * (v[K_R_TO] > .5f);
            rsend[c] = x[c] * lvl_dry * (v[K_R_DR] > .5f) + ovo[c] * lvl_ovl * (v[K_R_OR] > .5f) + dlo[c] * lvl_dly * (v[K_R_LDR] > .5f) + txo[c] * lvl_txt * (v[K_R_TR] > .5f);
            if (pre_on) { m->rb_pre[c] += pre_g * (rsend[c] - m->rb_pre[c]); rsend[c] -= m->rb_pre[c]; }
        }
        /* 8-line FDN reverb */
        float rin = (rsend[0] + rsend[1]) * .5f, ro[8], wet[2] = { 0, 0 };
        for (int i = 0; i < 8; i++) {
            float rp = (float)m->rbw[i] - rev_rt[i]; if (rp < 0) rp += m->rbl[i];
            ro[i] = lerp_buf(m->rb[i], m->rbl[i], rp);
        }
        float sum = 0; for (int i = 0; i < 8; i++) sum += ro[i];
        sum *= .25f;
        /* shimmer: octave-up two-tap shifter on the tail */
        float shv = 0;
        if (shim > 0.001f) {
            float lin = (wet[0] + sum) * .5f; (void)lin;
            m->sh[m->shw] = sum;
            float d1 = 1.f - fmodf(m->shph, 1.f), d2 = 1.f - fmodf(m->shph + .5f, 1.f);
            float t1 = (float)m->shw - d1 * 2048.f, t2 = (float)m->shw - d2 * 2048.f;
            float s1 = lerp_buf(m->sh, 8192, t1), s2 = lerp_buf(m->sh, 8192, t2);
            float w1 = .5f - .5f * cosf(2 * PI * d1), w2 = .5f - .5f * cosf(2 * PI * d2);
            shv = s1 * w1 + s2 * w2;
            m->shph += 1.f / 2048.f; if (m->shph >= 1000.f) m->shph -= 1000.f;   /* octave up: delay shrinks 1 sample per sample */
            m->shw = (m->shw + 1) % 8192;
        }
        for (int i = 0; i < 8; i++) {
            float fbv = ro[i] - sum;
            m->rb_lp[i] += (1.f - rdamp) * (fbv - m->rb_lp[i]);
            float w = rin * .35f + m->rb_lp[i] * rdec + shv * shim * .5f * (i & 1);
            m->rb[i][m->rbw[i]] = tanhf(w);
            if (++m->rbw[i] >= m->rbl[i]) m->rbw[i] = 0;
            wet[i & 1] += ro[i];
        }
        wet[0] *= .35f; wet[1] *= .35f;
        float o[2];
        for (int c = 0; c < 2; c++) {
            float s = mix[c] + wet[c] * ramt * 1.6f + shv * shim * ramt * .6f;
            /* filter */
            if (fon) {
                float t = s;
                if (fbp) { t = svf(&m->f1[c], t, fg_lp, fk, 2); if (f24) t = svf(&m->f2[c], t, fg_lp, fk, 2); }
                else {
                    if (fc < -.02f) { t = svf(&m->f1[c], t, fg_lp, fk, 0); if (f24) t = svf(&m->f2[c], t, fg_lp, fk, 0); }
                    else if (fc > .02f) { t = svf(&m->f1[c], t, fg_hp, fk, 1); if (f24) t = svf(&m->f2[c], t, fg_hp, fk, 1); }
                }
                s = t;
            }
            /* master fx */
            if (mft == 1) { m->mf_hp[c] += mf_g * (s - m->mf_hp[c]); float hf = s - m->mf_hp[c]; s += tanhf(hf * 4.f) * .5f * mfa; }
            else if (mft == 2) { s = (1.f - mfa) * s + mfa * tanhf(s * (1.f + 2.5f * mfa)) / tanhf(1.f + 2.5f * mfa) * 1.0f; m->mf_lp[c] += (1.f - mfa * .4f) * (s - m->mf_lp[c]); s = m->mf_lp[c]; }
            else if (mft == 3) { float y = s * (1.f + 3.f * mfa) + .2f * mfa; y = y > 0 ? tanhf(y) : 1.2f * tanhf(y * .8f); y -= .2f * mfa * .6f; s = (1.f - mfa) * s + mfa * y; }
            /* DC block + master + soft limit */
            float y = s - m->dcx[c] + .995f * m->dcy[c]; m->dcx[c] = s; m->dcy[c] = y;
            o[c] = tanhf(y * master);
        }
        for (int c = 0; c < 2; c++) {
            float q = o[c] * 32767.f; q = q > 32767.f ? 32767.f : q < -32768.f ? -32768.f : q;
            out[2 * n + c] = (int16_t)lrintf(q);
        }
    }
}

static void eng_render(void *p, int16_t *out, int frames) { (void)p; memset(out, 0, frames * 4); }
static const mpc_engine_t E = { eng_create, eng_destroy, eng_midi, eng_set, eng_get, eng_render, eng_process, NULL };
const mpc_engine_t *mpc_engine(void) { return &E; }
