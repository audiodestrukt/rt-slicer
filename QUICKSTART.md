# Audio Slicer VST - Quick Start Guide

## Setup (5 Minutes)

### 1. Download JUCE
```bash
cd AudioSlicerVST
git clone https://github.com/juce-framework/JUCE.git
```

### 2. Build the Plugin
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 3. Locate Your Plugin
The built plugin will be in:
- **VST3**: `build/AudioSlicerVST_artefacts/Release/VST3/AudioSlicer.vst3`
- **AU** (macOS): `build/AudioSlicerVST_artefacts/Release/AU/AudioSlicer.component`
- **Standalone**: `build/AudioSlicerVST_artefacts/Release/Standalone/AudioSlicer`

### 4. Install Plugin
Copy the plugin to your DAW's plugin folder:

**macOS**:
- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- AU: `~/Library/Audio/Plug-Ins/Components/`

**Windows**:
- VST3: `C:\Program Files\Common Files\VST3\`

**Linux**:
- VST3: `~/.vst3/`

## First Use (2 Minutes)

### In Your DAW:

1. **Insert Plugin** on an audio track
2. **Route Audio** to the track (microphone, instrument, etc.)
3. **Watch the Waveform** display - you should see incoming audio
4. **Play Some Audio** - the plugin will automatically create slices
5. **Create a MIDI Track** and route it to the plugin
6. **Trigger Slices** with notes C3 through D#4 (60-75)

## Quick Settings for Different Sources

### 🥁 Drums
```
Sensitivity:   0.7 - 0.9
Threshold:     -30 to -20 dB
Min Length:    0.05 - 0.1 s
Max Length:    0.5 - 1.0 s
```

### 🎸 Guitar/Bass
```
Sensitivity:   0.5 - 0.7
Threshold:     -25 to -15 dB
Min Length:    0.1 - 0.3 s
Max Length:    1.0 - 3.0 s
```

### 🎤 Vocals
```
Sensitivity:   0.6 - 0.8
Threshold:     -30 to -20 dB
Min Length:    0.1 - 0.5 s
Max Length:    2.0 - 4.0 s
```

### 🎹 Pads/Ambient
```
Sensitivity:   0.2 - 0.4
Threshold:     -20 to -10 dB
Min Length:    0.5 - 1.0 s
Max Length:    3.0 - 8.0 s
```

## MIDI Mapping Cheat Sheet

```
Piano Layout:
C3 (60)  = Slice 1     C4 (72)  = Slice 13
C#3(61)  = Slice 2     C#4(73)  = Slice 14
D3 (62)  = Slice 3     D4 (74)  = Slice 15
D#3(63)  = Slice 4     D#4(75)  = Slice 16
E3 (64)  = Slice 5
F3 (65)  = Slice 6
F#3(66)  = Slice 7
G3 (67)  = Slice 8
G#3(68)  = Slice 9
A3 (69)  = Slice 10
A#3(70)  = Slice 11
B3 (71)  = Slice 12
```

## Workflow Examples

### Live Looping
1. Play guitar/synth into plugin
2. Wait for 16 slices to fill (watch orange highlight)
3. Use MIDI keyboard to trigger slices rhythmically
4. Layer slices for complex arrangements

### Beat Slicing
1. Play a drum loop into the plugin
2. Adjust sensitivity to catch each hit
3. Use MIDI pads to retrigger hits
4. Create new patterns on the fly

### Texture Building
1. Feed ambient audio/field recordings
2. Set longer slice lengths
3. Trigger overlapping slices
4. Create evolving soundscapes

## Troubleshooting in 30 Seconds

**No audio showing?**
- Check your audio interface settings
- Verify track is armed/monitoring

**No slices being created?**
- Lower Threshold
- Increase Sensitivity

**Too many tiny slices?**
- Increase Min Length
- Lower Sensitivity

**MIDI not working?**
- Check MIDI routing to plugin
- Verify notes are in C3-D#4 range

## Pro Tips

1. **Pre-fill slots** before performing by playing 30-60 seconds of audio
2. **Use velocity** on MIDI notes for dynamics
3. **Click slice boxes** in the UI to test sounds
4. **Automate parameters** in your DAW for evolving slicing behavior
5. **Orange highlight** shows which slot is recording next

## Getting Help

- Check the full README.md for detailed documentation
- Look at the UI - orange = recording, blue = has audio, green = playing
- Each slice shows its MIDI note number at the bottom

---

**You're ready to slice! Have fun! 🎵✨**
