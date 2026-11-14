# Changelog

## 0.6.4 (2026-11-14)
Tested on an Akai Force (2026-11-14): filter zone direction and the touch bar fix confirmed.

* **Touch bar left behind**: part of the bar stayed on the screen when you left the TOUCH page and showed over other plugins. The bar's picture is now cleared before the plane is switched off, the switch-off is repeated until the display accepts it, and the bar is cleared again on exit.
* **Filter zone direction**: the FILTER zone now opens from left to right (closed at its left edge, open at its right). New DRIVER choices X REV and Y REV make any zone run the other way.
* **TOUCH page**: no ZONE knob, no per-zone panels and no TOUCH X knob; nine RANGE knobs (Z1 A to Z3 C) sit on one page, and every zone moves three controls at once. Documented in section 11 of the guide.
* **Sound clean-up**: fast texture heads are low-passed so they no longer alias into harsh highs; resonance and filter level are limited; cutoff and levels are smoothed; texture and delay feedback and the master and DUB sums use a soft limiter instead of hard clipping; the join of the texture memory is faded.
* **Presets**: the 65 presets are kept and tamed (caps on resonance, feedback, shimmer and effect amounts, and on the wet parts added together); 4 added from the MEMENTO manual's factory FLOWs (M.HEAD, DEJA-VU, FRAGMENT, VINYL): 69 in all.

## 0.6.1 (2026-11-14)
Not yet tested on a Force.

* Two step modulators (SEQ 1, SEQ 2): 16 steps, 23 destinations plus a second destination each, division, speed, length, gate type per sequence and per step, ADSR. They follow the song position.
* Touch bar (TOUCH page): three zones, up to three destinations each, by position or height, hold or return.
* Browser editor served by the plugin: every control live in both directions, step drawing, 8 pattern slots per modulator, preset naming, rename, export and import. An EDITOR button on the SOUND page shows and hides its address.
* PRESET page; 65 factory presets (about half are modulator patterns), also in MPC's PRESET menu; up to 964 user presets kept in the plugin's `user` folder across updates.
* DUB loop follows the song position, so it records and plays on the bar.
* Third texture engine, VINYL (one head with variable speed and direction); BLOCK now fades across the join of the frozen memory; texture stereo width.
* Input gain and noise gate (threshold, attack, release, hold, floor).
* Filter position OUT / IN / MIX; master effect position OUT / IN / MIX / LINK; exciter frequency.
* SPIN output (stereo), reverb limiter, shimmer pitch, tape feedback, reverb noise; delay times in milliseconds as an option.
* The band at the top of every page shows the preset name, the DUB loop state and the tempo received from the project.
* New pages keep the look of the first five (same knobs, switches, letters and gold); the first five pages are unchanged.

## 0.3.1 (2026-11-08)
First public release.

* Five pages: PLAY, DELAY + REVERB, TEXTURE, FRAGMENT, ROUTING. Panel laid out by hand, every control where the panel editor put it.
* DUB layer (a tempo-synced loop you can record into), stereo tempo delay, TEXTURE (multi-head memory and fragment grains), 8-line reverb with shimmer, bipolar filter, master effects.
* 14-way routing matrix and a DELAY / TEXTURE order switch.
* Black and gold knobs and switches, large readable labels.
* Knob filmstrips use the same file type as the other L'Cronx plugins (126 px, 128 frames).
