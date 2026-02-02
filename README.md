# Audio Slicer VST Plugin

A JUCE-based VST plugin that automatically slices live incoming audio and allows you to retrigger slices with MIDI notes.

## Features

- **Live Audio Slicing**: Automatically detects transients in incoming audio and creates slices
- **16 Slice Slots**: Fixed number of slots that wrap around when filled
- **MIDI Triggering**: Trigger slices with MIDI notes C3-D#4 (notes 60-75)
- **Real-time Visualization**: 
  - Live waveform display of incoming audio
  - 4x4 grid showing all 16 slices with waveforms
  - Visual feedback for recording, playing, and active slices
- **Adjustable Parameters**:
  - **Sensitivity**: Controls how easily transients are detected (0.0-1.0)
  - **Threshold**: Minimum audio level for transient detection (-60dB to 0dB)
  - **Min Slice Length**: Minimum duration for a slice (0.01s-2.0s)
  - **Max Slice Length**: Maximum duration for a slice (0.1s-10.0s)
- **Polyphonic Playback**: Multiple slices can play simultaneously
- **Click to Trigger**: Click on any slice in the UI to trigger it manually

## How It Works

1. **Audio Recording**: The plugin continuously records incoming audio
2. **Transient Detection**: A transient detection algorithm monitors the audio for sudden increases in energy
3. **Slice Creation**: When a transient is detected and the current recording meets the minimum length requirement, a new slice is created
4. **Slot Management**: Slices fill slots 1-16 in order, then wrap back to slot 1, overwriting previous content
5. **MIDI Triggering**: Send MIDI note-on messages to trigger slices (C3=slot 1, C#3=slot 2, etc.)
6. **Playback**: Triggered slices play back independently while audio continues to be recorded

## MIDI Note Mapping

| Slice | MIDI Note | Note Number |
|-------|-----------|-------------|
| 1     | C3        | 60          |
| 2     | C#3       | 61          |
| 3     | D3        | 62          |
| 4     | D#3       | 63          |
| 5     | E3        | 64          |
| 6     | F3        | 65          |
| 7     | F#3       | 66          |
| 8     | G3        | 67          |
| 9     | G#3       | 68          |
| 10    | A3        | 69          |
| 11    | A#3       | 70          |
| 12    | B3        | 71          |
| 13    | C4        | 72          |
| 14    | C#4       | 73          |
| 15    | D4        | 74          |
| 16    | D#4       | 75          |

## UI Layout

```
┌─────────────────────────────────────┐
│          Audio Slicer VST           │
├─────────────────────────────────────┤
│  [Live Waveform Display]            │
├─────────────────────────────────────┤
│  ┌─────┬─────┬─────┬─────┐          │
│  │  1  │  2  │  3  │  4  │          │
│  │[wav]│[wav]│[wav]│[wav]│          │
│  ├─────┼─────┼─────┼─────┤          │
│  │  5  │  6  │  7  │  8  │          │
│  │[wav]│[wav]│[wav]│[wav]│          │
│  ├─────┼─────┼─────┼─────┤  Slice   │
│  │  9  │ 10  │ 11  │ 12  │  Grid    │
│  │[wav]│[wav]│[wav]│[wav]│          │
│  ├─────┼─────┼─────┼─────┤          │
│  │ 13  │ 14  │ 15  │ 16  │          │
│  │[wav]│[wav]│[wav]│[wav]│          │
│  └─────┴─────┴─────┴─────┘          │
├─────────────────────────────────────┤
│  Sensitivity: [────────○──]         │
│  Threshold:   [────○──────] dB      │
│  Min Length:  [○─────────] s        │
│  Max Length:  [────○─────] s        │
└─────────────────────────────────────┘
```

### Visual Indicators

- **Orange highlight**: Currently recording slice
- **Blue background**: Slice contains audio (active)
- **Green background**: Slice is currently playing
- **Cyan waveform**: Visual representation of slice content
- **White progress bar**: Playback position indicator

## Building the Plugin

### Prerequisites

1. **JUCE Framework**: Download from [https://github.com/juce-framework/JUCE](https://github.com/juce-framework/JUCE)
2. **CMake**: Version 3.15 or higher
3. **C++ Compiler**: Supporting C++17 (GCC, Clang, MSVC)

### Build Instructions

#### Option 1: Using CMake (Recommended)

```bash
# Clone JUCE into the project directory
git clone https://github.com/juce-framework/JUCE.git

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# The plugin will be in: build/AudioSlicerVST_artefacts/Release/
```

#### Option 2: Using Projucer

1. Open Projucer (from JUCE)
2. Create a new "Audio Plug-In" project
3. Copy the source files into your project
4. Configure build settings:
   - Plugin Name: AudioSlicer
   - Plugin Manufacturer Code: Mnfr
   - Plugin Code: Slcr
   - Enable VST3, AU, and Standalone formats
   - Enable MIDI Input
5. Save and open in your IDE (Xcode/Visual Studio)
6. Build the project

### Platform-Specific Notes

#### macOS
```bash
# Install Xcode command line tools
xcode-select --install

# Build
cmake -B build -G Xcode
cmake --build build --config Release
```

#### Windows
```bash
# Using Visual Studio 2019 or later
cmake -B build -G "Visual Studio 16 2019"
cmake --build build --config Release
```

#### Linux
```bash
# Install dependencies
sudo apt-get install libasound2-dev libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libfreetype6-dev

# Build
cmake -B build
cmake --build build --config Release
```

## Usage Guide

### In Your DAW

1. **Load the Plugin**: Insert AudioSlicer on an audio track
2. **Route Audio**: Ensure audio is being sent to the plugin
3. **Adjust Parameters**: 
   - Start with default settings
   - Increase **Sensitivity** if transients aren't being detected
   - Lower **Threshold** if the audio is quiet
   - Adjust **Min/Max Length** based on your material
4. **Create MIDI Track**: Set up a MIDI track routed to the plugin
5. **Trigger Slices**: 
   - Play MIDI notes C3-D#4 to trigger slices
   - Use velocity for dynamics
   - Layer multiple slices for complex patterns

### Tips for Best Results

#### Transient-Rich Material
- Drums, percussion, vocals with clear attacks
- Use **higher sensitivity** (0.7-0.9)
- **Lower threshold** (-30dB to -20dB)
- **Shorter min length** (0.05s-0.2s)

#### Sustained Material
- Pads, drones, ambient sounds
- Use **lower sensitivity** (0.2-0.4)
- **Higher threshold** (-20dB to -10dB)
- **Longer max length** (2.0s-8.0s)

#### Live Performance
- Set **min length** around 0.1s to avoid tiny slices
- Use **max length** to control slice duration
- Pre-fill all 16 slots before performing
- Map MIDI controller to trigger slices

### Common Workflows

#### 1. Live Looper
```
1. Play incoming audio (guitar, synth, vocals)
2. Let the plugin automatically capture slices
3. Trigger slices rhythmically with MIDI controller
4. Layer slices to build complex arrangements
```

#### 2. Drum Slicing
```
1. Play a drum beat into the plugin
2. Adjust sensitivity to catch each hit
3. Set short min/max lengths (0.05s - 0.5s)
4. Trigger individual hits from MIDI keyboard
5. Create new patterns or fills
```

#### 3. Ambient Texture Builder
```
1. Input ambient audio or field recordings
2. Set longer slice lengths (1.0s - 5.0s)
3. Lower sensitivity to capture longer phrases
4. Trigger slices in overlapping patterns
5. Create evolving soundscapes
```

## Technical Details

### Architecture

- **Sample Rate**: Supports all standard sample rates (44.1kHz, 48kHz, 96kHz, etc.)
- **Latency**: Minimal processing latency
- **Buffer Size**: Adaptive to host settings
- **Channel Configuration**: Stereo input/output

### Transient Detection Algorithm

The plugin uses an energy-based transient detection algorithm:

1. **Energy Calculation**: RMS energy of incoming audio blocks
2. **Ratio Analysis**: Compares current energy to previous energy
3. **Threshold Check**: Energy must exceed threshold in dB
4. **Cooldown Period**: Prevents multiple detections (100ms default)
5. **Sensitivity Scaling**: Adjusts required energy ratio

### Memory Management

- **Per-Slice Buffer**: Up to 10 seconds at current sample rate
- **Visualization Buffer**: 2 seconds circular buffer
- **Total Memory**: ~20MB at 48kHz (approximately)

## Troubleshooting

### No Slices Being Created
- Check that audio is reaching the plugin (watch waveform display)
- Lower the **Threshold** parameter
- Increase the **Sensitivity** parameter
- Verify **Min Length** isn't too high

### Too Many Slices
- Increase **Min Length** to prevent short slices
- Lower **Sensitivity** to reduce detection
- Raise **Threshold** to ignore quiet sounds

### Slices Cut Off Too Early
- Increase **Max Length** parameter
- Check transient detection isn't triggering too frequently

### No MIDI Response
- Verify MIDI is being sent to the plugin
- Check MIDI note range (C3-D#4 / 60-75)
- Ensure slices are active (have audio content)

### Audio Clicks or Pops
- Increase your DAW's buffer size
- Check CPU usage (multiple simultaneous slices)

## Customization

### Modifying Slice Count

To change from 16 slices to a different number:

1. Open `PluginProcessor.h`
2. Change `static constexpr int maxSlices = 16;` to your desired count
3. Update the MIDI note mapping range
4. Adjust UI grid layout in `PluginEditor.cpp` (currently 4x4)

### Adding Features

Suggested enhancements:
- Save/load slice presets
- Adjustable MIDI note ranges
- Individual slice volume controls
- Slice reverse/pitch shift
- Export slices as audio files
- Loop points for slices

## License

This project uses the JUCE framework. Please review JUCE licensing terms:
- GPL v3 for open-source projects
- Commercial license required for closed-source applications

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

## Credits

Built with [JUCE](https://juce.com/) - The Cross-Platform C++ Framework

## Version History

- **v1.0.0** (2024)
  - Initial release
  - 16 slice slots
  - Transient detection
  - MIDI triggering
  - Real-time visualization
  - Parameter automation

---

**Enjoy slicing! 🎵**
