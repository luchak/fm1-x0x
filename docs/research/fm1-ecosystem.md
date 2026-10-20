# The FM-1 firmware ecosystem and its hardware

Research pass of 2026-10-17. GitHub repositories were cloned and read; anything marked [search]
comes from search summaries of pages that couldn't be opened (baudgirl.com, Synth Anatomy, Reddit,
retailers). The scene dates from July to October 2026.

Two corrections to common assumptions: the stock engine is **6-operator** FM, and the keys **can**
sense velocity (experimentally, below).

## 1. Stock FM-1 (V15) and the hardware

**Engine**
- msfa/Dexed-derived 6-op FM, 32 algorithms, 12 voices of 468 B each with oldest-voice stealing.
  64-sample blocks at 44,118 Hz, integer DSP
  ([FM-1-RE architecture](https://github.com/AL-255/FM-1-RE/blob/main/docs/architecture.md)).
- 128 presets, a 16-step sequencer, a 7-mode arpeggiator, 6 effects [search:
  [Synthtopia](https://www.synthtopia.com/content/2026/07/13/m-vave-introduces-fm-1-fm-pocket-synthesizer/)].
- Engine and effects reportedly **mono**, mirrored to both channels [search:
  [Synth Anatomy review](https://synthanatomy.com/2026/07/m-vave-fm-1-review-low-budget-pocket-fm-ynthesizer-with-iconic-sounds.html)].
- Reviewers' complaints: no rests or ties, no pattern chaining, no realtime recording [search;
  [Elektronauts](https://www.elektronauts.com/t/m-vave-fm-1/252170?page=3)].

**MIDI and audio**
- USB-MIDI, class-compliant; stock also has BLE-MIDI.
- The 3.5 mm TRS jack is **MIDI IN only**: UART RX on PH8, no TX pin, optocoupled
  ([lunar-modulator hardware doc](https://github.com/ip2k/lunar-modulator/blob/main/docs/01-hardware.md)).
- Stock enumerates a USB audio device, 24-bit stereo at 44.1 kHz.
- Jacks: stereo out and MIDI in only. **No analog audio input.** Headphones mute the speaker [search].
- **Uncertain: which DAC.** FM-1-RE says the internal DAC; Felucca and fm1-nes drive I2S (ALNK0) to an
  external codec, 24-bit in 32-bit words, at 24 MHz/544 = 44,117.6 Hz
  ([Felucca fm1_audio.h](https://github.com/hugelton/Felucca/blob/main/firmware/hal/fm1_audio.h)).

**Power and board**
- 3.7 V, 2000 mAh battery, about 12 h of play [search]
  ([PCB notes](https://github.com/ip2k/lunar-modulator/blob/main/notes/2026-09-29-baudgirl-fm1va-and-pcb-photos.md)).
- Speaker connector J13. No debug header.

**Controls**
- 14 buttons and 27 keys, each with an LED. One 11x6 diode matrix behind two 74HC595s; LEDs light on
  the scanned column. All 7 encoders sit in the matrix and are decoded in software. MASTER is a pot on
  the ADC.
- Lesson: Felucca missed small, fast knob turns until it decoded every scan frame
  ([#126](https://github.com/hugelton/Felucca/issues)).

**Velocity**
- [Velo](https://github.com/charlesvestal/fm1-velocity) and [MONK-1](https://github.com/charlesvestal/fm1-monk)
  read the rubber domes through the ADC. Only 20 of the 27 keys sense well, with a margin of 10-20
  counts; both call it experimental.

**Chip**
- AC791N/WL82: 578 KB SRAM, about 514 KB usable after the caches. No SDRAM. 1 MiB Puya flash.
- **The clock:** FM-1-RE says 240 MHz; X0X measured **360 MHz**, set by the boot loader, above the SDK's
  320 MHz table (X0X `docs/plans/2026-10-04-dual-core-and-clock.md`). Melodee and Jangada agree on 360.

## 2. Felucca (Hügelton Instruments / Leo Kuroshita)

- 1.6.1, GPL-3.0; the maintainer has stepped back to occasional maintenance
  ([README](https://github.com/hugelton/Felucca)). Most open firmwares descend from it
  ([awesome-fm-1](https://github.com/cicloid/awesome-fm-1)).
- 13 engines: ANALOG, FM6 (Dexed), PHASE, LOFI, SAMPLE, VOICE, TRIO, WHEEL, GRAIN, PHYS, NOISE, SLICE,
  DRUM (synthesized kits) ([Engines](https://github.com/hugelton/Felucca/wiki/Engines)).
- 4 tracks share **8 voices** ([core.h](https://github.com/hugelton/Felucca/blob/main/firmware/src/core.h));
  128-frame half-buffers (about 2.9 ms); an optional second core renders parts.
- **The 85 % shed guard** ([audio.c](https://github.com/hugelton/Felucca/blob/main/firmware/src/audio.c)):
  when two consecutive halves exceed 85 % of the half's time (render plus the timer interrupt nested in
  it), it fades one voice over the next block, and one more each half while the overload lasts. Victims:
  the quietest released voice first, then the youngest held voice, never a POLY part's lowest note or a
  mono/unison lead. No load figures published.
- **UI** ([Controls](https://github.com/hugelton/Felucca/wiki/Controls)): tap a page button to cycle its
  pages; hold it for a "quick layer" on keys and knobs; double-tap to keep the layer. OCT+ confirms, OCT-
  goes back. ALGORITHM picks the track everywhere; PRESETS changes the sound from any page that isn't a
  list. Hold SAVE to undo. Hold LFO and turn a knob for a momentary change. Pressable controls breathe
  rather than blink.
- **Sequencer** ([Features](https://github.com/hugelton/Felucca/wiki/Features)): 64 steps with chords,
  ties, accent, slide, chance, ratchets and trig conditions (including FILL); p-locks plus an automation
  list of 128 records shared by the 4 tracks; micro-nudge; tools EUCLID, RANDOM and COOK; songs of up to
  16 sections; autosave only when stopped and idle.
- **Limits:** the issues are mostly UI bugs; 0.9 → 1.0 could soft-brick a unit
  ([fm1-custom-fw RE notes](https://github.com/aroum/fm1-custom-fw/blob/main/REVERSE_ENGINEERING.md)).
  Requests: per-track MIDI channels, MPE, velocity/aftertouch as mod sources, resampling into sample
  slots, humanize, an init patch, a reset-to-default gesture
  ([Discussions](https://github.com/hugelton/Felucca/discussions)).

## 3. Baud Girl FM-1+VA (closed source)

- A binary patch of V15: about 90 hook sites plus about 110 KB of appended code. It moves the settings
  store (global settings reset once). The install pages send telemetry to baudgirl.com
  ([analysis](https://github.com/ip2k/lunar-modulator/blob/main/notes/2026-09-29-baudgirl-fm1va-and-pcb-photos.md)).
- VA engine (its own claims): sine, saw, triangle, square; "Super" mode stacks up to 6 detuned copies;
  sub, noise, PWM, drift; LP12/LP24/BP/HP filter with its own envelope. **12 voices on sine/saw, 8 on
  triangle/square**; the filter works on FM presets at 9-12 voices.
- Sequencer and UI [search: [manual](https://baudgirl.com/work/FM-1+VA/manual)]: 64 steps as 16-step
  pages or a "Flow" 8x8 dot matrix; ratchets, chance, slide; move/copy steps, stretch/squeeze;
  realtime recording; MIDI CC. SELECT scrolls, ALGORITHM edits the value, SEL enters, HOME goes back.
  Four assignable knobs on every screen (hold HOME and turn to reassign); knob banks (Global = tempo,
  arp rate, arp gate, glide).
- Praised for fixing stock's limits [search: [Synth Anatomy](https://synthanatomy.com/2026/09/baud-girl-fm-1-va-custom-m-vave-fm-1-firmware.html)];
  one complaint found, a slow arpeggiator. There is also an SMK-37+VA version.

## 4. Melodee, the RP2040 tool, X0X, others

- **Melodee 1.4.1** ([repo](https://github.com/keremimo/melodee)): Felucca 1.0 derivative. Engines
  PROPHET (Prophet-5 Rev 4, all 200 programs), FM6, CZ-1, SID, DRUM (808/909). Voice caps: Prophet 8
  on dual core or 5 on one, FM6 16, CZ 8, SID 4, DRUM 8. **A weighted global BUDGET** of 8-32 points
  (default 16; a solo Prophet track gets 5 voices). UI: KNOB 1 across, KNOB 2 down, SELECT pages, OCT+
  enters, OCT- back; HOME is a "Stage" view. Its OVERCLOCK menu was removed after black-screen and
  boot-mode failures; it never touches the clock now.
- **FM-1-transporter** by kurogedelic ([repo](https://github.com/kurogedelic/FM-1-transporter)): a
  XIAO RP2040 on the FM-1's USB D+/D- forces the mask-ROM boot mode; dumps the 1 MiB in about 3 s and
  writes only 0x4000-0x93000.
- **X0X, what users say:** Synth Anatomy was enthusiastic and noted an early worst-case load of about
  102 % [search]. Issues ([list](https://github.com/charlesvestal/fm1-x0x/issues)): core-0 crashes with
  the second core on (#21); a boot crash looping in safe mode (#20); a crash saving a polyrhythm (#17);
  USB audio not detected (#4); headphones quieter than stock, BPM display jitter (#3). Requests: a
  Juno-106 engine, MIDI CC in/out, per-part MIDI channels, multichannel USB audio, patterns that survive
  upgrades.
- **Others** ([awesome-fm-1](https://github.com/cicloid/awesome-fm-1), [fm1-custom-fw](https://github.com/aroum/fm1-custom-fw)):
  [SLOOP](https://github.com/isod89/sloop-fm1) (3 synth parts sharing 8 voices + drums with 6; dual
  core "about twice the room"); Groove OS (commercial, FM + VA, "up to 20 voices", on V15);
  [Jangada](https://github.com/zednaked/jangada) (superwave, up to 6 copies); FOMAJAM (SUPERSAW);
  OberVave (8-voice poly); FM-106 (6-voice Juno-106); [MYCELIUM](https://github.com/megagott/mycelium-fm1)
  (psytrance/goa grooves, untested on hardware). Starting points: [Dinghy](https://github.com/zvenson/dinghy)
  (minimal template), [felucca-bsp](https://github.com/hugelton/felucca-bsp) (MPL-2.0 hardware layer),
  [Felucca-Engines](https://github.com/hugelton/Felucca-Engines) (MIT), [fm1-emu](https://github.com/joakimeriksson/fm1-emu).

## 5. Schwung

- Charles Vestal's MIT framework for extra modules on the **Ableton Move**
  ([repo](https://github.com/charlesvestal/schwung)): aarch64 Linux, PREEMPT_RT, C/C++ plugins plus JS
  UIs, 44.1 kHz, 128-frame int16 stereo blocks ([API.md](https://github.com/charlesvestal/schwung/blob/main/docs/API.md)).
  Far more compute than the AC79. 163 modules in the catalog; 9W9 and 8W8 are by athousanddetails.
- **Trance-relevant:** **NuSaw** (MIT, about 1.8k lines: 8 voices of 3-25 PolyBLEP saws with drift and
  stereo spread, SVF, Juno-style chorus, ping-pong; [repo](https://github.com/charlesvestal/schwung-nusaw)),
  OB-Xd, Hera (Juno-60), Noisemaker (TAL, 6 voices), TinyK, Tablor (wavetable). Too heavy: JE-8086
  (JP-8000) and Osirus (Virus), both DSP56300 emulations. Effects: Trance Gate, Ducker, PushNPull,
  TapeDelay, Junologue Chorus, CloudSeed, MVerb, Dragonfly Hall, Filter. Sequencing: Super Arp, 6W6,
  CW-78. Only NuSaw's licence was checked.

## 6. CPU figures and wishlists

- A 128-frame half has a **2,901 us** deadline. Melodee: 5 Prophet voices peak at 2,141 us (26 %
  headroom) ([AUDIO_PERFORMANCE.md](https://github.com/keremimo/melodee/blob/main/docs/AUDIO_PERFORMANCE.md));
  its RAM: data+bss 94.8 of 98.3 KB, pool 186 of 344 KB.
- Jangada: 12 held voices + drums + sequencer about 31 % of core 0 on one core, about 21 % with both
  (one unit, 360 MHz) ([BUILDING.md](https://github.com/zednaked/jangada/blob/main/BUILDING.md)).
- **The second core is risky:** on some units core 1 misreads RAM at the boot loader's supplies
  (1.26/1.40 V). Jangada fixed its unit at 1.38/1.50 V; X0X runs 1.38/1.60 V (M-VAVE's own). Every
  firmware using it now detects bad reads and falls back to one core.
- **Wishlists:** per-track MIDI channels, MIDI CC out, velocity/aftertouch as mod sources, MPE,
  resampling, multichannel USB audio, conditional trigs and fills, humanize, patterns that survive
  firmware updates, an init patch, a reset-to-default gesture.

## Design lessons

1. Degrade rather than drop out, and show the load.
2. Treat the second core as optional: detect misreads, fall back, leave the clock alone.
3. Keep flash writes away from playback (Baud Girl spaces preset writes 3 s apart; Melodee defers
   writes while voices sound). Never write the flash head 0x0-0x3FFF.
4. A shared UI grammar has emerged: OCT+ confirms, OCT- goes back; the four knobs are the page;
   hold a button for its layer; ALGORITHM selects the track; PRESETS changes the sound from anywhere.
5. Decode the encoders on every scan frame.
6. The expected baseline: a web installer, a browser emulator, a host simulator, a way back to V15.
