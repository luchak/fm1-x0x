# Handoff: from these notes to the new repo

> **Moved, 2026-10-20:** this now lives in https://github.com/luchak/fm1-perception
> (`docs/`). This copy is no longer updated.

Everything the design work produced is in this branch (`trance-groovebox-proposals` of luchak/fm1-x0x).
Nothing depends on the session that wrote it.

| What | Where (here) | Suggested place (new repo) |
|---|---|---|
| The design: three proposals, B chosen | `docs/plans/2026-10-17-trance-groovebox-proposals.md` | `docs/design/proposals.md` |
| The research behind it, with sources | `docs/research/` | `docs/research/` |
| ReBirth .rbs readers | `docs/research/tools/` | `tools/rbs/` (the start of the .rbs importer) |
| The texture census template (not filled in yet) | `docs/plans/texture-census.xlsx` | `docs/census/texture-census.xlsx` |
| A CLAUDE.md for the new repo | `docs/handoff/CLAUDE.md` | `CLAUDE.md` at the root |

## 1. Decisions

| # | Decision | Recommendation | Status |
|---|---|---|---|
| 1 | Proposal | B | **Decided 2026-10-20**: B, with machines sized by cost (a costly machine gets fewer voices, or a hard cap) |
| 2 | Tracks | 11 + FX | Open |
| 3 | Flash | One project, two sample slots | Open |
| 4 | Key velocity | Ignore for now | Open |
| 5 | Codebase | A new repo: an app on fm1-base, with X0X's DSP and UI pieces imported (section 2) | New repo: decided. Base: open |
| 6 | Delay vs reverb memory | Half-rate delay, one reverb at a time | Open |
| 7 | Texture track | CH + OH share a track, TEX takes the other | Open |

Start a `docs/design/decisions.md` in the new repo: one line per decision, with its date and why.
Sessions read it before re-arguing anything.

## 2. The base

**The family tree** (as of 2026-10-20):

| Base | What it is | Licence |
|---|---|---|
| M-VAVE V15 | The stock firmware. **Baud Girl's FM-1+VA starts here**, as a binary patch: V15's app with about 90 small edits and 110 KB of code appended, calling stock functions at fixed addresses. The VM partition is moved to fit a 691 KB app. Closed. Verified by package diff in ip2k/lunar-modulator. Groove OS is also built on V15. | M-VAVE's (closed) |
| Felucca | The root of the open firmwares: Leo Kuroshita's, with 13 engines and 4 tracks. Its maintainer has stepped back. | GPL-3.0-only |
| felucca-bsp | Felucca's chip and board layer, split out: clock, interrupts, flash, USB (MIDI, audio), UART, the loader, packaging. No sound, no UI. | MPL-2.0 |
| **fm1-base** | Charles Vestal's platform on felucca-bsp: the layer X0X, FoMni-1, CTL-1 and his other firmwares share, made into one maintained base. Screen, fonts, panel, LEDs, audio, MIDI over USB and TRS, storage, the screen reader, backup, optional velocity and Bluetooth, **the host simulator (X0X's, same scripts)**, the browser emulator, the build tool, the installer pages, and a skill for porting a Felucca fork onto it. The template costs 77 KB of flash, 27 KB of RAM and 68 KB of pool. Beta: `plat.h` may still change. An app can bring **its own second core** (hooks merged 2026-10-19). | MPL-2.0 (the app picks its own) |
| X0X | Felucca's platform, improved, plus X0X's instrument. Most of its platform now lives in fm1-base; X0X itself hasn't moved onto fm1-base yet. | GPL-3.0-only |
| Dinghy | A minimal template on Felucca's platform as SLOOP carries it (a 4-voice sine). | GPL-3.0-only |

**Recommendation: a new app on fm1-base, importing from X0X what B keeps.**
- B replaces X0X's instrument almost entirely: the sequencer's data model, the engine framework, the
  screens. Forking X0X would mostly mean deleting. What we'd keep is X0X's platform, and fm1-base is
  that platform, maintained.
- **Platform fixes** reach us from fm1-base, a pinned submodule we update deliberately. With a fork,
  they would have to be merged out of X0X, which will itself be moving.
- **The test bench carries over:** the same host simulator and scripts, and the browser emulator.
- **A lean platform** leaves more flash and RAM for the instrument.

**What comes over from X0X, file by file, with its provenance in CREDITS and LICENSING:**
- **The 909 voices** (the 9W9 port, GPL-3).
- **The 303:** X0X's float port of Open303 (Open303 is MIT; X0X's changes are GPL-3).
- **TB-3PO** (GPL-3).
- **fxbus:** CLASSIC and ROOM. ROOM is your own work on `fdn-reverb`, so its licence is yours to choose.
- **The rest of X0X's code:** master, `fastmath.h` (fm1-base already has a copy), motion lanes, undo,
  the UI widgets (knob strip, lists, readout, help cards), the reference tests.
- **X0X's second-core code** (`hal/fm1_cpu1.*`, with parts from Melodee, GPL-3), through fm1-base's
  own-second-core hooks: `asm`, `vendored`, `.c1_text`, `APP_CSTART`, `APP_FAULT`. See
  `skill/felucca-to-fm1-base/PORTING.md`.
- **ER-99's 909 samples** (GPL-3).

**Licence consequence:** with any GPL-3 part imported, the app is GPL-3.0-only overall, which is fine.
An app with no GPL parts could take any licence.

**Watch out for:**
- **BB Gen** came into X0X *with its author's permission*, and its repository has no licence. Ask
  mestela before reusing it, or write our own slice shuffler.
- **fm1-base is beta.** Pin a commit, read its changes before updating, and expect some `plat.h`
  churn.
- **X0X's own platform oddities don't come with fm1-base:** the sample slots and its flash map. Do
  them in the app, with fm1-base's store and `plat_data*` windows.
- **Ask Charles:** a short note to Charles Vestal is worth sending. He's moving his firmwares onto
  fm1-base; the 909, 303 and FX modules might become a shared library rather than copies.

**When forking X0X would be better:** if the aim were something on the device this week, with X0X's
engines intact, evolving step by step (Proposal A's path). For B, the fm1-base start is cleaner.

## 3. First steps in the new repo

1. **Start from fm1-base's template:** copy `template/` and add fm1-base as a submodule pinned to a
   commit, the way the template's README describes. Give it its own `fm1.toml`:
   - `name`;
   - `id_prefix`: each firmware has its own package identity (the template is `FM-1_11`, X0X
     `FM-1_9…`). Pick one no one uses;
   - the USB names;
   - the app's sources.
2. **Docs:** copy the files in the table above. From a clone of luchak/fm1-x0x:
   ```
   git checkout origin/trance-groovebox-proposals -- docs/research docs/handoff docs/plans/texture-census.xlsx \
       docs/plans/2026-10-17-trance-groovebox-proposals.md
   ```
   Then move them to their new places in the new repo. Replace the README with a short statement of
   intent, link back here, and note in this branch that the design has moved.
3. **A store magic and object layout of its own,** so a project saved by another firmware is never
   read as ours.
4. **A `.gitignore` for audio:** `*.wav *.flac *.mp3 *.aif*`, except fixtures the host generates.
   Reference recordings stay local or in Drive; only measurements get committed.

## 4. Where each kind of work can run

- **A cloud session** (claude.ai/code) can:
  - edit code and docs;
  - build and run the host simulator and tests, once the environment has a setup script that
    installs the JieLi toolchain (`tools/get_toolchain.sh`) and fetches the AC79 SDK files. The
    `session-start-hook` skill can write that script.
  - It can't touch the FM-1.
- **The device** needs you, or a local Claude Code session on the machine the FM-1 is plugged into.
  M0 is mostly device measurement, so plan for that: the cloud session writes a PERF-test build plus
  a short checklist; you run it and paste the numbers into `docs/measurements/`. fm1-base's
  `tools/fm1_remote.py` can drive the panel and read the screen over USB, which helps with this.
- **The census** is yours, whenever you get to it. Commit the filled-in workbook (your own notes are
  fine to commit). Then ask a session to read it and update SCATTER's defaults and the texture
  presets (proposal, section 4.11).

## 5. Working rhythm

- One milestone (or part of one) per branch and PR, with its gate written into the PR description.
- **M0's gate decides the shape of everything else:** tracks, voices, delay length, which machines get
  caps. Nothing in M1+ should assume numbers M0 hasn't measured.
- Update the proposal whenever a decision or a measurement changes it, so the docs stay the single
  source of truth.

## 6. Kickoff prompt for the first session in the new repo

> Read `CLAUDE.md`, `docs/design/proposals.md` (sections 0, 2, 4 and 7), `docs/design/decisions.md`
> and `docs/research/README.md`. Proposal B is decided, with machines sized by cost. The base is
> fm1-base (the submodule), with pieces imported from X0X (charlesvestal/fm1-x0x, plus luchak's
> `fdn-reverb` branch for ROOM). We're starting M0. In this session:
> 1. Check the app builds for the host and the device from fm1-base's template under our own name,
>    package identity and USB names, and that the host tests run.
> 2. Bring in X0X's second-core code through fm1-base's own-second-core hooks (PORTING.md), and
>    X0X's PERF/profiling approach, so we can measure on the device.
> 3. Plan the M0 DSP spikes (proposal, section 7), and write the checklist I'll run on the FM-1.
>    Import from X0X only what the spikes need (the 909 voice, Open303, ROOM), recording each file's
>    origin and licence.
> Don't start M1. Record decisions in `docs/design/decisions.md`.
