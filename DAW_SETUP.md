# DAW-Specific Setup Instructions

## Ableton Live

### Setup
1. **Preferences** → **Plug-Ins** → **VST3 Plug-In Custom Folder**
2. Add your VST3 folder path
3. Click **Rescan**

### Usage
1. Create an **Audio Track**
2. Enable **Monitoring** (speaker icon)
3. Insert **AudioSlicer** on the track
4. Create a **MIDI Track** below
5. Set MIDI track output to **Audio Track** → **AudioSlicer**
6. Arm MIDI track
7. Play MIDI notes C3-D#4 to trigger slices

### MIDI Mapping
- Use **MIDI Map Mode** (Cmd/Ctrl + M)
- Map MIDI controller to trigger notes
- Map knobs to plugin parameters

---

## Logic Pro X

### Setup
1. **Logic Pro X** → **Preferences** → **Plug-In Manager**
2. Ensure AudioSlicer is enabled
3. Reset & Rescan if needed

### Usage
1. Create **Audio Track** (Cmd + Option + A)
2. Enable **Input Monitoring**
3. Insert **AudioSlicer** (Audio FX → AudioSlicer)
4. Create **Software Instrument Track**
5. Set output to **Audio Track** → **AudioSlicer**
6. Use on-screen keyboard or MIDI controller

### Sidechain Setup
1. Route audio to AudioSlicer track
2. Use external audio input if needed
3. Set monitoring to **On**

---

## FL Studio

### Setup
1. **Options** → **Manage plugins**
2. Ensure VST3 path is added
3. Scan for plugins

### Usage
1. Add AudioSlicer to **Mixer Track**
2. Route audio source to that mixer track
3. Create **MIDI Out** device in Channel Rack
4. Route MIDI Out to AudioSlicer track
5. Play notes in Piano Roll

### Alternative
1. Use **Patcher** for complex routing
2. Route both audio and MIDI into AudioSlicer

---

## Pro Tools

### Setup
1. **Setup** → **Preferences** → **Plug-Ins**
2. Set VST3 folder location
3. Rescan plug-ins

### Usage
1. Create **Audio Track**
2. Enable **Input Monitoring** (I button)
3. Insert **AudioSlicer** (Plug-In → AudioSlicer)
4. Create **Instrument Track** for MIDI
5. Route MIDI output to AudioSlicer
6. Record-enable MIDI track

### MIDI Setup
- Use **MIDI Input Selector** window
- Map MIDI controller to track
- Set note range C3-D#4

---

## Reaper

### Setup
1. **Options** → **Preferences** → **VST**
2. Add VST3 path
3. **Re-scan**

### Usage
1. Create **Track**
2. **Right-click track** → **FX** → **AudioSlicer**
3. Set track input to your audio source
4. Enable **Record Monitoring** (red speaker icon)
5. Create another track for MIDI
6. Route MIDI track output to AudioSlicer track

### ReaLearn Integration
- Map MIDI controller to trigger notes
- Automate parameters via ReaLearn

---

## Bitwig Studio

### Setup
1. **Settings** → **Plug-Ins** → **Locations**
2. Add VST3 folder
3. Rescan

### Usage
1. Create **Audio Track**
2. Enable **Monitoring**
3. Add **AudioSlicer** to FX chain
4. Create **Note Track** below
5. Set Note track output to **AudioSlicer**
6. Arm Note track

### Modulation
- Use Bitwig's modulators on plugin parameters
- Map Grid controller for slice triggering

---

## Studio One

### Setup
1. **Studio One** → **Options** → **Locations** → **VST Plug-Ins**
2. Add path and scan
3. Restart if needed

### Usage
1. Create **Audio Track**
2. Enable **Monitor**
3. Insert **AudioSlicer**
4. Create **Instrument Track** for MIDI
5. Set output to **AudioSlicer** track
6. Play MIDI notes

### Macro Controls
- Assign plugin parameters to Macro controls
- Quick access during performance

---

## Cubase

### Setup
1. **Studio** → **VST Plug-in Manager**
2. Add VST3 path
3. Update plugin information

### Usage
1. Create **Audio Track**
2. Enable **Monitor** button
3. Insert **AudioSlicer** (Inserts slot)
4. Create **MIDI Track**
5. Route MIDI output to AudioSlicer
6. Record-enable MIDI track

### Expression Maps
- Create Expression Map for slices
- Map individual slices to keys
- Quick slice selection

---

## Reason

### Setup (Requires VST support - Reason 11+)
1. **Preferences** → **Advanced** → **VST Folders**
2. Add VST3 path
3. Rescan

### Usage
1. Add AudioSlicer via **Browser**
2. Route audio input via cables
3. Create **MIDI Track** in Sequencer
4. Route MIDI to AudioSlicer
5. Play and trigger

---

## Common Issues Across DAWs

### Plugin Not Appearing
- Ensure plugin is in correct format (VST3/AU)
- Check plugin path in DAW preferences
- Rescan plugins
- Check plugin architecture (64-bit)
- Restart DAW

### No Audio Coming In
- Enable track monitoring
- Check audio interface settings
- Verify input routing
- Check buffer size (increase if crackling)

### MIDI Not Triggering
- Verify MIDI routing to plugin
- Check MIDI note range (C3-D#4)
- Enable MIDI track recording
- Test with virtual keyboard

### Performance Issues
- Increase buffer size
- Reduce number of simultaneous slices
- Freeze/bounce tracks
- Close other applications

---

## Performance Tips by DAW

### Ableton Live
- Freeze tracks when not recording
- Use "Reduced Latency" mode for monitoring

### Logic Pro X
- Enable "Low Latency Mode" (Cmd + Option + K)
- Use I/O buffer size wisely

### FL Studio
- Enable "Smart Disable" for plugins
- Adjust buffer length in Audio Settings

### Pro Tools
- Use "Low Latency Monitoring"
- Increase H/W Buffer Size

### Reaper
- Enable "Anticipative FX processing"
- Adjust "Audio buffering" settings

### Bitwig
- Use "Track Freeze"
- Enable "Multi-Core Processing"

### Studio One
- Enable "Dropout Protection"
- Use "Native Low-Latency Monitoring"

### Cubase
- Enable "ASIO-Guard"
- Use "Constrain Delay Compensation"

---

## MIDI Controller Setup

### General Steps
1. Connect MIDI controller
2. Enable controller in DAW MIDI settings
3. Route controller to MIDI track
4. Map notes C3-D#4 to pads/keys
5. Optionally map CC to parameters

### Recommended Controllers
- **Akai MPD** series (drum pads)
- **Novation Launchpad** (grid layout)
- **Native Instruments Maschine** (16 pads = 16 slices)
- **Ableton Push** (for Ableton users)
- **Any MIDI keyboard** (octave C3-C4)

### Mapping Suggestions
```
Pad Layout (4x4):
[1 ] [2 ] [3 ] [4 ]    C3  C#3 D3  D#3
[5 ] [6 ] [7 ] [8 ]    E3  F3  F#3 G3
[9 ] [10] [11] [12]    G#3 A3  A#3 B3
[13] [14] [15] [16]    C4  C#4 D4  D#4
```

---

For more help, consult your DAW's manual or visit the AudioSlicer documentation.
