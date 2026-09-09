#pragma once

#include <JuceHeader.h>
#include <vector>
#include <atomic>

//==============================================================================
/**
 * Audio slice data structure
 */
struct AudioSlice
{
    juce::AudioBuffer<float> buffer;
    int startSample = 0;
    int lengthSamples = 0;
    bool isActive = false;
    int playbackPosition = 0;
    bool isPlaying = false;
    float gain = 1.0f;
    
    void clear()
    {
        buffer.clear();
        startSample = 0;
        lengthSamples = 0;
        isActive = false;
        playbackPosition = 0;
        isPlaying = false;
    }
};

//==============================================================================
/**
 * Transient detector for automatic slice point detection
 */
class TransientDetector
{
public:
    TransientDetector();
    
    void prepare(double sampleRate);
    bool detectTransient(const float* audioData, int numSamples);
    
    void setSensitivity(float sensitivity); // 0.0 to 1.0
    void setThreshold(float threshold);     // in dB
    
private:
    float calculateEnergy(const float* data, int numSamples);
    
    double sampleRate = 44100.0;
    float threshold = -20.0f;
    float sensitivity = 0.5f;
    float previousEnergy = 0.0f;
    int cooldownSamples = 0;
    int cooldownCounter = 0;
};

//==============================================================================
/**
 * Main audio processor for the slicer plugin
 */
class AudioSlicerAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioSlicerAudioProcessor();
    ~AudioSlicerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Audio slice management
    static constexpr int maxSlices = 16;

    // Ceiling on how much audio one slice can hold. This single constant governs
    // BOTH the slice/recording buffer allocation and the upper bound of the
    // "Max Slice Length" parameter, so the two can never disagree: a slice is
    // copied out of the recording buffer using the parameter's value, so a
    // parameter that outran the allocation would read past the end of it.
    //
    // Cost is maxSlices * seconds * 2ch * 4B * sampleRate (plus one more
    // slice-sized recording buffer), so 10s at 48kHz is ~66MB -- fine for a
    // desktop VST3, but far too much for an AUv3 extension, where the host may
    // keep many plugins resident at once. iOS therefore gets a tighter ceiling
    // (~13MB at 48kHz).
#if JUCE_IOS
    static constexpr float maxSliceSeconds = 2.0f;
#else
    static constexpr float maxSliceSeconds = 10.0f;
#endif
    
    const AudioSlice& getSlice(int index) const { return slices[index]; }
    int getCurrentSliceIndex() const { return currentSliceIndex.load(); }
    
    // Get copy of incoming audio buffer for visualization
    void getIncomingAudioBuffer(juce::AudioBuffer<float>& destBuffer);
    
    // Parameters
    juce::AudioParameterFloat* sensitivityParam;
    juce::AudioParameterFloat* thresholdParam;
    juce::AudioParameterFloat* minSliceLengthParam;
    juce::AudioParameterFloat* maxSliceLengthParam;

    // Trigger a slice for playback (public for UI access)
    void triggerSlice(int sliceIndex, float velocity);

private:
    //==============================================================================
    void processIncomingAudio(const juce::AudioBuffer<float>& buffer);
    void processMidiMessages(juce::MidiBuffer& midiMessages);
    void recordAudioToSlice(const juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    //==============================================================================
    std::array<AudioSlice, maxSlices> slices;
    std::atomic<int> currentSliceIndex{0};
    
    TransientDetector transientDetector;
    
    // Recording buffer for incoming audio
    juce::AudioBuffer<float> recordingBuffer;
    int recordingPosition = 0;
    bool isRecording = false;
    int currentSliceSamples = 0;
    
    // Visualization buffer (circular buffer for UI)
    juce::AudioBuffer<float> visualizationBuffer;
    int visualizationWritePos = 0;
    juce::CriticalSection visualizationLock;
    
    // Audio parameters
    double currentSampleRate = 44100.0;
    int samplesPerBlock = 512;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSlicerAudioProcessor)
};
