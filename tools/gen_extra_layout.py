#!/usr/bin/env python3
"""The pages added after the user's five: SEQ 1, GATES 1, SEQ 2, GATES 2, FLOW, SOUND, TOUCH (PRESET is layout_preset.conf).
Writes layout_extra.conf, the page backgrounds (same back.png, gold 36 px labels, no cat), the fader art and the touch bar picture."""
import os
from PIL import Image, ImageDraw, ImageFont
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__))); os.chdir(root)
A = '/mnt/user-data/attachments/e0ddf512-381c-594a-86d6-73fdf36c5556/'
FONT = '/home/claude/mvp/tools/html_art/fonts/TitilliumWeb-SemiBold.ttf'
GOLD = (220, 189, 133, 255)
back = Image.open(A + '77bd7dd2-image.png').convert('RGBA').resize((1300, 646), Image.LANCZOS)
f = ImageFont.truetype(FONT, 36)
BAR = (40, 72, 1200, 180)      # the touch bar on the TOUCH page, page pixels (pad/layout.txt)
def bar_img(w, h):
    S = 2; im = Image.new('RGBA', (w * S, h * S), (8, 8, 8, 255)); d = ImageDraw.Draw(im); ff = ImageFont.truetype(FONT, 36 * S)
    for z in range(3):
        x0 = z * w * S // 3
        d.rounded_rectangle([x0 + 6 * S, 6 * S, x0 + w * S // 3 - 6 * S, h * S - 6 * S], 14 * S, fill=(16, 16, 15, 255), outline=GOLD, width=3 * S)
        d.text((x0 + w * S // 6, h * S // 2), ('FILTER', 'DELAY', 'SPACE')[z], font=ff, fill=(138, 122, 90, 255), anchor='mm')
    return im.resize((w, h), Image.LANCZOS)
def page(name, labels, bar=False):
    bg = Image.new('RGBA', (1280, 628), (0, 0, 0, 255)); c = Image.new('RGBA', bg.size, (0, 0, 0, 0)); c.paste(back, (-12, 62)); bg.alpha_composite(c)
    if bar: bg.paste(bar_img(BAR[2], BAR[3]), (BAR[0], BAR[1]))
    d = ImageDraw.Draw(bg)
    for cx, cy, t in labels: d.text((cx, cy - 1), t, font=f, fill=GOLD, anchor='mm')
    bg.convert('RGB').save('skinart/%s.png' % name)
R1 = [82, 326, 570, 813, 1057]; R2 = [178, 432, 686, 940, 1194]; R3 = [93, 345, 597, 849, 1101]; X6 = [110, 322, 534, 746, 958, 1170]
K = 'strip=skinart/knob_strip.png frames=129 ns=0 vs=0'
T = 'img=skinart/sw_off.png img_on=skinart/sw_on.png w=141 h=141 ns=0'
o = []
def knob(x, y, lab, key, extra=''): o.append('knob cx=%d cy=%d r=58 label="%s" key=%s %s%s' % (x, y, lab, key, K, extra))
def tog(x, y, lab, key, extra=''): o.append('toggle cx=%d cy=%d label="%s" key=%s %s%s' % (x, y, lab, key, T, extra))
def tab(name, art): o.extend(['', '[tab %s]' % name, 'art file=skinart/%s.png x=0 y=86 w=1280 h=628 fit=stretch' % art])
# SEQ pages
SEQL = ['DEST', 'DEPTH', 'DIV', 'LENGTH', 'GATE']; SEQK = ['dest', 'amt', 'div', 'len', 'gate']
ENVL = ['ATK', 'DEC', 'SUS', 'REL', 'SPEED']; ENVK = ['atk', 'dec', 'sus', 'rel', 'spd']
page('page6', [(x, 414, t) for x, t in zip(R2, SEQL)] + [(x, 597, t) for x, t in zip(R3, ENVL)])
GL = [str(i) for i in range(1, 17)] + ['DEST 2', 'DEPTH 2']
page('page9', [(X6[i % 6], (232, 414, 597)[i // 6], t) for i, t in enumerate(GL)])
for n in (1, 2):
    q = 'sq%d_' % n
    tab('SEQ %d' % n, 'page6')
    for i in range(16): o.append('slider_v cx=%d cy=248 w=56 h=150 label="" key=s%d_%d img=skinart/fd_thumb.png base=skinart/fd_track.png ns=0 vs=0' % (70 + i * 76, n, i + 1))
    for x, k, l in zip(R2, SEQK, SEQL): knob(x, 414, l, q + k)
    for x, k, l in zip(R3, ENVK, ENVL): knob(x, 597, l, q + k)
    o.append('qlinks "SEQ %d" = %s' % (n, ','.join(q + k for k in SEQK + ENVK)))
    o.append('qlinks "STEPS %d" = %s' % (n, ','.join('s%d_%d' % (n, i) for i in range(1, 17))))
    tab('GATES %d' % n, 'page9')
    keys = ['t%d_%d' % (n, i) for i in range(1, 17)] + [q + 'dest2', q + 'amt2']
    for i, k in enumerate(keys): knob(X6[i % 6], (232, 414, 597)[i // 6], GL[i], k)
    o.append('qlinks "GATES %d" = %s' % (n, ','.join(keys[:16])))
    o.append('qlinks "DEST 2 %d" = %s' % (n, ','.join(keys[16:])))
# FLOW
FL1 = ['GATE THR', 'GATE ATK', 'GATE REL', 'GATE FLR', 'VINYL SPD']; FL2 = ['FLT POS', 'FX POS', 'OUTPUT', 'SPIN SPD', 'SPIN DST']
page('page8', [(x, 232, t) for x, t in zip(R1, FL1)] + [(x, 414, t) for x, t in zip(R2, FL2)] + [(93, 597, 'REV LIM')])
tab('FLOW', 'page8')
for x, k, l in zip(R1, ['gt_thr', 'gt_atk', 'gt_rel', 'gt_flr', 'tx_speed'], FL1): knob(x, 232, l, k)
knob(R2[0], 414, 'FLT POS', 'flt_pos'); knob(R2[1], 414, 'FX POS', 'mf_pos'); tog(R2[2], 414, 'OUTPUT', 'out_mode')
knob(R2[3], 414, 'SPIN SPD', 'sp_speed'); knob(R2[4], 414, 'SPIN DST', 'sp_dist'); tog(93, 597, 'REV LIM', 'rv_lim')
o.append('qlinks "FLOW" = gt_thr,gt_atk,gt_rel,gt_flr,tx_speed,flt_pos,mf_pos,out_mode,sp_speed,sp_dist,rv_lim')
# SOUND
S1 = ['IN GAIN', 'GATE HOLD', 'WIDTH', 'DL RANGE', 'DL MS']; S2 = ['SHIM PIT', 'TAPE FB', 'REV NOISE', 'EXC FREQ']
page('page10', [(x, 232, t) for x, t in zip(R1, S1)] + [(x, 414, t) for x, t in zip(R2, S2)] + [(93, 597, 'EDITOR')])
tab('SOUND', 'page10')
knob(R1[0], 232, 'IN GAIN', 'in_gain'); knob(R1[1], 232, 'GATE HOLD', 'gt_hold'); knob(R1[2], 232, 'WIDTH', 'tx_width'); tog(R1[3], 232, 'DL RANGE', 'dl_range'); knob(R1[4], 232, 'DL MS', 'dl_ms')
for x, k, l in zip(R2, ['rv_shp', 'rv_tfb', 'rv_noise', 'mf_freq'], S2): knob(x, 414, l, k)
o.append('toggle cx=93 cy=597 label="EDITOR" key=ed_show img=skinart/bt_off.png img_on=skinart/bt_on.png w=141 h=141 ns=0')
o.append('readout cx=720 cy=597 w=1000 h=80 label="" key=ed_addr box=0 tsize=40 tcolor=dcbd85 tweight=semibold talign=left when=ed_show:ON')
o.append('qlinks "SOUND" = in_gain,gt_hold,tx_width,dl_range,dl_ms,rv_shp,rv_tfb,rv_noise,mf_freq,ed_show')
# TOUCH: the bar's rectangle stays empty of controls (the plugin draws and reads it itself)
# three zones along the bar, each moves three things at once: nine RANGE knobs, no panels, no zone switch
SL = [(R2[i], 414) for i in range(5)] + [(R3[i], 597) for i in range(4)]
TL = ['Z%d %s' % (z, c) for z in (1, 2, 3) for c in 'ABC']; TK = ['tz%d_r%s' % (z, c) for z in (1, 2, 3) for c in 'abc']
page('page11', [(x, y, t) for (x, y), t in zip(SL, TL)], bar=True)
tab('TOUCH', 'page11')
for (x, y), l, k in zip(SL, TL, TK): knob(x, y, l, k)
o.append('qlinks "ZONE 1" = tz1_ra,tz1_rb,tz1_rc')
o.append('qlinks "ZONE 2" = tz2_ra,tz2_rb,tz2_rc')
o.append('qlinks "ZONE 3" = tz3_ra,tz3_rb,tz3_rc')
open('layout_extra.conf', 'w').write('\n'.join(o) + '\n')
# fader art, touch bar picture and its place
S = 4
t = Image.new('RGBA', (56 * S, 150 * S), (0, 0, 0, 0)); d = ImageDraw.Draw(t)
d.rounded_rectangle([18 * S, 2 * S, 38 * S, 148 * S], 8 * S, fill=(10, 10, 10, 255), outline=(138, 122, 90, 255), width=2 * S)
t.resize((56, 150), Image.LANCZOS).save('skinart/fd_track.png')
h = Image.new('RGBA', (56 * S, 24 * S), (0, 0, 0, 0)); d = ImageDraw.Draw(h)
d.rounded_rectangle([2 * S, 2 * S, 54 * S, 22 * S], 6 * S, fill=(34, 34, 34, 255), outline=GOLD, width=3 * S); d.line([10 * S, 12 * S, 46 * S, 12 * S], fill=GOLD, width=2 * S)
h.resize((56, 24), Image.LANCZOS).save('skinart/fd_thumb.png')
os.makedirs('pad', exist_ok=True)
bar_img(BAR[2], BAR[3]).resize((747, 747), Image.LANCZOS).save('pad/grid.png')
open('pad/layout.txt', 'w').write("# MORI touch bar: its place on the TOUCH page, in page pixels (the page is 1280 x 628; 0,0 is its top-left).\n# id  x   y   w    h\npad   %d %d %d %d\n" % BAR)
print('extra pages written')
