#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * Waveform display component for incoming audio
 */
class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay(AudioSlicerAudioProcessor& p);
    ~WaveformDisplay() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    void timerCallback() override;
    
    AudioSlicerAudioProcessor& audioProcessor;
    juce::AudioBuffer<float> displayBuffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};

//==============================================================================
/**
 * Slice display component showing all 16 slices
 */
class SliceGridDisplay : public juce::Component, private juce::Timer
{
public:
    SliceGridDisplay(AudioSlicerAudioProcessor& p);
    ~SliceGridDisplay() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    
private:
    void timerCallback() override;
    void drawSlice(juce::Graphics& g, const AudioSlice& slice, juce::Rectangle<int> bounds, int sliceNumber);
    
    AudioSlicerAudioProcessor& audioProcessor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SliceGridDisplay)
};

//==============================================================================
/**
 * Main plugin editor
 */
class AudioSlicerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AudioSlicerAudioProcessorEditor(AudioSlicerAudioProcessor&);
    ~AudioSlicerAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AudioSlicerAudioProcessor& audioProcessor;
    
    WaveformDisplay waveformDisplay;
    SliceGridDisplay sliceGridDisplay;
    
    juce::Label titleLabel;
    juce::Label instructionsLabel;
    
    juce::Slider sensitivitySlider;
    juce::Label sensitivityLabel;
    
    juce::Slider thresholdSlider;
    juce::Label thresholdLabel;
    
    juce::Slider minLengthSlider;
    juce::Label minLengthLabel;
    
    juce::Slider maxLengthSlider;
    juce::Label maxLengthLabel;
    
    std::unique_ptr<juce::SliderParameterAttachment> sensitivityAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> thresholdAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> minLengthAttachment;
    std::unique_ptr<juce::SliderParameterAttachment> maxLengthAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSlicerAudioProcessorEditor)
};
