# PRESET page background: the same back.png, labels and gold as the other pages, no cat
from PIL import Image, ImageDraw, ImageFont
A='/mnt/user-data/attachments/e0ddf512-381c-594a-86d6-73fdf36c5556/'
FONT='/home/claude/sunskiefer/bustedxxl/third_party/mpc-vst-plugins/tools/html_art/fonts/TitilliumWeb-SemiBold.ttf'
import os
if not os.path.exists(FONT): FONT='/home/claude/mvp/tools/html_art/fonts/TitilliumWeb-SemiBold.ttf'
back=Image.open(A+'77bd7dd2-image.png').convert('RGBA').resize((1288,646),Image.LANCZOS)
bg=Image.new('RGBA',(1280,628),(0,0,0,255))
c=Image.new('RGBA',bg.size,(0,0,0,0)); c.paste(back,(-12,-8+70)); bg.alpha_composite(c)
d=ImageDraw.Draw(bg); f=ImageFont.truetype(FONT,36)
for cx,cy,t in [(150,414,'-10'),(390,414,'PREV'),(640,414,'PRESET'),(890,414,'NEXT'),(1130,414,'+10'),
                (240,597,'SAVE NEW'),(640,597,'OVERWRITE'),(1040,597,'DELETE')]:
    d.text((cx,cy-1),t,font=f,fill=(220,189,133,255),anchor='mm')
bg.convert('RGB').save('skinart/page5.png')
