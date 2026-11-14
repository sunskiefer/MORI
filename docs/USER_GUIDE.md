# MORI user guide

MORI is an effect for the Akai Force and MPC OS. It listens to whatever you send it, remembers it in three different ways, and
blends the memories back with the original. This guide explains how the sound travels, what every control does, and how to
play it as an instrument.

## 1. How the sound travels

```
 input ──┬─────────────── DRY ─────────────────────────────────────┐
         │                                                          │
         ├──> DUB loop (records the input) ── DUB ──────────────────┤
         │                                                          │
         └─ routing matrix ─> DELAY ─> TEXTURE ─> (or TEXTURE ─> DELAY, set by ORDER)
                                  each one can also be heard on its own (DLY, TXT)
 all four parts are mixed ─> REVERB ─> FILTER ─> MASTER FX ─> MASTER
```

Four things can be heard: the **dry** input, the **DUB** loop, the **DELAY**, and the **TEXTURE**. The four level knobs on
the PLAY page (DRY, DUB, DLY, TXT) set how loud each is. After the mix the sound goes through the reverb, the filter, the
master effect and the master level.

The **ROUTING** page decides which part feeds which. It is a patch bay with switches: for example DRY>DLY says whether the
dry input is sent into the delay, and DUB>REV says whether the DUB loop is sent into the reverb.

## 2. The pages

| Page | What is on it |
|---|---|
| PLAY | the four levels and MASTER, the DUB recorder, BLOCK, the filter, the master effect |
| DELAY + REVERB | both delay times, the delay character, the whole reverb |
| TEXTURE | the multi-head memory and its tone |
| FRAGMENT | the grain engine |
| ROUTING | the routing switches and the DELAY / TEXTURE order |
| SEQ 1, SEQ 2 | the two step modulators: 16 faders and their settings |
| GATES 1, GATES 2 | a gate type for every step, and each modulator's second destination |
| FLOW | the input gate, VINYL SPD, where the filter and master effect sit, SPIN output, reverb limiter |
| SOUND | input gain, texture width, delay range, shimmer pitch, tape feedback, reverb noise, exciter frequency, the EDITOR button |
| TOUCH | the touch bar and its three zones |
| PRESET | pick, save and delete presets |

The band at the top of every page shows the current preset on the left (a star after the name means the sound has been changed since it
was loaded) and, on the right, the state of the DUB loop and the tempo MORI is getting from the project, for example
`DUB PLAY  120.0 BPM SYNC`. SYNC means MORI is following the song position, FREE means the transport is stopped, NO TEMPO means the
project is not sending a tempo and MORI is using 120 BPM.

The Q-Links 1 to 8 follow the first row of controls on the page and 9 to 16 the second, in the order shown on screen.

## 3. PLAY

* **DRY** – level of the unprocessed input. Turn it to zero to hear only the effect, which is how MORI works best on a send.
* **DUB** – level of the DUB loop.
* **DLY** – level of the delay.
* **TXT** – level of the texture.
* **MASTER** – the final level. A soft limiter sits after it, so pushing it rounds the peaks instead of clipping them.

### DUB: the loop recorder

DUB is a loop that follows your project tempo. It records what comes in, and plays it back over and over.

* **DUB SIZE** – the loop length, 1 to 16 beats at the project tempo.
* **REC** – what the loop does with new sound:
  * **OFF** – plays the loop and records nothing.
  * **ADD** – overdubs: new sound is added on top, and the older layers fade slightly each time round, so a loop slowly renews itself.
  * **CHANGE** – replaces the loop with what comes in, as it plays.
  * **FORGET** – erases the loop as it plays, so it fades out in one pass.
* **CLEAR** – a tap that wipes the whole loop at once.

The loop is always the same length as your beats and, while the project plays, it follows the song position: whatever you record
lands on the bar it was played on and comes back on that bar. With the transport stopped the loop runs free. If the project
reports no tempo, MORI assumes 120 BPM (the band at the top of the page then says NO TEMPO).

### BLOCK

BLOCK stops the TEXTURE memory from recording. The texture keeps playing what it already holds, so it becomes a frozen sound
you can keep reshaping with the TEXTURE and FRAGMENT controls. Turn BLOCK off to let it listen again.

### FILTER

One knob, **CUTOFF**, does two jobs. In the centre the filter is off. Turning left closes a low-pass filter (the sound gets darker),
turning right closes a high-pass filter (the sound gets thinner).

* **RES** – resonance, the whistle at the cutoff.
* **SLOPE** – 12 dB or 24 dB per octave. 24 dB is steeper and more dramatic.
* **TYPE** – LP+HP is the two-way sweep above. BP is a band-pass: the knob moves the centre of a band that lets only a slice of
  the sound through.

### MASTER FX

* **M.FX** – OFF, EXCITER (adds a little air to the top), TAPE SAT (soft tape-like saturation that also rounds the highs) or TUBE (warm, slightly lopsided saturation).
* **M.AMT** – how much of it.

## 4. DELAY + REVERB

### DELAY

A stereo delay that follows the project tempo.

* **LEFT / RIGHT** – the delay time of each side as a note value: from 2 bars down to 1/64, with straight, dotted and triplet versions of each. Different values on the two sides give you ping-pong and polyrhythms.
* **OFFSET** – 0 to 7: stretches the right side a little more at each step, so the two sides drift apart and the echoes smear into a wide stereo cloud.
* **AMNESIA** – feedback: how many repeats you hear. Low gives one or two echoes. High keeps the sound circling for a long time. It is limited so it will not run away.
* **FDBACK** – a filter inside the feedback loop. To the left the repeats get darker each time round (like an old tape echo), to the right they get thinner and lighter. In the centre they stay as they are.
* **MODE** – TAPE or JUMP. In TAPE, when you change a delay time the echoes glide to the new time, so the pitch bends like a tape machine being sped up or slowed down. In JUMP the time changes instantly, with no bend.
* **TAPE AMT** – how long the glide takes in TAPE mode, from a quick slur to a slow dive.

### REVERB

An eight-line reverb. Each source can be switched on or off for the reverb in ROUTING.

* **TYPE** – CLASSIC, HALL, ROOM, PLATE or CATHEDRAL. ROOM is small and close, CATHEDRAL is the largest and longest.
* **REV AMT** (the REV% knob) – how much reverb is added.
* **REV DIM** – the size of the space; it also lengthens the tail.
* **DAMP** – how quickly the tail loses its high frequencies. High values make a soft, dark tail.
* **PRE FLT** – removes low frequencies before the reverb, so the bass stays tight and the reverb stays clear. Turn it up for a thin, airy wash.
* **SHIM %** – adds an octave-up shimmer to the tail. A little is angelic; a lot is a rising cloud.

## 5. TEXTURE

TEXTURE keeps a short memory of what it hears and plays it back in new ways. There are three engines, chosen with **ENGINE**:
M.HEAD (this page), FRAGMENT (next page) and VINYL (one playback head whose speed is the VINYL SPD knob on the FLOW page: 100 is
normal speed, 0 stops it, negative values play backwards, like a hand on a record).

### M.HEAD: the multi-head memory

Imagine a loop of tape of a few seconds with several playback heads that all read the same recording, each at its own speed
and direction. Some read slower (lower in pitch), some faster (higher), some backwards. Together they make a swirling,
harmonic cloud out of whatever you played.

* **SIZE** – how long the memory is, from a quarter of a second to 8 seconds. Short sizes give a metallic, pitched repeat. Long sizes give a drifting wash.
* **HEADS** – how many heads, 2 to 8. More heads is thicker and more complex.
* **MODE** – what the heads do:
  * **MIXED** – a blend of speeds and directions, the all-round setting.
  * **FWD** – all heads play forwards at different speeds.
  * **REV** – all heads play backwards.
  * **FWD+REV** – the heads alternate forwards and backwards.
  * **SLOW** – low, slow, heavy: the heads play at half and quarter speed.
  * **FAST** – high and bright: the heads play at double and quadruple speed.
* **TXT FLT** – a filter on the texture only. Left darkens it, right removes the lows.
* **TXT FX** and **EFFECT** – a texture-only effect and how much of it: NONE, FOLD (a wavefolder that adds bright harmonics), CRUSH (reduces the resolution for a gritty, digital sound) or RINGMOD (a metallic, bell-like ring).
* **BOOST** – raises the texture level by up to three times, for when the cloud is quiet.

## 6. FRAGMENT

FRAGMENT cuts the memory into small pieces called grains and scatters them. Choose it with **ENGINE** on the TEXTURE page.

* **DENSITY** – how many grains start per second. Low is sparse, individual blips. High is a continuous cloud.
* **GR SIZE** – the length of each grain, from 10 to 500 milliseconds. Short grains sound granular and buzzy. Long grains sound like slices of the original.
* **PITCH** – shifts every grain up or down by up to two octaves.
* **P RAND** – each grain gets a random pitch offset of up to an octave up or down from the PITCH value. A little makes it shimmer; a lot makes it unpredictable.
* **SPREAD** – how far back in the memory the grains are taken from. Low takes the most recent sound. High scatters them across the whole memory.
* **REVERSE** – the chance that a grain plays backwards.
* **FDBACK** – feeds the grains back into the memory, so the cloud feeds on itself and builds. Use it carefully; it grows.

SIZE (on the TEXTURE page) sets the length of the memory the grains are taken from.

## 7. ROUTING

Fourteen switches, plus ORDER. Every switch is on by default, so out of the box everything feeds everything.

| Switch | What it does |
|---|---|
| DRY>OUT, DRY>DLY, DRY>TEX, DRY>REV | where the dry input goes: the output, the delay, the texture, the reverb |
| DUB>OUT, DUB>DLY, DUB>TEX, DUB>REV | the same for the DUB loop |
| DLY>OUT, DLY>TEX, DLY>REV | the delay's output goes to the mix, the texture or the reverb |
| TXT>OUT, TXT>REV, TXT>DLY | the texture's output goes to the mix, the reverb or the delay |
| ORDER | DLY>TEX puts the delay first and feeds it into the texture. TEX>DLY puts the texture first and feeds it into the delay |

Only one of DLY>TEX and TXT>DLY has an effect at a time, depending on ORDER: in DLY>TEX the delay feeds the texture, and in TEX>DLY the texture feeds the delay.

## 8. SEQ 1 and SEQ 2: the step modulators

A step modulator moves a control for you, in time with the project. Each one has 16 steps; the height of a step's fader is how far
that step pushes the control. This is what turns MORI from a delay and a reverb into something that makes patterns.

* **The 16 faders** – draw the pattern. A fader at the bottom is a silent step.
* **DEST** – which control the modulator moves: the four levels and MASTER, delay feedback and tone, texture filter, effect and boost,
  VINYL SPD, grain density, pitch, size and spread, CUTOFF and RES, reverb amount, size and shimmer, the master effect amount, and the
  SPIN speed and distance.
* **DEPTH** – how far, and in which direction. On a level (DRY, DUB, DLY, TXT, MASTER) the modulator works as a gate: a full fader
  lets the sound through and an empty one closes it by the DEPTH amount (a negative DEPTH turns that round). On every other control
  the fader is added to where the knob stands (a negative DEPTH subtracts).
* **DIV** – the length of one step, from a whole bar to a 32nd, with triplets.
* **SPEED** – multiplies the step rate, from a quarter to sixteen times.
* **LENGTH** – how many of the 16 steps play before the pattern starts again. Two modulators with different lengths drift against each other.
* **GATE** – how long each step lasts: LNK (the whole step, joined to the next), LNG (three quarters) or SHT (one quarter).
* **ATK, DEC, SUS, REL** – the shape of each step: how fast it rises, falls to its sustain level, and lets go.

While the project plays the modulators follow the song position; with the transport stopped they run free at the project tempo.

### GATES 1 and GATES 2

One knob per step sets that step's own gate: SEQ (use the modulator's GATE), OFF (skip the step), SHT, LNG or LNK. The last two
knobs, **DEST 2** and **DEPTH 2**, give the modulator a second control to move with the same pattern.

## 9. FLOW

* **GATE THR, GATE ATK, GATE REL, GATE FLR** – a noise gate on the input. THR at zero switches it off; raise it until hiss between
  notes disappears. FLR is how much sound is still let through when the gate is closed.
* **VINYL SPD** – the playback speed of the VINYL texture engine.
* **FLT POS** – where the filter sits: OUT (on everything, at the end), IN (on the input, before the loop, delay and texture) or
  MIX (after the delay and texture, before the reverb).
* **FX POS** – the same for the master effect, plus LINK (it follows the filter).
* **OUTPUT** – STEREO, or SPIN: the sound circles between left and right. **SPIN SPD** and **SPIN DST** set how fast and how wide.
* **REV LIM** – a limiter on the reverb, for long shimmering tails.

## 10. SOUND

* **IN GAIN** – input level, up to 24 dB louder or quieter.
* **GATE HOLD** – how long the input gate stays open after the sound drops under the threshold.
* **WIDTH** – stereo width of the texture, from mono to full.
* **DL RANGE** and **DL MS** – SYNC keeps the delay times as note values. MS turns LEFT and RIGHT into free times: the knobs span from
  zero to DL MS milliseconds.
* **SHIM PIT** – the pitch of the reverb's shimmer: an octave up, a fifth up, a fourth up or an octave down.
* **TAPE FB** – feeds the reverb back into the delay, for echoes that bloom.
* **REV NOISE** – a little noise into the reverb, for air.
* **EXC FREQ** – where the exciter (MASTER FX) starts to add brightness.
* **EDITOR** – shows or hides the address of the browser editor (see 13).

## 11. TOUCH

The bar at the top of the page is played with a finger. It is cut into **three zones side by side**: the left third is **FILTER**,
the middle third **DELAY**, the right third **SPACE**. Every zone moves **three things at once**, so one finger gesture changes a
whole group of settings, not a single knob. There is no zone switch and no per-zone panel: all three zones are on the page together.

### The nine knobs

The page shows nine RANGE knobs, three per zone, named after the zone and the slot: **Z1 A, Z1 B, Z1 C** (FILTER), **Z2 A, B, C** (DELAY),
**Z3 A, B, C** (SPACE). A RANGE says how far that zone pushes its target while the finger is on it:

* turn it to the right to push the target up, to the left to push it down, centre = nothing;
* the push is **added** to wherever the target's own knob is set, so the bar never fights your settings: let go (with RELEASE on RETURN) and the sound is back to what the knobs say.

### What each zone does out of the box

| Zone | A | B | C |
|---|---|---|---|
| 1 FILTER (left) | CUTOFF, -70: closes the filter | RES, +35: more resonance as it closes | M.AMT, +40: more master FX |
| 2 DELAY (middle) | DLY FBK, +45: more repeats | DLY TONE, -50: darker repeats | DLY, +40: more delay level |
| 3 SPACE (right) | REV AMT, +60: more reverb | REV DIM, +45: bigger room | SHIMMER, +50: more shimmer |

### The FILTER zone opens from left to right

Put a finger at the **left edge** of the FILTER zone and the filter is **closed** (a dark, resonant low-pass). Slide to the **right** and it
**opens** until it is fully open at the zone's right edge. This is because zone 1's DRIVER is **X REV** (see below) and its CUTOFF range is
negative. Lift the finger and, with RELEASE = RETURN, the filter swings back open by itself.

### Settings that are not on the Force page

The DEST (what a slot moves), DRIVER and RELEASE settings of each zone are in the **browser editor** (section 13, TOUCH group) and are
stored in presets. The Force page keeps only the nine RANGE knobs so that nothing repeats. In the editor:

* **DEST A, B, C** – any of the 23 modulation destinations (the same list as the step modulators).
* **DRIVER** – what position the zone follows:
  * **X** – the finger's place across the zone: 0 at its left edge, 1 at its right edge.
  * **X REV** – the same, the other way round: 1 at the left edge, 0 at the right. (FILTER uses this.)
  * **Y** – the finger's height in the bar: 0 at the bottom, 1 at the top.
  * **Y REV** – the other way round.
* **RELEASE** – **HOLD** keeps the last value when the finger lifts; **RETURN** lets it fall back to nothing in about a quarter of a second.

### Playing it from a Q-Link or automation

The Q-Link pages **ZONE 1, ZONE 2, ZONE 3** each carry that zone's three RANGE knobs. The finger position is also available as the controls
TOUCH X and TOUCH Y in the browser editor and to automation, so a sequence in your DAW can play the bar.

### The bar only exists on the TOUCH page

The bar is drawn by the plugin over MPC's screen. It is shown **only while MORI's TOUCH page is on screen**. If you go to another MORI page,
another MPC screen, or another plugin, the bar is cleared at once, and it is switched off again every time until the display accepts it. (Before
0.6.4 a part of the bar could stay behind on other screens and other plugins; that was a bug.)

## 12. PRESET

MORI has 69 factory presets (the first four, M.HEAD, DEJA-VU, FRAGMENT and VINYL, are the factory FLOWs of the MEMENTO manual: their engine and zone order, everything else neutral) and room for 964 of your own.

* **The name at the top** – the current preset. The arrows beside it step to the previous and next one.
* **PREV, NEXT, -10, +10** and the **PRESET** knob – move through the list. The data wheel works on the preset number too.
* **SAVE NEW** – stores the sound as it is now as a new preset (USER 001, USER 002 and so on; rename it in the browser editor).
* **OVERWRITE** – replaces the selected preset with the sound as it is now. **DELETE** removes it. Factory presets cannot be changed.

Your presets are kept in the plugin's `user` folder on the Force and stay there when you install a new version of MORI. The factory
presets are also listed in MPC's own PRESET menu.

About half the factory bank are patterns made by the step modulators (Trance Gate, Sidechain Verb, Filter Stairs, Grain Melody,
Vinyl Brake Bar, Poly Gates 16 x 12). The DUB loop presets start with REC on ADD: play into them, then turn REC to OFF to keep the loop.

## 13. The browser editor

MORI carries its own editor: a page you open in a browser on a phone, tablet or computer.

1. Connect the Force to your network (Wi-Fi or cable) and load MORI.
2. On the SOUND page press **EDITOR**. An address appears, for example `http://192.168.1.23:8765`. Press EDITOR again to hide it.
3. On a device on the same network, type that address into a browser.

What it does:

* Every control of MORI, grouped like the pages. Moving one in the browser moves it on the Force, and moving one on the Force (screen,
  Q-Link or automation) moves it in the browser.
* **SEQ 1 / SEQ 2** – draw the 16 bars with a finger or the mouse, set each step's gate type, and keep patterns in 8 slots per
  modulator (Save 1 to 8, Load 1 to 8).
* **TOUCH** – the touch bar, playable from the browser.
* **PRESETS** – the whole list. Click one to load it. Type a name and press Save new, or Rename selected. Overwrite and Delete work on
  your own presets. **Export my presets** downloads them as a text file; **Import presets** adds the presets from such a file, so
  they can be backed up and shared.

Notes: the editor works only while MORI is loaded. It has no password, so anyone on the same network can open it. A second MORI in
the same project gets the next address (8766, and so on).

## 14. Using it musically

**A trance gate from a pad.** SEQ 1: DEST DRY, DEPTH 100, DIV 1/16, GATE LNG. Draw a pattern with gaps. Add a dotted delay and a plate
reverb so the gaps fill with echoes.

**Sidechain without a compressor.** SEQ 1: DEST REV AMT, DEPTH about -70, DIV 1/16, four steps falling from full to empty, LENGTH 4.
The reverb ducks on every beat and swells back.

**A melody from one note.** FRAGMENT engine, PITCH at -12. SEQ 1: DEST GR PITCH, DEPTH 50, DIV 1/8. Fader heights become notes
(a quarter of the way up is a fifth, half way an octave).

**Tape stop at the end of the bar.** VINYL engine, DRY at zero. SEQ 1: DEST VINYL SPD, DEPTH -25, DIV 1/16, the last four faders
rising. The sound slows to a halt and snaps back on the next bar.

**Two patterns that drift.** SEQ 1 on DLY with LENGTH 16, SEQ 2 on TXT with LENGTH 12. They line up again only every three bars.

**Play the bar.** On the TOUCH page slide a finger across zone 1 to sweep the filter, into zone 2 to push the echoes, into zone 3
to open the reverb. Set a zone's RELEASE to RETURN for effects that fall back when you let go.


**A dub echo on a snare or a vocal.** Put MORI on a send. Set DRY to zero. Set LEFT to 1/4 and RIGHT to 1/8 dotted, AMNESIA to about 50 percent, FDBACK to the left so each repeat gets darker, then add a little reverb. Ride the DLY level by hand to throw the echo only where you want it.

**Bending time.** In DELAY set MODE to TAPE and TAPE AMT high. Change LEFT while the sound is playing: the echoes glide and bend in pitch like tape being slowed down. Switch to JUMP for hard, rhythmic changes.

**Build a loop that evolves.** Set DUB SIZE to 4 beats and REC to ADD. Play a phrase while the loop goes round, then set REC to OFF to keep it. Turn REC to FORGET to let it dissolve over one pass, or to CHANGE to swap in new material. Send the loop into TEXTURE (DUB>TEX on) and turn DRY down for a texture made only from your own loop.

**Freeze a moment.** Play a chord or a hit into TEXTURE, then switch BLOCK on. The memory stops recording and the heads keep turning on what it holds. Move TXT FLT, MODE and HEADS to reshape the frozen sound, then switch BLOCK off to let the next sound in.

**A pad from a single hit.** Use the FRAGMENT engine with DENSITY high, GR SIZE long, SPREAD about half and SHIM in the reverb. A short sound fed into it becomes a slow cloud.

**Glitch and stutter.** FRAGMENT with a short GR SIZE (about 20 ms), low DENSITY, and REVERSE and P RAND turned up scatters a beat into bright, unpredictable fragments. Add CRUSH on the texture for grit.

**Slow, deep, heavy.** TEXTURE mode SLOW, SIZE long, a low TXT FLT sweep and some FDBACK on the delay make the sound sink. Switch to FAST for the opposite.

**Reshape the order.** ORDER decides whether the delay feeds the texture or the other way round. With DLY>TEX the echoes are blurred into a cloud; with TEX>DLY the cloud itself is echoed in time with the project.

**Rhythmic gating of the whole sound.** Use the Q-Links to move CUTOFF and RES while the effect plays, or automate them. A band-pass (TYPE: BP) with RES raised sweeps like a voice.

## 15. Q-Links and automation

Every control is a plugin parameter, so every one can be automated and every one can be reached from the Q-Links. The Q-Links 1 to 8
take the first row of controls on the page, and 9 to 16 the second row, in the order shown on the screen.

## 16. Tips

* Use MORI on a send, and set DRY to zero, so you can control how much of the effect you hear from the send level.
* Start with ROUTING at its defaults and turn parts off one by one to learn what each contributes.
* The feedback controls (AMNESIA, FRAGMENT FDBACK) build up. Raise them slowly.
* The DUB loop, the delay and the step modulators are in time with the project tempo; the texture is not. The texture lengths are in seconds.
* If the band at the top says NO TEMPO, everything timed runs at 120 BPM.
