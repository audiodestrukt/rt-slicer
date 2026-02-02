# Troubleshooting Guide

## Build Issues

### JUCE Not Found

**Error**: `JUCE not found at /path/to/JUCE`

**Solution**:
```bash
# Clone JUCE into project directory
git clone https://github.com/juce-framework/JUCE.git

# Or set JUCE_DIR environment variable
export JUCE_DIR=/path/to/your/JUCE

# Or specify in CMake
cmake -DJUCE_DIR=/path/to/your/JUCE ..
```

### CMake Version Too Old

**Error**: `CMake 3.15 or higher is required`

**Solution**:
```bash
# macOS
brew upgrade cmake

# Ubuntu/Debian
sudo apt-get update
sudo apt-get install cmake

# Or download from https://cmake.org/download/
```

### Missing Dependencies (Linux)

**Error**: Various linker errors about missing libraries

**Solution**:
```bash
# Ubuntu/Debian
sudo apt-get install build-essential libasound2-dev libx11-dev \
    libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev \
    libgl1-mesa-dev libglu1-mesa-dev

# Fedora
sudo dnf install alsa-lib-devel freetype-devel libX11-devel \
    libXrandr-devel libXinerama-devel libXcursor-devel mesa-libGL-devel

# Arch
sudo pacman -S alsa-lib freetype2 libx11 libxrandr libxinerama libxcursor mesa
```

### Compilation Errors

**Error**: `'juce::AudioProcessor' has not been declared`

**Solution**:
```bash
# Check JUCE version (need 6.0 or higher)
cd JUCE
git fetch --tags
git checkout 7.0.9  # or latest stable version

# Clean and rebuild
rm -rf build
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Xcode Build Fails (macOS)

**Error**: Code signing errors

**Solution**:
1. Open the Xcode project
2. Select the target
3. Go to "Signing & Capabilities"
4. Uncheck "Sign to Run Locally" or add your Apple Developer account

### Visual Studio Build Fails (Windows)

**Error**: MSBuild version issues

**Solution**:
```bash
# Use correct generator
cmake -G "Visual Studio 16 2019" -A x64 ..

# Or use Visual Studio 2022
cmake -G "Visual Studio 17 2022" -A x64 ..
```

---

## Runtime Issues

### Plugin Not Loading in DAW

**Symptom**: Plugin doesn't appear in plugin list

**Checklist**:
1. ✓ Plugin copied to correct location?
   - macOS AU: `~/Library/Audio/Plug-Ins/Components/`
   - macOS VST3: `~/Library/Audio/Plug-Ins/VST3/`
   - Windows VST3: `C:\Program Files\Common Files\VST3\`
   - Linux VST3: `~/.vst3/`

2. ✓ DAW rescanned plugins?
   - Most DAWs: Preferences → Plugins → Rescan

3. ✓ Plugin architecture matches DAW?
   - Check if both are 64-bit

4. ✓ Permissions correct? (macOS)
   ```bash
   # Remove quarantine attribute
   xattr -dr com.apple.quarantine /path/to/AudioSlicer.component
   ```

5. ✓ Security settings (macOS)?
   - System Preferences → Security & Privacy → Allow

**Debug**:
```bash
# Check if plugin is valid (macOS)
codesign -dv /path/to/AudioSlicer.component

# Validate AU (macOS)
auval -v aufx Slcr Mnfr

# Check VST3 (all platforms)
# Use VST3 plugin validator from Steinberg
```

### No Audio Input

**Symptom**: Waveform display shows flat line

**Solutions**:

1. **Check Audio Routing**:
   - Verify audio source is routed to plugin track
   - Enable input monitoring on track
   - Check audio interface settings

2. **Check Buffer Size**:
   - Try increasing buffer size (512 or 1024 samples)
   - Some interfaces need larger buffers

3. **Check Latency Settings**:
   - Disable "low latency mode" in DAW
   - Check if plugin is being bypassed

4. **Verify in Standalone Mode**:
   ```bash
   # Run standalone version
   ./build/AudioSlicerVST_artefacts/Release/Standalone/AudioSlicer
   
   # Check if audio input works there
   # If yes, problem is DAW routing
   # If no, problem is plugin or system audio
   ```

### No Slices Being Created

**Symptom**: Incoming audio visible but no slices fill up

**Solutions**:

1. **Adjust Sensitivity**:
   - Increase sensitivity to 0.7-0.9
   - Lower threshold to -40 dB or below

2. **Check Audio Levels**:
   - Input audio might be too quiet
   - Boost input gain before plugin
   - Check "Threshold" parameter

3. **Verify Min/Max Length**:
   - Min Length might be too high
   - Try 0.05 - 0.1 seconds for min length
   - Ensure Max > Min

4. **Debug Mode** (requires rebuild):
   ```cpp
   // Add to PluginProcessor.cpp, in processIncomingAudio()
   DBG("Current slice samples: " << currentSliceSamples);
   DBG("Transient detected: " << transientDetector.detectTransient(...));
   ```

### MIDI Not Triggering Slices

**Symptom**: Slices created but MIDI notes don't trigger them

**Solutions**:

1. **Verify MIDI Routing**:
   - Check MIDI track output is set to plugin
   - Enable MIDI input on plugin track
   - Test with different MIDI channel

2. **Check Note Range**:
   - Plugin responds to notes 60-75 (C3-D#4)
   - Try playing C3 (middle C, MIDI note 60)
   - Check octave setting on keyboard

3. **Test with Mouse**:
   - Click on slice boxes in UI
   - If clicking works, problem is MIDI routing
   - If clicking doesn't work, problem is playback engine

4. **Check Slice State**:
   - Slices must be "active" (blue in UI)
   - Orange = currently recording
   - Grey = empty, won't trigger

5. **MIDI Monitor**:
   ```bash
   # macOS - show incoming MIDI
   # Use MIDI Monitor app from https://www.snoize.com/MIDIMonitor/
   
   # Or add debug to processMidiMessages():
   DBG("MIDI Note: " << message.getNoteNumber() 
       << " Velocity: " << message.getVelocity());
   ```

### Slices Play But No Sound

**Symptom**: UI shows slices playing (green) but no audio output

**Solutions**:

1. **Check Output Routing**:
   - Verify track output is not muted
   - Check sends are not pre-fader with fader down
   - Ensure plugin is not bypassed

2. **Check Velocity**:
   - Try with maximum velocity (127)
   - Some keyboards send very low velocity

3. **Test Different Slice**:
   - Click different slices
   - One slice might be corrupted

4. **Check Sample Rate**:
   - Mismatch can cause silence
   - Check DAW project sample rate
   - Rebuild with correct sample rate

### Audio Clicks and Pops

**Symptom**: Glitches when triggering or during playback

**Solutions**:

1. **Increase Buffer Size**:
   - Go to DAW audio settings
   - Try 512 or 1024 samples
   - Trade latency for stability

2. **Reduce Simultaneous Slices**:
   - Playing 10+ slices simultaneously = high CPU
   - Limit to 4-6 simultaneous slices

3. **Check CPU Usage**:
   - Close other applications
   - Freeze/bounce other tracks
   - Disable heavy plugins

4. **Optimize Plugin**:
   ```cpp
   // Reduce visualization update rate
   // In PluginEditor.cpp:
   startTimerHz(15); // Instead of 30
   ```

5. **Enable Multi-core Processing**:
   - Check DAW preferences
   - Enable multi-threading/multi-core

### Slices Getting Cut Off

**Symptom**: Slices stop playing before they finish

**Solutions**:

1. **Increase Max Length**:
   - Set to 5-10 seconds
   - Check audio doesn't exceed buffer size

2. **Check Memory**:
   - Monitor system RAM
   - Each slice can be up to 10 seconds
   - 16 slices × 10s × 2 channels = ~18MB

3. **Verify Recording**:
   - Watch orange highlight during recording
   - Should stay on one slot for full duration
   - If switching too fast, increase min length

---

## UI Issues

### Waveform Not Updating

**Symptom**: Flat line in waveform display

**Solutions**:

1. **Check Timer**:
   - UI updates at 30 FPS
   - If frozen, timer might have stopped
   - Close and reopen plugin editor

2. **Thread Safety**:
   - Visualization lock might be deadlocked
   - Restart DAW
   - Report bug if persistent

3. **Resolution**:
   - Try resizing plugin window
   - Force repaint

### Slice Grid Not Showing Waveforms

**Symptom**: Slices are blue but no waveform visible

**Solutions**:

1. **Check Slice Length**:
   - Very short slices might not render well
   - Increase Min Length parameter

2. **Graphics Performance**:
   - Lower resolution display?
   - Try on different monitor
   - Check GPU acceleration in DAW

3. **Rebuild UI** (dev):
   ```cpp
   // Increase stroke thickness in SliceGridDisplay::drawSlice()
   g.strokePath(path, juce::PathStrokeType(2.0f)); // Instead of 1.0f
   ```

### Plugin Window Blank/Black

**Symptom**: Plugin opens but shows nothing

**Solutions**:

1. **Graphics Driver**:
   - Update GPU drivers
   - Try software rendering mode

2. **OpenGL Issues**:
   - JUCE uses OpenGL by default
   - Disable if problematic:
   ```cpp
   // In PluginEditor constructor
   // Comment out any OpenGL context creation
   ```

3. **Window Scaling**:
   - Check OS display scaling
   - 200%+ scaling can cause issues
   - Try 100% scaling

---

## Performance Issues

### High CPU Usage

**Symptom**: Plugin uses >20% CPU

**Solutions**:

1. **Optimize Settings**:
   - Reduce visualization update rate (15 FPS)
   - Increase audio buffer size
   - Limit simultaneous playing slices

2. **Build Type**:
   - Ensure using Release build, not Debug
   ```bash
   cmake --build . --config Release
   ```

3. **Profiling** (advanced):
   ```cpp
   // Add to process block
   juce::ScopedNoDenormals noDenormals; // Already there
   
   // Profile specific sections
   auto start = Time::getHighResolutionTicks();
   // ... code ...
   auto elapsed = Time::getHighResolutionTicksPerSecond() 
                  / (Time::getHighResolutionTicks() - start);
   DBG("Process time: " << elapsed << "ms");
   ```

### Memory Leak

**Symptom**: Memory usage grows over time

**Solutions**:

1. **Check Builds**:
   - Use Release build (better memory management)
   - Debug builds can show false positives

2. **Monitor**:
   ```bash
   # macOS
   leaks AudioSlicer
   
   # Linux
   valgrind --leak-check=full ./AudioSlicer
   
   # Windows
   # Use Visual Studio memory profiler
   ```

3. **Common Causes**:
   - Forgetting to stop timers
   - Not clearing buffers
   - Circular references

### Latency Issues

**Symptom**: Noticeable delay between trigger and sound

**Solutions**:

1. **Buffer Size**:
   - Reduce to 128 or 256 samples
   - Check PDC (plugin delay compensation)

2. **Processing Latency**:
   - Plugin should report 0 latency
   - Check with:
   ```cpp
   double getTailLengthSeconds() const override { return 0.0; }
   ```

3. **Monitor Latency**:
   - Use DAW's latency compensation
   - Enable "low latency mode" if available

---

## Data Issues

### Slices Sound Wrong

**Symptom**: Triggered slice plays different audio than expected

**Solutions**:

1. **Timing Issue**:
   - Transient detected in wrong place
   - Adjust sensitivity/threshold
   - Increase min length

2. **Buffer Corruption**:
   - Rare, but possible
   - Try clearing all slices (right-click in UI)
   - Reload plugin

3. **Sample Rate Mismatch**:
   - Check project sample rate
   - Verify audio interface sample rate
   - All should match (48kHz recommended)

### State Not Saving

**Symptom**: Parameters reset when reopening project

**Solutions**:

1. **Check Serialization**:
   - Verify `getStateInformation()` is called
   - Add debug logging:
   ```cpp
   void getStateInformation(juce::MemoryBlock& destData) override
   {
       DBG("Saving state...");
       // ... existing code ...
   }
   ```

2. **DAW Project Settings**:
   - Ensure "Save plugin state" is enabled
   - Some DAWs require explicit save

3. **File Permissions**:
   - Check write permissions on project folder
   - Try different save location

### Slices Not Persisting

**Symptom**: Slices disappear when reopening

**Current Behavior**: Slices are NOT saved (by design)
- Only parameters are saved
- Slices are meant for live use

**To Save Slices** (requires implementation):
```cpp
// Add to getStateInformation()
for (int i = 0; i < maxSlices; ++i)
{
    if (slices[i].isActive)
    {
        // Base64 encode audio data
        // Add to state tree
    }
}
```

---

## Integration Issues

### Multiple Instances

**Symptom**: Using 2+ instances causes problems

**Solutions**:

1. **Memory**:
   - Each instance uses ~20MB
   - Check available RAM
   - Close other applications

2. **CPU**:
   - Multiple instances = multiple detection threads
   - Freeze non-essential tracks
   - Use bounced audio when possible

3. **MIDI Conflicts**:
   - Each instance responds to same notes
   - Route MIDI selectively per instance

### Automation Not Working

**Symptom**: DAW automation doesn't affect parameters

**Solutions**:

1. **Check Parameter Setup**:
   - Verify parameters are automatable
   - Look for automation lane in DAW
   - Check parameter is visible in automation list

2. **Write Mode**:
   - Enable write/latch mode in DAW
   - Record automation pass
   - Check automation is actually written

3. **Parameter Smoothing**:
   - Changes might be too fast
   - Add smoothing if needed

---

## Emergency Procedures

### Plugin Crashes DAW

**Immediate Action**:
1. Remove plugin from track
2. Restart DAW
3. Try standalone version
4. Check crash logs

**Crash Log Locations**:
```
macOS: ~/Library/Logs/DiagnosticReports/
Windows: Event Viewer → Windows Logs → Application
Linux: /var/log/syslog or dmesg
```

### Can't Remove Plugin

**Solution**:
```bash
# macOS
rm -rf ~/Library/Audio/Plug-Ins/Components/AudioSlicer.component
rm -rf ~/Library/Audio/Plug-Ins/VST3/AudioSlicer.vst3

# Windows
# Delete from: C:\Program Files\Common Files\VST3\

# Linux
rm -rf ~/.vst3/AudioSlicer.vst3
```

Then rescan plugins in DAW.

### Complete Reset

**Nuclear Option**:
```bash
# Remove all plugin files
rm -rf ~/Library/Audio/Plug-Ins/*/AudioSlicer.*

# Clear DAW plugin cache
# (location varies by DAW - check manual)

# Remove preferences (loses all settings)
rm -rf ~/Library/Preferences/com.yourcompany.AudioSlicer.*

# Rebuild from scratch
cd AudioSlicerVST
rm -rf build
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

---

## Getting Help

### Before Reporting Issues

1. ✓ Check this guide
2. ✓ Try standalone version
3. ✓ Test in different DAW
4. ✓ Check JUCE version
5. ✓ Verify build configuration

### Information to Include

```
- OS: macOS 14.1 / Windows 11 / Ubuntu 22.04
- DAW: Ableton Live 11.3 / Logic Pro X 10.8
- Plugin Format: VST3 / AU
- JUCE Version: 7.0.9
- Build Type: Release / Debug
- Sample Rate: 48000 Hz
- Buffer Size: 512 samples
- Error Message: [exact error text]
- Steps to Reproduce: [detailed steps]
- Crash Log: [if applicable]
```

### Support Channels

- GitHub Issues: [repository URL]
- JUCE Forum: https://forum.juce.com/
- Audio Developer Discord: [invite link]

---

**Still stuck? Post your issue with the above information!**
