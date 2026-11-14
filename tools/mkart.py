import json
from PIL import Image, ImageDraw, ImageFont
A='/mnt/user-data/attachments/e0ddf512-381c-594a-86d6-73fdf36c5556/'
L=json.load(open('/mnt/user-data/outputs/save/MORI_layout_2026-11-08_FINAL.json'))
FONT='/home/claude/sunskiefer/bustedxxl/third_party/mpc-vst-plugins/tools/html_art/fonts/TitilliumWeb-SemiBold.ttf'
IM={'back.png':Image.open(A+'77bd7dd2-image.png').convert('RGBA'),'cat.png':Image.open(A+'6af8ecbc-image.png').convert('RGBA')}
HDR=70
for p in L['pages']:
    bg=Image.new('RGBA',(1280,628),(0,0,0,255))
    for im in sorted(p.get('images',[]),key=lambda i:i.get('z',0)) if False else p.get('images',[]):
        t=IM[im['file']].resize((im['w'],im['h']),Image.LANCZOS)
        bg.alpha_composite(t,(0,0)) if False else None
        # paste with clipping
        x,y=im['x'],im['y']+HDR
        c=Image.new('RGBA',bg.size,(0,0,0,0)); c.paste(t,(x,y)); bg.alpha_composite(c)
    d=ImageDraw.Draw(bg); f=ImageFont.truetype(FONT,36)
    for lb in p['labels']:
        txt=lb['text'].replace('_',' ').upper()
        cx=lb['x']+lb['w']/2; cy=lb['y']+HDR+lb['h']/2
        d.text((cx,cy),txt,font=f,fill=(220,189,133,255),anchor='mm')
    bg.convert('RGB').save('skinart/page%d.png'%p['screen'])
    print(p['screen'],[i['file'] for i in p.get('images',[])],p.get('images') and [k for k in p['images'][0]])
# switch + button art (141x141), scaled 4x
def sw(on):
    S=4;W=141*S;im=Image.new('RGBA',(W,W),(0,0,0,0));d=ImageDraw.Draw(im)
    pw,ph=int(141*.46*S),int(141*.8*S);x0=(W-pw)//2;y0=(W-ph)//2
    d.rounded_rectangle([x0,y0,x0+pw,y0+ph],12*S,fill=(10,10,10,255),outline=(220,189,133,255),width=3*S)
    tw=int(pw*.6);th=int(ph*.38);tx=x0+int(pw*.2);ty=y0+int(ph*(.52 if on else .10))
    d.rounded_rectangle([tx,ty,tx+tw,ty+th],6*S,fill=(40,40,40,255),outline=(220,189,133,255),width=2*S)
    im.resize((141,141),Image.LANCZOS).save('skinart/sw_%s.png'%('on' if on else 'off'))
sw(0);sw(1)
def bt(on):
    S=4;W=141*S;im=Image.new('RGBA',(W,W),(0,0,0,0));d=ImageDraw.Draw(im);r=int(141*.72*S/2);c=W//2
    d.ellipse([c-r,c-r,c+r,c+r],fill=(34,34,34,255) if not on else (70,60,40,255),outline=(138,122,90,255) if not on else (220,189,133,255),width=3*S)
    im.resize((141,141),Image.LANCZOS).save('skinart/bt_%s.png'%('on' if on else 'off'))
bt(0);bt(1)
