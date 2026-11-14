# Building MORI

MORI is built with the open mpc-vst-plugins framework (wrapper, skin builder and release packer).

1. `python3 tools/gen_params.py` writes `params.json` and `src/mori_keys.h`.
2. `python3 tools/genlayout.py` writes `layout.conf` from `layout/MORI_layout_FINAL.json` (the panel as arranged in the panel editor).
3. `gen_vst.py vst.json` builds `params.h`, the skin and the plugin-list entry.
4. Compile for ARMv7 hard-float, glibc 2.31 or older, for example with zig:
   `python3 -m ziglang cc -target arm-linux-gnueabihf.2.31 -O2 -s -fPIC -shared -fvisibility=hidden -std=gnu11 -Ibuild -I<framework>/wrapper -Isrc src/mori.c <framework>/wrapper/vst2_wrap.c -lm -lpthread -Wl,--no-undefined -o build/mori.so`
   For 0.6.x add `src/xyz_pad.c` to the sources and `-ldl` to the libraries.
5. Other generated files: `tools/gen_presets.py` (factory bank: `presets.json`, `src/mori_presets.h`), `tools/gen_web.py` (the editor page and parameter list: `src/mori_web_data.h`), `tools/gen_extra_layout.py` (the pages after ROUTING, called by `genlayout.py`), `tools/skin_post.py` (the TOUCH page markers, run by `gen_vst.py`).
6. `release.py --extra pad:pad --user-data user` packs `MORI-<version>-mpc-armv7.zip`.

The knob filmstrips must stay 126 px, 128 frames: other sizes made the knobs slide on the Force.
