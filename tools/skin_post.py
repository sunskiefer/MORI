#!/usr/bin/env python3
"""MORI: after the skin is built, paint the two 2x2 marker blocks into the TOUCH page's background(s), so the plugin can find that
page on the display and show/read the touch bar only there (same markers as the Pseudo X/Y pad). Usage: skin_post.py <skin dir>."""
import json, os, sys
from PIL import Image
MARK_TL = [(23, 7, 91), (91, 7, 23), (7, 91, 23), (23, 91, 7)]
MARK_BR = [(91, 23, 7), (7, 23, 91), (61, 7, 61), (7, 61, 61)]
d = sys.argv[1]; p = os.path.join(d, "Plugin Skins"); p = p if os.path.isdir(p) else d
here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
tabs = [l.strip()[5:-1] for l in open(os.path.join(here, "layout.conf")) if l.startswith("[tab ")]
ti = tabs.index("TOUCH"); n = 0
for fn in sorted(os.listdir(p)):
    if not (fn.startswith("sh_bg_%d" % ti) and fn.endswith(".png")) or not fn[len("sh_bg_%d" % ti)] in "._": continue
    im = Image.open(os.path.join(p, fn)).convert("RGB")
    assert im.size == (1280, 628), (fn, im.size)
    for k in range(4):
        im.putpixel((k & 1, k >> 1), MARK_TL[k]); im.putpixel((1278 + (k & 1), 626 + (k >> 1)), MARK_BR[k])
    im.save(os.path.join(p, fn)); n += 1
assert n, "no TOUCH page background found"
print("skin_post: markers on %d TOUCH background(s)" % n)
