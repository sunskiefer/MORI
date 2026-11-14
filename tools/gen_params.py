#!/usr/bin/env python3
# Single source for MORI's parameter list: writes params.json and src/mori_keys.h (enum, same order).
import json
DIV = ["2 BAR","1 BAR","1/2","1/4","1/8","1/16","1/32","1/64",
       "1 BAR.","1/2.","1/4.","1/8.","1/16.","1/32.","1/64.",
       "1 BAR T","1/2 T","1/4 T","1/8 T","1/16 T","1/32 T","1/64 T"]
def pct(k,n,d=0,lo=0,hi=100): return dict(key=k,name=n,min=lo,max=hi,default=d,display="int",unit="%")
def opt(k,n,o,d=0): return dict(key=k,name=n,options=o,default=d)
def tog(k,n,d=1): return dict(key=k,name=n,options=["OFF","ON"],default=d)
S = []
def sec(label, items): S.append((label, items))
sec("LEVELS",[pct("dry","DRY",100),pct("ovl","DUB",100),pct("dly","DLY",60),pct("txt","TXT",70),pct("master","MASTER",80)])
sec("FILTER",[pct("flt_cut","CUTOFF",0,-100,100),pct("flt_res","RES",0),opt("flt_slope","SLOPE",["12dB","24dB"],0),opt("flt_type","TYPE",["LP+HP","BP"],0)])
sec("DELAY",[opt("dl_l","LEFT",DIV,4),opt("dl_r","RIGHT",DIV,11),pct("dl_fb","AMNESIA",40),pct("dl_fltfb","FDBACK",0,-100,100),
             opt("dl_mode","MODE",["TAPE","JUMP"],0),pct("dl_tape","TAPE AMT",30),dict(key="dl_off",name="OFFSET",min=0,max=7,default=0,display="int")])
sec("TEXTURE",[opt("tx_eng","ENGINE",["M.HEAD","FRAGMENT","VINYL"],0),dict(key="tx_heads",name="HEADS",min=2,max=8,default=4,display="int"),
               opt("tx_mode","MODE",["MIXED","FWD","REV","FWD+REV","SLOW","FAST"],0),dict(key="tx_size",name="SIZE",min=0.25,max=8,default=2,unit="s"),
               pct("tx_flt","TXT FLT",0,-100,100),pct("tx_fxa","TXT FX",0),opt("tx_fx","EFFECT",["NONE","FOLD","CRUSH","RINGMOD"],0),
               pct("tx_boost","BOOST",0),tog("tx_block","BLOCK",0)])
sec("FRAGMENT",[pct("fr_dens","DENSITY",50,5,100),dict(key="fr_pitch",name="PITCH",min=-24,max=24,default=0,display="int",unit="st"),
                dict(key="fr_size",name="GR SIZE",min=10,max=500,default=150,display="int",unit="ms"),pct("fr_spread","SPREAD",30),
                dict(key="fr_prand",name="P RAND",min=0,max=12,default=0,display="int",unit="st"),pct("fr_rev","REVERSE",0),pct("fr_fb","FDBACK",0)])
sec("DUB",[opt("ov_rec","REC",["OFF","ADD","CHANGE","FORGET"],0),dict(key="ov_clear",name="CLEAR",type="trigger",momentary=True),
               dict(key="ov_size",name="DUB SIZE",min=1,max=16,default=4,display="int",unit="beats")])
sec("REVERB",[pct("rv_amt","REV AMT",30),pct("rv_dim","REV DIM",50),pct("rv_damp","DAMP",50),opt("rv_type","TYPE",["CLASSIC","HALL","ROOM","PLATE","CATHEDRAL"],0),
              pct("rv_pre","PRE FLT",0),pct("rv_shim","SHIM %",0)])
sec("MASTER FX",[opt("mf_type","M.FX",["OFF","EXCITER","TAPE SAT","TUBE"],0),pct("mf_amt","M.AMT",30)])
sec("ROUTING",[tog("r_do","DRY>OUT",1),tog("r_dd","DRY>DLY",1),tog("r_dt","DRY>TEX",1),tog("r_dr","DRY>REV",1),
               tog("r_oo","DUB>OUT",1),tog("r_od","DUB>DLY",1),tog("r_ot","DUB>TEX",1),tog("r_or","DUB>REV",1),
               tog("r_ldo","DLY>OUT",1),tog("r_ldt","DLY>TEX",1),tog("r_ldr","DLY>REV",1),
               tog("r_to","TXT>OUT",1),tog("r_tr","TXT>REV",1),tog("r_td","TXT>DLY",1),
               opt("order","ORDER",["DLY>TEX","TEX>DLY"],0)])
sec("PRESET",[dict(key="preset",name="PRESET",min=0,max=1023,default=0,display="int"),
              dict(key="preset_name",name="PRESET NAME",display="string",min=0,max=1,default=0),
              dict(key="ps_prev",name="PREV",type="trigger",momentary=True,step_of="preset",step_delta=-1),
              dict(key="ps_next",name="NEXT",type="trigger",momentary=True,step_of="preset",step_delta=1),
              dict(key="ps_prev10",name="-10",type="trigger",momentary=True,step_of="preset",step_delta=-10),
              dict(key="ps_next10",name="+10",type="trigger",momentary=True,step_of="preset",step_delta=10),
              dict(key="ps_save",name="SAVE NEW",type="trigger",momentary=True),
              dict(key="ps_over",name="OVERWRITE",type="trigger",momentary=True),
              dict(key="ps_del",name="DELETE",type="trigger",momentary=True)])
DEST=["OFF","DRY","DUB","DLY","TXT","DLY FBK","DLY TONE","TXT FLT","TXT FX","BOOST","VINYL SPD","DENSITY","GR PITCH","GR SIZE","SPREAD","CUTOFF","RES","REV AMT","REV DIM","SHIMMER","M.AMT","SPIN SPD","SPIN DST","MASTER"]
SQDIV=["1/1","1/2","1/4","1/8","1/16","1/32","1/4 T","1/8 T","1/16 T"]
for n in (1,2):
    q="sq%d_"%n
    sec("SEQ %d"%n,[opt(q+"dest","DEST",DEST,0),pct(q+"amt","DEPTH",50,-100,100),opt(q+"div","DIV",SQDIV,3),
        dict(key=q+"len",name="LENGTH",min=1,max=16,default=16,display="int"),opt(q+"gate","GATE",["LNK","LNG","SHT"],0),
        pct(q+"atk","ATK",5),pct(q+"dec","DEC",30),pct(q+"sus","SUS",100),pct(q+"rel","REL",20)]+
        [dict(key="s%d_%d"%(n,i),name="STEP %d"%i,min=0,max=100,default=100,display="int",unit="%") for i in range(1,17)])
sec("FLOW",[pct("gt_thr","GATE THR",0),pct("gt_atk","GATE ATK",10),pct("gt_rel","GATE REL",30),pct("gt_flr","GATE FLR",0),
            opt("flt_pos","FLT POS",["OUT","IN","MIX"],0),opt("mf_pos","FX POS",["OUT","IN","MIX","LINK"],0),opt("out_mode","OUTPUT",["STEREO","SPIN"],0),
            pct("sp_speed","SPIN SPD",30),pct("sp_dist","SPIN DST",60),tog("rv_lim","REV LIM",0),
            dict(key="tx_speed",name="VINYL SPD",min=-200,max=200,default=100,display="int",unit="%")])
for n in (1,2):
    sec("SEQ %d GATES"%n,[opt("sq%d_spd"%n,"SPEED",["/4","/2","x1","x2","x4","x8","x16"],2),opt("sq%d_dest2"%n,"DEST 2",DEST,0),pct("sq%d_amt2"%n,"DEPTH 2",50,-100,100)]+
        [opt("t%d_%d"%(n,i),"TYPE %d"%i,["SEQ","OFF","SHT","LNG","LNK"],0) for i in range(1,17)])
sec("SOUND",[dict(key="in_gain",name="IN GAIN",min=-24,max=24,default=0,display="int",unit="dB"),pct("gt_hold","GATE HOLD",0),pct("tx_width","WIDTH",100),
             opt("dl_range","DL RANGE",["SYNC","MS"],0),dict(key="dl_ms",name="DL MS",min=50,max=4000,default=1000,display="int",unit="ms"),
             opt("rv_shp","SHIM PIT",["+12","+7","+5","-12"],0),pct("rv_tfb","TAPE FB",0),pct("rv_noise","REV NOISE",0),pct("mf_freq","EXC FREQ",50)])
T=[dict(key="tch_x",name="TOUCH X",min=-5,max=5,default=0),dict(key="tch_y",name="TOUCH Y",min=-5,max=5,default=0),opt("tz_sel","ZONE",["Z1","Z2","Z3"],0)]
for z in (1,2,3):
    ZD={1:[("CUTOFF",-70),("RES",35),("M.AMT",40)],2:[("DLY FBK",45),("DLY TONE",-50),("DLY",40)],3:[("REV AMT",60),("REV DIM",45),("SHIMMER",50)]}[z]
    for e,(dn,rg) in zip("abc",ZD): T+=[opt("tz%d_d%s"%(z,e),"DEST %s"%e.upper(),DEST,DEST.index(dn)),pct("tz%d_r%s"%(z,e),"RANGE %s"%e.upper(),rg,-100,100)]
    T+=[opt("tz%d_drv"%z,"DRIVER",["X","X REV","Y","Y REV"],{1:1,2:0,3:0}[z]),opt("tz%d_rel"%z,"RELEASE",["HOLD","RETURN"],1)]
sec("TOUCH",T)
sec("INFO",[dict(key="ed_addr",name="EDITOR",display="string",min=0,max=1,default=0),dict(key="st_info",name="STATUS",display="string",min=0,max=1,default=0),tog("ed_show","EDITOR",0)])
P=[];secs=[]
for label,items in S:
    secs.append({"label":label,"keys":[i["key"] for i in items]}); P+=items
json.dump({"name":"MORI","params":P,"sections":secs},open("params.json","w"),indent=1)
with open("src/mori_keys.h","w") as f:
    f.write("/* generated by gen_params.py: do not edit */\n#pragma once\nenum {\n")
    for p in P: f.write("  K_%s,\n"%p["key"].upper())
    f.write("  K_COUNT\n};\nstatic const float KMINV[K_COUNT]={%s};\nstatic const float KMAXV[K_COUNT]={%s};\n" % (",".join("%ff"%(0 if "options" in p else p.get("min",0)) for p in P), ",".join("%ff"%((len(p["options"])-1) if "options" in p else p.get("max",1)) for p in P)))
    f.write("static const char *const KEYS[K_COUNT]={\n")
    for p in P: f.write('  "%s",\n'%p["key"])
    f.write("};\n")
    f.write("static const float KDEF[K_COUNT]={\n")
    for p in P: f.write("  %.6ff,\n"%float(p.get("default",0)))
    f.write("};\n")
print(len(P),"params")
