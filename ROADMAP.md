# RipSlice roadmap

Possible directions, not commitments. Ordered by how much each would change what
the app *is*, not by effort.

The governing constraint: **RipSlice's value is that you pick it up and
immediately hear something.** LiveSlice — its ancestor — had 32 tracks, 64
loops, 256 slices per loop and a per-slice parameter sequencer. This is 16 pads
and four sliders on purpose. Anything below is worth doing only if it survives
the question *"does this add setup between picking it up and playing?"*

---

## 1. MIDI-controlled recording and slicing

**The reason the project exists.** Today MIDI notes 60–75 only *play* slices
already captured; capture itself is automatic, cut by transient detection or by
`maxSliceLengthParam`. The performer has no say over where a slice begins.

This is the feature Dan wanted from LiveSlice, which got it in **1.47** — its
second-to-last release, on a Windows-only VST:

> More midi automation options: slicing and recording can now be controlled by
> midi (for example: hit a midi-key to record, use mod-wheel to auto slice the
> recording)

**Shape:** a note that arms/starts recording, and a note (or a pad hit) that
says *cut here, now*. Automatic transient slicing stays as the default; this is
manual override.

**Cost:** the finalize primitive already exists — `recordAudioToSlice()` plus
the `currentSliceIndex` advance is precisely what the transient path calls, so a
MIDI cut reuses it. The work is timing: `processBlock` currently runs
`processIncomingAudio()` and `processMidiMessages()` as two separate passes, so
MIDI is block-granular. Sample-accurate cuts need MIDI events interleaved into
the record loop by `metadata.samplePosition`. An afternoon, not a rewrite.

**Simplicity check:** passes. Adds no UI — it's a MIDI mapping.

---

## 2. Freezing, cycling and layering

Three connected ideas, and together they change the app's basic loop from
"endlessly overwriting" to "curate while you play".

### Freeze a slot

Mark a slot as keep. Capture then **skips frozen slots** when it advances, so
good material survives while the rest keeps cycling. This is the single highest
value-per-line item on the list.

- Per-slice `isFrozen`, atomic — the UI thread toggles it, the audio thread
  reads it. `currentSliceIndex` advance skips frozen slots.
- **Gesture: long-press a pad.** Adds no chrome, and the grid is already the
  whole interface.
- Frozen pads need to look obviously different — this is state you must be able
  to read mid-performance at arm's length.
- **Edge case with teeth:** all 16 frozen means there is nowhere to record.
  Capture must visibly pause rather than silently stop. Decide and show it.
- Freezing over MIDI fits the MIDI-control theme (see 1).

**The fork to settle first — what does "for use later" mean?** Nothing about
the audio is persisted today; `getStateInformation` saves four parameter values
and no samples. Within-session freezing is easy. Surviving an app restart means
writing ~13 MB of audio somewhere: in an AUv3's state blob that is abusive to
the host, so on iOS it wants the app's Documents folder, which drags in file
management — a real threat to "no setup". **These are different features.
Decide which one is meant before building either.**

### Layer instead of overwrite

Rather than replacing an unfrozen slot, mix the new audio into what is already
there, so repeated passes accumulate density. Composes exactly with freeze:
frozen slots are locked, unfrozen ones evolve.

Worth a **decay amount** so older layers fade as new ones arrive — the tape-loop
regeneration idea. Without it everything silts up into mud within a few passes.
One knob, and it turns the whole grid into an instrument that drifts as you
play it.

**Simplicity check:** passes if it is the *default* behaviour of cycling rather
than a mode you switch into.

---

## 3. Slices that land on usable trigger samples

The detector was outright broken until 2026-09-09 — the cooldown ran 64x too
long and hits after silence were undetectable, so 2 of 40 test hits registered.
That is fixed (40 of 40), and **much of what felt like poor slice-point
selection was simply deafness.** Re-listen before building anything below.

What remains, in descending order of how much it affects whether a slice is
playable:

- **Backtrack to the onset.** A transient is detected *after* energy has risen,
  so the attack's first milliseconds land at the end of the *previous* slice.
  Clipped attacks are the classic reason sliced hits feel weak. At detection,
  walk back through the recording buffer to the local energy minimum and cut
  there.
- **Snap to zero crossings, plus a 1-3 ms fade at each end.** Removes clicks at
  slice boundaries.
- **Trim trailing silence** so a pad's length matches its sound.
- **Reject junk.** A near-silent or too-short slice should not consume a slot.
- **Level-match slices** so pads play back at comparable loudness.

**Simplicity check:** all invisible. No new controls, slices just get better.

---

## 4. Stem slicing

Suggested by a friend of Dan's: slice *stems* rather than one mixed signal, so a
pad can hold (say) just the drums or just the bass out of the incoming audio.

Three quite different techniques hide under this name, and they are not
interchangeable:

| Approach | How | Real-time? | Fit |
|---|---|---|---|
| **Frequency bands** | 3–4 crossover filters, each band sliced independently | Yes, trivially | Cheap and predictable, but a kick and a bassline share a band — it is not separation, it is EQ |
| **Harmonic/percussive (HPSS)** | Split transient from tonal content | Yes, with modest latency | "Drums vs everything else" is usually what people actually want from stems; far cheaper than ML |
| **ML source separation** (Demucs, Spleeter) | Learned 4-stem model | Not really — needs a lookahead window | Large model files and memory, against a 13 MB budget; a different product |

**Recommended starting point: HPSS, or bands as a first sketch.** ML separation
on live input is a poor fit for this app on this hardware, and the simplicity
cost is severe.

**The design fork that matters more than the DSP:** does a stem *multiply the
pads* (4 bands × 16 slots = 64 pads) or does each pad keep holding one slice
with its bands separable at playback (solo/mute a band per pad)? The first
detonates the 4×4 grid that makes the app legible on a phone; the second keeps
16 pads and adds a per-pad control. **Prefer the second.** Decide this before
writing any DSP.

**Simplicity check:** the biggest threat on this list. Needs a shape that does
not add a mode.

---

## 5. Slices that look like what they sound like

Two small display ideas, no new controls.

- **Average alongside peak.** Pad waveforms currently plot max-|amplitude| only,
  so a kick and a kick-with-a-quiet-hat are identical. LiveSlice 1.47 again:
  *"visualizes both the average and the loudest frequency so there's a visual
  difference between a bassdrum with or without hihat — even when the hihat is
  quiet."*
- **Colour by content, not just state.** Pads are currently blue (holds audio) /
  green (playing) / orange (recording). Tinting by spectral centroid would let
  you find a slice by eye instead of hunting. LiveSlice: *"Each slice is coloured
  based on the content so you can easily tell them apart."*

**Simplicity check:** passes — strictly additive to what is already drawn.

---

## 6. Revisit the iOS slice-length ceiling

`maxSliceSeconds` is 2 s on iOS against 10 s on desktop, which took the buffer
allocation from ~66 MB to ~13 MB — necessary, because an AUv3 shares a host with
other plugins. But 2 s is a real functional reduction and was chosen on
reasoning, not on use.

If it feels short in practice, the honest fix is not simply raising it: it is
storing slices at their actual length rather than allocating all 16 at the
maximum. That would need reallocation off the audio thread, or a shared ring
buffer with slices as views into it.

**Do this before anyone has presets worth keeping.**

---

## Not features, but blocking

- **Persistent Apple Development certificate.** Each CI archive currently mints
  a throwaway one; roughly four more builds and archives start failing with
  "maximum number of certificates". One portal round trip fixes it permanently.
- **macOS build.** An earlier macOS build would not run on Dan's Mac —
  unverified cause, likely Gatekeeper on an un-notarized bundle or an
  architecture mismatch. Notarization needs a Developer ID Application
  certificate the team does not have.
- **The site.** audiodestrukt.com still links to the pre-move
  `dnewcome/rt-slicer` and tags the project "VST3 / Windows / macOS".
