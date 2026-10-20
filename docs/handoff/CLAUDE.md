# CLAUDE.md (draft for the new repo; copy to its root)

## What this is

Firmware for the M-VAVE FM-1: a groovebox for 1996-99 trance and progressive house, with goa, the 2020s
throwback (Oprofessionell, Bliss Inc) and Framewerk-style progressive breaks as secondary targets.

- **Architecture:** Proposal B of `docs/design/proposals.md`, Elektron-style:
  - 11 sound tracks plus an FX track;
  - a machine per track (909 voices, ACID, SAW, VA, FM, SAMPLE);
  - the same chain and pages behind every machine;
  - generators as per-track trig sources;
  - a harmony lane;
  - song rows, capture and lanes.
- **Lineage:** an app on fm1-base (charlesvestal/fm1-base, MPL-2.0, itself on hugelton/felucca-bsp),
  pinned as a submodule. It imports DSP and UI pieces from X0X (charlesvestal/fm1-x0x; ROOM from
  luchak/fm1-x0x's `fdn-reverb`). Because those pieces are GPL-3, the app is GPL-3.0-only.
  - Each imported file's origin and licence goes in CREDITS/LICENSING.
  - BB Gen needs its author's permission again before reuse.
- **What the user cares about most:** playability, easy tweaking, sound quality, an orthogonal
  design (no special-cased machines), and capturing arrangements.

## Where things are

- `docs/design/proposals.md`: the design, its budgets and the milestone plan (section 7).
- `docs/design/decisions.md`: decisions made, with dates and reasons. Read it before re-arguing one.
- `docs/research/`: the research behind the design, with sources and confidence marks.
- `docs/census/`: the texture census. Its Summary sheet feeds SCATTER's defaults and the texture
  presets.
- `docs/measurements/`: numbers from the device (PERF tests, budgets).
- The platform is fm1-base (the submodule); the app sees only `plat.h`.
  - Don't edit fm1-base in place. Changes it needs go upstream, or through its app hooks.
  - Update the pin deliberately, after reading its changes.

## Milestones

- We work milestone by milestone (M0 → M7). **M0 measures first:**
  - per-voice cost of each machine on the device;
  - the I-cache;
  - RAM and flash;
  - single-core behaviour;
  - the delay/reverb masking stress test.
- Nothing past M0 should assume numbers M0 hasn't measured.
- Each milestone has a gate in section 7 of the proposal. Say plainly whether it's met.

## Engineering rules (inherited from X0X; keep them)

- **The audio ISR never touches flash, allocates, or waits.** UI changes reach the engines through the
  command queue, or as single aligned word stores.
- **No `double` in the audio path:** the FPU is single precision, and the build fails if a soft-double
  routine is linked.
- **No libm:** `fastmath.h` only.
- **The host and the device compute the same samples** (`-ffp-contract=off`), so golden audio from the
  host is the reference for the device.
- **CPU is budgeted:**
  - each machine has a measured cost per voice, and a project can't exceed the budget;
  - voices render batched by machine;
  - the per-track mix is fixed-order, so core assignment never changes a sample;
  - core 1 is optional: detect a bad second core and degrade to one.
- **Memory:** keep the pool headroom that `tools/build.py` checks. Reverb lines may be 8-bit
  block-float. The delay may not (int16 or 12-bit block-float, at half rate).
- **Flash:** never write 0x0-0x3FFF. Keep flash writes away from playback. Keep the rescue path and
  safe mode working.

## Building and testing

- `tests/run_tests.sh`: maths, sequencer, engines against their references, flash storage, and the
  whole app in the simulator (scripts in `tests/scenarios/`).
- `./build.sh`: the firmware. It needs the JieLi toolchain and the AC79 SDK; see `BUILDING.md`.
- `host/build_host.sh`: the simulator alone.
- **Cloud sessions can't reach the FM-1.** Anything that needs the device becomes a checklist for the
  user, and its results go in `docs/measurements/`.
- Before pushing: run the tests and re-read the diff; a red CI costs a cycle.

## Content and licences

- **No commercial audio or song files in git:** reference tracks and .rbs files stay local. Commit
  measurements and your own analysis, not the material.
- **Check a licence before porting.**
  - Compatible: MIT, BSD, Apache-2.0, GPL-3, CC BY 4.0.
  - Reference only: TAL-NoiseMaker (GPL-2-only); ReBirth's PCF shapes and anyone else's patterns.
  - Licensed model by model: the Moog-ladder collection.
- **Factory content** (sounds, phrases, gate patterns, demo songs) is written for this project.

## Style

- **Docs and the manual:** plain sentences, as X0X's manual does: what a control does, in the
  user's words.
- **Commit messages:** say what changed and why, in plain sentences.
