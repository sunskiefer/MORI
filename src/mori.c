#define _GNU_SOURCE
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
#include "mori_presets.h"
#include "mori_web_data.h"
#include "xyz_pad.h"
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SR 44100.0f
#define PI 3.14159265f
#define DLY_LEN (44100 * 4 + 8)        /* 4 s per side */
#define TEX_LEN (44100 * 8)            /* 8 s texture memory */
#define OVL_LEN (44100 * 20)           /* overlay loop up to 20 s */
#define NGR 16
#define NPRESET_MAX 1024                /* factory + user; the PRESET parameter runs 0..1023 */
#define NUSER_MAX (NPRESET_MAX - NFACTORY)
#define NAMELEN 24

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
    /* presets: factory bank + user presets kept in a file next to the plugin (no small slot cap) */
    int sel, nuser, blocks; char path[512];
    /* host position and tempo (song_pos), step modulators, gate, vinyl, freeze, spin */
    double ppq, hbpm; int playing;
    struct { double phase; float env, hold; int stage, last_on; } sq[2];
    float g_env, g_gain, vspd, vpos, sp_ph; int vinit, frz, was_blk;
    svf_t fi1[2], fi2[2]; float mi_hp[2], mi_lp[2];
    float vm[K_COUNT];
    /* touch bar (the pad thread writes tx_, ty_), zones */
    volatile float tx_, ty_; volatile unsigned tch_changed; unsigned tch_rep; overlay_t pad; int pad_on;
    float tzv[3], tlx, tly;
    int g_holdc, ov_any; float mm_hp[2], mm_lp[2]; svf_t fm1[2], fm2[2], fr1[2], fr2[2];
    /* pattern slots and the editor's web server */
    float pat[8][32]; int pat_used[8]; char pat_path[512];
    int srv_fd, srv_port, srv_on; volatile int srv_stop; pthread_t srv_th; unsigned rev; char addr[64];
    pthread_mutex_t plock;
    char (*uname)[NAMELEN + 1]; float (*uval)[NFV];
    unsigned char pend[K_COUNT]; int pend_scan;
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


static const float SQ_BEATS[9] = { 4.f, 2.f, 1.f, .5f, .25f, .125f, 2.f / 3, 1.f / 3, 1.f / 6 };
static const int MODD[24] = { -1, K_DRY, K_OVL, K_DLY, K_TXT, K_DL_FB, K_DL_FLTFB, K_TX_FLT, K_TX_FXA, K_TX_BOOST, K_TX_SPEED, K_FR_DENS,
    K_FR_PITCH, K_FR_SIZE, K_FR_SPREAD, K_FLT_CUT, K_FLT_RES, K_RV_AMT, K_RV_DIM, K_RV_SHIM, K_MF_AMT, K_SP_SPEED, K_SP_DIST, K_MASTER };

static const float SQ_SPD[7] = { .25f, .5f, 1.f, 2.f, 4.f, 8.f, 16.f };
/* moves one destination: gate = 1 treats the four levels and MASTER as a gate (the step modulators), 0 adds to every one (touch) */
static void mod_apply(mori_t *m, int dest, float a, float sv, int gate) {
    if (dest <= 0 || dest >= 24) return;
    int k = MODD[dest]; float rng = KMAXV[k] - KMINV[k], base = m->vm[k], r;
    if (gate && (dest <= 4 || dest == 23)) { float fa = fabsf(a); r = base * (1.f - fa * (a > 0 ? 1.f - sv : sv)); }
    else r = base + a * sv * rng;
    m->vm[k] = clampf(r, KMINV[k], KMAXV[k]);
}
/* one step modulator: 16 bars, a gate type per step (or the sequence's), an ADSR, two destinations */
static void seq_tick(mori_t *m, int s, int frames, float spb) {
    const float *v = m->v; int b = s ? K_SQ2_DEST : K_SQ1_DEST, xb = s ? K_SQ2_SPD : K_SQ1_SPD, tb = s ? K_T2_1 : K_T1_1;
    int len = (int)clampf(v[b + 3], 1, 16), di = (int)clampf(v[b + 2], 0, 8);
    float sb = SQ_BEATS[di] / SQ_SPD[(int)clampf(v[xb], 0, 6)];
    double stepf;
    if (m->ppq >= 0 && m->playing) stepf = m->ppq / sb;
    else { m->sq[s].phase += (double)frames / ((double)spb * sb); stepf = m->sq[s].phase; }
    double fl = floor(stepf); int idx = (int)fmod(fl, (double)len); if (idx < 0) idx += len;
    float frac = (float)(stepf - fl);
    float val = v[b + 9 + idx] / 100.f;
    int tt = (int)(v[tb + idx] + .5f), gt = (int)v[b + 4];            /* TYPE: SEQ (the sequence's GATE), OFF, SHT, LNG, LNK */
    float gl = tt == 1 ? 0.f : tt == 2 ? .25f : tt == 3 ? .75f : tt == 4 ? 1.f : (gt == 0 ? 1.f : gt == 1 ? .75f : .25f);
    int gate = val > .001f && frac < gl;
    float dt = frames / SR;
    float at = .001f + powf(v[b + 5] / 100.f, 2) * 1.5f, dc = .001f + powf(v[b + 6] / 100.f, 2) * 1.5f, rl = .001f + powf(v[b + 8] / 100.f, 2) * 2.f;
    float su = v[b + 7] / 100.f;
    if (gate && !m->sq[s].last_on) m->sq[s].stage = 1;
    if (!gate && m->sq[s].stage && m->sq[s].stage != 4) m->sq[s].stage = 4;
    m->sq[s].last_on = gate;
    if (gate) m->sq[s].hold = val;
    switch (m->sq[s].stage) {
    case 1: m->sq[s].env += dt / at; if (m->sq[s].env >= 1.f) { m->sq[s].env = 1.f; m->sq[s].stage = 2; } break;
    case 2: m->sq[s].env -= dt / dc * (1.f - su); if (m->sq[s].env <= su) { m->sq[s].env = su; m->sq[s].stage = 3; } break;
    case 3: m->sq[s].env = su; break;
    case 4: m->sq[s].env -= dt / rl; if (m->sq[s].env <= 0) { m->sq[s].env = 0; m->sq[s].stage = 0; } break;
    default: break;
    }
    float sv = m->sq[s].env * (gate ? val : m->sq[s].hold);
    mod_apply(m, (int)(v[b] + .5f), v[b + 1] / 100.f, sv, 1);
    mod_apply(m, (int)(v[xb + 1] + .5f), v[xb + 2] / 100.f, sv, 1);
}
/* the touch bar: three zones side by side; the finger's place in a zone (or its height) drives up to three destinations */
static void touch_tick(mori_t *m, int frames) {
    float x = m->tx_, y = m->ty_; m->v[K_TCH_X] = x; m->v[K_TCH_Y] = y;
    float u = (x + 5.f) / 10.f; int z = (int)(u * 3.f); if (z > 2) z = 2; if (z < 0) z = 0;
    float p = u * 3.f - z; if (p > 1.f) p = 1.f;
    int active = (m->pad_on && m->pad.touching) || x != m->tlx || y != m->tly;
    m->tlx = x; m->tly = y;
    float dt = frames / SR;
    for (int i = 0; i < 3; i++) {
        int b = K_TZ1_DA + i * (K_TZ2_DA - K_TZ1_DA);
        if (active && z == i) m->tzv[i] = ((int)m->v[b + 6] == 0) ? p : (y + 5.f) / 10.f;
        else if ((int)m->v[b + 7] == 1 && !(m->pad_on && m->pad.touching && z == i)) { m->tzv[i] -= dt * 4.f; if (m->tzv[i] < 0) m->tzv[i] = 0; }
        for (int e = 0; e < 3; e++) mod_apply(m, (int)(m->v[b + e * 2] + .5f), m->v[b + e * 2 + 1] / 100.f, m->tzv[i], 0);
    }
}
static float frz_fade(const mori_t *m, float rp, int txlen) {
    float d = fabsf(rp - (float)m->frz); if (d > txlen * .5f) d = txlen - d;
    return d >= 1024.f ? 1.f : d / 1024.f;
}

/* ---- presets -------------------------------------------------------------------------------------------------------- */
static int ps_total(mori_t *m) { return NFACTORY + m->nuser; }
static const float *ps_vals(mori_t *m, int n) { return n < NFACTORY ? FVAL[n] : m->uval[n - NFACTORY]; }
static void ps_label(mori_t *m, int n, char *b, int len) {
    if (n < NFACTORY) snprintf(b, len, "%s", FNAME[n]); else snprintf(b, len, "%s", m->uname[n - NFACTORY]);
}
static int ps_same(mori_t *m, int n) {
    const float *pv = ps_vals(m, n);
    for (int i = 0; i < NFV; i++) if (fabsf(m->v[FKEY[i]] - pv[i]) > 0.05f) return 0;
    return 1;
}
static void ps_mark_all(mori_t *m) { m->rev++; for (int i = 0; i < K_COUNT; i++) if (i != K_OV_CLEAR) m->pend[i] = 1; m->pend_scan = 0; }
static void ps_apply(mori_t *m, int n) {
    if (n < 0 || n >= ps_total(m)) return;
    const float *pv = ps_vals(m, n);
    for (int i = 0; i < NFV; i++) m->v[FKEY[i]] = pv[i];
    m->sel = n; ps_mark_all(m);
}
static void ps_file_write(mori_t *m) {
    if (!m->path[0]) return;
    char tmp[560]; snprintf(tmp, sizeof tmp, "%s.new", m->path);
    FILE *f = fopen(tmp, "w"); if (!f) return;
    for (int u = 0; u < m->nuser; u++) {
        fprintf(f, "%s\t", m->uname[u]);
        for (int i = 0; i < NFV; i++) fprintf(f, "%g%c", m->uval[u][i], i + 1 < NFV ? ' ' : '\n');
    }
    fclose(f); rename(tmp, m->path);
}
static void ps_file_read(mori_t *m) {
    if (!m->path[0]) return;
    FILE *f = fopen(m->path, "r"); if (!f) return;
    char *line = malloc(8192);
    while (line && m->nuser < NUSER_MAX && fgets(line, 8192, f)) {
        char *tab = strchr(line, '\t'); if (!tab) continue;
        *tab = 0; char *q = tab + 1; float vals[NFV]; int ok = 1;
        for (int i = 0; i < NFV; i++) { char *e; vals[i] = strtof(q, &e); if (e == q) { ok = 0; break; } q = e; }
        if (!ok) continue;
        snprintf(m->uname[m->nuser], NAMELEN + 1, "%s", line);
        memcpy(m->uval[m->nuser], vals, sizeof vals); m->nuser++;
    }
    free(line); fclose(f);
}
static void ps_save_new(mori_t *m) {
    if (m->nuser >= NUSER_MAX) return;
    int u = m->nuser++;
    snprintf(m->uname[u], NAMELEN + 1, "USER %03d", u + 1);
    for (int i = 0; i < NFV; i++) m->uval[u][i] = m->v[FKEY[i]];
    m->sel = NFACTORY + u; ps_file_write(m); ps_mark_all(m);
}
static void ps_overwrite(mori_t *m) {
    if (m->sel < NFACTORY) return;
    int u = m->sel - NFACTORY;
    for (int i = 0; i < NFV; i++) m->uval[u][i] = m->v[FKEY[i]];
    ps_file_write(m); ps_mark_all(m);
}
static void ps_delete(mori_t *m) {
    if (m->sel < NFACTORY) return;
    int u = m->sel - NFACTORY;
    for (int j = u; j + 1 < m->nuser; j++) { memcpy(m->uname[j], m->uname[j + 1], NAMELEN + 1); memcpy(m->uval[j], m->uval[j + 1], sizeof m->uval[j]); }
    m->nuser--; if (m->sel >= ps_total(m)) m->sel = ps_total(m) - 1;
    ps_file_write(m); ps_mark_all(m);
}

static int eng_get(void *p, const char *key, char *buf, int n);

/* ---- status line, pattern slots ------------------------------------------------------------------------------------- */
static void st_info(mori_t *m, char *buf, int n) {
    static const char *const RM[4] = { "", "REC ADD", "REC CHANGE", "FORGET" };
    int rm = (int)clampf(m->v[K_OV_REC], 0, 3);
    const char *dub = rm ? RM[rm] : (m->ov_any ? "PLAY" : "EMPTY");
    if (m->hbpm > 1) snprintf(buf, n, "DUB %s  %.1f BPM %s", dub, m->hbpm, (m->ppq >= 0 && m->playing) ? "SYNC" : "FREE");
    else snprintf(buf, n, "DUB %s  NO TEMPO", dub);
}
static void pat_write(mori_t *m) {
    if (!m->pat_path[0]) return;
    char tmp[560]; snprintf(tmp, sizeof tmp, "%s.new", m->pat_path);
    FILE *f = fopen(tmp, "w"); if (!f) return;
    for (int i = 0; i < 8; i++) if (m->pat_used[i]) { fprintf(f, "%d", i); for (int j = 0; j < 32; j++) fprintf(f, " %g", m->pat[i][j]); fprintf(f, "\n"); }
    fclose(f); rename(tmp, m->pat_path);
}
static void pat_read(mori_t *m) {
    FILE *f = m->pat_path[0] ? fopen(m->pat_path, "r") : NULL; if (!f) return;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        char *q = line, *e; long sl = strtol(q, &e, 10); if (e == q || sl < 0 || sl > 7) continue;
        q = e; float t[32]; int ok = 1;
        for (int j = 0; j < 32; j++) { t[j] = strtof(q, &e); if (e == q) { ok = 0; break; } q = e; }
        if (ok) { memcpy(m->pat[sl], t, sizeof t); m->pat_used[sl] = 1; }
    }
    fclose(f);
}
static void pat_save(mori_t *m, int slot, int seq) {
    if (slot < 0 || slot > 7) return;
    int sb = (seq == 2 ? K_SQ2_DEST : K_SQ1_DEST) + 9, tb = seq == 2 ? K_T2_1 : K_T1_1;
    for (int j = 0; j < 16; j++) { m->pat[slot][j] = m->v[sb + j]; m->pat[slot][16 + j] = m->v[tb + j]; }
    m->pat_used[slot] = 1; pat_write(m);
}
static void pat_load(mori_t *m, int slot, int seq) {
    if (slot < 0 || slot > 7 || !m->pat_used[slot]) return;
    int sb = (seq == 2 ? K_SQ2_DEST : K_SQ1_DEST) + 9, tb = seq == 2 ? K_T2_1 : K_T1_1;
    for (int j = 0; j < 16; j++) { m->v[sb + j] = m->pat[slot][j]; m->v[tb + j] = m->pat[slot][16 + j]; m->pend[sb + j] = m->pend[tb + j] = 1; }
    m->pend_scan = 0; m->rev++;
}

/* ---- the editor: a small web server on the Force (a browser on the same network opens http://<force>:8765) ------------- */
static void eng_set(void *p, const char *key, const char *val);
static void web_all(int fd, const char *b, size_t n) { while (n > 0) { ssize_t k = send(fd, b, n, MSG_NOSIGNAL); if (k <= 0) return; b += k; n -= (size_t)k; } }
static void web_send(int fd, const char *ctype, const char *extra, const char *body, size_t n) {
    char h[320]; int hl = snprintf(h, sizeof h, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %u\r\nCache-Control: no-store\r\n%sConnection: close\r\n\r\n", ctype, (unsigned)n, extra ? extra : "");
    web_all(fd, h, (size_t)hl); web_all(fd, body, n);
}
static void url_dec(char *s) {
    char *o = s;
    for (; *s; s++) {
        if (*s == '+') *o++ = ' ';
        else if (*s == '%' && s[1] && s[2]) { char h[3] = { s[1], s[2], 0 }; *o++ = (char)strtol(h, NULL, 16); s += 2; }
        else *o++ = *s;
    }
    *o = 0;
}
static int q_get(const char *q, const char *k, char *out, int n) {       /* value of k in "a=1&b=2", decoded */
    size_t kl = strlen(k); out[0] = 0;
    while (q && *q) {
        if (!strncmp(q, k, kl) && q[kl] == '=') {
            const char *v = q + kl + 1; int i = 0;
            while (*v && *v != '&' && i < n - 1) out[i++] = *v++;
            out[i] = 0; url_dec(out); return 1;
        }
        q = strchr(q, '&'); if (q) q++;
    }
    return 0;
}
static int js_str(char *o, int n, const char *s) {                         /* a JSON string, quotes included */
    int i = 0; if (n < 3) return 0; o[i++] = '"';
    for (; *s && i < n - 3; s++) { unsigned char c = (unsigned char)*s; if (c == '"' || c == '\\') o[i++] = '\\'; if (c >= 32) o[i++] = (char)c; }
    o[i++] = '"'; o[i] = 0; return i;
}
static void clean_name(char *nm) {                                          /* a preset name: one line, no tab, at most 24 */
    int j = 0; for (int i = 0; nm[i] && j < NAMELEN; i++) { unsigned char c = (unsigned char)nm[i]; if (c >= 32 && c != 127) nm[j++] = (char)c; }
    nm[j] = 0;
}
static void web_handle(mori_t *m, int fd) {
    size_t cap = 600000; char *req = malloc(cap + 1); if (!req) return;
    size_t got = 0; char *hend = NULL;
    while (got < cap) {
        ssize_t k = recv(fd, req + got, cap - got, 0); if (k <= 0) break;
        got += (size_t)k; req[got] = 0;
        if (!hend) hend = strstr(req, "\r\n\r\n");
        if (hend) {
            size_t need = 0; char *cl = strcasestr(req, "Content-Length:");
            if (cl && cl < hend) need = (size_t)strtoul(cl + 15, NULL, 10);
            if (got >= (size_t)(hend + 4 - req) + need) break;
        }
    }
    req[got] = 0;
    if (!hend) { free(req); return; }
    char *body = hend + 4;
    char method[8] = "", path[512] = "";
    sscanf(req, "%7s %511s", method, path);
    char *q = strchr(path, '?'); if (q) *q++ = 0; else q = path + strlen(path);
    size_t ocap = 200000; char *out = malloc(ocap); if (!out) { free(req); return; }
    int n = 0;
    if (!strcmp(path, "/") || !strcmp(path, "/index.html")) web_send(fd, "text/html; charset=utf-8", NULL, (const char *)WEB_PAGE, WEB_PAGE_LEN);
    else if (!strcmp(path, "/meta")) web_send(fd, "application/json", NULL, (const char *)WEB_META, WEB_META_LEN);
    else if (!strcmp(path, "/state")) {
        char nm[64], inf[96], t[160];
        eng_get(m, "preset_name", nm, sizeof nm); st_info(m, inf, sizeof inf);
        n += snprintf(out + n, ocap - n, "{\"rev\":%u,\"sel\":%d,\"nf\":%d,\"nu\":%d,\"touch\":%d,\"name\":", m->rev, m->sel, NFACTORY, m->nuser, m->pad_on ? m->pad.touching : 0);
        js_str(t, sizeof t, nm); n += snprintf(out + n, ocap - n, "%s,\"info\":", t);
        js_str(t, sizeof t, inf); n += snprintf(out + n, ocap - n, "%s,\"v\":[", t);
        for (int i = 0; i < K_COUNT; i++) n += snprintf(out + n, ocap - n, "%s%g", i ? "," : "", i == K_PRESET ? (float)m->sel : m->v[i]);
        n += snprintf(out + n, ocap - n, "]}");
        web_send(fd, "application/json", NULL, out, (size_t)n);
    } else if (!strcmp(path, "/presets")) {
        char t[80];
        pthread_mutex_lock(&m->plock);
        n += snprintf(out + n, ocap - n, "{\"f\":[");
        for (int i = 0; i < NFACTORY; i++) { js_str(t, sizeof t, FNAME[i]); n += snprintf(out + n, ocap - n, "%s%s", i ? "," : "", t); }
        n += snprintf(out + n, ocap - n, "],\"u\":[");
        for (int i = 0; i < m->nuser && (size_t)n < ocap - 100; i++) { js_str(t, sizeof t, m->uname[i]); n += snprintf(out + n, ocap - n, "%s%s", i ? "," : "", t); }
        n += snprintf(out + n, ocap - n, "]}");
        pthread_mutex_unlock(&m->plock);
        web_send(fd, "application/json", NULL, out, (size_t)n);
    } else if (!strcmp(path, "/set")) {
        char a[32], b[32];
        if (q_get(q, "i", a, sizeof a) && q_get(q, "v", b, sizeof b)) {
            int i = atoi(a);
            if (i >= 0 && i < K_COUNT && i != K_PRESET_NAME && i != K_ED_ADDR && i != K_ST_INFO) {
                float x = clampf((float)atof(b), KMINV[i], KMAXV[i]); char vb[32]; snprintf(vb, sizeof vb, "%g", x);
                if (i == K_PRESET) { int old = m->blocks; m->blocks = 100000; eng_set(m, "preset", vb); m->blocks = old > 200 ? old : 201; }
                else { eng_set(m, KEYS[i], vb); m->pend[i] = 1; m->pend_scan = 0; }
            }
        }
        web_send(fd, "text/plain", NULL, "ok", 2);
    } else if (!strcmp(path, "/cmd")) {
        char c[24] = "", a[80] = "", b[16] = "";
        q_get(q, "c", c, sizeof c);
        pthread_mutex_lock(&m->plock);
        if (!strcmp(c, "load")) { q_get(q, "n", a, sizeof a); ps_apply(m, atoi(a)); m->pend[K_PRESET] = m->pend[K_PRESET_NAME] = 1; m->pend_scan = 0; }
        else if (!strcmp(c, "save")) { q_get(q, "name", a, sizeof a); clean_name(a); ps_save_new(m); if (a[0] && m->sel >= NFACTORY) { snprintf(m->uname[m->sel - NFACTORY], NAMELEN + 1, "%s", a); ps_file_write(m); } }
        else if (!strcmp(c, "over")) ps_overwrite(m);
        else if (!strcmp(c, "del")) ps_delete(m);
        else if (!strcmp(c, "rename")) { q_get(q, "name", a, sizeof a); clean_name(a); if (a[0] && m->sel >= NFACTORY) { snprintf(m->uname[m->sel - NFACTORY], NAMELEN + 1, "%s", a); ps_file_write(m); ps_mark_all(m); } }
        else if (!strcmp(c, "patsave")) { q_get(q, "slot", a, sizeof a); q_get(q, "seq", b, sizeof b); pat_save(m, atoi(a), atoi(b)); }
        else if (!strcmp(c, "patload")) { q_get(q, "slot", a, sizeof a); q_get(q, "seq", b, sizeof b); pat_load(m, atoi(a), atoi(b)); }
        pthread_mutex_unlock(&m->plock);
        web_send(fd, "text/plain", NULL, "ok", 2);
    } else if (!strcmp(path, "/export")) {
        pthread_mutex_lock(&m->plock);
        for (int u = 0; u < m->nuser && (size_t)n < ocap - 2000; u++) {
            n += snprintf(out + n, ocap - n, "%s\t", m->uname[u]);
            for (int i = 0; i < NFV; i++) n += snprintf(out + n, ocap - n, "%g%c", m->uval[u][i], i + 1 < NFV ? ' ' : '\n');
        }
        pthread_mutex_unlock(&m->plock);
        web_send(fd, "text/plain; charset=utf-8", "Content-Disposition: attachment; filename=\"MORI_user_presets.txt\"\r\n", out, (size_t)n);
    } else if (!strcmp(path, "/import") && !strcmp(method, "POST")) {
        pthread_mutex_lock(&m->plock);
        char *ln = body, *save = NULL;
        for (ln = strtok_r(body, "\n", &save); ln && m->nuser < NUSER_MAX; ln = strtok_r(NULL, "\n", &save)) {
            char *tab = strchr(ln, '\t'); if (!tab) continue;
            *tab = 0; char *qq = tab + 1; float vals[NFV]; int ok = 1;
            for (int i = 0; i < NFV; i++) { char *e; vals[i] = strtof(qq, &e); if (e == qq) { ok = 0; break; } qq = e; }
            if (!ok) continue;
            char nm[80]; snprintf(nm, sizeof nm, "%s", ln); clean_name(nm); if (!nm[0]) continue;
            snprintf(m->uname[m->nuser], NAMELEN + 1, "%s", nm);
            for (int i = 0; i < NFV; i++) m->uval[m->nuser][i] = clampf(vals[i], KMINV[FKEY[i]], KMAXV[FKEY[i]]);
            m->nuser++;
        }
        ps_file_write(m); m->rev++;
        pthread_mutex_unlock(&m->plock);
        web_send(fd, "text/plain", NULL, "ok", 2);
    } else {
        const char *nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"; web_all(fd, nf, strlen(nf));
    }
    free(out); free(req);
}
static void *web_thread(void *p) {
    mori_t *m = p;
    while (!m->srv_stop) {
        fd_set rs; FD_ZERO(&rs); FD_SET(m->srv_fd, &rs);
        struct timeval tv = { 0, 250000 };
        if (select(m->srv_fd + 1, &rs, NULL, NULL, &tv) <= 0) continue;
        int c = accept(m->srv_fd, NULL, NULL); if (c < 0) continue;
        struct timeval to = { 2, 0 }; setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &to, sizeof to); setsockopt(c, SOL_SOCKET, SO_SNDTIMEO, &to, sizeof to);
        web_handle(m, c); close(c);
    }
    return NULL;
}
static void web_start(mori_t *m) {
    int fd = socket(AF_INET, SOCK_STREAM, 0); if (fd < 0) return;
    int one = 1; setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    int port = 0;
    for (int pt = 8765; pt < 8785 && !port; pt++) {
        struct sockaddr_in a; memset(&a, 0, sizeof a); a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_ANY); a.sin_port = htons((uint16_t)pt);
        if (bind(fd, (struct sockaddr *)&a, sizeof a) == 0) port = pt;
    }
    if (!port || listen(fd, 8) != 0) { close(fd); return; }
    m->srv_fd = fd; m->srv_port = port;
    char ip[40] = ""; struct ifaddrs *ifa = NULL;
    if (getifaddrs(&ifa) == 0) {
        for (struct ifaddrs *i = ifa; i && !ip[0]; i = i->ifa_next) {
            if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET) continue;
            struct sockaddr_in *sa = (struct sockaddr_in *)i->ifa_addr;
            if ((ntohl(sa->sin_addr.s_addr) >> 24) == 127) continue;
            inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof ip);
        }
        freeifaddrs(ifa);
    }
    if (ip[0]) snprintf(m->addr, sizeof m->addr, "http://%s:%d", ip, port); else snprintf(m->addr, sizeof m->addr, "port %d (no network)", port);
    if (pthread_create(&m->srv_th, NULL, web_thread, m) == 0) m->srv_on = 1; else { close(fd); m->addr[0] = 0; }
}
static void web_stop(mori_t *m) {
    if (!m->srv_on) return;
    m->srv_stop = 1; pthread_join(m->srv_th, NULL); close(m->srv_fd); m->srv_on = 0;
}

static void *eng_create(const char *dir) {
    mori_t *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->uname = calloc(NUSER_MAX, sizeof *m->uname); m->uval = calloc(NUSER_MAX, sizeof *m->uval);
    /* the user's own presets and patterns live in <plugin folder>/user, which the installer keeps across updates */
    char ud[480]; snprintf(ud, sizeof ud, "%s/user", dir && dir[0] ? dir : "/sdcard/Synths/L'Cronx - VST - MORI");
    if (dir && dir[0]) { char *sl = strrchr(ud, '/'); if (sl) { *sl = 0; sl = strrchr(ud, '/'); if (sl) snprintf(sl, sizeof ud - (size_t)(sl - ud), "/user"); } }
    mkdir(ud, 0755);
    snprintf(m->path, sizeof m->path, "%s/user_presets.txt", ud);
    if (m->uname && m->uval) ps_file_read(m); else m->path[0] = 0;
    pthread_mutex_init(&m->plock, NULL);
    snprintf(m->pat_path, sizeof m->pat_path, "%s/patterns.txt", ud);
    pat_read(m);
    for (int i = 0; i < K_COUNT; i++) m->v[i] = KDEF[i];
    m->bpm = 120; m->rng = 12345u; m->ppq = -1;
    for (int c = 0; c < 2; c++) {
        m->dl[c] = calloc(DLY_LEN, sizeof(float)); m->tx[c] = calloc(TEX_LEN, sizeof(float)); m->ov[c] = calloc(OVL_LEN, sizeof(float));
    }
    static const int base[8] = { 1117, 1303, 1523, 1747, 1949, 2161, 2377, 2593 };
    for (int i = 0; i < 8; i++) { m->rbl[i] = base[i] * 4; m->rb[i] = calloc(m->rbl[i] + 4, sizeof(float)); }
    m->sh = calloc(8192, sizeof(float));
    for (int i = 0; i < 8; i++) m->head_pos[i] = i * (TEX_LEN / 8.0f);
    if (dir && dir[0]) { pad_start(&m->pad, dir, &m->tx_, &m->ty_, &m->tch_changed); m->pad_on = 1; }
    web_start(m);
    return m;
}
static void eng_destroy(void *p) {
    mori_t *m = p; if (!m) return;
    web_stop(m);
    if (m->pad_on) pad_stop(&m->pad);
    for (int c = 0; c < 2; c++) { free(m->dl[c]); free(m->tx[c]); free(m->ov[c]); }
    for (int i = 0; i < 8; i++) free(m->rb[i]);
    free(m->sh); free(m->uname); free(m->uval); free(m);
}
static void eng_midi(void *p, const uint8_t *msg, int len) { (void)p; (void)msg; (void)len; }

static void eng_set(void *p, const char *key, const char *val) {
    mori_t *m = p; float x = (float)atof(val);
    if (!strcmp(key, "preset")) {
        int n = (int)(x + .5f); if (n < 0) n = 0; if (n >= ps_total(m)) n = ps_total(m) - 1;
        /* the host also sets every parameter when it restores a saved track, before audio runs: only an audible,
         * running instance loads the preset, so a restore keeps the knob values the track was saved with */
        if (m->blocks > 200 && n != m->sel) ps_apply(m, n); else m->sel = n;
        m->pend[K_PRESET] = m->pend[K_PRESET_NAME] = 1; m->pend_scan = 0; return;
    }
    if (x > 0.5f) {
        int did = 1;
        pthread_mutex_lock(&m->plock);
        if (!strcmp(key, "ps_save")) ps_save_new(m); else if (!strcmp(key, "ps_over")) ps_overwrite(m); else if (!strcmp(key, "ps_del")) ps_delete(m); else did = 0;
        pthread_mutex_unlock(&m->plock);
        if (did) { m->rev++; return; }
    }
    if (!strcmp(key, "lfo_bpm")) { if (x > 20 && x < 400) m->bpm = x; return; }
    for (int i = 0; i < K_COUNT; i++) if (!strcmp(key, KEYS[i])) {
        m->v[i] = x; m->rev++;
        if (i == K_TCH_X) m->tx_ = clampf(x, -5, 5);
        if (i == K_TCH_Y) m->ty_ = clampf(x, -5, 5);
        if (i == K_OV_CLEAR && x > 0.5f) { for (int c = 0; c < 2; c++) memset(m->ov[c], 0, OVL_LEN * sizeof(float)); m->ov_any = 0; }
        return;
    }
}
static int eng_get(void *p, const char *key, char *buf, int n) {
    mori_t *m = p;
    if (!strcmp(key, "preset")) { snprintf(buf, n, "%d", m->sel); return (int)strlen(buf); }
    if (!strcmp(key, "ed_addr")) { snprintf(buf, n, "%s", m->addr[0] ? m->addr : "editor off"); return (int)strlen(buf); }
    if (!strcmp(key, "st_info")) { st_info(m, buf, n); return (int)strlen(buf); }
    if (!strcmp(key, "preset_name")) {
        /* the preset the sound matches: the selected one, or any factory one it equals, else the selected one with a star */
        char b[NAMELEN + 2], nm[NAMELEN + 8]; int sel = m->sel < ps_total(m) ? m->sel : 0, show = sel, star = 0;
        if (!ps_same(m, sel)) {
            int hit = -1; for (int j = 0; j < NFACTORY && hit < 0; j++) if (ps_same(m, j)) hit = j;
            if (hit >= 0) show = hit; else star = 1;
        }
        ps_label(m, show, b, sizeof b);
        snprintf(nm, sizeof nm, "%d  %s%s", show + 1, b, star ? "*" : "");
        snprintf(buf, n, "%s", nm); return (int)strlen(buf);
    }
    for (int i = 0; i < K_COUNT; i++) if (!strcmp(key, KEYS[i])) { snprintf(buf, n, "%g", m->v[i]); return (int)strlen(buf); }
    return 0;
}
/* (also the two touch values a finger moved) */
/* tells the wrapper which parameters a preset load moved, so the screen's knobs follow (one key per call) */
int mpc_engine_changed(void *p, char *key, int len) {
    mori_t *m = p; if (!m) return 0;
    m->tch_rep |= __atomic_exchange_n(&m->tch_changed, 0u, __ATOMIC_ACQUIRE);
    if (m->tch_rep & 1u) { m->tch_rep &= ~1u; snprintf(key, len, "tch_x"); return 1; }
    if (m->tch_rep & 2u) { m->tch_rep &= ~2u; snprintf(key, len, "tch_y"); return 1; }
    while (m->pend_scan < K_COUNT) {
        int i = m->pend_scan++;
        if (m->pend[i]) { m->pend[i] = 0; snprintf(key, len, "%s", KEYS[i]); return 1; }
    }
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
    int t = (int)m->vm[K_RV_TYPE]; if (t < 0) t = 0; if (t > 4) t = 4;
    *decay = TD[t] + (m->vm[K_RV_DIM] / 100.f - .5f) * .12f;
    *size = TS[t] * (.5f + m->vm[K_RV_DIM] / 100.f);
    *damp = .15f + .8f * m->vm[K_RV_DAMP] / 100.f;
    if (*decay > .985f) *decay = .985f;
}

static void eng_song_pos(void *p, double ppq, double bpm, int playing, int frames) {
    mori_t *m = p; (void)frames; m->ppq = ppq; m->playing = playing; if (bpm > 20 && bpm < 400) { m->hbpm = bpm; m->bpm = (float)bpm; }
}
static float do_filter(svf_t *a, svf_t *b, float t, int fbp, int f24, float fc, float fg_lp, float fg_hp, float fk) {
    if (fbp) { t = svf(a, t, fg_lp, fk, 2); if (f24) t = svf(b, t, fg_lp, fk, 2); }
    else if (fc < -.02f) { t = svf(a, t, fg_lp, fk, 0); if (f24) t = svf(b, t, fg_lp, fk, 0); }
    else if (fc > .02f) { t = svf(a, t, fg_hp, fk, 1); if (f24) t = svf(b, t, fg_hp, fk, 1); }
    return t;
}
static float do_mfx(float s, int mft, float mfa, float mf_g, float *hp, float *lp) {
    if (mft == 1) { *hp += mf_g * (s - *hp); float hf = s - *hp; s += tanhf(hf * 4.f) * .5f * mfa; }
    else if (mft == 2) { s = (1.f - mfa) * s + mfa * tanhf(s * (1.f + 2.5f * mfa)) / tanhf(1.f + 2.5f * mfa); *lp += (1.f - mfa * .4f) * (s - *lp); s = *lp; }
    else if (mft == 3) { float y = s * (1.f + 3.f * mfa) + .2f * mfa; y = y > 0 ? tanhf(y) : 1.2f * tanhf(y * .8f); y -= .2f * mfa * .6f; s = (1.f - mfa) * s + mfa * y; }
    return s;
}
static void eng_process(void *p, const int16_t *in, int16_t *out, int frames) {
    ((mori_t *)p)->blocks++;
    mori_t *m = p;
    float spb0 = 60.f / m->bpm * SR;
    memcpy(m->vm, m->v, sizeof m->vm);
    seq_tick(m, 0, frames, spb0); seq_tick(m, 1, frames, spb0); touch_tick(m, frames);
    float *v = m->vm;
    int blocked0 = v[K_TX_BLOCK] > .5f;
    if (blocked0 && !m->was_blk) m->frz = m->tw;
    m->was_blk = blocked0;
    float lvl_dry = v[K_DRY] / 100.f, lvl_ovl = v[K_OVL] / 100.f, lvl_dly = v[K_DLY] / 100.f, lvl_txt = v[K_TXT] / 100.f;
    float master = v[K_MASTER] / 100.f; master = master * master * 1.4f;
    int order = (int)v[K_ORDER];
    /* delay times in samples */
    float spb = spb0;
    float tl, tr;
    if ((int)v[K_DL_RANGE] == 1) {          /* MS: the LEFT and RIGHT knobs span 0 .. DL MS in 22 steps */
        float ms = v[K_DL_MS] * SR / 1000.f;
        tl = clampf(((int)v[K_DL_L] % 22 + 1) / 22.f * ms, 64, DLY_LEN - 16);
        tr = clampf(((int)v[K_DL_R] % 22 + 1) / 22.f * ms * (1.f + v[K_DL_OFF] / 8.f), 64, DLY_LEN - 16);
    } else {
        tl = clampf(DIVB[(int)v[K_DL_L] % 22] * spb, 64, DLY_LEN - 16);
        tr = clampf(DIVB[(int)v[K_DL_R] % 22] * spb * (1.f + v[K_DL_OFF] / 8.f), 64, DLY_LEN - 16);
    }
    float ing = powf(10.f, v[K_IN_GAIN] / 20.f), tfb = v[K_RV_TFB] / 100.f, rnz = v[K_RV_NOISE] / 100.f, wid = v[K_TX_WIDTH] / 100.f;
    int g_holdn = (int)(v[K_GT_HOLD] / 100.f * .5f * SR);
    static const float SHR[4] = { 2.f, 1.4983f, 1.3348f, .5f };
    float shr = SHR[(int)clampf(v[K_RV_SHP], 0, 3)], sh_inc = fabsf(shr - 1.f) / 2048.f; int sh_up = shr > 1.f;
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
    if (m->ppq >= 0 && m->playing) {        /* the loop follows the song position, so it records and plays on the bars */
        int t = (int)(fmod(m->ppq, (double)v[K_OV_SIZE]) * spb); if (t >= ovlen) t = ovlen - 1;
        int d = t > m->ow ? t - m->ow : m->ow - t; if (d > 2 && ovlen - d > 2) m->ow = t;
    }
    /* noise gate on the input */
    int gate_on = v[K_GT_THR] > 1.f;
    float g_thr = powf(10.f, (-80.f + v[K_GT_THR] * .6f) / 20.f), g_flr = v[K_GT_FLR] / 100.f;
    float g_ca = 1.f - expf(-1.f / ((.5f + v[K_GT_ATK] / 100.f * 49.5f) * .001f * SR)), g_cr = 1.f - expf(-1.f / ((5.f + v[K_GT_REL] / 100.f * 995.f) * .001f * SR));
    int fpos = (int)v[K_FLT_POS], mpos = (int)v[K_MF_POS]; if (mpos == 3) mpos = fpos;
    int fin = fpos == 1, min_ = mpos == 1;
    float spin_hz = .05f * powf(120.f, v[K_SP_SPEED] / 100.f), spin_d = v[K_SP_DIST] / 100.f; int spin_on = (int)v[K_OUT_MODE] == 1;
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
    float mf_g = 1.f - expf(-2 * PI * 1000.f * powf(8.f, v[K_MF_FREQ] / 100.f) / SR);
    float rev_rt[8];
    for (int i = 0; i < 8; i++) rev_rt[i] = clampf(m->rbl[i] * rsize / 4.f * 1.f, 64, (float)m->rbl[i] - 4);

    for (int n = 0; n < frames; n++) {
        float x[2] = { in[2 * n] / 32768.f, in[2 * n + 1] / 32768.f };
        x[0] *= ing; x[1] *= ing;
        if (gate_on) {
            float lvl = fmaxf(fabsf(x[0]), fabsf(x[1]));
            m->g_env += (lvl > m->g_env ? g_ca : g_cr) * (lvl - m->g_env);
            int open = m->g_env > g_thr;
            if (open) m->g_holdc = g_holdn; else if (m->g_holdc > 0) { m->g_holdc--; open = 1; }
            float tg = open ? 1.f : g_flr;
            m->g_gain += (tg - m->g_gain) * (tg > m->g_gain ? g_ca : g_cr);
            x[0] *= m->g_gain; x[1] *= m->g_gain;
        }
        for (int c = 0; c < 2; c++) {
            if (fin && fon) x[c] = do_filter(&m->fi1[c], &m->fi2[c], x[c], fbp, f24, fc, fg_lp, fg_hp, fk);
            if (min_) x[c] = do_mfx(x[c], mft, mfa, mf_g, &m->mi_hp[c], &m->mi_lp[c]);
        }
        float ovo[2], dlo[2] = { 0, 0 }, txo[2] = { 0, 0 };
        /* overlay loop: read, then write by mode */
        for (int c = 0; c < 2; c++) {
            float o = m->ov[c][m->ow];
            ovo[c] = o;
            if (ovmode == 1) { m->ov[c][m->ow] = o * .995f + x[c] * .7f; if (fabsf(x[c]) > .001f) m->ov_any = 1; }
            else if (ovmode == 2) { m->ov[c][m->ow] = x[c]; if (fabsf(x[c]) > .001f) m->ov_any = 1; }
            else if (ovmode == 3) m->ov[c][m->ow] = 0;
        }
        if (++m->ow >= ovlen) m->ow = 0;
        float dly_in[2], tex_in[2];
        for (int pass = 0; pass < 2; pass++) {
            int do_delay = (order == 0) ? pass == 0 : pass == 1;
            if (do_delay) {
                for (int c = 0; c < 2; c++) {
                    dly_in[c] = x[c] * (v[K_R_DD] > .5f) + ovo[c] * (v[K_R_OD] > .5f) + (order == 1 ? txo[c] * (v[K_R_TD] > .5f) : 0) + m->rb_wet[c] * tfb * .5f;
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
                            txo[c] += lerp_buf(m->tx[c], txlen, rp) * g * pan * (blocked ? frz_fade(m, rp, txlen) : 1.f);
                        }
                    }
                } else if ((int)v[K_TX_ENG] == 1) {
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
                        float ff = blocked ? frz_fade(m, rp, txlen) : 1.f;
                        txo[0] += lerp_buf(m->tx[0], txlen, rp) * w * ff * (1.f - m->gr[g].pan * .6f) * .5f;
                        txo[1] += lerp_buf(m->tx[1], txlen, rp) * w * ff * (.4f + m->gr[g].pan * .6f) * .5f;
                        m->gr[g].pos += m->gr[g].inc; m->gr[g].age += 1;
                        if (m->gr[g].age >= m->gr[g].len) m->gr[g].on = 0;
                    }
                }
                if ((int)v[K_TX_ENG] == 2) {
                    /* vinyl: one read head on the memory, its speed set by VINYL SPD (100 = normal, 0 = stopped, negative = backwards) */
                    if (!m->vinit) { m->vpos = (float)wpos - txlen * .5f; m->vinit = 1; }
                    m->vspd += (v[K_TX_SPEED] / 100.f - m->vspd) * .002f;
                    m->vpos += m->vspd;
                    while (m->vpos < 0) m->vpos += txlen; while (m->vpos >= txlen) m->vpos -= txlen;
                    float ff = blocked ? frz_fade(m, m->vpos, txlen) : 1.f;
                    for (int c = 0; c < 2; c++) txo[c] += lerp_buf(m->tx[c], txlen, m->vpos) * ff;
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
                { float md = (txo[0] + txo[1]) * .5f, sd = (txo[0] - txo[1]) * .5f * wid; txo[0] = md + sd; txo[1] = md - sd; }
                m->rm_ph += 2 * PI * 150.f / SR; if (m->rm_ph > 2 * PI) m->rm_ph -= 2 * PI;
            }
        }
        /* mix and reverb send */
        float mix[2], rsend[2];
        for (int c = 0; c < 2; c++) {
            mix[c] = x[c] * lvl_dry * (v[K_R_DO] > .5f) + ovo[c] * lvl_ovl * (v[K_R_OO] > .5f) + dlo[c] * lvl_dly * (v[K_R_LDO] > .5f) + txo[c] * lvl_txt * (v[K_R_TO] > .5f);
            rsend[c] = x[c] * lvl_dry * (v[K_R_DR] > .5f) + ovo[c] * lvl_ovl * (v[K_R_OR] > .5f) + dlo[c] * lvl_dly * (v[K_R_LDR] > .5f) + txo[c] * lvl_txt * (v[K_R_TR] > .5f);
            if (fpos == 2 && fon) {
                mix[c] = do_filter(&m->fm1[c], &m->fm2[c], mix[c], fbp, f24, fc, fg_lp, fg_hp, fk);
                rsend[c] = do_filter(&m->fr1[c], &m->fr2[c], rsend[c], fbp, f24, fc, fg_lp, fg_hp, fk);
            }
            if (mpos == 2) mix[c] = do_mfx(mix[c], mft, mfa, mf_g, &m->mm_hp[c], &m->mm_lp[c]);
            if (pre_on) { m->rb_pre[c] += pre_g * (rsend[c] - m->rb_pre[c]); rsend[c] -= m->rb_pre[c]; }
        }
        /* 8-line FDN reverb */
        float rin = (rsend[0] + rsend[1]) * .5f + (frand(m) - .5f) * rnz * .03f, ro[8], wet[2] = { 0, 0 };
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
            float f1 = fmodf(m->shph, 1.f), f2 = fmodf(m->shph + .5f, 1.f);
            float d1 = sh_up ? 1.f - f1 : f1, d2 = sh_up ? 1.f - f2 : f2;
            float t1 = (float)m->shw - d1 * 2048.f, t2 = (float)m->shw - d2 * 2048.f;
            float s1 = lerp_buf(m->sh, 8192, t1), s2 = lerp_buf(m->sh, 8192, t2);
            float w1 = .5f - .5f * cosf(2 * PI * d1), w2 = .5f - .5f * cosf(2 * PI * d2);
            shv = s1 * w1 + s2 * w2;
            m->shph += sh_inc; if (m->shph >= 1000.f) m->shph -= 1000.f;   /* octave up: delay shrinks 1 sample per sample */
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
        m->rb_wet[0] = wet[0]; m->rb_wet[1] = wet[1];
        float o[2];
        for (int c = 0; c < 2; c++) {
            float rvt = wet[c] * ramt * 1.6f + shv * shim * ramt * .6f;
            if (v[K_RV_LIM] > .5f) rvt = tanhf(rvt);
            float s = mix[c] + rvt;
            if (fpos == 0 && fon) s = do_filter(&m->f1[c], &m->f2[c], s, fbp, f24, fc, fg_lp, fg_hp, fk);
            if (mpos == 0) s = do_mfx(s, mft, mfa, mf_g, &m->mf_hp[c], &m->mf_lp[c]);
            /* DC block + master + soft limit */
            float y = s - m->dcx[c] + .995f * m->dcy[c]; m->dcx[c] = s; m->dcy[c] = y;
            o[c] = tanhf(y * master);
        }
        if (spin_on) {
            m->sp_ph += 2 * PI * spin_hz / SR; if (m->sp_ph > 2 * PI) m->sp_ph -= 2 * PI;
            float th = PI * .25f * (1.f + spin_d * sinf(m->sp_ph)), mid = (o[0] + o[1]) * .5f, sd = (o[0] - o[1]) * .5f;
            o[0] = mid * cosf(th) * 1.4142f + sd; o[1] = mid * sinf(th) * 1.4142f - sd;
        }
        for (int c = 0; c < 2; c++) {
            float q = o[c] * 32767.f; q = q > 32767.f ? 32767.f : q < -32768.f ? -32768.f : q;
            out[2 * n + c] = (int16_t)lrintf(q);
        }
    }
}

static void eng_render(void *p, int16_t *out, int frames) { (void)p; memset(out, 0, frames * 4); }
static const mpc_engine_t E = { eng_create, eng_destroy, eng_midi, eng_set, eng_get, eng_render, eng_process, eng_song_pos };
const mpc_engine_t *mpc_engine(void) { return &E; }
