# Building MORI

MORI is built with the open mpc-vst-plugins framework (wrapper, skin builder and release packer).

1. `python3 tools/gen_params.py` writes `params.json` and `src/mori_keys.h`.
2. `python3 tools/genlayout.py` writes `layout.conf` from `layout/MORI_layout_FINAL.json` (the panel as arranged in the panel editor).
3. `gen_vst.py vst.json` builds `params.h`, the skin and the plugin-list entry.
4. Compile for ARMv7 hard-float, glibc 2.31 or older, for example with zig:
   `python3 -m ziglang cc -target arm-linux-gnueabihf.2.31 -O2 -s -fPIC -shared -fvisibility=hidden -std=gnu11 -Ibuild -I<framework>/wrapper -Isrc src/mori.c <framework>/wrapper/vst2_wrap.c -lm -lpthread -Wl,--no-undefined -o build/mori.so`
5. `release.py` packs `MORI-<version>-mpc-armv7.zip`.

The knob filmstrips must stay 126 px, 128 frames: other sizes made the knobs slide on the Force.
