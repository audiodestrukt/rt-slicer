#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TransientDetector::TransientDetector()
{
    cooldownSamples = 4410; // 100ms at 44.1kHz
}

void TransientDetector::prepare(double sr)
{
    sampleRate = sr;
    cooldownSamples = static_cast<int>(sampleRate * 0.1); // 100ms cooldown
    previousEnergy = 0.0f;
    cooldownCounter = 0;
}

bool TransientDetector::detectTransient(const float* audioData, int numSamples)
{
    if (cooldownCounter > 0)
    {
        cooldownCounter--;
        return false;
    }
    
    float currentEnergy = calculateEnergy(audioData, numSamples);
    
    // Convert threshold from dB to linear
    float thresholdLinear = juce::Decibels::decibelsToGain(threshold);
    
    // Calculate energy increase ratio
    float energyRatio = previousEnergy > 0.0001f ? currentEnergy / previousEnergy : 0.0f;
    
    // Adjust sensitivity (higher sensitivity = lower ratio needed)
    float requiredRatio = 1.5f + (1.0f - sensitivity) * 3.0f;
    
    bool transientDetected = (currentEnergy > thresholdLinear) && (energyRatio > requiredRatio);
    
    if (transientDetected)
    {
        cooldownCounter = cooldownSamples;
    }
    
    previousEnergy = currentEnergy;
    return transientDetected;
}

float TransientDetector::calculateEnergy(const float* data, int numSamples)
{
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / numSamples);
}

void TransientDetector::setSensitivity(float sens)
{
    sensitivity = juce::jlimit(0.0f, 1.0f, sens);
}

void TransientDetector::setThreshold(float thresh)
{
    threshold = thresh;
}

//==============================================================================
AudioSlicerAudioProcessor::AudioSlicerAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // Add parameters
    addParameter(sensitivityParam = new juce::AudioParameterFloat(
        "sensitivity", "Sensitivity", 0.0f, 1.0f, 0.5f));
    
    addParameter(thresholdParam = new juce::AudioParameterFloat(
        "threshold", "Threshold", -60.0f, 0.0f, -20.0f));
    
    addParameter(minSliceLengthParam = new juce::AudioParameterFloat(
        "minLength", "Min Slice Length", 0.01f, 2.0f, 0.1f));
    
    addParameter(maxSliceLengthParam = new juce::AudioParameterFloat(
        "maxLength", "Max Slice Length", 0.1f, 10.0f, 4.0f));
    
    // Initialize slices
    for (auto& slice : slices)
    {
        slice.clear();
    }
}

AudioSlicerAudioProcessor::~AudioSlicerAudioProcessor()
{
}

//==============================================================================
const juce::String AudioSlicerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioSlicerAudioProcessor::acceptsMidi() const
{
    return true;
}

bool AudioSlicerAudioProcessor::producesMidi() const
{
    return false;
}

bool AudioSlicerAudioProcessor::isMidiEffect() const
{
    return false;
}

double AudioSlicerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioSlicerAudioProcessor::getNumPrograms()
{
    return 1;
}

int AudioSlicerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioSlicerAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String AudioSlicerAudioProcessor::getProgramName(int index)
{
    return {};
}

void AudioSlicerAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void AudioSlicerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;
    
    transientDetector.prepare(sampleRate);
    
    // Prepare recording buffer (10 seconds max per slice)
    int maxSamples = static_cast<int>(sampleRate * 10.0);
    recordingBuffer.setSize(2, maxSamples);
    recordingBuffer.clear();
    
    // Prepare visualization buffer (2 seconds)
    int vizSamples = static_cast<int>(sampleRate * 2.0);
    visualizationBuffer.setSize(2, vizSamples);
    visualizationBuffer.clear();
    
    // Prepare all slice buffers
    for (auto& slice : slices)
    {
        slice.buffer.setSize(2, maxSamples);
        slice.buffer.clear();
        slice.clear();
    }
    
    recordingPosition = 0;
    isRecording = true;
}

void AudioSlicerAudioProcessor::releaseResources()
{
}

bool AudioSlicerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    return true;
}

void AudioSlicerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Clear any extra output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // Update detector parameters
    transientDetector.setSensitivity(*sensitivityParam);
    transientDetector.setThreshold(*thresholdParam);
    
    // Process incoming audio for slicing
    processIncomingAudio(buffer);
    
    // Create output buffer for mixed playback
    juce::AudioBuffer<float> outputBuffer(totalNumOutputChannels, buffer.getNumSamples());
    outputBuffer.clear();
    
    // Process MIDI messages to trigger slices
    processMidiMessages(midiMessages);
    
    // Mix playing slices into output
    for (auto& slice : slices)
    {
        if (slice.isPlaying && slice.isActive)
        {
            int samplesToPlay = juce::jmin(buffer.getNumSamples(), 
                                           slice.lengthSamples - slice.playbackPosition);
            
            if (samplesToPlay > 0)
            {
                for (int channel = 0; channel < totalNumOutputChannels; ++channel)
                {
                    auto* outputData = outputBuffer.getWritePointer(channel);
                    auto* sliceData = slice.buffer.getReadPointer(channel % slice.buffer.getNumChannels());
                    
                    for (int i = 0; i < samplesToPlay; ++i)
                    {
                        outputData[i] += sliceData[slice.playbackPosition + i] * slice.gain;
                    }
                }
                
                slice.playbackPosition += samplesToPlay;
                
                if (slice.playbackPosition >= slice.lengthSamples)
                {
                    slice.isPlaying = false;
                    slice.playbackPosition = 0;
                }
            }
        }
    }
    
    // Copy mixed output to buffer
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        buffer.copyFrom(channel, 0, outputBuffer, channel, 0, buffer.getNumSamples());
    }
    
    // Update visualization buffer
    {
        juce::ScopedLock lock(visualizationLock);
        int vizBufferSize = visualizationBuffer.getNumSamples();
        
        for (int channel = 0; channel < juce::jmin(totalNumInputChannels, visualizationBuffer.getNumChannels()); ++channel)
        {
            auto* vizData = visualizationBuffer.getWritePointer(channel);
            auto* inputData = buffer.getReadPointer(channel);
            
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                vizData[visualizationWritePos] = inputData[i];
                visualizationWritePos = (visualizationWritePos + 1) % vizBufferSize;
            }
        }
    }
}

void AudioSlicerAudioProcessor::processIncomingAudio(const juce::AudioBuffer<float>& buffer)
{
    if (!isRecording)
        return;
    
    int numSamples = buffer.getNumSamples();
    auto* leftChannel = buffer.getReadPointer(0);
    
    // Check for transients in blocks
    for (int i = 0; i < numSamples; i += 64)
    {
        int blockSize = juce::jmin(64, numSamples - i);
        
        if (transientDetector.detectTransient(leftChannel + i, blockSize))
        {
            // Transient detected - finalize current slice if it meets criteria
            float minLength = *minSliceLengthParam;
            float currentLength = currentSliceSamples / currentSampleRate;
            
            if (currentLength >= minLength && currentSliceSamples > 0)
            {
                // Store the slice
                recordAudioToSlice(recordingBuffer, 0, currentSliceSamples);
                
                // Move to next slice
                currentSliceIndex = (currentSliceIndex.load() + 1) % maxSlices;
                
                // Reset recording
                recordingPosition = 0;
                currentSliceSamples = 0;
            }
        }
    }
    
    // Continue recording
    float maxLength = *maxSliceLengthParam;
    int maxSamples = static_cast<int>(currentSampleRate * maxLength);
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* inputData = buffer.getReadPointer(channel);
        auto* recordData = recordingBuffer.getWritePointer(channel);
        
        for (int i = 0; i < numSamples; ++i)
        {
            if (recordingPosition < recordingBuffer.getNumSamples())
            {
                recordData[recordingPosition] = inputData[i];
            }
            
            if (channel == 0) // Only increment once per sample
            {
                recordingPosition++;
                currentSliceSamples++;
                
                // Check if we've hit max length
                if (currentSliceSamples >= maxSamples)
                {
                    recordAudioToSlice(recordingBuffer, 0, currentSliceSamples);
                    currentSliceIndex = (currentSliceIndex.load() + 1) % maxSlices;
                    recordingPosition = 0;
                    currentSliceSamples = 0;
                }
            }
        }
    }
}

void AudioSlicerAudioProcessor::recordAudioToSlice(const juce::AudioBuffer<float>& buffer, 
                                                    int startSample, int numSamples)
{
    int sliceIdx = currentSliceIndex.load();
    auto& slice = slices[sliceIdx];
    
    slice.lengthSamples = numSamples;
    slice.startSample = startSample;
    slice.playbackPosition = 0;
    slice.isActive = true;
    slice.isPlaying = false;
    
    // Copy audio data
    for (int channel = 0; channel < juce::jmin(buffer.getNumChannels(), slice.buffer.getNumChannels()); ++channel)
    {
        slice.buffer.copyFrom(channel, 0, buffer, channel, startSample, numSamples);
    }
}

void AudioSlicerAudioProcessor::processMidiMessages(juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int note = message.getNoteNumber();
            float velocity = message.getFloatVelocity();
            
            // Map MIDI notes to slices (MIDI note 60 (C3) = slice 0)
            int sliceIndex = note - 60;
            
            if (sliceIndex >= 0 && sliceIndex < maxSlices)
            {
                triggerSlice(sliceIndex, velocity);
            }
        }
    }
}

void AudioSlicerAudioProcessor::triggerSlice(int sliceIndex, float velocity)
{
    if (sliceIndex >= 0 && sliceIndex < maxSlices)
    {
        auto& slice = slices[sliceIndex];
        
        if (slice.isActive)
        {
            slice.playbackPosition = 0;
            slice.isPlaying = true;
            slice.gain = velocity;
        }
    }
}

void AudioSlicerAudioProcessor::getIncomingAudioBuffer(juce::AudioBuffer<float>& destBuffer)
{
    juce::ScopedLock lock(visualizationLock);
    
    int numSamples = juce::jmin(destBuffer.getNumSamples(), visualizationBuffer.getNumSamples());
    int numChannels = juce::jmin(destBuffer.getNumChannels(), visualizationBuffer.getNumChannels());
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        destBuffer.copyFrom(channel, 0, visualizationBuffer, channel, 0, numSamples);
    }
}

//==============================================================================
bool AudioSlicerAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioSlicerAudioProcessor::createEditor()
{
    return new AudioSlicerAudioProcessorEditor(*this);
}

//==============================================================================
void AudioSlicerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = juce::ValueTree("AudioSlicerState");
    
    state.setProperty("sensitivity", (double)*sensitivityParam, nullptr);
    state.setProperty("threshold", (double)*thresholdParam, nullptr);
    state.setProperty("minLength", (double)*minSliceLengthParam, nullptr);
    state.setProperty("maxLength", (double)*maxSliceLengthParam, nullptr);
    
    juce::MemoryOutputStream mos(destData, true);
    state.writeToStream(mos);
}

void AudioSlicerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    
    if (tree.isValid())
    {
        *sensitivityParam = tree.getProperty("sensitivity", 0.5f);
        *thresholdParam = tree.getProperty("threshold", -20.0f);
        *minSliceLengthParam = tree.getProperty("minLength", 0.1f);
        *maxSliceLengthParam = tree.getProperty("maxLength", 4.0f);
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioSlicerAudioProcessor();
}
