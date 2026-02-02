# Audio Slicer VST - Technical Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        Audio Slicer VST                         │
│                                                                 │
│  ┌───────────────┐      ┌──────────────┐      ┌─────────────┐ │
│  │  Audio Input  │──────▶│  Transient   │──────▶│   Slice     │ │
│  │   (Live)      │      │  Detector    │      │  Management │ │
│  └───────────────┘      └──────────────┘      └─────────────┘ │
│                                                        │         │
│  ┌───────────────┐                                   │         │
│  │  MIDI Input   │                                   │         │
│  │  (C3-D#4)     │───────────────────────────────────┘         │
│  └───────────────┘                  │                           │
│                                     ▼                           │
│                          ┌──────────────────┐                  │
│                          │  Playback Engine │                  │
│                          │  (Polyphonic)    │                  │
│                          └──────────────────┘                  │
│                                     │                           │
│                                     ▼                           │
│  ┌───────────────┐      ┌──────────────────┐                  │
│  │  Audio Output │◀─────│  Audio Mixing    │                  │
│  │   (Mixed)     │      │  (All Slices)    │                  │
│  └───────────────┘      └──────────────────┘                  │
│                                                                 │
│  ┌───────────────────────────────────────────────────────────┐ │
│  │                    UI (Editor)                            │ │
│  │  - Live Waveform Display                                 │ │
│  │  - 16 Slice Grid (4x4)                                   │ │
│  │  - Parameter Controls                                    │ │
│  └───────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

## Data Flow

### 1. Audio Recording Flow
```
Input Audio
    │
    ├──▶ Visualization Buffer (for UI)
    │
    └──▶ Recording Buffer
           │
           ├──▶ Transient Detection (block-by-block)
           │      │
           │      └──▶ Transient Found?
           │             │
           │            Yes ──▶ Finalize Current Slice
           │             │
           │            No ──▶ Continue Recording
           │
           └──▶ Max Length Reached? ──▶ Force Slice Creation
```

### 2. Slice Storage Flow
```
Finalized Slice
    │
    └──▶ Store in slices[currentSliceIndex]
           │
           ├──▶ Copy audio data
           ├──▶ Set length
           ├──▶ Mark as active
           └──▶ Increment index (wraps at 16)
```

### 3. MIDI Trigger Flow
```
MIDI Note On
    │
    └──▶ Parse Note Number
           │
           └──▶ Map to Slice Index (Note 60 = Slice 0)
                  │
                  └──▶ Slice Active?
                         │
                        Yes ──▶ Start Playback
                         │        ├─ Reset position
                         │        ├─ Set playing flag
                         │        └─ Store velocity
                         │
                        No ──▶ Ignore
```

### 4. Audio Playback Flow
```
Process Block
    │
    ├──▶ For Each Slice
    │      │
    │      └──▶ Is Playing?
    │             │
    │            Yes ──▶ Read Audio Data
    │             │        │
    │             │        ├─ Apply Gain (velocity)
    │             │        ├─ Mix to Output
    │             │        └─ Increment Position
    │             │             │
    │             │             └──▶ End Reached? ──▶ Stop Playback
    │             │
    │            No ──▶ Skip
    │
    └──▶ Output Mixed Audio
```

## Class Structure

```
AudioSlicerAudioProcessor (Main Processor)
    │
    ├── TransientDetector
    │   ├── detectTransient()
    │   ├── calculateEnergy()
    │   └── prepare()
    │
    ├── AudioSlice[16] (Array of slices)
    │   ├── buffer (audio data)
    │   ├── lengthSamples
    │   ├── playbackPosition
    │   ├── isActive
    │   ├── isPlaying
    │   └── gain
    │
    ├── recordingBuffer (temporary storage)
    ├── visualizationBuffer (for UI)
    │
    ├── processBlock() (main audio callback)
    │   ├── processIncomingAudio()
    │   ├── processMidiMessages()
    │   └── mix playing slices
    │
    ├── triggerSlice()
    └── recordAudioToSlice()

AudioSlicerAudioProcessorEditor (UI)
    │
    ├── WaveformDisplay
    │   ├── paint() (draws incoming audio)
    │   └── timerCallback() (updates 30 FPS)
    │
    ├── SliceGridDisplay
    │   ├── paint() (draws all 16 slices)
    │   ├── drawSlice() (individual slice rendering)
    │   ├── mouseDown() (click to trigger)
    │   └── timerCallback() (updates 30 FPS)
    │
    └── Parameter Controls (sliders)
```

## Memory Layout

```
Per Instance Memory Usage (at 48kHz):

┌─────────────────────────────────────────┐
│ AudioSlice[16]                          │  ~18.5 MB
│   16 × (10 sec × 48000 × 2 ch × 4 bytes)│
├─────────────────────────────────────────┤
│ recordingBuffer                         │   ~1.9 MB
│   (10 sec × 48000 × 2 ch × 4 bytes)    │
├─────────────────────────────────────────┤
│ visualizationBuffer                     │   ~0.7 MB
│   (2 sec × 48000 × 2 ch × 4 bytes)     │
├─────────────────────────────────────────┤
│ Other data structures                   │   ~0.1 MB
└─────────────────────────────────────────┘
Total: ~21 MB per instance
```

## Thread Safety

```
Audio Thread (Real-time)
    │
    ├── processBlock()
    │     ├── Read: slices[] (thread-safe atomic reads)
    │     ├── Write: slices[] playback state
    │     └── Write: visualizationBuffer (with lock)
    │
    └── Must be lock-free for audio processing

UI Thread (Non-real-time)
    │
    ├── timerCallback() (30 FPS)
    │     ├── Read: slices[] (may miss updates, acceptable)
    │     └── Read: visualizationBuffer (with lock)
    │
    └── Can tolerate occasional frame drops

Critical Sections:
    ├── visualizationLock (for buffer access)
    └── Atomic operations for currentSliceIndex
```

## Signal Processing Chain

```
Input Sample
    │
    ├──▶ Transient Detection Path
    │      │
    │      ├─ Buffer N samples
    │      ├─ Calculate RMS energy
    │      ├─ Compare to previous energy
    │      ├─ Apply threshold
    │      └─ Signal if transient detected
    │
    ├──▶ Recording Path
    │      │
    │      ├─ Write to recordingBuffer
    │      ├─ Check transient signal
    │      └─ Finalize slice if needed
    │
    └──▶ Playback Path
           │
           ├─ For each active slice:
           │    ├─ Read from slice.buffer
           │    ├─ Apply gain
           │    └─ Accumulate to output
           │
           └─ Output mixed samples
```

## Parameter Processing

```
Sensitivity Parameter (0.0 - 1.0)
    │
    └──▶ Maps to required energy ratio
           │
           Formula: RequiredRatio = 1.5 + (1.0 - sensitivity) × 3.0
           │
           Result: 1.5 (high) to 4.5 (low)

Threshold Parameter (-60 to 0 dB)
    │
    └──▶ Converts to linear scale
           │
           Formula: Linear = 10^(dB/20)
           │
           Result: 0.001 to 1.0

Min/Max Length (seconds)
    │
    └──▶ Converts to samples
           │
           Formula: Samples = seconds × sampleRate
           │
           Used for: Slice validation and auto-finalization
```

## Timing Diagram (Single Slice Lifecycle)

```
Time ──────────────────────────────────────────────────────▶

Audio    ████████████████████████████████████████
Input    (continuous)

Transient        ▲                    ▲
Detection        │                    │
                 │                    │
Recording   ╔════╧════╗          ╔════╧════╗
Buffer      ║ Slice 1 ║          ║ Slice 2 ║
            ╚═════════╝          ╚═════════╝
            │         │          │
            │  Store  │          │
            └────┐    │          │
                 ▼    │          │
Slice[0]    ████████  │          │
                      │          │
            currentSliceIndex    │
                 0 ──────────────┼─▶ 1
                                 │
MIDI                             │    █  █
Notes                            │   ██ ██
                                 │   Note 60
                                 │    │
Playback                         └────┼──▶ ████████
                                      │    (Slice 0)
                                      │
                                      └──▶ ████
                                           (Slice 0)
```

## State Machine (Per Slice)

```
          ┌─────────────┐
          │   EMPTY     │
          │ (inactive)  │
          └──────┬──────┘
                 │
                 │ Recording complete
                 ▼
          ┌─────────────┐
          │   READY     │◀───────┐
          │  (active)   │        │
          └──────┬──────┘        │
                 │               │
                 │ MIDI trigger  │ Playback complete
                 ▼               │
          ┌─────────────┐        │
          │  PLAYING    │────────┘
          │             │
          └─────────────┘
                 │
                 │ Overwritten by new recording
                 ▼
          ┌─────────────┐
          │   EMPTY     │
          │             │
          └─────────────┘
```

## Performance Characteristics

```
Operation                 Complexity    Typical Time
─────────────────────────────────────────────────────
Process Block            O(n × s)       < 1ms
  n = samples per block
  s = active slices

Transient Detection      O(n)           < 0.1ms
  n = samples per block

Slice Storage            O(n)           < 0.5ms
  n = slice length

MIDI Processing          O(m)           < 0.01ms
  m = MIDI messages

UI Refresh               O(1)           16-33ms (30-60 FPS)

Waveform Drawing         O(w)           1-5ms
  w = display width

Total CPU per block:     ~1-2% @ 512 samples, 48kHz
```

## Buffer Size Impact

```
Buffer Size    Latency    CPU Usage    Stability
────────────────────────────────────────────────
64 samples     1.3 ms     Higher       Lower
128 samples    2.7 ms     Medium       Medium
256 samples    5.3 ms     Medium       Good
512 samples    10.7 ms    Lower        Best
1024 samples   21.3 ms    Lowest       Best

(at 48 kHz sample rate)

Recommendation: 256-512 for live use
```

## Optimization Points

### Hot Paths (Critical for Real-time)
```
1. processBlock()
   └── Must complete within buffer time
       └── Typical budget: 512 samples @ 48kHz = 10.7ms

2. Slice playback mixing
   └── O(n) per active slice
       └── Optimize: SIMD operations, minimize branches

3. Transient detection
   └── O(n) per block
       └── Optimize: Look-up tables, integer math
```

### Cold Paths (Can be slower)
```
1. UI updates (30 FPS acceptable)
2. Parameter changes (immediate response not critical)
3. Slice storage (happens between blocks)
4. MIDI message parsing (low message rate)
```

## Failure Modes & Recovery

```
Failure Mode              Detection              Recovery
──────────────────────────────────────────────────────────
Buffer overflow           Check index bounds     Wrap around
Memory allocation fail    Null pointer check     Skip operation
Sample rate mismatch      Validate in prepare    Resample
MIDI flood                Rate limiting          Drop messages
UI freeze                 Watchdog timer         Restart timer
Audio glitch              Energy spike check     Ignore frame
Lock timeout              Try-lock pattern       Skip update
```

## Extension Points

```
Future Features:
├── Per-slice effects
│   └── Add DSP chain to AudioSlice
├── Slice export
│   └── Add export method to processor
├── Undo/Redo
│   └── Add command pattern for state changes
├── Preset management
│   └── Serialize/deserialize slice data
├── Network sync
│   └── Add OSC/network transport layer
└── Advanced detection
    └── ML-based transient detection
```

---

## Key Design Decisions

1. **Fixed Slice Count**: Simplifies memory management and MIDI mapping
2. **Circular Buffer**: Automatic overwriting of oldest slices
3. **Energy-based Detection**: Balance between accuracy and CPU usage
4. **Polyphonic Playback**: Multiple slices can play simultaneously
5. **Lock-free Audio Path**: No mutexes in process block for real-time safety
6. **30 FPS UI**: Balance between responsiveness and CPU usage
7. **Stereo Only**: Simplifies implementation while covering 99% of use cases

---

This architecture provides a solid foundation for live audio slicing while maintaining real-time performance guarantees.
