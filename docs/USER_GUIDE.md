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

## 2. The five pages

| Page | What is on it |
|---|---|
| PLAY | the four levels and MASTER, the DUB recorder, BLOCK, the filter, the master effect |
| DELAY + REVERB | both delay times, the delay character, the whole reverb |
| TEXTURE | the multi-head memory and its tone |
| FRAGMENT | the grain engine |
| ROUTING | the routing switches and the DELAY / TEXTURE order |

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

The loop is always the same length as your beats, so it stays in time with the project. If the host reports no tempo, MORI assumes 120 BPM.

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

TEXTURE keeps a short memory of what it hears and plays it back in new ways. There are two engines, chosen with **ENGINE**:
M.HEAD (this page) and FRAGMENT (next page).

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

## 8. Using it musically

**A dub echo on a snare or a vocal.** Put MORI on a send. Set DRY to zero. Set LEFT to 1/4 and RIGHT to 1/8 dotted, AMNESIA to about 50 percent, FDBACK to the left so each repeat gets darker, then add a little reverb. Ride the DLY level by hand to throw the echo only where you want it.

**Bending time.** In DELAY set MODE to TAPE and TAPE AMT high. Change LEFT while the sound is playing: the echoes glide and bend in pitch like tape being slowed down. Switch to JUMP for hard, rhythmic changes.

**Build a loop that evolves.** Set DUB SIZE to 4 beats and REC to ADD. Play a phrase while the loop goes round, then set REC to OFF to keep it. Turn REC to FORGET to let it dissolve over one pass, or to CHANGE to swap in new material. Send the loop into TEXTURE (DUB>TEX on) and turn DRY down for a texture made only from your own loop.

**Freeze a moment.** Play a chord or a hit into TEXTURE, then switch BLOCK on. The memory stops recording and the heads keep turning on what it holds. Move TXT FLT, MODE and HEADS to reshape the frozen sound, then switch BLOCK off to let the next sound in.

**A pad from a single hit.** Use the FRAGMENT engine with DENSITY high, GR SIZE long, SPREAD about half and SHIM in the reverb. A short sound fed into it becomes a slow cloud.

**Glitch and stutter.** FRAGMENT with a short GR SIZE (about 20 ms), low DENSITY, and REVERSE and P RAND turned up scatters a beat into bright, unpredictable fragments. Add CRUSH on the texture for grit.

**Slow, deep, heavy.** TEXTURE mode SLOW, SIZE long, a low TXT FLT sweep and some FDBACK on the delay make the sound sink. Switch to FAST for the opposite.

**Reshape the order.** ORDER decides whether the delay feeds the texture or the other way round. With DLY>TEX the echoes are blurred into a cloud; with TEX>DLY the cloud itself is echoed in time with the project.

**Rhythmic gating of the whole sound.** Use the Q-Links to move CUTOFF and RES while the effect plays, or automate them. A band-pass (TYPE: BP) with RES raised sweeps like a voice.

## 9. Q-Links and automation

Every control is a plugin parameter, so every one can be automated and every one can be reached from the Q-Links. The Q-Links 1 to 8
take the first row of controls on the page, and 9 to 16 the second row, in the order shown on the screen.

## 10. Tips

* Use MORI on a send, and set DRY to zero, so you can control how much of the effect you hear from the send level.
* Start with ROUTING at its defaults and turn parts off one by one to learn what each contributes.
* The feedback controls (AMNESIA, FRAGMENT FDBACK) build up. Raise them slowly.
* The DUB loop and the delay are in time with the project tempo; the texture is not. The texture lengths are in seconds.
