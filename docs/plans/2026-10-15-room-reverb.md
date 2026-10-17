# ROOM: the second reverb type

ROOM is X0X's second reverb (REV TYPE), next to CLASSIC (9W9's four combs and two allpasses). It is
an 8-line feedback delay network: smoother and denser than CLASSIC, with no metallic ring, at
CLASSIC's decay and loudness for the same knob settings. The code is in `firmware/src/dsp/fxbus.c`
(the ROOM section) and `fxbus.h` (`fx_room_t`); the test is `tests/host/reverb_test.c`.

## Constraints

- **Memory: ROOM fits in CLASSIC's buffer.** `tools/build.py` fails a build with less than 8 KB of
  pool free, and the image is close to that, so ROOM can't add memory. The two types share CLASSIC's
  reverb buffer (18,008 B, `fx_rv_buf_t`: a union of the two layouts), and `fx_room_t` is 17,676 B.
  Switching type clears the buffer (the tail
  is cut).
- **CPU.** The FX bus runs on core 0 in the audio interrupt. ROOM costs about 41 timer ticks per
  sample (1 tick = 15 cycles = 0.18 % of the core), CLASSIC about 29.
- **Host and device compute the same samples** (`-ffp-contract=off`), so the DSP is plain C, and
  the host tests check what the device runs.

## Signal flow

1. **High-pass** (RvLowCut, CLASSIC's own filter).
2. **Four allpass diffusers** in series: 211, 157, 397 and 293 samples, gains 0.7, 0.7, 0.65 and 0.65,
   stored as int16 at x4096. ROOM's lines average about 38 ms, so at the default decay a sound makes
   only about 10 trips around the network; the diffusers supply the early density. With two of them
   ROOM sounded empty.
3. **The network:** 8 delay lines, each followed by a one-pole damper, mixed by an 8-point
   Walsh-Hadamard transform (scaled by 1/sqrt(8), so it is orthogonal), with the input added to each
   line at a sign (`+ + - - + - - +`).
4. **Outputs:** two rows of the mix, before the input is added: L is row 1 (`+ - + - + - + -`), R is
   row 6 (`+ + - - - - + +`). The signs keep L, R and the input uncorrelated.

### The lines

Rings of 1137, 1297, 1457, 1553, 1749, 1877, 2101 and 2213 samples; lines 0, 2, 4 and 6 delay one
sample more in their dampers, so the delays are 1138, 1297, 1458, 1553, 1750, 1877, 2102 and 2213
(25.8 to 50.2 ms):

- **No common factor but 2** between any two delays (the odd ones are primes, the others twice an odd
  number), so no two lines have a resonance in common but at DC and Nyquist.
- **Spread over about an octave,** the shortest just above CLASSIC's shortest comb, so the room
  sounds the same size. Shorter sets gave tremolo and a low-mid resonance.
- **Two residues modulo 16 in the rings** (lines 0 to 3 are 1 mod 16, lines 4 to 7 are 5;
  `_Static_assert`s check it). This is for speed: see the block format below.
- **Four residues in the delays** (1, 2, 5 and 6), for the sound: see below.

The set was chosen by ABX against an earlier one; it was distinguishable but not worse. Another
pairwise-coprime set with the same offset had clear beating on long tails, so a new set needs a
listening test, not just the arithmetic.

### The delays' residues modulo 16

Every echo that has gone round the network k times has a delay that is a sum of k line delays. When
every delay is the same modulo 16 (r), that sum is `r k mod 16` whatever the path: at any time in the
early tail only a few trip counts are possible, so the network's output sits on a few places in
every 16 samples. Over a short window it is then close to a pulse train of period 16, and its
spectrum repeats every 44100 / 16 = 2756 Hz. The diffusers don't remove this (their spectrum
multiplies the network's), and it fades only as trip counts overlap, by about 0.8 s.

With delays of several residues, each trip shrinks that pattern's part at harmonic m of 2756 Hz by
`|phi(m)| = |mean over the lines of exp(2 pi i m d_j / 16)|`, at worst over m = 1 .. 15:

| Delays' residues | Fetch code | Worst `|phi|` per trip |
|---|---|---|
| One (the first set: all 3) | 1 copy | 1.00 (never) |
| Two adjacent (one ring residue, the dampers' sample) | 1 copy | 0.98 |
| Two ring residues | 2 copies | 0.98 |
| **Two ring residues 4 apart, and the dampers' sample (1, 2, 5, 6)** | **2 copies** | **0.71** |
| Eight, the best set | 8 copies | 0.31 |

Measured on ROOM's output (clicks and snare bursts, as the correlation of the detrended log spectrum
with itself 2756 Hz along, against its neighbouring lags; white noise gives 0), the first set showed
0.10 to 0.19 from 60 to 150 ms after a hit; this one 0.06 to 0.08 at 60 ms, and 0.01 to 0.07 from
100 ms. Eight fetch copies, one per line's own place, cost too many instruction-cache misses. The
residues 1 and 5 (both 1 mod 4) make each delayed line's delay twice an odd number, so no two
delays share a factor of 4: the pairs that also reach 0.71 but leave the delayed lines at 0, 4, 8 or
12 mod 16 would share 4 or 16, a resonance at 11 kHz or every 2756 Hz.

### Loop gain and damping

RvDecay and RvTone map to two decay times, at DC (`t_dc`) and at Nyquist (`t_ny`). For a line of
length L:

- the loop gain is `g = 10^(-3 L / t_dc)`, so every line decays at `t_dc` at DC;
- the damper is a one-pole lowpass, `lp = a lp + (1 - a) in`. A reference line of the lines' mean
  length gets the damper whose gain at Nyquist is `10^(-3 L / t_ny) / g`. Each line's damper then
  matches the reference's decay at 3 kHz: its response there, relative to DC, is the reference's to
  the power `L / L_ref` (a closed form: a quadratic in `a`).

A one-pole damper has two parameters, so a line can match its target decay at two frequencies only.
Matched at DC and Nyquist (the textbook choice), lines of different lengths drift apart in between,
the more so the larger `t_dc / t_ny`. At the longest, darkest setting (2.15 s against 0.13 s) the
shortest line decayed 2.2x more slowly than the longest at 1.4 kHz, and the network's modes on the
short lines rang on (a 1410 Hz ring on a snare's tail, 1.6x longer than the frequencies around it).
Matched at DC and 3 kHz, the lines keep within 1.15x of each other below 5 kHz at that setting (within
1.04x at the others); above 5 kHz, where tails are quiet and short, within 1.43x (it was 1.57x).
The ring is gone; blind preference tests (drums and a 303 line) heard no difference otherwise.
Matching at 4-5 kHz instead trades the mid band (1.24-1.35x) for the top (1.30-1.36x).

In the code, the input gain `c = g (1 - a) / sqrt(8)` (with the mix's scale) is folded into the step
used to decode the line's reads, so the damper itself is `z = a z + read`.

### Output compensation

The outputs are sums of the damped lines, so they carry the dampers' lowpass and the loop gain: on
their own they sound darker than the lines. Each output is passed through the inverse of the lines'
mean damper and gain:

`y = (s - tap_a s[-1]) tap_k`, with `tap_a` = mean a and `tap_k = sqrt(8) / (mean g (1 - tap_a))`.

That matches the sound of taking the outputs from the undamped lines to within 0.25 dB rms across
bands and 0.13 dB in level. Taking them from the mix's sums saves separate sums for the taps;
together with folding `c` into the decode, that cut the network from 140 to 107 instructions per
sample.

## Line storage: 8-bit block-float

16-bit lines would limit the mean line length to about 21 ms in this memory; 8-bit samples with a
fixed scale would put late tails a few steps above zero. Instead, each line is a ring of 16-sample
blocks, each block being 16 signed 8-bit mantissas and one exponent byte `e`, with a step of
`2^e / 128`:

- **The commit** (at the end of every 16 samples) picks the smallest step that keeps the block's peak
  under 127.5 steps and rounds each sample to the nearest step. The noise follows the tail down: it
  stays about 47 to 53 dB below the block's own peak. 8-bit and 16-bit mantissas were
  indistinguishable by ear.
- **A block whose peak is under 1e-6** (-120 dB) is stored as zeros. When every ring holds only zeros
  and the input has been silent (`FX_RM_QUIET` blocks in a row), ROOM goes idle: it clears its
  dampers, filter state and diffusers, and returns at once until input arrives.
- **A ring has L / 16 + 1 blocks,** the fewest that hold L samples. At the start of a block of
  writes, the read's first block is the one the write is about to replace: it still holds the
  samples from L / 16 + 1 blocks before, so the reads are fetched before that block's commit, at the
  start of each block, and the commit comes at its end. The block being written is kept as floats
  (`stage`) until its commit.

### Reads in blocks of 16

All the lines write together: the 16 samples in progress fill each line's `stage` block. At the start
of those 16 samples, each line's 16 reads are decoded into `read` at once. Because the read is L behind
the write, it starts at the same place within a block every time: `-L mod 16`. The 16 reads take the
rest of that block and the start of the next, and both are already in the ring (the newest sample
they reach was written L >= 16 samples earlier).

Because lines 0 to 3 are equal modulo 16, and lines 4 to 7, that place is one of two compile-time
constants. The decode is then straight code, with no per-line split or remainder loop, and the fetch
loop (over four pairs: line j at one place, line j + 4 at the other) has two copies of it. One copy
for all eight made the fetch about 40 % cheaper than one with per-line offsets read at run time;
eight copies (each line its own constant) cost too many instruction-cache misses.

The dampers' extra sample costs nothing: in each group of four, lines j and j + 2 hand the mix their
damper's value from before this sample's update, and are updated after the mix has read them.

## Knob mapping

With `x` = RvDecay / 127 and `y` = RvTone / 127:

- `ln t_dc = -1.7984 + 1.1228 x - 0.6857 x^2 + 2.1048 x^3 + 0.0597 y`
- `ln t_ny = min(ln t_dc, -2.0520 - 0.6055 x - 0.1850 y + 1.2173 x y + 0.2419 y^2 + 0.5170 x^2)`
- the output gain is `1.9338 + 0.3233 x + 0.1591 y`.

These are fitted to CLASSIC on a grid of 11 x 10 knob settings by `tools/fit_room.py`, which renders
both types with this firmware's fxbus on the host (`tools/fit_room_render.c`) and prints the C for
`fx_room_coeffs`; `tools/fit_room.py --check` measures the firmware's mapping as it is. It needs
numpy and scipy. Refit after changing the lines, the dampers or the outputs.

- **Decay:** CLASSIC's response to a 20 ms noise burst at a snare's send level, measured as the
  decay time (-3 to -23 dB of the backward-integrated energy) in 10 mel-spaced bands, 100 Hz to 10
  kHz. CLASSIC's decay depends on level (its 12-bit loop truncates small values), so the burst level
  matters. At each setting, `t_dc` and `t_ny` are fitted to CLASSIC's bands through ROOM's
  predicted per-band decay, then the two surfaces are fitted with the polynomials above.
- **Loudness:** at each setting, the output gain that matches ROOM's K-weighted (BS.1770) level to
  CLASSIC's, fitted as a plane.

Against CLASSIC, the result is within 14 % rms in decay time per band (at most 40 %, at RvDec 0,
where two parameters can't follow CLASSIC's band shape; the best fit at each setting does no better)
and within 0.1 dB rms in level (at most 0.4 dB).

The first coefficients were fitted with an earlier set of lengths, before the outputs were taken
from the mix and before the dampers' 3 kHz match. Their decay terms were within about 0.03 of these,
but their gain was up to 1.5 dB loud at low settings (0.7 dB rms).

## Tried and rejected

| Option | Why not |
|---|---|
| Half-rate network | The top rolls off; a full-rate early path didn't fix it |
| Delay-line modulation | Wobbly at the faster rate, no clear gain |
| 4 lines (same memory, doubled lengths) | Pumps, sounds like a hall, the first echo comes too late (52 ms) |
| 4 lines at the current spread | Buzzy |
| 2 diffusers | Empty at the default decay |
| mu-law companding of the lines | One fixed scale: late tails become a few coarse steps that recirculate |
| Integer arithmetic in the network | Slower on this core (357 vs 295 cycles per sample) |

## Cost on the device

Measured with the 24 MHz timer in test builds (not in the repo). Cycle costs on the AC79 core at
360 MHz:

| Operation | Cycles |
|---|---|
| FP add or multiply | 1.9, with no overlap between ops, dependent or not, and no data dependence |
| int / float conversion | 0.9 |
| Load / store | 1.9 / 1.3; a load paired with another op (`#` in the listing) is nearly free |
| Taken branch | 8 |
| Call and return | 44 |
| RAM read, any span or stride | 2.1 (no data-cache effects) |
| Code from flash, up to 16 KB warm | 1.0 per instruction |

What follows for the code:

- **Branches and calls are expensive,** so the loops that run per sample have constant trip counts
  where possible (the whole-block network, the decode), and the per-block work is in a few calls.
- **FP ops don't overlap,** so reordering doesn't hide their cost; only removing ops helps. This is
  why the gains are folded into the decode and the outputs come from the mix.
- **Registers are few** (the FPU uses the integer registers), so the work goes in phases over each
  chunk of up to 16 samples (high-pass, diffusers, network), and the dampers in two groups of four.
  All eight in one loop spilled.

ROOM's profile is about 41 ticks per sample against CLASSIC's 29: high-pass 3, diffusers 8, fetch
6, network 17, commit 8. The PERF TEST (load / peak, %), both types in one test build (code layout
moves these figures by about a point between builds):

| | Factory | All five | Worst |
|---|---|---|---|
| CLASSIC | 41.0 / 59 | 54.5 / 67 | 68.1 / 87 |
| ROOM | 44.0 / 53 | 57.7 / 70 | 71.6 / 91 |

The same ROOM code run from RAM measured 0.6 to 1.8 points lower, growing with load. Code from
flash costs more inside the audio interrupt than in a warm loop, for reasons not yet understood.
