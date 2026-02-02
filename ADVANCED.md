# Advanced Features & Customization Guide

## Understanding the Transient Detector

### Algorithm Explanation

The transient detection algorithm uses an energy-based approach:

```
Energy(t) = √(Σ(sample²) / N)
Ratio = Energy(t) / Energy(t-1)

Transient detected when:
1. Energy(t) > Threshold (in linear scale)
2. Ratio > RequiredRatio (scaled by sensitivity)
3. Cooldown period has elapsed
```

### Parameter Interactions

**Sensitivity** (0.0 - 1.0):
- Controls the energy ratio required for detection
- Formula: `RequiredRatio = 1.5 + (1.0 - sensitivity) * 3.0`
- At 1.0: Ratio needs to be > 1.5 (very sensitive)
- At 0.0: Ratio needs to be > 4.5 (very insensitive)

**Threshold** (-60 dB to 0 dB):
- Converted to linear: `Linear = 10^(dB/20)`
- Acts as a gate - audio below this is ignored
- Prevents noise from triggering slices

**Cooldown Period**:
- Hardcoded to 100ms (4410 samples at 44.1kHz)
- Prevents multiple detections of the same transient
- Scales with sample rate

### Fine-Tuning Strategies

#### For Very Fast Material (32nd notes at 140 BPM)
```cpp
// Modify in TransientDetector::prepare()
cooldownSamples = static_cast<int>(sampleRate * 0.05); // 50ms
```

#### For Slow, Evolving Material
```cpp
// Increase cooldown to avoid false triggers
cooldownSamples = static_cast<int>(sampleRate * 0.2); // 200ms
```

## Code Customization

### Changing Slice Count

**File**: `PluginProcessor.h`

```cpp
// Change from 16 to 32 slices
static constexpr int maxSlices = 32;
```

Then update UI grid in `PluginEditor.cpp`:

```cpp
// Change from 4x4 to 8x4 or 4x8
int sliceWidth = bounds.getWidth() / 8;  // 8 columns
int sliceHeight = bounds.getHeight() / 4; // 4 rows

// Update loop logic
int row = i / 8;
int col = i % 8;
```

### Adding Slice Export

Add to `PluginProcessor.h`:

```cpp
public:
    void exportSlice(int sliceIndex, const juce::File& destination);
```

Implementation in `PluginProcessor.cpp`:

```cpp
void AudioSlicerAudioProcessor::exportSlice(int sliceIndex, const juce::File& destination)
{
    if (sliceIndex >= 0 && sliceIndex < maxSlices)
    {
        auto& slice = slices[sliceIndex];
        if (slice.isActive)
        {
            juce::WavAudioFormat format;
            std::unique_ptr<juce::AudioFormatWriter> writer;
            
            writer.reset(format.createWriterFor(
                new juce::FileOutputStream(destination),
                currentSampleRate,
                slice.buffer.getNumChannels(),
                24, // bit depth
                {}, // metadata
                0   // quality
            ));
            
            if (writer != nullptr)
            {
                writer->writeFromAudioSampleBuffer(slice.buffer, 0, slice.lengthSamples);
            }
        }
    }
}
```

### Adding Loop Playback

Modify `AudioSlice` in `PluginProcessor.h`:

```cpp
struct AudioSlice
{
    // ... existing members ...
    bool loopEnabled = false;
    
    // ... existing methods ...
};
```

Update playback in `processBlock()`:

```cpp
// In the slice playback section
if (slice.loopEnabled && slice.playbackPosition >= slice.lengthSamples)
{
    slice.playbackPosition = 0; // Loop back
}
else if (slice.playbackPosition >= slice.lengthSamples)
{
    slice.isPlaying = false;
    slice.playbackPosition = 0;
}
```

### Adding Pitch Shifting

Use JUCE's `juce::dsp::Oversampling` and resampling:

```cpp
// In PluginProcessor.h
#include <juce/juce_dsp/juce_dsp.h>

struct AudioSlice
{
    // ... existing members ...
    float pitchShift = 1.0f; // 1.0 = original, 2.0 = octave up, 0.5 = octave down
};
```

Implementation requires resampling during playback - more complex.

### Adding Reverse Playback

Simple reverse flag:

```cpp
struct AudioSlice
{
    // ... existing members ...
    bool reversed = false;
};
```

Update playback logic:

```cpp
int sampleIndex = slice.reversed 
    ? (slice.lengthSamples - slice.playbackPosition - 1)
    : slice.playbackPosition;
    
outputData[i] += sliceData[sampleIndex] * slice.gain;
```

## Performance Optimization

### Reducing Memory Usage

Current setup: ~20MB per instance
- 16 slices × 10 seconds × 48kHz × 2 channels × 4 bytes

To reduce:

```cpp
// In prepareToPlay(), reduce max slice length
int maxSamples = static_cast<int>(sampleRate * 5.0); // 5 seconds instead of 10
```

### CPU Optimization

**1. Reduce Visualization Update Rate**

In `PluginEditor.cpp`:

```cpp
// Change from 30 FPS to 15 FPS
startTimerHz(15);
```

**2. Optimize Waveform Drawing**

```cpp
// Draw every Nth sample instead of all
int stride = 4;
for (int i = 0; i < displayWidth; i += stride)
{
    // ... drawing code ...
}
```

**3. Disable Unused Features**

Comment out visualization updates in `processBlock()` if not needed:

```cpp
// Comment out this section if UI not visible
// {
//     juce::ScopedLock lock(visualizationLock);
//     // ... visualization buffer update ...
// }
```

## Advanced MIDI Features

### Velocity-Based Slice Selection

Map MIDI velocity to different behaviors:

```cpp
void AudioSlicerAudioProcessor::triggerSlice(int sliceIndex, float velocity)
{
    if (sliceIndex >= 0 && sliceIndex < maxSlices)
    {
        auto& slice = slices[sliceIndex];
        
        if (slice.isActive)
        {
            slice.playbackPosition = 0;
            slice.isPlaying = true;
            
            // Map velocity to playback speed
            slice.playbackSpeed = 0.5f + velocity; // 0.5x to 1.5x speed
            
            // Or map to filter cutoff, effects, etc.
        }
    }
}
```

### MIDI CC Control

Add CC parameter mapping:

```cpp
// In processMidiMessages()
if (message.isController())
{
    int ccNumber = message.getControllerNumber();
    float ccValue = message.getControllerValue() / 127.0f;
    
    switch (ccNumber)
    {
        case 1: // Modulation wheel
            *sensitivityParam = ccValue;
            break;
        case 71: // Resonance
            *thresholdParam = juce::jmap(ccValue, -60.0f, 0.0f);
            break;
        // Add more CC mappings
    }
}
```

### MIDI Learn

Implement parameter learn mode (requires UI additions):

```cpp
class AudioSlicerAudioProcessor
{
    // ... existing code ...
    
private:
    std::map<int, juce::AudioParameterFloat*> ccMappings;
    bool midiLearnMode = false;
    juce::AudioParameterFloat* learnTarget = nullptr;
    
public:
    void enableMidiLearn(juce::AudioParameterFloat* parameter)
    {
        midiLearnMode = true;
        learnTarget = parameter;
    }
    
    void processMidiForLearn(int ccNumber)
    {
        if (midiLearnMode && learnTarget != nullptr)
        {
            ccMappings[ccNumber] = learnTarget;
            midiLearnMode = false;
            learnTarget = nullptr;
        }
    }
};
```

## Adding Effects Per Slice

### Basic Filter

Add to `AudioSlice`:

```cpp
struct AudioSlice
{
    // ... existing members ...
    juce::dsp::IIR::Filter<float> filter;
    float filterCutoff = 20000.0f;
    
    void prepareFilter(double sampleRate)
    {
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, filterCutoff);
        filter.coefficients = coeffs;
    }
};
```

Apply during playback:

```cpp
float sample = sliceData[slice.playbackPosition + i];
sample = slice.filter.processSample(sample);
outputData[i] += sample * slice.gain;
```

### Envelope

Add ADSR envelope per slice:

```cpp
struct AudioSlice
{
    // ... existing members ...
    juce::ADSR envelope;
    juce::ADSR::Parameters envelopeParams;
    
    void prepareEnvelope(double sampleRate)
    {
        envelope.setSampleRate(sampleRate);
        envelopeParams.attack = 0.01f;
        envelopeParams.decay = 0.1f;
        envelopeParams.sustain = 0.7f;
        envelopeParams.release = 0.2f;
        envelope.setParameters(envelopeParams);
    }
    
    void startEnvelope()
    {
        envelope.noteOn();
    }
};
```

Apply envelope:

```cpp
float envValue = slice.envelope.getNextSample();
outputData[i] += sliceData[slice.playbackPosition + i] * slice.gain * envValue;
```

## Preset Management

### Save Preset

```cpp
void AudioSlicerAudioProcessor::savePreset(const juce::File& file)
{
    auto state = juce::ValueTree("AudioSlicerPreset");
    
    // Save parameters
    state.setProperty("sensitivity", (double)*sensitivityParam, nullptr);
    state.setProperty("threshold", (double)*thresholdParam, nullptr);
    state.setProperty("minLength", (double)*minSliceLengthParam, nullptr);
    state.setProperty("maxLength", (double)*maxSliceLengthParam, nullptr);
    
    // Optionally save audio slices (base64 encode)
    for (int i = 0; i < maxSlices; ++i)
    {
        if (slices[i].isActive)
        {
            // Encode slice audio data...
        }
    }
    
    auto xml = state.createXml();
    xml->writeTo(file);
}
```

### Load Preset

```cpp
void AudioSlicerAudioProcessor::loadPreset(const juce::File& file)
{
    if (auto xml = juce::XmlDocument::parse(file))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        
        if (state.isValid())
        {
            *sensitivityParam = state.getProperty("sensitivity", 0.5f);
            *thresholdParam = state.getProperty("threshold", -20.0f);
            // ... load other parameters ...
        }
    }
}
```

## Building Additional UI Features

### Add Slice Clear Button

In `PluginEditor.h`:

```cpp
class SliceGridDisplay : public juce::Component
{
    // ... existing code ...
private:
    void mouseUp(const juce::MouseEvent& event) override;
};
```

In `PluginEditor.cpp`:

```cpp
void SliceGridDisplay::mouseUp(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        // Calculate slice index...
        // Clear the slice
        auto& slice = const_cast<AudioSlice&>(audioProcessor.getSlice(sliceIndex));
        slice.clear();
    }
}
```

### Add Waveform Zoom

```cpp
class WaveformDisplay
{
    float zoomLevel = 1.0f;
    
    void mouseWheelMove(const juce::MouseEvent& e, 
                       const juce::MouseWheelDetails& wheel) override
    {
        zoomLevel += wheel.deltaY * 0.5f;
        zoomLevel = juce::jlimit(0.1f, 10.0f, zoomLevel);
        repaint();
    }
};
```

## Testing & Debugging

### Add Debug Logging

```cpp
// In PluginProcessor.cpp
#define DEBUG_LOGGING 1

#if DEBUG_LOGGING
    #define LOG(x) DBG(x)
#else
    #define LOG(x)
#endif

// Usage
LOG("Slice " << sliceIndex << " triggered with velocity " << velocity);
LOG("Transient detected at sample " << recordingPosition);
```

### Performance Profiling

```cpp
void AudioSlicerAudioProcessor::processBlock(...)
{
    #if JUCE_DEBUG
    auto startTime = juce::Time::getMillisecondCounterHiRes();
    #endif
    
    // ... processing ...
    
    #if JUCE_DEBUG
    auto elapsedTime = juce::Time::getMillisecondCounterHiRes() - startTime;
    if (elapsedTime > 1.0) // More than 1ms
        LOG("ProcessBlock took " << elapsedTime << "ms");
    #endif
}
```

## Integration Examples

### OSC Control

Add OSC support (requires juce_osc module):

```cpp
#include <juce_osc/juce_osc.h>

class AudioSlicerAudioProcessor : public juce::OSCReceiver::Listener<>
{
    void oscMessageReceived(const juce::OSCMessage& message) override
    {
        if (message.getAddressPattern() == "/slice/trigger")
        {
            int sliceIndex = message[0].getInt32();
            float velocity = message[1].getFloat32();
            triggerSlice(sliceIndex, velocity);
        }
    }
};
```

### Network Sync

Sync multiple instances over network (advanced):
- Use UDP broadcast for slice triggering
- Share slice data between instances
- Synchronized recording start/stop

---

## Further Reading

- [JUCE Documentation](https://docs.juce.com/)
- [JUCE Forum](https://forum.juce.com/)
- [The Audio Programmer (YouTube)](https://www.youtube.com/c/TheAudioProgrammer)
- [Creating Audio Plugins in JUCE (Book)](https://www.amazon.com/Creating-Audio-Plugins-JUCE-Frameworks/dp/1495173062)

---

**Happy Coding! 🎵💻**
