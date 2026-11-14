#!/usr/bin/env python3
"""Layout pictures of the pages for the README: the skin's page backgrounds with the controls drawn where layout.conf puts them.
They are made from the skin files, not photographed on a Force. Usage: make_screens.py <out dir> [first tab index]"""
import os, shlex, sys
from PIL import Image, ImageDraw, ImageFont
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__))); os.chdir(root)
S = "build/skin/L'Cronx - VST - MORI/Plugin Skins/"
F = ImageFont.truetype('/home/claude/mvp/tools/html_art/fonts/TitilliumWeb-SemiBold.ttf', 32); FB = ImageFont.truetype('/home/claude/mvp/tools/html_art/fonts/TitilliumWeb-SemiBold.ttf', 52)
GOLD = (220, 189, 133)
out = sys.argv[1]; first = int(sys.argv[2]) if len(sys.argv) > 2 else 0
os.makedirs(out, exist_ok=True)
tabs = []; cur = None
for line in open('layout.conf'):
    line = line.strip()
    if line.startswith('[tab '): cur = []; tabs.append(cur)
    elif cur is not None and line and not line.startswith(('#', 'qlinks', 'art ')): cur.append(line)
strip = Image.open('skinart/knob_strip.png').convert('RGBA'); fh = strip.width
def img(p): return Image.open(p).convert('RGBA')
for ti, ws in enumerate(tabs):
    if ti < first: continue
    bg = Image.open(S + 'sh_bg_%d.png' % ti).convert('RGBA'); d = ImageDraw.Draw(bg); si = 0
    for w in ws:
        t = shlex.split(w); k = t[0]; a = dict(x.split('=', 1) for x in t[1:] if '=' in x)
        if 'when' in a and not a['when'].endswith(('Z1',)): continue
        cx, cy = int(a.get('cx', 0)), int(a.get('cy', 0)) - 86
        if k == 'knob':
            fr = strip.crop((0, fh * 64, fh, fh * 65)); bg.alpha_composite(fr, (cx - fh // 2, cy - fh // 2))
        elif k in ('toggle', 'button'):
            im = img(a['img']); bg.alpha_composite(im, (cx - im.width // 2, cy - im.height // 2))
        elif k == 'slider_v':
            tr = img(a['base']); th = img(a['img']); h = int(a['h']); v = (100, 0, 100, 100, 0, 100, 100, 0, 100, 0, 100, 100, 0, 100, 0, 100)[si % 16]; si += 1
            bg.alpha_composite(tr, (cx - tr.width // 2, cy - h // 2)); y = cy + h // 2 - 12 - int(v / 100 * (h - 24)); bg.alpha_composite(th, (cx - th.width // 2, y - 12))
        elif k == 'stepper':
            ww, hh = int(a['w']), int(a['h']); d.rounded_rectangle([cx - ww // 2, cy - hh // 2, cx + ww // 2, cy + hh // 2], 10, fill=(10, 10, 10), outline=GOLD, width=3)
            d.text((cx, cy), '3  Trance Gate', font=FB, fill=GOLD, anchor='mm')
        elif k == 'readout':
            key = a.get('key'); ww = int(a['w'])
            if key == 'preset_name': d.text((cx - ww // 2, cy), '3  Trance Gate', font=F, fill=GOLD, anchor='lm')
            elif key == 'st_info': d.text((cx + ww // 2, cy), 'DUB EMPTY  120.0 BPM SYNC', font=F, fill=GOLD, anchor='rm')
    bg.convert('RGB').save(os.path.join(out, 'page_%d.png' % ti))
print('screens:', len(tabs) - first)
