# Research notes behind the trance groovebox proposals

Four research passes, made on 2026-10-17 for
`docs/plans/2026-10-17-trance-groovebox-proposals.md`, kept here so they outlive the session that
made them.

| File | What it covers |
|---|---|
| [fm1-ecosystem.md](fm1-ecosystem.md) | The FM-1's hardware, the stock firmware, Felucca, Baud Girl, Melodee, other firmwares, Schwung, CPU figures, wishlists |
| [genre-and-references.md](genre-and-references.md) | Gear and sound recipes for 1996-99 trance and prog house, goa, the 2020s revival and prog breaks; effects; arrangement and sequencing norms; the five reference tracks, with KiloMix decoded from its .rbs file |
| [dsp.md](dsp.md) | Supersaw, oscillators, filters, chorus, reverbs and delays, drums; open-source codebases and their licences; cycle counts to extrapolate from |
| [sequencing-and-ui.md](sequencing-and-ui.md) | Elektron's model and song mode, the Deluge's data model, late-90s grooveboxes, Euclid / Grids / Turing / Marbles and others, ReBirth's PCF, acid generators, tiny models and datasets, small-screen UI |
| [tools/](tools/) | `rbs_dump.py` and `rbs_timeline.py`: readers for ReBirth .rbs song files |

**How much to trust them**
- Most web pages could not be fetched in that session. Claims about websites mostly come from
  search-result summaries; code that was cloned and read (Grids, Marbles, the Turing Machine, the
  Deluge, Open303, Felucca and others) is verified.
- The notes mark their doubts: *(unverified)*, [U] uncertain, [est] estimate, [K] domain knowledge,
  (mem) from memory, [search] from a search summary.
- Nobody listened to the reference tracks. Their notes are reviewer descriptions plus era norms,
  except KiloMix, which was decoded from the song file itself.

**What isn't here, on purpose**
- No recordings and no song files. The KiloMix pattern dump was left out too: it's a transcription
  of someone else's song. The tools regenerate it from the .rbs file, which ships with
  https://github.com/varyform/re-rebirth (`public/songs/tgv-kilomix-98.rbs`).
