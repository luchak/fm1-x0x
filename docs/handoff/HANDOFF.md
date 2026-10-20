# Handoff: from these notes to the new repo

Everything the design work produced is in this branch (`trance-groovebox-proposals` of luchak/fm1-x0x).
Nothing depends on the session that wrote it.

| What | Where (here) | Suggested place (new repo) |
|---|---|---|
| The design: three proposals, B recommended | `docs/plans/2026-10-17-trance-groovebox-proposals.md` | `docs/design/proposals.md` |
| The research behind it, with sources | `docs/research/` | `docs/research/` |
| ReBirth .rbs readers | `docs/research/tools/` | `tools/rbs/` (the start of the .rbs importer) |
| The texture census template (not filled in yet) | `docs/plans/texture-census.xlsx` | `docs/census/texture-census.xlsx` |
| A CLAUDE.md for the new repo | `docs/handoff/CLAUDE.md` | `CLAUDE.md` at the root |

## 1. Decide first

- **Name and visibility.** The licence is inherited: GPL-3.0-only, from X0X and Felucca. A private repo
  is fine while it's in development; anyone you give a binary to must be able to get the source.
- **Confirm the open decisions** (proposal, section 9). What the conversation so far implies:

| # | Decision | Recommendation | Status |
|---|---|---|---|
| 1 | Proposal | B | Implied by starting a new repo; not confirmed |
| 2 | Tracks | 11 + FX | Open |
| 3 | Flash | One project, two sample slots | Open |
| 4 | Key velocity | Ignore for now | Open |
| 5 | Codebase | Fork X0X's chassis, in a new repo | New repo: decided |
| 6 | Delay vs reverb memory | Half-rate delay, one reverb at a time | Open |
| 7 | Texture track | CH + OH share a track, TEX takes the other | Open |

- Start a `docs/design/decisions.md` in the new repo: one line per decision, with its date and why.
  Sessions read it before re-arguing anything.

## 2. Bring X0X in with its history

Not a GitHub fork, which keeps the X0X name and network and can't be private. Not a squashed copy
either, which loses blame and makes upstream fixes painful to merge. Push X0X's history into the
new repo:

```
git clone https://github.com/luchak/fm1-x0x.git NEW && cd NEW
git remote rename origin x0x
git remote add upstream https://github.com/charlesvestal/fm1-x0x.git   # Charles's fixes
git remote add origin https://github.com/luchak/NEW.git
git checkout -b main x0x/main
git merge x0x/fdn-reverb        # ROOM (and REV TYPE); then any of your other branches you want:
                                # pingpong-width, delay-to-reverb, per-part-swing, swing-timing-fix,
                                # 303-drive-scaling, bd-sends-lane-fix
git push -u origin main
```

Keeping `x0x` and `upstream` as remotes means HAL, USB, dual-core and flash fixes can still be
merged later (`git fetch upstream && git merge upstream/main`). That stays easy only while the
chassis files keep their paths. So **leave the platform files where they are, and put the new
instrument in new directories**.

## 3. First commits in the new repo

1. **Docs:** copy the files in the table above. They're one checkout away, since this branch comes
   along with the `x0x` remote:
   ```
   git checkout x0x/trance-groovebox-proposals -- docs/research docs/handoff docs/plans/texture-census.xlsx \
       docs/plans/2026-10-17-trance-groovebox-proposals.md
   ```
   Then move them to their new places. Replace the README with a short statement of intent, link
   back here, and note in this branch that the design has moved.
2. **Identity, before anything goes on a device:**
   - **The package identity** the installers check. X0X and Felucca use `FM-1_9…`;
     `web/make_site.py` checks `FM-1_9\d\d`; `BUILDING.md` describes it. Find out what the update
     loader and M-UPGRADE accept before choosing a new one.
   - **The storage magic,** `ST_MAGIC` in `firmware/src/storage.c` ("X0X0"). With a new magic, a
     project saved by one firmware is never read as the other. That matters all the more because
     the flash layout will change.
   - **The USB strings** (`STR1` and `STR2` in `firmware/src/usb.c`), the build's output names, and
     the titles of the installer, emulator and manual.
3. **A `.gitignore` for audio:** `*.wav *.flac *.mp3 *.aif*`, except fixtures the host generates.
   Reference recordings stay local or in Drive; only measurements get committed.

## 4. Where each kind of work can run

- **A cloud session** (claude.ai/code) can:
  - edit code and docs;
  - build and run the host simulator and tests, once the environment has a setup script that
    installs what `BUILDING.md` lists (the JieLi toolchain via `tools/get_toolchain.sh`, the AC79
    SDK from gitee). The `session-start-hook` skill can write that script.
  - It can't touch the FM-1.
- **The device** needs you, or a local Claude Code session on the machine the FM-1 is plugged into.
  M0 is mostly device measurement, so plan for that: the cloud session writes a PERF-test build plus
  a short checklist; you run it and paste the numbers into `docs/measurements/`.
- **The census** is yours, whenever you get to it. Commit the filled-in workbook (your own notes are
  fine to commit). Then ask a session to read it and update SCATTER's defaults and the texture
  presets (proposal, section 4.11).

## 5. Working rhythm

- One milestone (or part of one) per branch and PR, with its gate written into the PR description.
- **M0's gate decides the shape of everything else:** tracks, voices, delay length. Nothing in M1+
  should assume numbers M0 hasn't measured.
- Update the proposal whenever a decision or a measurement changes it, so the docs stay the single
  source of truth.

## 6. Kickoff prompt for the first session in the new repo

> Read `CLAUDE.md`, `docs/design/proposals.md` (sections 0, 2, 4 and 7) and `docs/research/README.md`.
> We're starting milestone M0 of Proposal B. In this session:
> 1. Give the firmware its own identity: package identity (check what the loader accepts first),
>    storage magic, USB strings, output names. Explain what you changed and why.
> 2. Strip X0X's instrument (the 808, the parts and their UI) while keeping the chassis in section 2,
>    and keep the host build, the tests and the emulator green. Show me the list of what you'll remove
>    before deleting anything large.
> 3. Sketch the PERF-test harness for M0's DSP spikes, and the checklist I'll run on the device.
> Don't start M1. Record decisions in `docs/design/decisions.md`.
