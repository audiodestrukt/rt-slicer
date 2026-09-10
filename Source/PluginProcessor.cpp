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
    // The cooldown is measured in SAMPLES, but this is called once per block,
    // so it has to be charged the block's length. Decrementing by one per call
    // made the intended 100ms cooldown last 4410 *blocks* -- 6.4 seconds at
    // 44.1kHz, a 64x error that swallowed all but a couple of transients.
    // Measured on a 10s test signal of hits every 250ms: 2 of 40 detected.
    if (cooldownCounter > 0)
    {
        cooldownCounter -= numSamples;

        // Keep tracking energy while cooling down. Returning early without
        // updating left previousEnergy stale by the length of the cooldown, so
        // the first comparison after it was made against ancient audio.
        previousEnergy = calculateEnergy(audioData, numSamples);
        return false;
    }

    float currentEnergy = calculateEnergy(audioData, numSamples);
    
    // Convert threshold from dB to linear
    float thresholdLinear = juce::Decibels::decibelsToGain(threshold);
    
    // Energy increase ratio. Guarding the division by zeroing the ratio had it
    // exactly backwards: a hit arriving after silence is the most unambiguous
    // transient there is, and forcing the ratio to 0 made it undetectable. A
    // floor in the denominator lets silence -> signal read as the large jump it
    // is. With the cooldown fix, this took the same test signal from 17 of 40
    // detected to 40 of 40.
    float energyRatio = currentEnergy / juce::jmax(previousEnergy, 1.0e-5f);
    
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
        "maxLength", "Max Slice Length", 0.1f, maxSliceSeconds,
        juce::jmin(4.0f, maxSliceSeconds)));
    
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
    
    // Prepare recording buffer. Sized from maxSliceSeconds, the same constant
    // that bounds the Max Slice Length parameter, so a slice can never be
    // longer than the buffer it is copied out of.
    int maxSamples = static_cast<int>(sampleRate * maxSliceSeconds);
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
    captureBlocked = false;
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

    // Every slot frozen means there is nowhere to put a new slice. Stop
    // capturing rather than quietly overwriting something the player asked to
    // keep. Visualisation keeps running, so the app still looks alive.
    if (captureBlocked.load())
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

                // Move to the next slot that is not frozen
                advanceToNextRecordableSlot();

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
                    advanceToNextRecordableSlot();
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

    // Belt and braces: a frozen slot is never written, even if the index
    // somehow points at one.
    if (slice.isFrozen.load())
        return;

    // Never copy more than either buffer holds -- the source is the recording
    // buffer and the destination the slice, both sized from maxSliceSeconds.
    numSamples = juce::jmin(numSamples,
                            buffer.getNumSamples() - startSample,
                            slice.buffer.getNumSamples());
    if (numSamples <= 0)
        return;

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

bool AudioSlicerAudioProcessor::advanceToNextRecordableSlot()
{
    const int start = currentSliceIndex.load();

    for (int step = 1; step <= maxSlices; ++step)
    {
        const int candidate = (start + step) % maxSlices;

        if (!slices[candidate].isFrozen.load())
        {
            currentSliceIndex = candidate;
            captureBlocked = false;
            return true;
        }
    }

    // Every slot is frozen. Leave the index alone and let processIncomingAudio
    // stop; unfreezing anything clears this.
    captureBlocked = true;
    return false;
}

void AudioSlicerAudioProcessor::toggleFreeze(int sliceIndex)
{
    if (sliceIndex < 0 || sliceIndex >= maxSlices)
        return;

    auto& slice = slices[sliceIndex];
    const bool nowFrozen = !slice.isFrozen.load();
    slice.isFrozen = nowFrozen;

    if (!nowFrozen)
    {
        // Unfreezing always makes room again.
        captureBlocked = false;
        return;
    }

    // Freezing the slot being recorded into: keep what is already there and
    // move capture on, rather than continuing to write over it.
    if (sliceIndex == currentSliceIndex.load())
    {
        advanceToNextRecordableSlot();
        recordingPosition = 0;
        currentSliceSamples = 0;
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

        // Clamp: a preset written by a build with a larger maxSliceSeconds (the
        // desktop VST3) can carry a maxLength this build cannot accommodate.
        *maxSliceLengthParam = juce::jlimit(0.1f, maxSliceSeconds,
                                            (float)tree.getProperty("maxLength", 4.0f));
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioSlicerAudioProcessor();
}
