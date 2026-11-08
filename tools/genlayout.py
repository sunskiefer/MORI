import json
L=json.load(open('/mnt/user-data/outputs/save/MORI_layout_2026-11-08_FINAL.json'))
P=json.load(open('params.json')); pl=P if isinstance(P,list) else P['params']
nm={p['key']:p['name'] for p in pl}
T={0:'PLAY',1:'DELAY + REVERB',2:'TEXTURE',3:'FRAGMENT',4:'ROUTING'}
o=["# MORI panel: generated from MORI_layout_2026-11-08_FINAL.json by genlayout.py (exact x/y/w/h from the panel editor)",""]
for p in L['pages']:
    o.append("[tab %s]"%T[p['screen']])
    o.append("art file=skinart/page%d.png x=0 y=86 w=1280 h=628 fit=stretch"%p['screen'])
    keys=[]
    for it in p['items']:
        cx=it['x']+it['w']//2; cy=it["y"]+156+it["h"]//2; k=it['id']; keys.append(k)
        lab=nm[k].replace('"','')
        if it['kind']=='k':
            o.append('knob cx=%d cy=%d r=%d label="%s" key=%s strip=skinart/knob_strip.png frames=129 ns=0 vs=0'%(cx,cy,58,lab,k))
        elif it['kind']=='s':
            o.append('toggle cx=%d cy=%d label="%s" key=%s img=skinart/sw_off.png img_on=skinart/sw_on.png w=%d h=%d ns=0'%(cx,cy,lab,k,it['w'],it['h']))
        else:
            o.append('button cx=%d cy=%d label="" key=%s img=skinart/bt_off.png img_on=skinart/bt_on.png w=%d h=%d'%(cx,cy,k,it['w'],it['h']))
    o.append('qlinks "%s" = %s'%(T[p['screen']],','.join(keys[:16])))
    o.append("")
open('layout.conf','w').write("\n".join(o))
