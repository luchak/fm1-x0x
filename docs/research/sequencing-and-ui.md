# Sequencing, arrangement and UI references

Research pass of 2026-10-17. WebFetch failed everywhere; the code for Grids, Marbles, the Turing Machine and
the Deluge was read from cloned repositories (verified). The rest comes from manual or forum text in search
results. (mem) marks memory, to be checked.

## 1. Elektron

**Track/machine model.** The machine changes only the SRC page; filter, amp, FX sends, LFOs and the
sequencer are the same on every track.
- **Digitakt II:** 16 audio or MIDI tracks, 128 steps a track. SRC machines Oneshot, Werp, Stretch, Repitch,
  Grid; FLTR machines Multimode, Lowpass4, EQ, Comb-/+, Legacy. 3 LFOs a track; a Euclidean generator a
  track (two pulse generators with a boolean operator, unverified). Pages TRIG/SRC/FLTR/AMP/FX/MOD, 8
  parameters each (https://elektron.se/product/digitakt-ii).
- **Digitone II:** the same pages; FM Tone / FM Drum / Wavetone / Swarmer. "Perform Kit" keeps sound tweaks
  across pattern changes.
- **Syntakt:** 8 digital and 4 analog tracks, each with its own machine pool.
- **A4 and Rytm (mem):** fixed-voice page sets on the A4; kits, performance macros, scenes.

**Sequencer**
- P-lock: hold a step and turn a knob. A trigless lock changes parameters without a note; add a condition
  for a conditional lock.
- Trig conditions: X% (a fixed table from 1 to 99); A:B (B from 2 to 8; true on loop A of every B, so 2:3
  plays loops 2, 5, 8…); FILL / !FILL; PRE / !PRE (the last evaluated condition on the same track; trigs
  without a condition ignored); NEI / !NEI (the same, from the previous track); 1ST / !1ST; LST / !LST (the
  last loop before a queued change, newer OSes) (https://www.elektronauts.com/t/nei-and-pre-trig-conditions/51959).
- Micro-timing ±23/384 (1/24-step resolution) (mem).
- Retrig: rate 1/1 to 1/80 including tuplets, length, a velocity fade; all p-lockable (how rolls are made).
- Parameter slide: a slide trig glides every locked value from the previous trig over one step.
- Sound lock: a step can use a different sound from the project's sound pool.
- LFO (mem): SPD x MULT (synced or fixed), FADE, DEST, DEPTH, start phase SPH; waves tri/sine/square/saw/exp/
  ramp/random; modes FREE, TRIG, HOLD (S&H), ONE (one-shot), HALF. Phase can't be p-locked (a complaint).
- Scale per pattern or per track: LEN x SCALE (1/8 to 2x), a master LEN (can be INF), and a CHANGE length
  that sets when a queued pattern switches.
- Pattern changes: Sequential, Direct Start, Direct Jump, Temp Jump.

**Song mode** (OS 1.40, September 2022, on Digitakt, Syntakt and Digitone; built into Digitakt II and
Digitone II; Digitakt manual OS1.53,
https://www.elektron.se/wp-content/uploads/2026/09/Digitakt-User-Manual_ENG_OS1.53_260909.pdf):
```
Project { Song songs[16] }
Song    { Row rows[<=99]; End = STOP | LOOP }
Row     { label (VERSE/CHORUS/FILL… or pattern name); ptn;
          repeat 1(OFF)..32; length (steps played, default = pattern);
          bpm (or pattern's); per-track mute mask }
```
- FUNC+PLAY resets a row to the pattern's BPM, length and mutes; SONG MODE+LEFT loops the playing row;
  FUNC+DOWN inserts a copy of the row, FUNC+UP deletes it.
- Benchmark, the Octatrack arranger (mem): 8 arrangements of up to 256 rows; pattern rows add OFFSET, scenes
  and transpose; special rows LOOP (count or infinite, nestable), JUMP (forward only), HALT, REM (comment).
- Praise: rows reuse a pattern with different mutes; row length gives sections of any length.
- Complaints (https://www.elektronauts.com/t/trig-conditions-and-song-mode/241960): mutes made while playing
  get saved into the song; live pattern changes can't be recorded into a song; trig-condition counters
  aren't reset per row or chain; no fills per row, so many duplicate patterns; no per-row transpose or
  automation; A:B tops out at 8, so 16- and 32-bar fills are impossible.

## 2. Deluge (GPL-3, https://github.com/SynthstromAudible/DelugeFirmware)

Data model, from `song.h`, `clip_instance.h`, `note.h`, `note_row.h`, `iterance.h`:
```
Song     { sessionClips[]; arrangementOnlyClips[]; outputs(list);
           sections[] {numRepetitions, launchMIDI}; key; swing; tempo }
Output   { ClipInstance clipInstances[] }   // the arranger lane
ClipInstance { pos; length; Clip* clip }   // points to a shared clip
Clip     { section; loopLength; launchStyle DEFAULT|FILL|ONCE;
           activeIfNoSolo; armState (pending launch); repeatCount }
NoteRow  { loopLengthIfIndependent (polymeter); sequenceDirection; muted;
           per-row automation; default prob/iterance/fill; notes[] }
Note     { pos; length; velocity; lift; probability; iterance; fill }
Iterance { divisor 1..8; bitset<8> }        // "plays on loops 1 and 3 of 4";
                                           // divisor 0 = FIRST or LAST
```
- Song view: clips in rows (the community "grid" layout puts tracks in columns), mute and section columns; a
  section launches a group of clips and can repeat N times. Clip view: 16x8 pads (steps x notes); hold a pad
  and turn an encoder for note parameters. Arranger view: a row per output, time horizontal, pads are clip
  instances.
- Live capture: Record + Play in song view sets `RecordingMode::ARRANGEMENT`; a clip launch calls
  `beginInstance(song, recordPos)`, a stop `endInstance(pos)`; knob moves become arranger automation;
  editing a clip during capture clones it into an arrangement-only clip; anything after the record start is
  cleared.
- Community firmware: probability per row, iterance (combinable with probability and fill from 1.3), FILL
  notes, FILL clips, per-row Euclidean (official 4.0) (https://delugecommunity.com/features/community_features/).
- Lessons: record events that point to shared patterns, not copies; quantize launches; clone a pattern edited
  during capture and offer "make unique"; users complain capture is strictly linear and lost mid-clip mutes,
  so support punch-in and capture mutes (https://forums.synthstrom.com/discussion/comment/19460).

## 3. Late-90s grooveboxes

- **MC-303 (1996):** about 133 preset patterns, a 34-style arpeggiator, RPS (about 211 riffs, one per key);
  knob moves recordable into patterns.
- **MC-505 (1998):** 714 preset patterns; MEGAMix swaps single parts in from other patterns live; RPS: 60
  sets of up to 16 phrases, pointers to patterns (https://www.sweetwater.com/sweetcare/articles/copying-rps-phrases-roland-mc505/);
  a large arpeggiator "Beat Pattern" library (accent position and gate per step)
  (https://www.soundonsound.com/reviews/roland-mc505).
- **JP-8000/8080:** arp beat patterns (90 on the 8080), RPS on keys, Motion Control recording knob moves.
- **RM1x:** 960 preset patterns, 7,726 preset phrases; a section has 16 tracks with a phrase each; "Patch"
  puts any phrase on any track (https://www.soundonsound.com/reviews/yamaha-rm1x).
- **EA-1/ER-1:** Motion Sequence records knob moves per step, switchable and overwritable apart from the
  notes (https://www.soundonsound.com/reviews/korg-electribe-ea1-er1).
- Used as genre starting points, with parts swapped live and knob motion as the expressive layer. Takeaway:
  build the library from swappable per-track phrases, not whole patterns.

## 4. Procedural sequencers

**Euclidean** (Bjorklund, as in Grids `resources/lookup_tables.py`):
```
p=[[1]]*k+[[0]]*(n-k); while k: c=min(k,len(p)-k); k,p=c,[p[i]+p[k+i] for i in range(c)]+p[c:k]+p[k+c:]
```
Bresenham gives the same necklace, maybe rotated: `hit(i) = (i*k) % n < k`; rotation `hit((i+r) % n)`; a
second layer can carry accents.

**Grids** (https://github.com/pichenettes/eurorack/tree/master/grids; AVR code including Grids is GPL-3.0,
STM32 code including Marbles is MIT, per the repo README)
- Data: 25 nodes in `drum_map[5][5]`; each node 96 bytes = 3 parts (BD, SD, HH) x 32 steps of 32nd notes
  (24 ppqn, 3 pulses a step), one bar of 4/4. Each byte is an importance 0-255; most odd steps are 0.
- Map read: `i = x>>6, j = y>>6`; `lvl = mix(mix(n[i][j], n[i+1][j], x<<2), mix(n[i][j+1], n[i+1][j+1], x<<2), y<<2)`,
  with `mix(a,b,t) = (a*(255-t) + b*t) >> 8`.
- At step 0, per part: `pert_i = (rand8 * (chaos>>2)) >> 8` (0-63), skipped when the swing option is on.
- Each step: `lvl = min(255, lvl + pert_i)`; fire if `lvl > 255 - density_i`; accent if `lvl > 192`.
- Swing option: chaos becomes swing, `±(chaos*43 >> 8)` pulses, alternating on `step & 2`.
- Euclidean mode: `len = (knob>>3) + 1` (1-32), `fill = round((density>>3)/31 * len)`, on 16ths.
- Map source: "machine learning techniques" on "a large collection of electronic music tracks and drum
  loops"; the method isn't published.

**Turing Machine** (Workshop Computer port, GPL-3.0+,
`releases/03_Turing_Machine/Rev_1_5_Code/Turing.cpp` in https://github.com/TomWhitwell/Workshop_Computer):
```
s:uint16, L=2..16; each clock: r=uniform[safe,max-safe)
fb=(s>>(L-1))&1; if (r>=pot) fb^=1; s=(s<<1)|fb
note = pool[(s&0xFF)*poolSize>>8]   // pool = root + scale x octaves
```
Fully clockwise: never flips (a locked loop of L). Fully counter-clockwise: every bit flips (each pass
inverted, a loop of 2L). Noon: random; 3 or 9 o'clock: the loop "slips". Hardware extras: Write forces the
fed-back bit; Pulses gives bit gates; Volts sums the knob voltages of set bits.

**Marbles** (MIT, `marbles/random/`): T models Bernoulli (complementary or independent), clusters, drums,
divider, three-states, Markov. **Déjà vu**, a good universal mutation knob: `p = (2dv - 1)^2`; below 0.5,
with probability p write a new random value into the loop; above 0.5, with probability p jump to a random
point in the loop; at 0.5, loop exactly; loop length 1-16.

**Others**
- Torso T-1 (https://docs.torsoelectronics.com/t1/core-concepts/euclidean-rhythms/): Euclid core (steps,
  pulses, rotate, division); repeater (repeats, time, pace (speed up / slow down), offset (velocity ramp));
  groove (8 timing and velocity templates); accent; sustain; phrase/range (pitch contour from an LFO or
  cadence shapes); RANDOM + knob randomizes a parameter; up to 16 parameter "cycles" played in order.
- OP-Z step components: pulse, pulse hold, multiply, velocity, ramp up/down, random, portamento, sweep,
  tonality, jump, 3 conditional "sparks" (https://teenage.engineering/guides/op-z/step-components).
- Polyend Play: Fill (density, Euclid, kick/snare/hat generators, re-roll); Randomize (amount, re-roll, then
  commit); per-step Chance/Action (https://polyend.com/whats-up/polyend-play-10-ways-to-get-creative-using-chance-action/).
- Live 12 (mem): generators Rhythm, Seed, Shape, Stacks, Euclidean; transformations Arpeggiate, Connect,
  Ornament, Recombine, Span, Strum, Time Warp (https://www.ableton.com/en/live-manual/12/midi-tools/).
- Arps: note order x octave range x a 16-step accent/gate "beat pattern" from a library (JP-8000/MC-505).

## 5. ReBirth's PCF and acid generators

- **PCF is ReBirth 2.0's Pattern Controlled Filter:** an LP/BP filter whose cutoff follows a preset step
  pattern and restarts with each pattern, in effect a trance gate. The manual has "PCF Pattern Diagrams"
  (https://deepsonic.ch/deep/docs_manuals/propellerhead_rebirth_rb338_v2.01_manual.pdf; a clone:
  https://github.com/ReinerBforartists/pcf_rebirth).
- **Acid Banger** (CC BY 4.0, GPL-3-compatible per the FSF; https://github.com/vitling/acid-banger
  `src/pattern.ts`): root random in 16-30 with an offset from 8 hand-made sets such as `[0,0,12,24,27]`;
  per-step note chance 0.6 on quarters, 0.5 when `i%3==0`, 0.3 on 8ths, 0.1 otherwise; accent 30 %, glide
  10 %. Drums: four-on-the-floor or electro kick, offbeat hats, backbeat or "skip" snare.
- **Data:** no clean open 303 dataset, only fan transcriptions
  (https://www.scribd.com/document/481240011/Famous-TB-303-Patterns-Source).
  https://github.com/stephenhandley/DrumMachinePatterns is JSON of Bardet's *200/260 Drum Machine
  Patterns*: MIT code, but the patterns come from copyrighted books.

## 6. Tiny models

- GrooVAE/MusicVAE: LSTM VAEs over 9 drum classes, 2 bars, with velocity and offset; several million
  parameters (estimate), too big as is. Magenta Studio ships Drumify and Groove
  (https://magenta.tensorflow.org/groovae).
- GrooveTransformer (NIME 2024): a lightweight VAE in a Eurorack module, one-voice rhythm to a full groove
  (https://nime.org/proc/nime2024_39/).
- R-VAE: a small VAE trained on tiny user datasets with a 2-D latent space explored live, close to a learned
  Grids (https://eprints-gro.gold.ac.uk/29044/1/VigliensoniMcCallumFiebrink_ICCC2020.pdf).
- Vogl & Knees: EDM drum variations with a deviation slider (https://nime.org/proc/nime2017_rvogl/).
- **Options for the device:** (a) bake a learned map: sample a trained decoder on a 5x5 to 8x8 grid and
  store uint8 tables in Grids' format (144 B a node for 9 voices x 16 steps); run time is interpolation plus
  a threshold; the safest. (b) A tiny MLP: z (2-4) → 64 → 64 → probabilities, about 10k int8 weights.
  (c) A position-conditioned n-gram: P(hit | step mod 16, previous hits), or bass tokens of (interval, gate,
  accent, slide). (d) Mutation: déjà vu, or bit flips weighted by metrical position; a genetic algorithm
  scoring density and syncopation plus Hamming distance to the parent.
- **Datasets:** Groove MIDI Dataset (CC BY 4.0, 13.6 h, 1,150 files of human drummers, little electronic;
  https://magenta.tensorflow.org/datasets); E-GMD, Lakh MIDI, Slakh (mem; CC BY 4.0, but Lakh's songs are
  copyrighted); EDM-TR8 (CC BY 4.0, audio; https://zenodo.org/records/13257814). **No open trance MIDI
  dataset**; 50-200 hand-written phrases would cover the style.

## 7. Small-screen UI

- Baud Girl FM-1 VA: alternative firmware (September 2026) with a VA engine and an ACID sequencer mode
  (https://sonicstate.com/news/2026/09/29/free-custom-firmware-for-mvave-fm-1-/); see fm1-ecosystem.md.
- M8 (mem): 8 keys; screens on a 2-D map (Song → Chain → Phrase → Instrument → Table) reached with
  SHIFT+arrows and shown on a mini-map; EDIT+arrows changes values; one grammar on every screen
  (https://cs.uwaterloo.ca/~plragde/flane/TM8C/Prelude.html).
- Model:Cycles/Samples: one knob a parameter, the main macros always on knobs; hold a trig and turn to lock.
- OP-1/OP-Z: encoder colours match the screen.

**Sound tweaking and sequence editing on 4 encoders**
1. Hold, don't switch modes: no step held, the encoders edit the sound; a step held, they p-lock it on the
   visible page; step + black key for condition, micro-timing, retrig, slide.
2. Pages SRC/FLTR/AMP/FX/MOD with a MACRO home page of 4 parameters a machine.
3. The 3 extra encoders: track, page/row, and the focused trig property or a performance macro.
4. REC + turn records knob motion into locks.
5. Colour quadrants matching the encoders; a large value pop-up; a step strip with lock dots.
6. 16 white keys are steps; 11 black keys tracks/pages/functions; a keyboard mode for notes.

**Trance-specific additions:** A:B up to 16 or 32; an LST condition; per-row ramps for 16-32-bar sweeps;
retrig rate and velocity ramps; a PCF-style gate; Elektron song rows combined with Deluge-style capture
(each pattern or mute change starts a new row, quantized to the bar, identical rows merge into a repeat).
