# ROOM's 16-sample lattice: renders and measurements

Tools behind `room-line-delays` (docs/plans/2026-10-15-room-reverb.md, "The delays' residues modulo 16").
Not part of the build.

- `residues.py` - the shrink per trip for a set of delays, and their largest common factor.
- `abx_render.c` - ROOM's wet output for ABX: clicks or snare bursts, a hit every 2 s.
- `lattice_metric.py` - how strongly a render's early tail repeats every 2756 Hz (white noise: 0).

## A/B renders: `fdn-reverb` (one residue) against `room-line-delays`

From the repo root, with `build/gen` generated (`./build.sh`, or the steps in `host/build_host.sh`):

```
git worktree add ../x0x-old fdn-reverb
mkdir -p build/abx
for v in old new; do
  src=$([ $v = old ] && echo ../x0x-old || echo .)
  cc -O2 -ffp-contract=off -w -I$src/firmware/src -I$src/firmware/src/dsp -Ibuild/gen \
     -o build/abx/abx_$v tools/room_lattice/abx_render.c -lm
  for s in "clicks 73 127" "clicks 127 127" "clicks 40 115" "snare 100 70" "snare 127 115"; do
    set -- $s; build/abx/abx_$v build/abx/${1}_dec$2_tone$3_$v.wav $2 $3 $1
  done
done
python3 tools/room_lattice/lattice_metric.py build/abx/*.wav
```

The clicks are quiet (about -22 dBFS peak): turn them up, the same for A and B.

What it measured here (60 / 100 / 150 / 200 / 300 ms after the hit):

| Render | `fdn-reverb` | `room-line-delays` |
|---|---|---|
| clicks, RvDec 73, RvTone 127 | 0.15 0.12 0.12 0.05 0.03 | 0.01 0.03 0.00 0.03 0.01 |
| clicks, 127, 127 | 0.18 0.19 0.12 0.09 0.10 | 0.01 0.01 0.00 0.01 0.00 |
| clicks, 40, 115 | 0.12 0.11 0.10 0.04 0.01 | 0.01 0.05 0.02 0.05 0.01 |
| snare, 100, 70 | 0.16 0.10 0.12 0.06 0.03 | 0.02 0.04 0.04 0.03 0.01 |
| snare, 127, 115 | 0.14 0.13 0.13 0.09 0.04 | 0.01 0.03 0.05 0.03 0.01 |

## On the device (not checked yet)

- `fx_rm_damp4`: no spill from the reordered damper updates (lines j and j + 2 update after the mix).
- `fx_rm_keep`: straight loads and stores (12 floats per block).
- PERF test, ROOM against `fdn-reverb` (expected: about +0.2 tick of 41).
