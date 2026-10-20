# DSP for the trance groovebox

Research pass of 2026-10-17, for a chip with 2 single-issue cores at 360 MHz, a single-precision FPU, no
SIMD, about 430 KB RAM. "≈N cyc" means cycles per sample, worked out as clock x load / fs. [U] uncertain,
[est] estimate. Several hosts didn't resolve (kvraudio, forum.electro-smith, adamszabo.com, kth.se,
pjrc.com); those claims rest on search summaries and on code read from GitHub.

## 1. Supersaw

**Szabo, "How to Emulate the Super Saw"** (KTH 2010,
https://www.adamszabo.com/internet/adam_szabo_how_to_emulate_the_super_saw.pdf). The PDF couldn't be
opened; these coefficients agree across code ports, e.g. `common/roland.h` in
https://github.com/ycollet/psi-plugins (LGPL/GPL-2+) and komposter's `modules.c`.
- 7 saws.
- Detune curve, knob x in [0,1]:
  `10028.7312891634x^11 - 50818.8652045924x^10 + 111363.4808729368x^9 - 138150.6761080548x^8 +
  106649.6679158292x^7 - 53046.9642751875x^6 + 17019.9518580080x^5 - 3425.0836591318x^4 +
  404.2703938388x^3 - 24.1878824391x^2 + 0.6717417634x + 0.0030115596`
- Per-saw offsets, multiplied by the curve, asymmetric: -0.11002313, -0.06288439, -0.01952356, 0,
  +0.01991221, +0.06216538, +0.10745242.
- Mix: side = -0.73764x^2 + 1.2841x + 0.044372; centre = -0.55366x + 0.99785.
- A pitch-tracked HPF near f0 removes sub-fundamental aliasing; random phases at note-on. Szabo credits the
  aliasing with part of the "airiness".

**39C3 reverse engineering** (giulioz, December 2025,
https://media.ccc.de/v/39c3-from-silicon-to-darude-sand-storm-breaking-famous-synthesizer-dsps; the
JE-8086 chip emulator), per secondary write-ups:
- 7 naive saws on 24-bit integer phase accumulators at **88.2 kHz**.
- Integer detune table {0, +128, -128, +816, -824, +1408, -1440}, scaled by `(pitch x detune) >> 7`.
- Centre saw at fixed gain; sides scaled linearly by MIX; HPF after the sum. "No modulation, no phase
  tricks, no chorus".
- So Szabo's polynomials fit the knob mappings, not the algorithm. Random phase still reported [U].
- Clean summary: ÜBERSAW (MIT, Daisy, 96 kHz, one-pole HPF at f0), https://github.com/Alzy/ubersaw and its
  `docs/algorithm.md`.
- Braids SAW SWARM (MIT): 7 naive saws at 96 kHz, an SVF HPF following pitch, random phases on strike;
  fits Braids' ≈750 cyc on a 72 MHz Cortex-M3 in fixed point.

**At 44.1 kHz** naive saws alias much worse than at 88.2 kHz [est]:
- 7 polyBLEP saws with 1/dt precomputed per block, ≈15-25 cyc each, plus HPF: ≈130-180 cyc per voice.
- 2x naive saws plus a halfband decimator: similar cost, closer to the original's aliasing.
- One shared mip-mapped saw table in RAM: ≈8-12 cyc per saw.
- Avoid DPW in float at bass pitches (Surge's Modern oscillator does 3rd-order DPW in double).

Other supersaws: Surge XT (GPL-3, 16-voice unison), Vital (GPL-3, SIMD wavetables), the Deluge (unison up
to 8), dukesrg/logue-osc (licence not stated). DaisySP has none.

## 2. Band-limited oscillators and FM

**Daisy measurements** (STM32H750, Cortex-M7, 480 MHz, 10k cyc/sample at 48 kHz;
https://forum.electro-smith.com/t/how-many-oscillators-can-i-run/3524; rate and block size not stated):

| DaisySP object | Count at load | ≈cyc each |
|---|---|---|
| polyBLEP saw | 92 at 95 % | 103 |
| polyBLEP square | 46 at 96 % | 209 |
| `sinf` sine | 47 at 96 % | 204 |
| naive triangle | 105 at 99 % | 94 |

DaisySP is per-sample, not inlined, and divides inside the BLEP; tight block loops should be 3-6x cheaper
[est].

- polyBLEP (Välimäki/Huovilainen 2007) is the best MCU fit; polyBLAMP (Esqueda/Välimäki/Bilbao, DAFx-16)
  handles triangle and sync corners at similar cost.
- minBLEP needs a residual table (e.g. 32 taps x 64 phases ≈ 8 KB float); bursty cost. Teensy's
  `BandLimitedWaveform` uses such a step queue.
- Mip-maps: the Deluge uses about 20 per-octave tables shrinking from 2^13 to 2^9 samples; ≈8 KB int16 per
  waveform when halving per octave. Keep tables in SRAM: random reads from XIP flash miss the cache.
- Plaits (MIT): VA oscillators use polyBLEP with this-sample/next-sample (1 sample latency) and per-block
  parameter interpolation; the wavetable oscillator uses integrated int16 tables + differentiator +
  one-pole, no mip-maps; stmlib tables for sine, semitones→ratio, SVF tan.
- FM: sine from a 1024-entry float table (4 KB) with linear interpolation (≈8-10 ops), or a 5th/7th-order
  odd polynomial. Plaits' six-op engine (2 voices, DX7-compatible) fits a 72 MHz M4 at 48 kHz: ≤1500 cyc
  for 12 operators plus envelopes, so ≲100 cyc per operator all-in. PicoDexed (MIT, MSFA-based): 16 DX7
  voices at 24 kHz on an RP2040 at 250 MHz [U]. Estimates: 2-op ≈30-40 cyc, 4-op ≈70-100 cyc per voice.

## 3. Filters

- **Cytomic TPT SVF** (https://cytomic.com/technical-papers/, SvfLinearTrapOptimised2): ≈5 mul + 6 add plus
  output mixing, ≈15-30 cyc [est]; coefficients (one tan approximation, one divide) every 8-32 samples;
  stable under modulation; no oversampling while linear. Teensy's fixed-point 2x Chamberlin SVF: "≈4 % of
  Teensy 3.1", ≈87 cyc at 96 MHz.
- **Moog ladders:** Stilson-Smith (cheap, linear); Huovilainen (DAFx-04: 5 tanh a sample plus 2x
  oversampling; the old DaisySP MoogLadder measured ≈490 cyc each); the current DaisySP/Teensy
  `LadderFilter` (MIT, Huovilainen model, 4x linear oversampling); Zavalishin's ZDF ladder ("The Art of VA
  Filter Design", free PDF; ≈30-40 ops when linear); Mystran's cheap non-linear ZDF
  (https://www.kvraudio.com/forum/viewtopic.php?p=4925988: nonlinearities on half-sample-delayed state,
  treated as resistances; 4-5 tanh). A Padé tanh, x(27+x^2)/(27+9x^2), costs one divide. Licences per
  model in https://github.com/ddiakopoulos/MoogLadders: Huovilainen LGPL-3; Stilson, Microtracker,
  Krajeski, Hyperion Unlicense; Improved ISC; RK BSD.
- **303:** Open303 (MIT, https://github.com/RobinSchmidt/Open303). TeeBeeFilter is 4 one-poles with a
  1-pole HPF in the feedback path and polynomial-fit coefficients; its `shape()` nonlinearity is commented
  out. The original runs 4x oversampled in double, with a per-sample `pow()` and an elliptic anti-alias
  filter (X0X's port is float, 2x). ESP32 ports: copych/ESP32_Open303 and AcidBox (MIT,
  https://github.com/copych/AcidBox: 2x 303, 808-style drums, reverb, delay, compressor on a dual-core
  ESP32 at 44.1 kHz; no CPU figures). Diode ladders: karrikuh's cheap ZDF diode ladder; the KVR "Diode
  ladder filter" thread (MIT C++, recommends ≥2x) [U]; sst-filters' Diode Ladder and K35 from Odin 2
  (GPL-3, SSE). Estimate ≈150-250 cyc at 2x.
- **Oversampling:** linear TPT filters need none; driven resonant ladders need 2x. The Deluge enables 2x
  only when resonance is high and CPU allows, decimating crudely by taking every second sample. Use cubic
  or ADAA saturators.
- **"90s VA"** [opinion/U]: JP-8000, Nord Lead and early Virus filters were fairly clean digital 12/24 dB
  multimodes with saturation stages; the Virus's Moog-style mode came later. A TPT SVF (cascaded for
  24 dB) plus a ZDF ladder with input drive covers the target.

## 4. Chorus, phaser, flanger

- **TAL-NoiseMaker's chorus is GPL-2-only** (no "or later"), so it can't be combined with GPL-3
  (https://github.com/DISTRHO/DISTRHO-Ports, `ports-juce5/tal-noisemaker`). TAL-Chorus-LX is closed
  freeware. Reimplement from parameters: two units, each an L/R pair with **antiphase triangle LFOs**;
  rates 0.5 Hz (I) and 0.83 Hz (II); 7 ms maximum, sweeping 0.7-4.9 ms; allpass-style fractional read,
  one-pole LPF, DC block; wet x1.4 added to dry.
- **Plaits ensemble** (MIT, `plaits/dsp/fx/ensemble.h`): 2 x 511-sample lines, 3 taps a side at 120°;
  sine LFOs at 0.75 Hz and 6.57 Hz; centre 192 samples, ±176.
- Estimates: stereo Juno chorus ≈40-70 cyc with ≈2 x 310 samples of RAM. BBD-accurate models (Holters/Parker
  DAFx-18; Surge Ensemble, GPL-3) aren't needed.
- Phaser: 4-8 first-order allpasses at ≈4 ops each. Flanger: ≈20 cyc a channel. Widening: unison voices
  panned alternately, a 10-25 ms Haas delay, M/S gain, ping-pong.

## 5. Reverbs and delays

- **Dattorro plate** (https://ccrma.stanford.edu/~dattorro/EffectDesignPart1.pdf), lengths at 29,761 Hz:
  input allpasses 142/107/379/277; tank 672(+16)/4453/1800/3720 and 908(+16)/4217/2656/3163; 22,494
  samples in all. At 44.1 kHz ≈33.3k samples, **67 KB int16** (133 KB float); at 22.05 kHz **≈33 KB**.
- **MI FxEngine** (MIT): a power-of-two uint16 ring, one write pointer, linear interpolation, LFOs every
  32 samples. `FORMAT_12_BIT` is int16 scaled by 4096 (3 bits of headroom).

| Module | Buffer | Memory | Rate | Delay total |
|---|---|---|---|---|
| Clouds | 16384 x 12-bit | 32 KB | 32 kHz | 16,375 samples |
| Rings | 32768 x 16-bit | 64 KB | 48 kHz | 21,617 samples |
| Elements | 32768 x 16-bit | 64 KB | 32 kHz | — |

- **Freeverb** (public domain): Teensy mono 21 % of a Teensy 3.2 (≈457 cyc at 96 MHz, 22 KB); stereo 18 %
  of a Teensy 3.6 (≈735 cyc at 180 MHz, 45 KB); int16. Dattorro should be cheaper (≈100-200 cyc [est]) and
  closer to 90s plates; at 22.05 kHz with halfband resampling it halves CPU and memory.
- **Deluge reverbs:** Freeverb; "Mutable" (Rings-derived, a float 128 KB buffer); "Digital", a Dattorro
  scaled by `kRatio` (its `output_right` is computed from `left_sum`, which looks like a bug).
- **Faust `reverbs.lib`:** dattorro_rev, zita_rev1/fdnrev0 are STK-4.3; jpverb and greyhole MIT; vital_rev
  GPL-3.
- **Delays:** 1 s stereo at 44.1 kHz is 176 KB int16, 132 KB packed 12-bit, 88 KB 8-bit mu-law; 2 s int16
  (353 KB) won't fit. At 138 BPM a 1/4 is 435 ms and a dotted 8th 326 ms, so ~0.5 s a side covers most
  uses. Mu-law gives random access; Clouds' lo-fi mode is 8-bit mu-law at 16 kHz ("cassette with less
  hiss, more distortion"). Block floating point costs ≈8.3-8.5 bits a sample. ADPCM only works for fixed
  taps and re-quantizes every repeat.
  (The proposal later settled on int16 or 12-bit block-float at half rate for the delay, and 8-bit
  block-float for reverb lines only: see section 4.3 of the proposal.)

## 6. Drums

- **Plaits** (MIT, `plaits/dsp/drums/`): AnalogBassDrum (808-style pinged resonator with FM and drive);
  SyntheticBassDrum, commented "Inadvertently 909-ish" (distorted sine/triangle blend, FM envelope up to
  3.5x pitch sweep, a 5 ms click from a 5 kHz SVF LP plus noise burst, a transistor VCA, a one-pole tone
  filter); analog and synthetic snares; HiHat (6 squares at 1/1.304/1.466/1.787/1.932/2.536 of ~414 Hz, SVF
  BP with Q = 3 + 3·tone, a swing VCA, then an SVF HP). Peaks (MIT, fixed point) has 808-style BD/SD/HH.
- Estimates: kick ≈40-80, metallic hat ≈60-100, noise hat ≈20-30 cyc.
- 909 facts: the hats and cymbals are 6-bit samples; the clap is noise through a ~1.1 kHz BP (Q ~2) with the
  envelope retriggered about 4x at ~11 ms plus a tail [U]. Sampled 909 audio isn't automatically GPL-clean
  (X0X ships ER-99's samples under GPL-3; a synthesized metal bank is the fallback).

## 7. Codebases

- **Deluge** (GPL-3, https://github.com/SynthstromAudible/DelugeFirmware): RZ/A1L Cortex-A9 400 MHz **with
  NEON**, 64 MB SDRAM; Q31 plus NEON intrinsics. Polyphony "limited by CPU" (64 voices), with quality and
  voice culling driven by `cpuDireness`. Oscillators: mip-mapped saw/square, "analog" tables, crude saws at
  low pitch, PW by phase reset, sync, wavetable, FM, a DX7 engine. Filters: Q31 ladders (12/24/24-drive),
  an SVF. FX: chorus, flanger, phaser, grain, delay, reverbs, compressor.
- **Mutable Instruments** (STM32 code MIT, AVR code such as Grids GPL-3; https://github.com/pichenettes/eurorack):
  stmlib (Svf, OnePole, ParameterInterpolator), FxEngine, the ensemble, the drums, oscillators, Stages
  envelopes.
- **DaisySP** (MIT; ReverbSc etc. in DaisySP-LGPL), https://github.com/electro-smith/DaisySP: generic,
  unoptimized.
- **Teensy Audio** (MIT), https://github.com/PaulStoffregen/Audio: int16 with M4 SIMD (which this core lacks).
- **logue SDK** (BSD-3), https://github.com/korginc/logue-sdk: peterall/eurorack-prologue ports Plaits and
  Elements (Elements cut to 24+2 filters at 24 kHz for CPU and the 32 KB limit); the NTS-1 has about 2x the
  prologue's CPU.
- **Surge XT / sst-filters / sst-effects** (GPL-3): raw SSE; needs simde or scalarizing.
- **Vital** (GPL-3): SIMD throughout; too heavy as is.
- **Dexed** (GPL-3) / **MSFA** (Apache-2.0): integer FM core, a good fit.
- **OB-Xf** (GPL-3), https://github.com/surge-synthesizer/OB-Xf.
- **TAL-NoiseMaker:** GPL-2-only; reference only.
- **Airwindows** (MIT): mostly double precision; convert to float.
- **Faust:** licences per function.
- **AMY** (MIT), https://github.com/shorepine/amy: Juno-6 and DX7 engines; 6-8 Juno voices on an ESP32-S3
  reported [U].
- **JE-8086 / gearmulator:** chip emulators; nothing to lift.

## 8. Numbers to extrapolate from

| Platform | Data point |
|---|---|
| Plaits | STM32F373 M4F 72 MHz, 48 kHz, block 12: ≤1500 cyc for any engine |
| Rings | F405 168 MHz, 48 kHz: 3500 cyc; modal resolution 64/poly - 4 (60 modes mono) |
| Elements | F405 168 MHz, 32 kHz: 5250 cyc; 52+8 filters |
| Braids | M3 72 MHz, 96 kHz, no FPU: 750 cyc |
| Daisy synth threads | 4-8 DaisySP voices typical; one 8-voice VA went from 5 to 8 voices with a faster ladder |

M4 numbers should transfer roughly 1:1 to this single-issue core; multiply M7 (Daisy) numbers by about
1.3-1.8 [est]. Put hot loops and lookup tables in SRAM (32 KB shared I-cache, XIP); process one module
across the whole block at a time.

Budget sketch [est], of 16.3k cyc in all: 16 supersaw voices with filter 4-5k; 909 kit ≈0.6k; a 303 at 2x
≈0.35k; chorus 0.06k; half-rate Dattorro ≈0.2k; ping-pong delay 0.05k.
