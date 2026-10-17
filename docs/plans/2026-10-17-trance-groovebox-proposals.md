# A trance groovebox for the FM-1: three proposals

Status: proposal, for discussion. Nothing here is built or measured yet unless it says so. The
measurements quoted come from X0X's own notes (docs/plans/2026-10-04-*.md) and from other FM-1
firmwares; everything else is an estimate to be replaced by the measurements in milestone M0.

Target: the best FM-1 firmware for 1996-99 trance and progressive house, with goa, the 2020s
throwback (Oprofessionell, Bliss Inc) and Framewerk-style progressive breaks as secondary targets.
Reference tracks: Cass & Slide "Perception", Taucher "Waters", Bedrock "Set in Stone", Oprofessionell
"Gen Z", and T.G.ViRUS "KiloMix '98" (ReBirth 2.0's demo song) as a spirit, not a template.

---

## 0. The short version

**Recommendation: Proposal B, "Tracks"**: Elektron-style machine tracks on X0X's chassis.

- **12 uniform tracks:** 11 sound tracks, one per black key, plus an FX track that sequences the
  effects.
  - Each sound track runs a *machine* (909 drum voices, an ACID 303, a JP-style SAW, a VA, a 2-op
    FM, a SAMPLE/slice player).
  - Behind the machine every track has the same chain and the same pages: filter, amp, drive,
    a chorus/flanger/phaser insert, sends, ducking, two LFOs and a step modulator (which is
    also the trance gate).
- **One sequencer for every track:**
  - per-track length and speed
  - p-locks, conditions, retrigs, micro-timing, slides
  - phrase-aware fills (last bar of 8/16/32)
- **Generators are trig sources, not machines.** Any track can be driven by STEPS, EUCLID, GRIDS,
  ARP, ACID (TB-3PO), TURING, BREAK (BB Gen's slice shuffler) or PHRASE (a curated library with
  style maps). They all share SEED / MUTATE / EVOLVE / PRINT.
- **A harmony lane per pattern** (key, scale, up to 8 chord changes):
  - basslines, arps, chords and generated lines follow it;
  - one-bar riffs turn into progressions, which is how KiloMix gets chords out of two
    monophonic 303s;
  - generated parts stay in key.
- **The step modulator ("STEP", after ReBirth's PCF)** on every track: a tempo-synced step pattern
  with a shape per step, routed to amp, cutoff or anything else. Trance gates, PCF filter rhythms
  and rhythmic ducking are all the same mechanism, with a library of patterns.
- **Song mode in three layers, each one cheap:**
  - Elektron-style rows (pattern, per-track pattern overrides, repeats, length, mutes, transpose,
    tempo);
  - Deluge-style live capture into those rows;
  - song-length automation lanes that hold both drawn 16-64-bar ramps and recorded knob riding.
- **Texture and FX sounds** (beds, risers, zaps, ear candy) come from a few small additions rather
  than a new machine: a pitch/mod envelope, noise colours, a comb filter, sound locks, a SCATTER
  source for sparse ornaments, LAND-ON trigs, and effects abuse on the FX track (section 4.11).
- **CPU is budgeted by construction:**
  - each machine has a measured cost per voice, and the project can't exceed the budget;
  - voices render batched by machine, from RAM;
  - the per-track mix is fixed-order, so the result is the same whichever core rendered what;
  - when core 1 is missing, the budget halves gracefully.

Proposal A ("X0X-T", specialist parts) is the fast, safe way to a good trance sound, but it is
the special-cased architecture you want to avoid. Proposal C ("Unison", one deep universal voice
plus a mod matrix) is the most orthogonal on paper. It loses on 303/909 authenticity, CPU per voice
and speed of tweaking with four knobs. Section 6 compares them.

---

## 1. What we're designing for

### 1.1 The music, reduced to requirements

From the genre research (sources at the end; values are starting points, not gospel):

| Need | Where it comes from | Primitive it implies |
|---|---|---|
| Supersaw leads, stabs and pads | System F and Paul van Dyk era, "Waters", "Gen Z" | **SAW** machine: 7 saws, JP-8000 detune ratios and the Szabo mix curve, pitch-tracking HPF, random phase at note-on |
| Juno pads with chorus, PWM | Juno-60/106 everywhere (PvD, BT, goa) | **VA** with PWM + sub; **BBD chorus** send (I / II / I+II) |
| Offbeat bass, rolling 16th bass (KBBB), prog "rolling" bass | Trance / goa / "Set in Stone", "Perception" | VA mono + LP24 + short env; **bass rhythm library**; harmony **root follow** |
| Acid: one or two 303s | Goa ("a lot have two"), KiloMix | **ACID** machine (Open303, exists), two instances in budget |
| Dub chords (m7/m9/sus) into feedback delay | Bedrock, Way Out West | chord trigs + harmony lane; **delay with HP/LP in the loop, 50-70 % feedback** |
| Plucks and 16th arps into a 3/16 ping-pong | "For an Angel", "Perception" | **ARP** source with beat-pattern library; stereo **ping-pong delay**, synced, dotted/triplet |
| Trance gate, PCF | Gated pads; KiloMix's PCF on 303 #2 | **STEP** modulator on every track |
| 909 drums, tuned and driven; crash on the 1; snare rolls | Everything | 909 voices (9W9, exists); **retrigs with rate/velocity ramps**; phrase-aware conditions |
| Noise risers, 8-16-bar filter openings, 32-64-bar rides | All the references; prog house's slow builds | NOISE (VA osc mode) + **song automation lanes** |
| Big halls (2.5-6 s, pre-delay 20-60 ms), plates on claps | Lexicon/Quadraverb era | **hall FDN** (X0X's ROOM, extended) |
| Phaser/flanger on pads, hats, goa sweeps | The era's shared phasers, goa | **MOD FX** insert on every track (chorus / flanger / phaser) and on the group **BUS** |
| Chopped, filtered breaks with a 909 kick under them | Prog breaks, Bliss Inc | **SAMPLE/slice** machine + **BREAK** source (BB Gen) |
| Ducking | 2020s revival (lightly in 90s trance) | per-track **DUCK** amount from a chosen source track |
| 3-step cells over 4/4, polymeter | Goa, prog, KiloMix patterns 8-15 | per-track length/speed, "free-run" vs "reset per bar/phrase" |
| Almost no swing | Trance/goa straight, prog hats at 52-56 % | per-track swing on/off (the `per-part-swing` branch) |

Tempos: prog house 122-132, trance 132-140, goa 135-150, the revival 128-155 ("Gen Z" is 153),
prog breaks 125-135. Scales: minor and Dorian; Phrygian, Phrygian dominant, harmonic minor and
double harmonic for goa.

What the five references ask for in particular:

- **"Perception"** (1999): a bass that "takes a long time to wander in", then a slow build. That
  needs song lanes that run 32+ bars and a mute that releases at a phrase boundary.
- **"Waters"** (1996): floaty melodic trance, an offbeat bass, big pads, a long breakdown. Needs
  SAW/VA pads, a hall, and a lead that follows the chords.
- **"Set in Stone"** (1997, 11 minutes): dub stabs with the cutoff ridden by hand, a rolling 16th
  bass, a hypnotic sequenced line, 32-bar-plus DJ intro and outro. Needs chord trigs, a feedback
  delay, recorded knob riding, and long song rows.
- **"Gen Z"** (2022, 153 BPM): fast 16th bass, pads, goa-style resonant riffs, washy reverb, an
  ambient breakdown. Needs ducking and a hall at high tempo.
- **KiloMix '98** (parsed from the original .rbs):
  - 143 BPM; swing on the 909 hats only.
  - 303 #1 is a resonant lead through distortion, ridden constantly: about 6,000 cutoff moves in
    547 bars, up to 250 per 8 bars. 303 #2 is a bright bass/pluck shaped by the PCF.
  - The two 303s play 3-step cells; transposed one-bar patterns imply i-iv-VII-V and Dorian
    progressions.
  - A 3/16 delay sends on everything but the kick.
  - Fills: 16 accented snares before a change, a crash pattern at each section start.
  - What a modern take adds: real sustained harmony, a reverb, a sub under the 303s, ducking,
    stereo delay.

The lesson from KiloMix for song mode: **knob riding at 1/16 resolution has to survive into the
song**, not only bar-level pattern changes. That's why song lanes are sparse event lists rather
than a value per bar (section 4.8).

### 1.2 The box

| | What we have | What it means |
|---|---|---|
| CPU | 2 x 360 MHz (measured), single-precision FPU, no SIMD, divide in one instruction, no sqrt; about 8,160 cycles per sample per core at 44.1 kHz | X0X's numbers calibrate the host profiler: 1 host-ns per sample is about 0.27 % of a core. One Open303 with 2x oversampling and drive is about 19 % of a core; ROOM is about 7.5 % |
| Code | XIP from SPI flash through a **32 KB I-cache shared by both cores** | Many different machines at once evict each other. Render voices **batched by machine type**, keep hot loops in RAM (`.ram_text`) |
| Core 1 | Works on X0X at M-VAVE's supply levels, but misreads RAM on some units at the boot loader's (other firmwares now self-test and fall back) | Design for two cores, survive on one |
| RAM | X0X lays out 96 KB .data/.bss + 336 KB pool; the chip has about 514 KB usable after caches | Delay lines dominate. Reverb lines can be 8-bit block-float (as ROOM's are); the delay wants more bits at half rate (section 4.3) |
| Flash | 1 MB total. App slot 581 KB (X0X's image is 558 KB, of which roughly 120 KB is the 909's mu-law cymbals and hats); about 36 KB of project storage; three 80 KB sample slots | Content (presets, phrase libraries) must be tiny. One sample slot probably becomes project storage |
| Screen | 240 x 240 RGB565 over SPI | SPI time is the bottleneck; draw in bands, blit what changed |
| Keys | 27 (F3-G5): 16 white, 11 black, an LED each, two brightness levels. Velocity is readable only experimentally (about 20 of 27 keys sense well) | 16 steps + 11 tracks is a natural fit. Treat velocity as a later bonus; accents come from a modifier |
| Encoders | SELECT, PRESETS, ALGORITHM on the left; KNOB 1-4 on the right; no push | Four parameter knobs per page: pages must be well chosen, and a "quick" page matters |
| Buttons | FX, SEL (printed SCL), ENV, LFO, EDIT, GLO, HOME, SAVE, ARP, SEQ, PLAY, REC, OCT-, OCT+ | The printed labels map almost one to one onto Elektron's page buttons (section 4.9) |
| I/O | Stereo out; USB MIDI + USB audio out; TRS MIDI **in only** | No audio in, so no resampling from outside; MIDI clock/notes in from the jack |

---

## 2. Common ground (all three proposals)

Keep X0X's chassis and replace the instrument, as X0X did with Felucca. Worth keeping almost as is:

- **Platform:**
  - HAL (matrix scan, LEDs, LCD, audio, flash, the second core's start-up);
  - USB MIDI and audio, TRS MIDI in;
  - the update loader, web installer, safe mode and rescue path.
- **Storage:**
  - A/B objects with CRC; a torn save loads the previous one;
  - LZ compression;
  - USB backup/restore and its web page.
- **Test bench** (the most valuable part):
  - `host/x0x_host` runs the whole app from scripts into WAV + PNG;
  - golden audio, bit-exact with the device (`-ffp-contract=off`, no libm, no doubles);
  - the WASM emulator for remote testing;
  - the profiler build and its calibration against the device.
- **UI kit:** the knob strip, lists, questions, the big readout, help cards, the screen reader
  (announce.c), undo.
- **DSP:**
  - `fastmath.h`;
  - the 909 voices (9W9 port), Open303 (with its lite mode), TB-3PO, BB Gen;
  - fxbus (CLASSIC and ROOM reverbs, tape delay) and its branches (ping-pong width, delay-to-reverb);
  - master (compressor, pump, DJ filter, limiter);
  - the overload guard and the PERF test.

Rules carried over: the audio ISR never touches flash, allocates or waits; UI-to-engine changes go
through the command queue; nothing that would change the sound ships without a listening test.

---

## 3. Proposal A: "X0X-T", specialist parts

Fork X0X and swap its parts for trance parts. Each part keeps its own screen and its own logic,
as now.

| Part | Engine | Notes |
|---|---|---|
| DRUMS | 909 (11 tracks, as now) | Trance defaults: a longer tuned kick, drive, ride and crash forward |
| ACID | one Open303 + TB-3PO | As now |
| BASS | new mono VA | Offbeat / rolling presets, root follow |
| LEAD | new SAW, mono/duo with glide | JP-style supersaw |
| PAD | new SAW/VA, 6 voices, chord memory, arp, gate | Chorus send |
| BREAK | BB Gen, as now | Prog breaks |

- **Sequencing:** EUCLID on the drum tracks; arp and gate on PAD; TB-3PO on ACID; the BREAK
  generator as now.
- **Song:** X0X's song, plus bar-quantized capture (exists) and song lanes.
- The 808 goes (CPU and flash).

**Strengths**
- Fastest to a good-sounding instrument: perhaps a third of B's work, mostly new DSP plus X0X's
  existing UI patterns.
- Lowest risk on CPU, memory and timeline.
- The 909, 303 and break are already proven on the device.

**Weaknesses**
- **Not orthogonal.** Every feature is per part: Euclid only on drums, arp only on the pad, the gate
  only on the pad, TB-3PO only on the acid. Each new idea costs (features x parts) code and UI.
  This is exactly the TE-style special-casing you said you don't want.
- It hard-codes a track layout that suits trance and fits goa and breaks less well (one acid, one
  pad).

Use it as a fallback, or to ship something while B's sequencer is being built. It isn't the
destination.

---

## 4. Proposal B: "Tracks", Elektron-style machine tracks (recommended)

### 4.1 Tracks and machines

**Twelve tracks**
- Tracks 1-11 sit on the black keys. Any machine can go on any of them.
- Track 12 is the **FX track**, opened with the FX button. It has trigs, p-locks, LFOs and a STEP
  modulator like any other track, but its "machine" is the effects bus. This is Syntakt's idea,
  and it is what makes delay throws, KiloMix-style delay-pan sweeps and master-filter builds
  sequenceable.

The default trance template (a starting point, not a constraint):

| Key | Track | Machine | Default voices |
|---|---|---|---|
| 1 | KICK | BD909 | 1 |
| 2 | CLAP | CP909 (with an SD layer) | 1 |
| 3 | CH | HAT (909 samples) | 1, choke group A |
| 4 | OH | HAT | 1, choke group A |
| 5 | CYM | CYM (ride/crash/reverse crash, sample-lockable) | 2 |
| 6 | PERC | PERC (toms, conga, clave, rim, cowbell, shaker, zap) | 2 |
| 7 | BASS | VA | 1 (+1 for release overlap) |
| 8 | ACID | ACID | 1 |
| 9 | LEAD | SAW | 2 |
| 10 | PAD | SAW | 6 |
| 11 | ARP | VA | 4 |
| FX | FX | the bus | — |

**Machines** (the SRC page; everything after it is common):

| Machine | Source | Notes |
|---|---|---|
| BD909, SD909, CP909, TOM/RIM909 | X0X's 9W9 port, split into per-voice machines | Reference tests exist |
| HAT, CYM | ER-99 909 samples (mu-law, already in the image) + an optional synthesized metal bank | Choke groups become a track setting, not a special case |
| PERC | New: a small multi-model (resonant ping, 2-op FM, noise burst), drawing on Plaits' drum models (MIT: the "909-ish" synthetic kick, the snares, the 6-square hat); 8W8 parts where affordable | Model is p-lockable, so one track plays congas and a clave |
| VA | New: 2 polyBLEP oscillators (saw / pulse+PWM / tri / sine / noise), sub, sync, ring | Juno-ish with saw + pulse + sub; NOISE for risers |
| SAW | New: JP-8000 supersaw (below) | 7 saws, stereo spread, pitch-tracking HPF, random phase |
| FM | New: 2-op plus feedback, with an index envelope | Bells, plucks, goa blips and zaps, M1-ish organ. MSFA (Apache-2.0) if more operators are ever wanted |
| ACID | Open303 (exists) | Its filter *is* the track's filter: the FLT page edits the 303 filter |
| SAMPLE | X0X's slot player, generalized | One-shot, loop, or slice (note = slice), pitch, start, reverse. Plays the user flash slots |

**The SAW machine in more detail.** The reverse-engineered JP-8000 chip (the JE-8086 emulator,
39C3 talk) works like this:
- seven naive saws on integer phase accumulators at **88.2 kHz**;
- an integer detune table (0, +-128, +816/-824, +1408/-1440) scaled by pitch x DETUNE;
- the centre saw at a fixed gain, the six sides scaled linearly by MIX;
- a highpass after the sum.

Szabo's well-known polynomials (the 11th-order detune curve, the centre/side mix curves) describe
the *knob mappings*, measured from outside, rather than the algorithm itself.

At 44.1 kHz naive saws alias far worse than the original. So:
- **MODERN** (the default): seven polyBLEP saws, estimated 130-180 cycles per voice with the HPF;
- **VINTAGE**: naive saws at 2x through a halfband decimator, for the original's airy aliasing at
  a similar cost.

Both use the JP's detune and mix behaviour, with random phases at note-on, and spread the saws
in stereo. ÜBERSAW (MIT, a Daisy port with an algorithm write-up), Braids' SAW SWARM (MIT) and
NuSaw (MIT, Schwung) are references.

### 4.2 The common chain and modulation

Every sound track: **SRC → FILTER → AMP → DRIVE → MOD FX → pan/level → sends (REV, DLY, CHO) and
DUCK**, with its output going to MAIN or to the group BUS (section 4.3).

**FILTER**
- Types: LP24 (ZDF ladder with mild saturation), LP12 / BP / HP (ZDF SVF), OFF.
- Controls: cutoff, resonance, envelope amount, key tracking, ADSR.
- On drum tracks it is a post filter (HP on hats, LP on the kick).
- **The SVF** is Cytomic's TPT design: about 15-30 cycles a sample, with its coefficients (one
  tan, one divide) updated at the control rate. It's stable under fast modulation.
- **The ladder** is Zavalishin's ZDF ladder, with Mystran's cheap nonlinearity: the tanh terms
  sit on the delayed state, using a Padé tanh, which costs one divide, cheap on this core.
- **Oversampling at 2x** only when resonance and drive are both high, as the Deluge does.
  Otherwise these filters don't need it.
- The JP-8000, Nord Lead and early Virus filters were fairly clean digital 12/24 dB multimodes
  with a saturation stage, so this covers the target.

**AMP**
- ADSR (AD on drums), velocity and accent sensitivity.

**DRIVE**
- Shape (soft / hard / RAT-ish, reusing X0X's 303 drive types) and amount.

**MOD FX** (an insert, per track)
- Types: OFF / CHORUS (Juno I, II, I+II) / FLANGER (classic or through-zero) / PHASER (4, 6 or 8
  stages) / COMB (section 4.11).
- **Controls:**
  - rate, synced in notes or bars, or free;
  - depth, mix, and stereo, with the LFO in antiphase between left and right;
  - feedback: the flanger's and phaser's colour;
  - MANUAL, the delay or notch position. P-locks and song lanes can sweep it, for a jet flange
    into the drop.
- The LFO resets with the pattern or the phrase, so a slow sweep (one cycle per 4 or 8 bars)
  lands the same way every time.
- All of these are ordinary track parameters, so LFOs, STEP, p-locks and song lanes reach
  them.
- **Cost:** chorus about 40-80 cycles; flanger 20-30 a channel; phaser about 5 per allpass
  stage. It counts in the budget.
- **Memory:** the delay-based types take a 10-20 ms int16 line (20 ms for through-zero), 1-2 KB a
  track, from a pool of about six lines (section 4.10).
- **The shared CHORUS send stays.** Pads sent there are the 90s desk sound. The insert is for
  when one sound needs its own: a phased arp, flanged hats, a chorused bass.

**Through-zero flanging** (TZF), the big tape-style jet of the era:
- **How it works:** two copies of the signal, one delayed by a fixed amount D0 (1-10 ms) and one
  swept from 0 to 2 x D0, so the delay difference passes through zero.
  - With the polarity inverted, the copies cancel completely at the crossing: the dramatic
    "whoosh into silence".
  - With normal polarity, they double instead, a softer effect.
- **Controls:** feedback on the swept path for the jet's ring; left and right swept in opposite
  directions for width.
- **The crossing is exposed,** so the swept read uses cubic (Hermite) interpolation rather than
  linear, and the delay time is smoothed per sample.
- **Cost:** about 30-50 cycles a channel and a 20 ms line (about 2 KB a channel). Cheap.
- **How it's played:** usually as a gesture, not a steady LFO. A song-row ramp or a lane sweeps
  MANUAL across the last bar or two of a build, or the FX track's LFO in ONE mode runs a
  single sweep from a trigless lock.
- **On a single track it delays that track by D0,** a few milliseconds behind the rest. That's
  usually inaudible, but the master and the BUS are its natural homes, since there everything
  moves together.

**DUCK**
- How much this track ducks when the duck source (any track, usually KICK) triggers.
- One shared envelope per source, one multiply per track.
- This replaces X0X's special-case PUMP with an orthogonal version. The master PUMP becomes "duck
  the master".

**Modulation on every track**
- **LFO 1, LFO 2:** tri/sine/square/saw/random/S&H; synced (including dotted and triplet) or free;
  depth; fade; mode FREE / TRIG / ONE. Each has a single destination: any parameter on the track.
- **STEP** (the PCF idea), a 16- or 32-step pattern:
  - Each step has a level and a shape (hold, decay, attack, ramp). It is tempo-synced and resets
    with the pattern.
  - **Two destinations, each with its own depth:** AMP is the trance gate, CUTOFF is the PCF, and
    both together make a gated, filtered pad. Any parameter works; PAN makes a left/right gate.
  - **Rate:** 1/32 to 1/4, triplets included. It follows the track's swing.
  - **Shape:** DUTY (each step open for 25-100 % of its length), SMOOTH (edges of 0-20 ms, so it
    doesn't click), a per-step decay.
  - **It sits before the sends,** so the reverb and delay tails run on smoothly under the gate,
    which is the usual trance-gate sound. To chop the wash as well, the FX track's STEP gates a
    reverb return or the BUS.
  - It comes with a library of about 48 patterns. We author our own: ReBirth's 56 PCF diagrams are
    a reference, not something to copy.
  - **A track refers to its pattern by number,** from the library or one of 32 user slots, so it
    costs one p-lockable byte.
  - **On the STEP page, the white keys edit the pattern**, a direct use of the 16 keys.

Elektron's choice holds here: no general mod matrix. LFOs with a destination, the STEP modulator,
p-locks, slides and song lanes cover this music, and they all fit on four knobs.

### 4.3 Effects and master

The FX track's pages:

- **REVERB**, three types:
  - **HALL:** ROOM (exists), extended to halls, with decay up to about 8 s, pre-delay 0-80 ms, an
    input HPF, and slow line modulation for pads. Memory: 8-bit block-float lines, 18-36 KB.
  - **PLATE:** a Dattorro plate at 22.05 kHz with halfband resampling, written on MI's FxEngine
    pattern (MIT, 12-bit lines). About 33 KB in int16, less in 12-bit. It is cheap, and it is the
    era's plate sound for claps and leads.
  - **CLASSIC:** stays as the grainy type.
  - M0 decides whether HALL and PLATE share one buffer, as ROOM and CLASSIC do now.
- **DELAY**:
  - synced 1/16 to 1/2, dotted and triplet (3/16 is the trance and KiloMix time);
  - PING-PONG / STEREO / MONO with width (the `pingpong-width` branch);
  - feedback to about 90 %, HP and LP inside the loop;
  - TAPE wear and saturation (exists); delay-to-reverb (branch).
  - **Length:** a 1/4 note at 138 BPM is 435 ms; a dotted 1/4 at 122 BPM is 740 ms; a 1/2 at
    122 BPM is about 1 s. **1 s a side** covers these genres' tempos.
  - **Storage:** int16 at half rate (22.05 kHz) by default, 88 KB for stereo. 12-bit block-float
    is the fallback if RAM is short: 12-bit mantissas plus one exponent per 16 samples, about
    69 KB.
  - **Why not 8 bits:** the noise floor of an 8-bit format follows whatever is loudest at that
    moment.
    - Block-float is about 47-53 dB below the block's peak; mu-law about 38 dB below each
      sample, so it's worse still.
    - A faint echo sharing the line with a loud new note gets that noise. When the two sound
      very different (bright hat echoes under a dark pad, say), the noise isn't masked, and it
      re-quantizes on every repeat.
    - A reverb tail is diffuse; its network smears and damps the noise, and ROOM's listening
      tests heard no difference between 8- and 16-bit mantissas. A delay's echoes are distinct
      copies of a musical line, so the grit is audible.
  - **Half rate** costs little, since the repeats are low-passed at 3-5 kHz in the feedback loop
    anyway. A full-rate DIGI mode gets 0.5 s a side from the same buffer, for crisp short
    echoes.
  - Mu-law stays only as a deliberate LO-FI character, like Clouds' cassette mode.
  - **Modulated reads** (tape wobble, through-zero) work on either format: any sample decodes
    from its block's exponent. Only the writes go a block at a time.
- **CHORUS:** a Juno-style BBD, modes I / II / I+II. A send, so all the pads share it, as on a
  90s desk.
  - **It is written from the published parameters,** not from TAL's code: TAL-NoiseMaker's chorus
    is GPL-2-only and can't be combined with GPL-3. The parameters: antiphase triangle LFOs at
    about 0.5 Hz (I) and 0.83 Hz (II), a sweep of about 0.7-4.9 ms, an interpolated read, a
    one-pole lowpass.
  - About 40-70 cycles and 2-3 KB.
  - Plaits' ensemble (MIT) is a second, string-machine type.
- **BUS:** one group bus.
  - Any track's output can go to BUS instead of MAIN. The FX track gives the bus a DJ filter, a
    MOD FX and a level its STEP can gate.
  - That covers the classic group moves with no per-track work:
    - high-pass the drums through a breakdown;
    - jet-flange the drums into the drop;
    - phase the whole kit;
    - gate the pad and the arp together.
  - The tracks' own sends leave before the bus, so the bus filter doesn't cut their reverb.
- **MASTER:**
  - DJ filter, compressor (with gentle 2:1 "glue" presets), drive, limiter: all of X0X's;
  - a MOD FX slot after the effect returns, for whole-mix sweeps (usually the through-zero
    flanger, the way a DJ flanges the whole record into a drop);
  - a simple tilt EQ.
  - 90s masters were not loud; the factory presets shouldn't be either.

### 4.4 Voices, CPU and the two cores

**Fixed voices per track, with a static budget**
- Each machine has a **measured worst-case cost per voice**, taken on the device in M0 and stored
  as a table.
- A track's POLY setting (1-8 for synths) reserves voices.
- The project must fit under the budget: about 80 % of two cores, minus FX and master. When you
  change a machine or POLY, the header shows a budget bar, and POLY clamps rather than letting the
  project overload.
- Melodee's weighted voice budget is precedent.
- X0X's overload guard stays as a safety net, not as the plan.

**Batched, RAM-resident rendering**
- Voices of the same machine render in one loop: one copy of the code in the I-cache, however many
  tracks use it.
- The hot loops live in RAM, but only those M0 shows the I-cache can't hold. RAM code competes
  with delay memory (section 4.10).
- Envelopes, LFOs, STEP and lock ramps run at a 32-sample control rate. Cutoff and amplitude are
  smoothed per sample.
- Notes start at their exact sample: the voice begins mid-block.

**Determinism across cores**
- Each track renders into its own buffer (mono or stereo, 64-frame sub-blocks: about 6 KB in all).
  Core 0 then mixes them and their sends **in track order**.
- So which core rendered what doesn't change a single sample, and the host's golden files stay
  valid.
- Tracks are assigned to cores per block, by longest-first bin packing on the active cost.

**One core**
- If core 1 fails its self-test, the budget halves. Tracks over the budget lose voices
  (newest-first stealing within the track), and the 303s drop to 1x oversampling, as X0X's guard
  already does.
- The screen says so once.

**A planning estimate for the template above**, in % of one core, anchored on X0X's measurements.
M0 replaces these numbers.

| | Est. |
|---|---|
| Six drum tracks (909 voices, samples, perc) | 8-10 |
| ACID (Open303, 2x, drive) | 19 (X0X's figure) |
| PAD, SAW x 6 (polyBLEP saws, stereo, ZDF LP) | 18-24 |
| LEAD, SAW x 2 | 6-8 |
| BASS, VA x 1-2 | 3-4 |
| ARP, VA x 4 | 8-10 |
| Reverb (hall) + delay + chorus send | 11-13 |
| MOD FX on 3-4 tracks, plus the BUS | 3-5 |
| Master | 7 (X0X's figure: 25 host-ns) |
| Sequencer, modulation, mixing | 5-6 |
| **Total** | **about 95-105 % of one core, so roughly half of two** |

That leaves room for a second ACID (goa, KiloMix), 8 pad voices, or 2x oversampling on the SAW
filters. On one core it doesn't fit, and the fallback has to bite.

An outside check: extrapolating from published Cortex-M4/M7 figures (Plaits, Rings, Daisy),
the DSP research puts 16 supersaw voices with filters at 4-5k of the 16.3k cycles a sample the two
cores have. That agrees with the table above to within its error bars. X0X's own calibration, host
instructions against measured device load, is still the better anchor.

### 4.5 Sequencer

**Pattern model**
- A pattern holds 12 tracks plus pattern settings: bar length, swing, harmony lane, auto-fill.
- As in X0X, **each track can play a different pattern** (MC-505 MEGAMix). PRESETS on a track's
  screen cues that track only.

**Per track**
- **Length:** 1-64 steps.
- **Speed:** 1/8x, 1/4x, 1/2x, 3/4x, 1x, 3/2x, 2x. A pad at 1/4x plays 16 bars in 64 steps.
- **Mode:** FREE-RUN or RESET (per bar, or per phrase).
- **Also:** swing on/off, transpose, FOLLOW (section 4.6), default note, velocity and length.

**Per trig**
- Note (or a chord on poly tracks); velocity; accent; length (or tie).
- Slide: 303 semantics on ACID, a parameter slide elsewhere.
- Micro-timing in 1/24ths of a step.
- **Retrig:** rate 1/16 to 1/64 and triplets, length, velocity ramp up or down, and a pitch ramp
  for rising rolls.
- Condition.
- P-locks on any parameter, including **trigless locks**.

**Conditions**
- Elektron's set: X %, A:B, FILL / !FILL, PRE / !PRE, NEI / !NEI, 1ST / !1ST, LST / !LST.
- Plus **phrase conditions: "last bar of 8 / 16 / 32" and "first bar of 8 / 16 / 32"**, counted
  from the song row (or from PLAY when looping a pattern).
- A:B goes up to 16 (Elektron's 8 is a common complaint).
- **Auto-fill** (a pattern setting: every 4 / 8 / 16 bars) turns FILL on for the last bar of each
  phrase, so a snare-roll trig with FILL plays itself.

**P-locks**
- Generalize X0X's lane pool: one 64-value lane per (track pattern, parameter), 72 B, about 256
  lanes per project.
- Adjacent locks glide, as now.

**Cueing**
- Pattern cues quantize to 1, 2, 4, 8 or 16 bars (DJ-style changes on the 8 and 16).
- Mutes can quantize too.

### 4.6 Harmony

Each pattern has a **key** (root + scale: minor, Dorian, Phrygian, Phrygian dominant, harmonic
minor, double harmonic, major) and a **chord lane**: up to 8 changes, each a step position, a
degree, a quality (min, maj, sus2, sus4, m7, m9, 7, add9) and an inversion.

Each track has **FOLLOW**:

| FOLLOW | What it does | Use |
|---|---|---|
| OFF | Plays the notes as written | Drums, exact lines |
| SCALE | Snaps notes into the key | Leads, generated lines |
| ROOT | Transposes the track by the current chord's root | A one-bar offbeat bass becomes a progression |
| CHORD | Chord trigs voice the current chord; ARP arpeggiates it | Pads, stabs, arps |

- A song row can transpose the key (the last-chorus key change).
- This is the single most "genre" feature in the proposal. Trance is built on i-VI-III-VII-type
  progressions, and every generator below becomes musical when it follows the lane.

### 4.7 Trig sources: generators, libraries, mutation

Each track has a **SOURCE**, set on the ARP page (the button's label fits). The source produces
trig events (gate, note or degree, velocity or accent, slide, length). The track's own steps either
**ADD** to them or **MASK** them: steps say where the source may play, as X0X's break does now.

| Source | What it is | Origin |
|---|---|---|
| STEPS | The manual steps | — |
| EUCLID | Pulses / steps / rotate, plus a second Euclid layer for accents | Bjorklund; Grids' table code |
| GRIDS | MI Grids' algorithm (5x5 map, 3 lanes, density threshold, chaos). Map X/Y is shared per pattern, so all GRIDS tracks morph together; each track picks its lane and density | Grids (GPL-3). Plus **our own trance/prog/goa map** in Grids' format |
| ARP | Order (up, down, up/down, as played, random, chord) x octaves 1-4 x a 16-step **beat pattern** from a library (accent, gate and rest per step), as the JP-8000 and MC-505 did | New |
| ACID | TB-3PO (exists); Acid Banger's rules as a second flavour (CC BY 4.0) | schwung-tb3po, vitling/acid-banger |
| TURING | Shift register, length 2-16, flip probability (locked, slipping, random) | Music Thing (algorithm) |
| BREAK | BB Gen's slice shuffler (complexity, anchor, roll, fill, retrig odds), emitting slice numbers for a SAMPLE track | BB Gen (exists) |
| PHRASE | A curated per-role phrase library (below), browsed by number or on a **style map** | New |

**Every source shares the same controls**
- **SEED.**
- **NEW** (OCT+): a new seed.
- **MUTATE** (OCT-): uses Marbles' déjà-vu curve as the amount, i.e. `p = (2dv - 1)^2`. Below
  half it rewrites steps with probability p; above half it jumps within the loop.
- **EVOLVE:** auto-mutate every 1-16 bars.
- **PRINT:** writes the result into steps for hand editing.
- All of these are undoable, and a pattern is reproducible from its seed.

**Libraries and "tiny models"** (in order of build)
1. **Hand-written phrase corpus**, versioned in the repo as text and compiled to flash tables.
   About 300 phrases of 16-32 steps, a few bytes per step, 8-10 KB:
   - BASS: offbeat, double offbeat, rolling, KBBB, gallop, prog syncopation, breaks;
   - ARP shapes; STAB/dub-chord rhythms; LEAD motifs;
   - GATE/STEP patterns;
   - DRUM roles: kick variants, hats, perc, rolls;
   - ACID lines.
   There's no open trance dataset, and the style is regular enough that writing
   them by hand works better than scraping. RM1x's lesson: swappable *per-track* phrases, not whole patterns.
2. **Style maps:** for each role, 25 phrases placed on a 5x5 map by feel (x: straight → rolling;
   y: sparse → busy), read Grids-style: per-step importance interpolated, thresholded by density.
   A bass map turns "offbeat to KBBB" into one knob. It costs 1-2 KB per role.
3. **In-style mutation:** an order-2 n-gram over (interval, gate, accent, slide) learned from the
   corpus at build time and baked into tables. It's used by MUTATE when the source is PHRASE.
4. **Later, optional:** a learned drum map (R-VAE / GrooVAE-style) sampled on a grid offline and
   stored in Grids' format, about 2-4 KB. Nothing neural runs on the device.

**A ReBirth .rbs importer** (host tool; the parser exists from this research) turns old ReBirth
303 and drum patterns into FM-1 patterns or library entries. It's an on-ramp, not a ReBirth clone.

### 4.8 Song: rows, capture, lanes

**Rows** (Elektron's model, fixing its common complaints)
```
row  { label (INTRO / BUILD / BREAK / DROP / OUTRO / none);
       pattern; per-track pattern overrides (MEGAMix);
       repeats 1..64; length (steps; default = pattern);
       mutes (12 bits); transpose; tempo (optional) }
song { rows[<=128]; end = LOOP | STOP; lanes[8] }
```

- **Phrase counters reset at each row**, so phrase conditions and fills work per section.
- **Loop the playing row** for as long as you like (extend a breakdown live), then let it go on.
- **Jump:** white keys jump to a row on the SONG screen.

**Capture** (Deluge's lesson)
- REC + PLAY on the SONG screen records what you do: pattern cues, per-track pattern swaps,
  mutes, transposes.
- It works bar by bar from the start row, and consecutive identical bars merge into one row's
  repeat count.
- Rows point at shared patterns, never copies. Editing a pattern used elsewhere asks "only here?"
  and clones it.
- Punch-in: start capture at any row and overwrite only from there.

**Lanes** (song-length automation)
- **Eight lanes**, each assigned to any parameter, including the four performance macros (below).
- Stored as **sparse change lists** at 1/16 resolution:
  - a (position, value) pair costs 3 bytes;
  - a 16 KB cap is about 5,000 changes, which is KiloMix-grade riding;
  - linear interpolation between points on playback.
- **Two ways in**, both writing the same lane:
  - Draw a **ramp** on a row: start, end, curve; "open the lead's cutoff over this 16-bar build".
  - **Record** riding during capture.
- Lanes sit above p-locks and pattern motion: the lane offsets or overrides the base, the per-step
  locks still play.

### 4.9 Controls and screens

The FM-1's printed labels already read like Elektron's page buttons:

| Control | Tap | Hold |
|---|---|---|
| Black keys | Select track 1-11 (audition when stopped) | **Hold + turn a knob: that track's QUICK page**, without leaving the screen you're on |
| White keys | Steps 1-16 of the current 16 (trig on/off). On HOME: patterns. On SONG: rows. On STEP: the step pattern | **Hold + knobs: p-lock**; hold + SELECT: condition; hold + PRESETS: micro-timing; hold a step then tap another: tie/length |
| ALGORITHM | Track (and the value in lists; tracks in KEYS mode) | |
| PRESETS | Pattern cue: all tracks on HOME, this track on its own screens | |
| SELECT | Page within the section; rows in lists | |
| OCT- / OCT+ | Steps 1-16 ... 49-64; octave in KEYS; MUTATE / NEW on ARP | |
| EDIT | **SRC**: the machine's pages, page 1 = QUICK (the four "money" knobs) | |
| ENV | **FLT** and **AMP** pages | Accent while entering; FILL on HOME |
| LFO | **MOD**: LFO 1, LFO 2, STEP | Slide while entering |
| FX | The **FX track** (its steps on the white keys, its pages on the knobs) | |
| ARP | **SOURCE**: the track's generator | |
| SEQ | **TRACK**: length, speed, mode, swing, follow, transpose. A held step on SEQ shows the **TRIG** page: note, velocity, length, retrig. Second page: the pattern's **KEY and CHORDS** | |
| HOME | **PERFORM**: white = patterns, black = mutes, knobs = 4 macros. Again: **SONG**. Again: **MIX** | + white: pattern from anywhere; + black: mute from anywhere; + REC/PLAY: undo/redo |
| SEL (SCL) | The screen as a list, including what's on no page (sound browser, save-as, clear, print) | + SAVE etc.: functions |
| SAVE | Save | Copy, then a key pastes (step, track, pattern, row) |
| REC | Record: knob motion, live keys (KEYS mode on melodic tracks), song capture on SONG | |
| PLAY | Start / stop | |

**QUICK pages**, for example:
- ACID: CUTOFF / RESO / ENVMOD / DECAY.
- SAW: DETUNE / MIX / CUTOFF / RELEASE.
- VA: CUTOFF / RESO / ENV / DECAY.
- BD: TUNE / DECAY / ATTACK / DRIVE.
- HAT: DECAY / TUNE / TONE / LEVEL.

**Performance macros** (HOME's four knobs, also lane targets)
- FILTER (the master DJ filter); SPACE (scales every reverb and delay send); DUB (delay feedback
  and a send throw); TENSION (assignable to up to four parameters with ranges).
- Hold HOME + turn a macro to assign it, as in Baud Girl's assignable knobs.
- ENV held + turn: back to the default (X0X).

**The screen**
- **Header:** the track chip in its colour with the machine name, page dots, pattern (and cue),
  tempo, transport, the CPU budget dot, unsaved.
- **Main area:** the page's picture (filter curve, envelope, LFO, STEP pattern, Euclid circle,
  Grids map, harmony lane) **above a mini-grid of all 11 tracks x 16 steps**, so the whole pattern
  is always visible.
- **Knob strip:** four cells with the big readout on turn (X0X's).

**LEDs**
- White keys: trigs bright, locks and conditions dim, the playhead moving.
- Black keys: selected bright, tracks with trigs dim, muted dark.

**Common tasks**, as a playability check (the M3 tests use these):

| Task | Presses |
|---|---|
| Open the bass's filter | black 7, turn K1 (QUICK is the default page) |
| Offbeat open hats | black 4, white 3, 7, 11, 15 |
| 5-in-16 Euclidean perc | black 6, ARP, K1 to EUCLID, K2 to 5 |
| Lock one step's cutoff | hold the white key, turn K1 |
| A rising snare roll in fill bars | black 2, SEQ, hold the step, set RETRIG 1/32 with a ramp, SELECT to FILL |
| Gate the pad | black 10, LFO x3 (STEP), K1 picks a gate pattern, K2 depth |
| Kick out for the breakdown, back on the 16 | HOME, black 1 (mutes quantized to 16 bars) |
| Record a 16-bar filter ride into the song | HOME twice (SONG), REC, PLAY, ride K1 |
| Key change for the last chorus | SONG, select the row, TRANSPOSE +2 |
| Tweak the kick's decay while editing the pad | hold black 1, turn K2 |

### 4.10 Memory and flash

**RAM** (planning numbers, revised after adding the mod FX, the bus, sound locks and textures;
M0 audits the real map and looks for the roughly 80 KB that X0X's layout doesn't use)

| | Lean | Middle | Everything maxed |
|---|---|---|---|
| Delay, stereo, half rate: 1 s a side in 12-bit block-float / 1 s in int16 / 1.5 s in int16 | 69 | 88 | 132 |
| Reverb: one type at a time (HALL and PLATE share a buffer) / both at once (the plate's long delays in 8-bit block-float, its allpasses in int16) | 36 | 36 | 58 |
| MOD FX lines: chorus send, master and BUS through-zero (stereo, 20 ms), per-track lines (a pool of 6, or all 11) | 19 | 19 | 28 |
| Screen band buffer: 240 x 24 / 240 x 48 (X0X's canvas is 69 KB) | 12 | 23 | 23 |
| Patterns: 16 x 12 tracks x 64 trigs x 3 B (micro-timing, conditions and retrigs in lanes, as X0X keeps chance), or 4 B | 37 | 37 | 49 |
| Lock lanes (72 B each: 256 / 320) | 18 | 23 | 23 |
| Sound pool (32-64 sounds), STEP user patterns, harmony | 6 | 6 | 9 |
| Song rows + lanes (16 KB cap) | 19 | 19 | 19 |
| Undo journal (old values per edit; X0X's whole-project shadow doesn't scale) | 16 | 16 | 16 |
| Voices (about 26), track buffers, FX state | 30 | 35 | 40 |
| **Hot code in RAM** (missing from the first version of this table) | 16 | 32 | 48 |
| USB, backup, the rest of .data/.bss (as X0X) | 25 | 28 | 35 |
| **Total** | **about 300** | **about 360** | **about 480** |

Against the 432 KB X0X lays out today:
- **Lean and middle fit,** the middle with about 70 KB to spare.
- **Everything maxed doesn't fit** unless the audit finds most of the remaining 80 KB.

The new features themselves are small: mod FX, through-zero, the bus, sound locks and textures
come to about 25-35 KB together. The big items are what they always were, and they are also the
levers, in order of value:

1. **The delay** (69-132 KB).
   - Half rate is nearly free in sound: the repeats are low-passed at 3-5 kHz in the feedback loop
     anyway. It halves the memory, or doubles the time.
   - 1 s a side is enough for 120-155 BPM.
   - 12-bit block-float instead of int16 saves another 19 KB.
   - Full rate stays for a clean DIGI mode with short times.
   - Don't go to 8 bits on the delay (section 4.3).
2. **Hot code in RAM** (16-48 KB). Move only what M0 shows the I-cache can't hold. If the cache
   copes, this is near zero.
3. **One reverb at a time** (saves 33 KB against two). Era desks often had a plate and a hall, but
   one good hall plus a plate type you switch to is a reasonable trade.
4. **Per-track MOD FX lines from a pool of six,** not eleven (saves about 9 KB). Phasers need no
   line, so this only limits how many tracks flange or chorus at once.
5. **Smaller screen bands** (saves 11 KB, at the cost of more SPI transactions).
6. **COMB at track level, not per voice:** it is a MOD FX type with one line, tuned by the last
   note. As a per-voice filter it would have cost about 1.7 KB a voice, 40 KB across the voices.
7. **STEP patterns by reference:** library or user slot numbers, 1 byte each, not 64 bytes per
   track per pattern.

**Flash**
- **App image:** at most 581 KB. Drop the 808 and the built-in break loops; keep the 909's roughly 120
  KB of mu-law cymbals and hats; tables and libraries come to about 20 KB.
- **Project storage:** A/B with LZ, about 96 KB, by turning one of the three 80 KB sample slots
  into storage. One project on the device; more through USB backup.
- **User samples:** two slots, about 14 s at 22 kHz ADPCM, for breaks and stabs.
- **Open question:** the 0xE5000-0xFC000 region (92 KB) that no one has yet said is safe. Worth
  finding out in M0.

### 4.11 Textures and FX sounds

The music of the era is full of background texture: noise sweeps, wind and air, zaps and lasers,
ring-mod clangs, FM buzzes, self-oscillating filter whistles, sampled voices and field
recordings, reverse cymbals. Much of it came off sample CDs into samplers, or from abusing a synth
and its effects. (This section is from domain knowledge, not from the research pass; the census
at the end is how to check it.)

**What the range is.** Sort these sounds two ways: what they *do* in the arrangement, and what
they're *made of*.

What they do:

| Role | What it is | Where it goes | Examples |
|---|---|---|---|
| Bed | Long, quiet, continuous | Intros, outros, breakdowns: the space left when the drums are out | Filtered noise "air", wind, ocean, rain, drones, a slowly sweeping pad |
| Motion | Rhythmic texture with no melody | Under the groove, panned wide | S&H bleeps, filtered noise rhythms, random 303 squelches, gated noise |
| Transition | Leads into or out of a phrase boundary, 1-16 bars long | Before the drop, the end of a phrase, the start of a breakdown | Noise riser, pitch riser, downlifter, reverse cymbal, tape stop, sub drop |
| Punctuation | A one-shot on a boundary | On the 1 of a section | Crash, impact with a long reverb tail, vocal shout |
| Ear candy | Sparse, irregular ornaments | One every 4-8 bars, often near phrase ends, varied each time | Laser, zap, FM blip run, panned ring-mod hit, a delay-thrown vocal snippet |

What they're made of:
- **noise:** white, pink, or crackle, through a filter;
- **resonant pitch:** a self-oscillating filter, a sine with a pitch envelope;
- **inharmonic:** ring mod, FM at non-integer ratios, comb resonance;
- **samples:** voices, field recordings, cymbals.

**How they're used.**
- Most of these sounds are made by their *effects*. A 50 ms blip into a long ping-pong and a hall
  is a texture; dry, it's a click.
- They sit high-passed, quiet and wide.
- They mark phrase boundaries: the arrangement's 8/16/32 grid is where transitions and
  punctuation go.
- Ear candy keeps changing (a different pitch, pan and sound each time), and that is what keeps an
  8-10 minute track alive.
- Goa leans hardest on motion and ear candy (bleeps, zaps, film dialogue). Prog house leans on
  beds and long transitions. Trance has the riser into the drop, the crash and impact on the 1, and
  a bed under the breakdown. The 2020s revival adds lo-fi grit and field recordings.
- KiloMix has none of this (ReBirth had no samples or noise); it's one of the things a modern take
  adds.

**How B handles them.** Mostly with what is already in the design. Textures need a few small
additions, not a new machine.

*Small additions to the sound*
- **MOD ENV:** an assignable AD/ADSR on every synth track's MOD page, default destination pitch.
  Lasers, zaps, risers, sub drops and tape stops all need a pitch envelope, and the chain as
  written above had none.
- **Noise colours** for the VA's noise oscillator: white, pink, DUST (sparse random impulses, for
  crackle, rain and vinyl) and DIGITAL (stepped S&H noise at a set rate, for radio and computer
  chirps). All four are cheap.
- **COMB, as a MOD FX type:** a short tuned feedback delay, plus or minus, tuned by the last note
  or a fixed pitch. Metallic resonance and flanged noise. It is one line per track (per voice
  would cost about 40 KB across all the voices).
- **CRUSH and DECIMATE drive shapes:** bit depth and sample rate, for lo-fi grit.
- **On the FX track:**
  - REVERB FREEZE: a lock turns the tail into an infinite drone.
  - Delay feedback past self-oscillation, with a limiter in the loop.
  - Delay TIME slides, for pitch warps.
  - Delay HOLD: input off, feedback 100 %, which makes the delay line a beat-repeat and stutter
    looper. A slide on the read speed gives a tape stop. It needs no extra memory.
- **Reverse playback** on the SAMPLE machine. The 909 crash already in the image, played
  backwards, is the reverse cymbal, so it costs no sample space.

*Small additions to the sequencer*
- **Sound locks** (Elektron's idea): a trig can play a different sound from the project's sound
  pool, within the track's machine. One texture track can then play a riser on bar 8, a zap on
  bar 3 and a laser on bar 12. The lock is stored as one more lane.
- **SCATTER**, a new trig source for ear candy:
  - DENSITY: events per 8 bars, 0.25 to 8.
  - GRID: where they may land (1/16, 1/8, offbeats, beats).
  - LATE: weighting toward the end of the phrase.
  - SET: which sound-lock slots it picks from, with no immediate repeats.
  - AVOID: a track whose steps it keeps off, e.g. the clap.
  - Seed, MUTATE and EVOLVE, like every source.
  - Per-hit pitch and pan variety comes from an LFO in S&H/TRIG mode, which already exists.
- **LAND-ON trigs:** a trig option that starts the sound early, so its *end* lands on the step.
  The sequencer looks ahead (up to 4 bars), which it can do because patterns and songs are
  deterministic; chance is decided when the sound starts. It is for reverse cymbals and reversed
  vocals. For risers, the existing tools are enough: a trig with a long attack and a ramp on a song
  row.
- **Phrase conditions** (section 4.5) already place transitions and punctuation; song lanes
  (section 4.8) already carry beds and 16-bar risers.
- **An ATMOS performance macro:** scales SCATTER densities and bed levels together. It is one of
  the assignable macros, so it's a preset, not a feature.

*Track layout*
- Textures need about two tracks: a synthesized one (VA or FM: beds, risers, zaps, bleeps) and a
  sample one (crash, reverse crash, impacts, voices, field recordings).
- The template gets them like this:
  - **The CYM track becomes "CYM/SAMPLES":** ride, crash and the user's one-shots by sound lock.
  - **CH and OH share one HAT track,** with OPEN as a per-trig lock, the way the 909 itself
    shares and chokes them. That frees track 4 for **TEX**.
- The cost: writing offbeat open hats over closed 16ths takes a few more presses than with two
  tracks. The alternative is giving up PERC.
- This is a template choice; any track can be anything.

*Content*
- About 30 synthesized texture presets in the factory set:
  - transitions: RISER 8/16, DOWNLIFTER, TAPE STOP, SUB DROP, IMPACT, REVERSE CRASH (land-on);
  - beds: WIND, OCEAN, AIR, CRACKLE;
  - ear candy and motion: LASER, ZAP, GOA BLEEPS, BUBBLES, RING CLANG, METAL BUZZ, ROBOT, WHISTLE,
    RADIO, ACID SQUELCH.
- They cost a few KB of flash, no samples.
- **The sample slots carry the user's own one-shots.** A per-sample 11 kHz option doubles what
  fits, and is honest to the era: the samplers' lo-fi rates were part of the sound.

**Pinning down the range: a texture census.**
- Listen through 6-10 tracks (the five references plus a few goa and prog house), and log every
  texture event in a spreadsheet:
  - bar.beat;
  - length (beats or bars);
  - role (bed / motion / transition / punctuation / ear candy);
  - material (noise / resonant / inharmonic / tonal / voice / field);
  - movement (static / up / down / random / rhythmic);
  - space (dry / reverb / delay throw / pan);
  - section (intro / build / break / drop / outro).
- An hour or two of listening gives the real numbers: events per 8 bars in each section, typical
  lengths, how many land on phrase boundaries, which materials each genre uses.
- Those set SCATTER's defaults and the preset list, and settle whether LAND-ON earns its place.
- The template is `docs/plans/texture-census.xlsx`:
  - a Log with dropdowns, where times convert to bars and phrase lines from each track's BPM and
    first downbeat;
  - Tracks and Sections sheets;
  - a Summary that computes densities, phrase alignment, lengths and materials.
- I can't listen, but I can measure WAVs you give me (onsets, spectral flatness, the exposed
  intros and breakdowns) to check the log.

---

## 5. Proposal C: "Unison", one deep voice and a mod matrix

Twelve uniform tracks again, but only two engines:

- **SYNTH: one superset voice for every melodic track.**
  - Two oscillators, each saw / pulse / supersaw / FM pair / noise / sample, plus a sub, with
    sync, ring or FM between them.
  - Two filters (ladder LP24 and multimode SVF, serial or parallel, plus a "diode" mode for acid).
  - Three ADSRs, two LFOs, STEP.
  - An 8-slot mod matrix (envelopes, LFOs, velocity, accent, STEP, macros, key, random → any
    parameter).
- **DRUM: one superset percussion voice.**
  - A tone oscillator with pitch envelope and shaper, noise through BP/HP, a click, a 6-oscillator
    metal bank, and a sample layer, mixed.

The sequencer, harmony, sources, song and FX are B's.

**Strengths**
- The most orthogonal: every melodic track has identical pages, presets move between tracks, and
  there is no machine list to learn.
- Two engines means the smallest code, the easiest on the shared I-cache.
- A deep, flexible synth.

**Weaknesses**
- **Authenticity.** The 303's character is its coupled filter, accent and slide behaviour; the
  909's is its specific circuits. A universal voice approximates both, or grows "modes" until it
  is B's machines in disguise. For this music the 909 and the 303 are not optional.
- **CPU per voice.** A superset voice pays for its routing even when it's simple, so expect fewer
  pad voices than B for the same budget.
- **Tweaking on four knobs.** More pages per voice, and a mod matrix is fiddly without a big
  screen and many knobs. Getting to a good sound takes longer, which works against "easy
  tweaking".
- Little of X0X's proven DSP carries over unchanged.

The parts of C worth taking into B: the VA machine can be a modest superset (it already is: two
oscillators, modes, sync, ring); and the STEP modulator plus LFOs with any destination give most of
a mod matrix's reach.

---

## 6. How they stack up

| Goal | A: X0X-T | B: Tracks | C: Unison |
|---|---|---|---|
| 1996-99 trance / prog sound | Strong: proven 909/303, new SAW/VA | Strong: same DSP, more voices where needed | Good: weaker 909/303 character |
| Goa, revival, prog breaks | OK: one acid, one pad, fixed layout | Strong: two acids, any layout, slices + BREAK source | Good |
| Playability | Strong: X0X's grammar | Strong: track keys, quick pages, hold-to-tweak, perform view | OK: deeper pages |
| Easy tweaking | Strong per part, inconsistent between parts | Strong: same pages everywhere after SRC; QUICK pages | Weaker: mod matrix on four knobs |
| Song / arrangement | Good: X0X's song + lanes | Strong: rows + capture + lanes + phrase fills | Strong (same as B) |
| Step + procedural sequencing | Patchy: generators tied to parts | Strong: every source on every track, harmony | Strong |
| Orthogonality | Weak | Strong (machines only change SRC) | Strongest |
| CPU / memory risk | Low | Medium (budgeted; M0 measures) | Higher (superset voices) |
| Effort | Smallest (about 1x) | Large (about 3x) | Largest (about 3.5x) |
| Room to grow | Poor: features x parts | Good: a new machine or source plugs in once | Good |

On effort: X0X went from a design doc to its 1.0 in about a week with heavy
automation. So these are relative sizes, not calendars; the gates below matter more than the
dates.

---

## 7. Plan for B: milestones, gates, validation

**Strategy: a thin vertical slice on the device first, then widen.** The risks are CPU, I-cache
and feel, and only the device answers them.

### M0: Measure (the go/no-go)
- Fork the chassis; strip the instrument; keep the build, tests, emulator and installer green.
- **DSP spikes on the host and on the device's PERF test, each measured as cycles per voice:**
  - a SAW voice (polyBLEP vs naive at 2x + HPF, mono vs stereo);
  - a VA voice with ZDF ladder at 1x vs 2x, and an SVF;
  - 2-op FM;
  - the BBD chorus;
  - the hall reverb at 4-8 s, and the Dattorro plate at half rate;
  - stereo ping-pong at half rate, int16 against 12-bit block-float, with a masking stress test
    for each format:
    - faint bright echoes under a loud dark pad;
    - echoes alone in silence;
    - dub stabs at 70-90 % feedback for 30 s.
    It's scored by noise per band against a float reference, then listened to. The same test
    runs on the reverb's 8-bit lines.
  - one 909 voice as a machine.
- **I-cache test:** 6 machine types active, with and without RAM-resident loops, with the screen
  still vs animating.
- **Memory and flash audit:** the real SRAM map against the 514 KB; the 0xE5000 region; the image
  breakdown.
- **Single-core budget,** with core 1 deliberately off.
- **Gate:** the template in 4.4 at default polyphony at or under 60 % on the busier core, peak
  under 80 %; the memory plan fits with 32 KB to spare. If not: cut tracks to 8 + FX, or pad
  voices to 4, before writing any UI.

### M1: Data model and sequencer (host only)
- Patterns, trigs, locks (generalized lanes), conditions (including phrase conditions), retrigs,
  micro-timing, speed and length, cue quantize, harmony lane and FOLLOW.
- Song rows and lanes; storage objects with LZ and A/B; undo journal.
- **Validation:**
  - event-exact unit tests (extending `seq_test.c`) for every trig feature;
  - polymeter and speed invariants;
  - swing (the `swing-timing-fix` cases);
  - phrase counters per row;
  - save, load and torn-save tests.

### M2: Engine framework and the first machines, on the device
- Voice pool and budget, per-track buffers, fixed-order mix, the per-block core scheduler, the
  control rate, LFOs, STEP, MOD ENV, the common filter, MOD FX (with COMB), DUCK, noise colours,
  CRUSH and DECIMATE.
- **Machines:** BD/CP/HAT/CYM (from 9W9), ACID, SAW, VA.
- **FX:** hall, ping-pong, chorus send, MOD FX inserts, the BUS; X0X's master.
- A throwaway minimal UI is enough here.
- **Validation:**
  - 909 and 303 reference tests still pass after the split;
  - the SAW matches the JP-8000 detune ratios and mix curve;
  - the chorus matches measured Juno chorus behaviour;
  - bit-exact host and device;
  - device load within M0's figures;
  - the first listening session on a 1997-style loop.

### M3: The UI grammar, and the emulator
- Every screen in 4.9; KEYS mode; locks, conditions and retrigs in the UI; copy and paste; LEDs;
  screen-reader words.
- The WASM emulator updated so others can try it.
- **Validation:**
  - the task table in 4.9 as scripted scenarios (press budgets checked);
  - on the device: build the "1997 loop" from an empty project in under 10 minutes;
  - a few FM-1 owners try it in the emulator.

### M4: Sources, harmony and the library
- EUCLID, GRIDS (Grids' map + our map), ARP with beat patterns, ACID, TURING, BREAK + SAMPLE,
  PHRASE with style maps.
- MUTATE / NEW / EVOLVE / PRINT; the corpus compiler; the .rbs importer.
- Sound locks, SCATTER, LAND-ON trigs; reverb FREEZE and delay HOLD on the FX track.
- **Validation:**
  - determinism by seed;
  - "generator sweeps": scripts that render each source across a grid of settings into one WAV,
    for listening;
  - generated lines stay in key under every FOLLOW mode.

### M5: Song and performance
- The SONG screen; capture with merging and punch-in; row loop and jump; lanes (ramps and
  recording); macros; cue and mute quantize; auto-fill.
- **Validation:**
  - a full 8-minute arrangement built on the device (intro, build, breakdown, drop, outro);
  - a captured live take that reloads identically;
  - lane capacity under KiloMix-style riding;
  - power-cut during save.

### M6: Sound design and content
- About 100 factory sounds; five templates: TRANCE 97 (138 BPM), PROG 98 (128), GOA (145,
  Phrygian), REVIVAL (150), BREAKS (130); demo songs (original, not covers).
- Loudness-matched presets.
- The texture census (section 4.11), then about 30 texture presets and SCATTER defaults set
  from it.
- **Validation:**
  - Scenario recreations of each reference's *parts* (offbeat bass and gated pad à la "Waters",
    dub stabs à la "Set in Stone", the late bass and build of "Perception", "Gen Z"'s fast bass
    and wash, a two-acid KiloMix-style jam). These are for listening tests only, never shipped.
  - Blind A/B against DAW mock-ups of the same parts.
  - Aliasing checks on the SAW and VA at the top of the keyboard.

### M7: Beta
- Two-hour song soak on one core and on two, with no dropouts.
- External clock and MIDI; migration between beta formats; the manual; the installer and release.

**All the way through:**
- Golden audio hashes per scenario.
- A CPU regression gate in CI: host instruction counts per scenario, failing on +5 %.
- The PERF test after every DSP change that reaches the device.

---

## 8. Risks

1. **The I-cache.** Eleven tracks of five machine types may thrash a shared 32 KB cache. The
   mitigations are batching and RAM-resident loops; M0 measures it before anything depends on it.
2. **Memory for long delays and halls.** 8-bit block-float has worked once, for ROOM's reverb
   lines. The delay needs 12-16 bits at half rate, because faint echoes under loud, different new
   material expose an 8-bit noise floor. M0's masking stress test checks both.
3. **Core 1 on some units.** Mitigated by the single-core budget, but a single-core FM-1 gets a
   smaller instrument. The manual should say so plainly.
4. **UI depth on four knobs.** Twelve tracks x (SRC + FLT + AMP + MOD + FX + SOURCE + TRACK) is a
   lot of pages. The QUICK pages, hold-a-track tweaks and the always-visible mini-grid are what keep
   it playable. M3's task tests are the check.
5. **Content effort.** The phrase corpus, style maps, gate library and factory sounds are real
   work, and they decide how "genre" the box feels as much as the DSP does.
6. **Licences.** All of these are compatible with GPL-3, except where noted:
   - Grids' code is GPL-3.
   - Mutable's STM32 code (Plaits, Marbles, stmlib, FxEngine) is MIT.
   - Acid Banger is CC BY 4.0.
   - NuSaw, ÜBERSAW and Open303 are MIT; MSFA is Apache-2.0.
   - **The exceptions:**
     - TAL-NoiseMaker is GPL-2-only, so it is a reference only.
     - The Moog-ladder collection's models are licensed one by one, from LGPL-3 to the
       Unlicense, so check each before lifting.
     - ReBirth's PCF shapes and anyone else's patterns are references only.
   - Recordings used for listening tests stay private.
   - X0X already ships ER-99's 909 hat and cymbal samples under GPL-3. If their provenance ever
     becomes a question, the fallback is a synthesized metal bank (Plaits' 6-square hat).

## 9. Decisions for you

1. **B, A-then-B, or something else?** I'd go straight to B and use A's machine list as M2's
   content.
2. **Track count:** 11 + FX (recommended, one per black key), or 8 + FX (cheaper and simpler, but
   drums get tight), or more with banks.
3. **Flash:** one project on the device plus two sample slots (recommended), or more projects and
   one slot.
4. **Velocity:** ignore the experimental key velocity for now (recommended), or prototype it in M3.
5. **Codebase and name:** a fork of X0X (recommended; GPL-3), in this repo or a new one.
6. **Delay vs reverb memory:** a half-rate delay and one reverb at a time (recommended), or a
   full-rate delay and two reverbs if the M0 audit finds the RAM.
7. **Texture tracks in the template:** CH and OH share one HAT track to free a TEX track
   (recommended), or keep both hat tracks and give up PERC.

---

## Sources

FM-1 and its firmwares:
- X0X's own notes (this repo): `docs/plans/2026-10-04-dual-core-and-clock.md`,
  `2026-10-04-performance.md`, `2026-10-15-room-reverb.md`, `2026-10-04-song-motion.md`.
- FM-1-RE architecture: https://github.com/AL-255/FM-1-RE/blob/main/docs/architecture.md
- lunar-modulator hardware notes (MIDI-in-only TRS, matrix, chip, Baud Girl analysis):
  https://github.com/ip2k/lunar-modulator/blob/main/docs/01-hardware.md
- Felucca: https://github.com/hugelton/Felucca (engines, the 85 % guard in `audio.c`, the wiki's
  Controls and Features pages)
- Melodee (voice budget, dual-core notes): https://github.com/keremimo/melodee
- Jangada (dual-core voltages): https://github.com/zednaked/jangada
- fm1-velocity / MONK-1 (experimental key velocity): https://github.com/charlesvestal/fm1-velocity
- Index of FM-1 firmwares: https://github.com/cicloid/awesome-fm-1
- NuSaw (MIT supersaw, Juno chorus, ping-pong): https://github.com/charlesvestal/schwung-nusaw

Genre and references (mostly from search summaries; parts of tracks 1-4 still need a listening
pass):
- For an Angel gear and tempo: https://en.wikipedia.org/wiki/For_an_Angel,
  https://djmag.com/features/how-paul-van-dyks-angel-changed-trance-forever
- JP-8000 supersaw (7 naive saws at 2x + HPF): https://www.gearnews.com/jp-8000-supersaw-synth/;
  detune ratios: https://www.kvraudio.com/forum/viewtopic.php?p=8572590; detune curve:
  https://schollz.com/til/221103
- Goa technique thread: https://gearspace.com/board/electronic-music-instruments-and-electronic-music-production/846372-ultimate-goa-trance-technique-thread.html
- Late-90s mixing habits: https://www.kvraudio.com/forum/viewtopic.php?p=8555143
- Juno chorus BBD: https://modulargrid.net/e/soundforce-chorus-6-
- Reference tracks: https://www.discogs.com/master/34225-Cass-Slide-Perception,
  https://www.discogs.com/master/100573-Taucher-Waters,
  https://www.discogs.com/release/39418-John-Digweed-and-Nick-Muir-Present-Bedrock-Set-In-Stone,
  https://ute-rec.bandcamp.com/album/ute008, https://www.decks.de/track/oprofessionell-ute008/cjh-pm
- KiloMix: parsed from `tgv-kilomix-98.rbs` in https://github.com/varyform/re-rebirth

DSP:
- Szabo, "How to Emulate the Super Saw" (KTH 2010):
  https://www.adamszabo.com/internet/adam_szabo_how_to_emulate_the_super_saw.pdf. The curves are
  also in https://github.com/ycollet/psi-plugins (`common/roland.h`).
- JP-8000 DSP reverse engineering, 39C3:
  https://media.ccc.de/v/39c3-from-silicon-to-darude-sand-storm-breaking-famous-synthesizer-dsps
- ÜBERSAW: https://github.com/Alzy/ubersaw
- Mutable Instruments (Braids, Plaits drums and ensemble, FxEngine, stmlib):
  https://github.com/pichenettes/eurorack
- Cytomic SVF: https://cytomic.com/technical-papers/
- Moog ladder models and their licences: https://github.com/ddiakopoulos/MoogLadders
- Open303: https://github.com/RobinSchmidt/Open303; AcidBox (ESP32): https://github.com/copych/AcidBox
- Dattorro, "Effect Design Part 1": https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf
- Daisy oscillator counts: https://forum.electro-smith.com/t/how-many-oscillators-can-i-run/3524
- TAL-NoiseMaker (GPL-2-only): https://github.com/DISTRHO/DISTRHO-Ports
- Deluge DSP (Q31 + NEON): https://github.com/SynthstromAudible/DelugeFirmware

Sequencing and UI:
- Elektron trig conditions: https://www.elektronauts.com/t/nei-and-pre-trig-conditions/51959;
  song mode: Digitakt manual OS 1.53; complaints: https://www.elektronauts.com/t/trig-conditions-and-song-mode/241960
- Deluge firmware (song/arranger/iterance model): https://github.com/SynthstromAudible/DelugeFirmware,
  https://delugecommunity.com/features/community_features/
- Mutable Instruments Grids and Marbles: https://github.com/pichenettes/eurorack
- Turing Machine (Workshop Computer port): https://github.com/TomWhitwell/Workshop_Computer
- Torso T-1: https://docs.torsoelectronics.com/t1/core-concepts/euclidean-rhythms/
- OP-Z step components: https://teenage.engineering/guides/op-z/step-components
- MC-505 and RM1x: https://www.soundonsound.com/reviews/roland-mc505,
  https://www.soundonsound.com/reviews/yamaha-rm1x; EA-1 motion sequence:
  https://www.soundonsound.com/reviews/korg-electribe-ea1-er1
- ReBirth 2.0 manual (PCF pattern diagrams): https://deepsonic.ch/deep/docs_manuals/propellerhead_rebirth_rb338_v2.01_manual.pdf
- Acid Banger: https://github.com/vitling/acid-banger
- R-VAE: https://eprints-gro.gold.ac.uk/29044/1/VigliensoniMcCallumFiebrink_ICCC2020.pdf;
  GrooVAE: https://magenta.tensorflow.org/groovae
