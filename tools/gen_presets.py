#!/usr/bin/env python3
"""Writes presets.json: MORI's factory bank. Every preset sets every parameter (framework rule)."""
import json, os
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.dirname(here)
params = json.load(open(os.path.join(root, "params.json")))["params"]
params = [p for p in params if not p.get("momentary") and p["key"] not in ("preset", "preset_name", "tch_x", "tch_y", "tz_sel", "ed_addr", "st_info", "ed_show")]
base = {}
for p in params:
    d = p["default"]
    base[p["key"]] = p["options"][d] if p.get("options") else d
# a quiet, playable starting point
INIT = dict(dry=100, ovl=0, dly=0, txt=0, master=80, rv_amt=0)   # what a preset leaves out is off, so each one is its named sound
presets = []
def P(name, **kw):
    v = dict(base); v.update(INIT); v.update(kw)
    # house limits: wet parts sit under the dry sound, resonance and drive stay musical, feedback cannot run away
    v["dly"] = min(v["dly"], 62); v["txt"] = min(v["txt"], 72); v["rv_amt"] = min(v["rv_amt"], 42)      # near the default balance (60 / 70 / 30)
    s = v["dly"] + v["txt"] + v["rv_amt"]                  # all the wet parts together stay under the dry sound's headroom
    if s > 130:
        for k in ("dly", "txt", "rv_amt"): v[k] = round(v[k] * 130 / s)
    if v["rv_type"] in ("CATHEDRAL", "HALL"): v["rv_dim"] = min(v["rv_dim"], 70)
    v["flt_res"] = min(v["flt_res"], 38); v["mf_amt"] = min(v["mf_amt"], 40); v["tx_fxa"] = min(v["tx_fxa"], 40)
    v["dl_fb"] = min(v["dl_fb"], 58); v["fr_fb"] = min(v["fr_fb"], 30); v["tx_boost"] = min(v["tx_boost"], 20); v["rv_shim"] = min(v["rv_shim"], 45)
    for n in (1, 2):
        if v["sq%d_dest" % n] in ("TXT FX", "M.AMT", "RES"): v["sq%d_amt" % n] = max(-45, min(45, v["sq%d_amt" % n]))
    for k in kw:
        assert k in base, k
    presets.append({"name": name[:24], "values": {k: v[k] for k in base}})

def seq(n, dest, amt, div, steps, gate="LNK", length=None, atk=5, dec=30, sus=100, rel=20):
    """One step modulator's settings; steps repeat to fill 16."""
    st = (list(steps) * 16)[:16]
    d = {"sq%d_dest" % n: dest, "sq%d_amt" % n: amt, "sq%d_div" % n: div, "sq%d_len" % n: length or min(16, len(steps) if len(steps) > 4 else 16),
         "sq%d_gate" % n: gate, "sq%d_atk" % n: atk, "sq%d_dec" % n: dec, "sq%d_sus" % n: sus, "sq%d_rel" % n: rel}
    for i, v in enumerate(st): d["s%d_%d" % (n, i + 1)] = v
    return d
def PM(name, *seqs, **kw):
    d = {}
    for s in seqs: d.update(s)
    d.update(kw); P(name, **d)
FOUR = [100, 0, 0, 0]; OFFB = [0, 0, 100, 0]; EIGHT = [100, 0]
TRANCE = [100, 0, 100, 100, 0, 100, 100, 0, 100, 0, 100, 100, 0, 100, 0, 100]
EUC5 = [100, 0, 0, 100, 0, 0, 100, 0, 0, 100, 0, 0, 100, 0, 0, 0]
EUC7 = [100, 0, 100, 0, 100, 100, 0, 100, 0, 100, 0, 100, 100, 0, 100, 0]
RAMP = [0, 7, 13, 20, 27, 33, 40, 47, 53, 60, 67, 73, 80, 87, 93, 100]
DOWN = RAMP[::-1]
DUCK = [0, 45, 80, 100]
STAIR = [10, 10, 40, 40, 70, 70, 100, 100, 70, 70, 40, 40, 10, 10, 55, 85]
RND = [80, 20, 100, 45, 0, 65, 30, 90, 15, 100, 50, 0, 75, 35, 95, 10]
MELO = [0, 0, 29, 0, 50, 50, 29, 0, 79, 0, 50, 29, 100, 0, 79, 50]      # semitone steps for GR PITCH: 0 7 12 19 24
SYNC = [100, 0, 0, 100, 0, 100, 0, 0, 100, 0, 100, 0, 0, 100, 100, 0]

# ---- starting points
# the factory FLOWs from the MEMENTO manual (chapter 7): engine and zone order as listed there, everything else neutral
P("M.HEAD", tx_eng="M.HEAD", order="DLY>TEX", tx_heads=4, tx_mode="MIXED", dly=40, txt=55, rv_amt=22)
P("DEJA-VU", tx_eng="M.HEAD", order="TEX>DLY", tx_heads=4, tx_mode="MIXED", dly=40, txt=55, rv_amt=22)
P("FRAGMENT", tx_eng="FRAGMENT", order="DLY>TEX", dly=40, txt=55, rv_amt=22)
P("VINYL", tx_eng="VINYL", order="TEX>DLY", dly=30, txt=60, rv_amt=18)
P("MORI Init", dly=60, txt=70, ovl=100, rv_amt=30)
P("Clean Send", dry=0, dly=70, rv_amt=30)

# ---- patterns: the step modulators make the rhythm
PM("Trance Gate", seq(1, "DRY", 100, "1/16", TRANCE, gate="LNG", atk=2, rel=8), dry=100, rv_amt=28, rv_type="PLATE", dly=35, dl_l="1/8.", dl_r="1/4", dl_fb=40)
PM("Euclid Chop 5", seq(1, "DRY", 100, "1/16", EUC5, gate="SHT", atk=1, rel=12), dry=100, dly=55, dl_l="1/8.", dl_r="1/8", dl_fb=45, rv_amt=25)
PM("Sidechain Verb", seq(1, "REV AMT", -70, "1/16", [100, 55, 20, 0], atk=1, dec=10), dry=100, rv_amt=75, rv_type="HALL", rv_dim=70)
PM("Pumping Echo", seq(1, "DLY", 100, "1/16", DUCK, atk=1), dry=100, dly=85, dl_l="1/8.", dl_r="1/4", dl_fb=60, dl_fltfb=-35, rv_amt=20)
PM("Filter Stairs", seq(1, "CUTOFF", 38, "1/8", STAIR, atk=3), dry=100, flt_cut=-80, flt_res=50, flt_slope="24dB", dly=45, dl_l="1/8.", dl_r="1/4", dl_fb=40, rv_amt=18)
PM("Triplet Wobble", seq(1, "CUTOFF", 35, "1/8 T", [100, 40, 0], length=3, atk=20, rel=30), dry=100, flt_cut=-75, flt_res=60, flt_slope="24dB", rv_amt=15)
PM("Random Filter Talk", seq(1, "CUTOFF", 80, "1/16", RND, atk=4), dry=100, flt_type="BP", flt_cut=-40, flt_res=65, dly=40, dl_l="1/16", dl_r="1/8.", dl_fb=35, rv_amt=20)
PM("Feedback Swells", seq(1, "DLY FBK", 55, "1/2", [0, 100, 30, 100], length=4, atk=60, rel=60), dry=100, dly=75, dl_l="1/4", dl_r="1/8.", dl_fb=35, dl_fltfb=-40, rv_amt=25)
PM("Darkening Repeats", seq(1, "DLY TONE", -45, "1/4", [0, 50, 100, 50], length=4, atk=40), dry=100, dly=75, dl_l="1/8.", dl_r="1/4", dl_fb=62, rv_amt=22)
PM("Shimmer Swell", seq(1, "SHIMMER", 80, "1/1", [0, 100], length=2, atk=85, rel=85), dry=80, rv_amt=60, rv_type="CATHEDRAL", rv_dim=85, rv_pre=30)
PM("Breathing Room", seq(1, "REV DIM", 60, "1/2", [0, 60, 100, 30], length=4, atk=70, rel=70), dry=100, rv_amt=45, rv_type="HALL", rv_dim=30)
PM("Poly Gates 16 x 12", seq(1, "DLY", 100, "1/16", SYNC, gate="LNG", atk=1, rel=10), seq(2, "TXT", 100, "1/16", EUC7[:12], gate="SHT", length=12, atk=1, rel=15),
   dry=100, dly=80, txt=85, dl_l="1/8", dl_r="1/8.", dl_fb=50, tx_heads=4, tx_size=0.5, rv_amt=22)
PM("Spin Cycle", seq(1, "SPIN SPD", 70, "1/4", [0, 30, 100, 30], length=4, atk=30), dry=100, out_mode="SPIN", sp_speed=20, sp_dist=90, dly=50, dl_l="1/8.", dl_r="1/4", dl_fb=40, rv_amt=25)
PM("Saturation Hits", seq(1, "M.AMT", 85, "1/16", FOUR, atk=1, dec=25, sus=0), dry=100, mf_type="TUBE", mf_amt=10, rv_amt=15)

# ---- texture patterns
PM("Rhythm Heads", seq(1, "TXT", 100, "1/16", EUC7, gate="LNG", atk=1, rel=10), seq(2, "TXT FLT", 70, "1/4", [0, 100, 40, 70], length=4, atk=30),
   dry=70, txt=95, tx_heads=6, tx_mode="MIXED", tx_size=0.75, tx_flt=-40, rv_amt=22)
PM("Crush Steps", seq(1, "TXT FX", 85, "1/8", RND, atk=2), dry=70, txt=90, tx_heads=4, tx_size=1, tx_fx="CRUSH", tx_fxa=5, rv_amt=15)
PM("Ring Sequence", seq(1, "TXT FX", 90, "1/16", SYNC, gate="SHT", atk=1, rel=25), dry=70, txt=90, tx_heads=5, tx_size=1.5, tx_fx="RINGMOD", tx_fxa=5, rv_amt=30, rv_type="HALL")
PM("Fold Pulse", seq(1, "TXT FX", 75, "1/8", EIGHT, atk=10, rel=25), seq(2, "BOOST", 50, "1/2", [0, 100], length=2, atk=60), dry=65, txt=85, tx_fx="FOLD", tx_fxa=10, tx_heads=4, tx_size=1, rv_amt=20)
PM("Grain Melody", seq(1, "GR PITCH", 50, "1/8", MELO, atk=1), dry=40, txt=100, tx_eng="FRAGMENT", fr_pitch=-12, fr_dens=85, fr_size=140, fr_spread=25, rv_amt=40, rv_shim=20)
PM("Grain Arp Up", seq(1, "GR PITCH", 50, "1/16", [0, 29, 50, 79, 100, 79, 50, 29], length=8, atk=1), dry=50, txt=95, tx_eng="FRAGMENT", fr_pitch=0, fr_dens=90, fr_size=90, fr_spread=20, rv_amt=35)
PM("Density Pulses", seq(1, "DENSITY", 85, "1/8", [100, 0, 40, 0, 100, 20, 0, 60], length=8, atk=3, rel=20), dry=55, txt=100, tx_eng="FRAGMENT", fr_dens=8, fr_size=70, fr_spread=60, fr_prand=3, rv_amt=35)
PM("Grain Size Morph", seq(1, "GR SIZE", 80, "1/1", RAMP, atk=20), seq(2, "SPREAD", 70, "1/2", [0, 100, 50, 100], length=4, atk=50), dry=45, txt=100, tx_eng="FRAGMENT", fr_size=15, fr_dens=70, fr_spread=10, rv_amt=40, rv_dim=70)
PM("Scatter Bursts", seq(1, "SPREAD", 100, "1/16", EUC5, gate="SHT", atk=1, rel=30), seq(2, "TXT", 100, "1/16", TRANCE, gate="LNG", atk=1, rel=8), dry=60, txt=95, tx_eng="FRAGMENT", fr_dens=75, fr_size=50, fr_spread=0, fr_rev=30, rv_amt=25)
PM("Vinyl Brake Bar", seq(1, "VINYL SPD", -25, "1/16", [0] * 12 + [25, 50, 75, 100], atk=10, rel=10), dry=0, txt=100, tx_eng="VINYL", tx_size=2, tx_speed=100, rv_amt=18)
PM("Vinyl Scratch 16ths", seq(1, "VINYL SPD", 62, "1/16", [100, 0, 100, 0, 0, 100, 100, 0], length=8, atk=3, rel=3), dry=30, txt=100, tx_eng="VINYL", tx_size=1, tx_speed=-150, rv_amt=12)
PM("Vinyl Half Speed", dry=0, txt=100, tx_eng="VINYL", tx_size=4, tx_speed=50, rv_amt=25, rv_type="HALL")
PM("Vinyl Wow", seq(1, "VINYL SPD", 2, "1/2", [0, 100, 30, 80], length=4, atk=80, rel=80), dry=0, txt=100, tx_eng="VINYL", tx_size=2, tx_speed=95, mf_type="TAPE SAT", mf_amt=45, rv_amt=15)

# ---- dub loop patterns (set REC to ADD, play, then REC OFF to keep the loop)
PM("Dub Loop Gate", seq(1, "DUB", 100, "1/16", SYNC, gate="LNG", atk=1, rel=10), dry=100, ovl=100, ov_rec="ADD", ov_size=4, dly=35, dl_l="1/8.", dl_r="1/4", dl_fb=35, rv_amt=20)
PM("Evolving Dub", seq(1, "TXT FLT", 80, "1/1", [0, 40, 100, 60], length=4, atk=80, rel=80), seq(2, "DLY FBK", 40, "1/2", [0, 100], length=2, atk=70, rel=70),
   dry=60, ovl=80, ov_rec="ADD", ov_size=8, txt=85, dly=55, tx_heads=5, tx_size=3, tx_flt=-50, dl_l="1/4", dl_r="1/8.", dl_fb=40, rv_amt=40, rv_dim=75)
PM("Loop Stutter Chop", seq(1, "DUB", 100, "1/32", [100, 0, 100, 0, 100, 100, 0, 0], length=8, gate="SHT", atk=1, rel=5), dry=100, ovl=100, ov_rec="ADD", ov_size=2, rv_amt=12)
PM("Loop Filter Ride", seq(1, "CUTOFF", 33, "1/4", [0, 30, 60, 100, 60, 30, 0, 100], length=8, atk=40), dry=80, ovl=100, ov_rec="ADD", ov_size=4, flt_cut=-70, flt_res=40, flt_slope="24dB", dly=40, dl_fb=40, rv_amt=22)
PM("Loop Into Grains", seq(1, "GR PITCH", 25, "1/4", [0, 100, 58, 0], length=4, atk=2), dry=40, ovl=60, ov_rec="ADD", ov_size=4, txt=100, tx_eng="FRAGMENT", fr_dens=80, fr_size=160, fr_spread=40, r_oo="ON", rv_amt=38, rv_shim=20)
PM("Dub Forget Wash", seq(1, "REV AMT", 45, "1/2", [0, 100], length=2, atk=75, rel=75), dry=100, ovl=100, ov_rec="FORGET", ov_size=8, rv_amt=30, rv_type="CATHEDRAL", rv_dim=80)
P("Dub Loop 4 Beats", dry=100, ovl=100, ov_rec="ADD", ov_size=4, dly=40, dl_l="1/8.", dl_r="1/4", dl_fb=35, rv_amt=20)
P("Loop Swap", dry=100, ovl=100, ov_rec="CHANGE", ov_size=4, dly=45, dl_l="1/4", dl_r="1/8.", dl_fb=40, rv_amt=22)

# ---- delays
P("Dub Echo", dry=100, dly=75, dl_l="1/4", dl_r="1/8.", dl_fb=50, dl_fltfb=-45, rv_amt=22, rv_type="ROOM")
P("Dotted Dub Throw", dry=100, dly=80, dl_l="1/4.", dl_r="1/4", dl_fb=58, dl_fltfb=-55, dl_tape=40, rv_amt=28)
P("Ping Pong Eighths", dry=100, dly=70, dl_l="1/8", dl_r="1/4", dl_fb=45, rv_amt=18, rv_type="ROOM")
P("Triplet Bounce", dry=100, dly=72, dl_l="1/4 T", dl_r="1/8 T", dl_fb=52, dl_fltfb=-20, rv_amt=20)
P("Tape Slowdown", dry=100, dly=78, dl_l="1/2", dl_r="1/4.", dl_fb=62, dl_fltfb=-60, dl_tape=90, rv_amt=30, rv_type="PLATE")
P("Wide Offset Cloud", dry=80, dly=85, dl_l="1/4", dl_r="1/4", dl_off=6, dl_fb=64, dl_fltfb=-30, rv_amt=35, rv_dim=70)
P("Ghost Repeats", dry=85, dly=70, dl_l="1/2", dl_r="1/2.", dl_fb=72, dl_fltfb=-70, rv_amt=42, rv_dim=75, rv_damp=70)
P("Snare Dub", dry=100, dly=80, dl_l="1/8.", dl_r="1/4", dl_fb=48, dl_fltfb=-40, rv_amt=38, rv_type="PLATE", rv_dim=40)
P("Vocal Throw", dry=100, dly=65, dl_l="1/4", dl_r="1/2", dl_fb=35, dl_fltfb=-25, rv_amt=30, rv_type="PLATE", rv_pre=20, mf_type="EXCITER", mf_amt=25)
P("Gated Noisy Input", dry=100, gt_thr=55, gt_atk=5, gt_rel=25, dly=60, dl_l="1/8.", dl_r="1/4", dl_fb=45, rv_amt=22)

# ---- textures
P("Metal Heads", dry=70, txt=90, tx_heads=6, tx_size=0.5, tx_flt=10, rv_amt=22)
P("Reverse Wash", dry=60, txt=100, tx_heads=5, tx_mode="REV", tx_size=3, tx_flt=-25, rv_amt=40, rv_dim=70)
P("Slow Heavy Sink", dry=60, txt=100, tx_mode="SLOW", tx_size=4, tx_flt=-45, tx_boost=30, dly=30, dl_fb=55, dl_fltfb=-50, rv_amt=35, rv_type="CATHEDRAL")
P("Bright Fast Swarm", dry=70, txt=80, tx_heads=8, tx_mode="FAST", tx_size=1.5, tx_flt=25, rv_amt=25, rv_shim=20)
P("Long Memory Drift", dry=50, txt=100, tx_size=8, tx_flt=-20, tx_boost=20, rv_amt=45, rv_dim=85)
P("Grain Cloud", dry=40, txt=100, tx_eng="FRAGMENT", fr_dens=85, fr_size=220, fr_spread=60, rv_amt=40, rv_dim=70, rv_shim=25)
P("Pad From One Hit", dry=0, txt=100, tx_eng="FRAGMENT", tx_size=3, fr_dens=90, fr_size=450, fr_spread=50, fr_fb=35, rv_amt=55, rv_dim=85, rv_shim=35, rv_type="CATHEDRAL")
P("Glitch Stutter", dry=70, txt=100, tx_eng="FRAGMENT", tx_size=0.5, fr_dens=25, fr_size=20, fr_spread=40, fr_rev=45, fr_prand=5, tx_fx="CRUSH", tx_fxa=30, rv_amt=15)
P("Backwards Grains", dry=50, txt=100, tx_eng="FRAGMENT", fr_dens=65, fr_size=180, fr_rev=100, fr_spread=50, rv_amt=35)

# ---- reverbs and tone
P("Vocal Plate", dry=100, rv_amt=32, rv_type="PLATE", rv_dim=45, rv_damp=40, rv_pre=25)
P("Cathedral Bloom", dry=70, rv_amt=60, rv_type="CATHEDRAL", rv_dim=90, rv_damp=55, rv_pre=15, rv_lim="ON")
P("Octave Shimmer", dry=70, rv_amt=55, rv_type="HALL", rv_dim=80, rv_shim=65, rv_pre=35, rv_damp=35, rv_lim="ON")
P("Lo-fi Tape", dry=100, dly=60, dl_l="1/8.", dl_r="1/8", dl_fb=45, dl_tape=70, flt_cut=-35, mf_type="TAPE SAT", mf_amt=60, rv_amt=18)
P("Driven Input Echo", dry=100, mf_pos="IN", mf_type="TUBE", mf_amt=65, dly=65, dl_l="1/4", dl_r="1/8.", dl_fb=45, dl_fltfb=-35, rv_amt=22)
P("Filtered Input Wash", dry=100, flt_pos="IN", flt_type="BP", flt_cut=10, flt_res=45, txt=70, tx_heads=5, tx_size=2, rv_amt=45, rv_dim=75)

# ---- everything together
PM("Ambient Machine", seq(1, "TXT FLT", 70, "1/1", [0, 50, 100, 30], length=4, atk=85, rel=85), seq(2, "SHIMMER", 60, "1/1", [100, 0, 50, 0], length=4, atk=85, rel=85),
   dry=40, ovl=60, ov_rec="ADD", ov_size=8, dly=65, txt=85, dl_l="1/2", dl_r="1/4.", dl_fb=60, dl_fltfb=-40, tx_heads=6, tx_size=5, tx_flt=-40, rv_amt=55, rv_dim=85, rv_type="CATHEDRAL")
PM("Broken Radio", seq(1, "DRY", 100, "1/32", RND, gate="SHT", atk=1, rel=6), seq(2, "CUTOFF", 90, "1/8", RND[::-1], atk=2), dry=100, flt_type="BP", flt_cut=-50, flt_res=60, mf_type="TAPE SAT", mf_amt=55, dly=40, dl_l="1/16", dl_r="1/8 T", dl_fb=45, rv_amt=18)
PM("Texture Into Delay Pulse", seq(1, "TXT", 100, "1/8", EIGHT, gate="LNG", atk=1, rel=12), dry=60, dly=75, txt=90, order="TEX>DLY", dl_l="1/4", dl_r="1/8.", dl_fb=55, tx_heads=4, tx_size=1.5, rv_amt=30)
json.dump({"presets": presets}, open(os.path.join(root, "presets.json"), "w"), indent=1)
print(len(presets), "presets")

# C header for the engine: the same bank, values as numbers (option labels become their index)
keys = [p["key"] for p in params]
with open(os.path.join(root, "src", "mori_presets.h"), "w") as f:
    f.write("/* generated by tools/gen_presets.py: do not edit */\n#pragma once\n")
    f.write("#define NFV %d\n#define NFACTORY %d\nstatic const int FKEY[NFV] = {%s};\n" % (len(keys), len(presets), ", ".join("K_" + k.upper() for k in keys)))
    f.write("static const char *const FNAME[NFACTORY] = {%s};\n" % ", ".join(json.dumps(p["name"]) for p in presets))
    f.write("static const float FVAL[NFACTORY][NFV] = {\n")
    for pr in presets:
        row = []
        for p in params:
            v = pr["values"][p["key"]]
            row.append(str(p["options"].index(v)) if p.get("options") else repr(float(v)))
        f.write("  {%s},\n" % ", ".join(row))
    f.write("};\n")
