#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <map>

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

    // Touch/mouse. On iOS every finger is a separate MouseInputSource, so these
    // are called once per touch and the grid is playable with several fingers
    // at once. mouseDrag lets a finger slide across pads and retrigger each one.
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void drawSlice(juce::Graphics& g, const AudioSlice& slice, juce::Rectangle<int> bounds, int sliceNumber);

    // Single source of truth for the grid geometry: paint() and hit-testing both
    // go through these, so a tap can never resolve to a different cell than the
    // one drawn under the finger.
    juce::Rectangle<int> getSliceBounds(int sliceIndex) const;
    int sliceIndexAt(juce::Point<int> position) const;

    // Triggers the slice under the event, if it differs from the one this touch
    // last triggered. Returns the index now held by this touch, or -1.
    void triggerFromEvent(const juce::MouseEvent& event);

    static constexpr int gridColumns = 4;
    static constexpr int gridRows = 4;

    // Which slice each active touch most recently triggered, keyed by mouse
    // source index. Per-source so that dragging one finger cannot cancel the
    // retrigger tracking of another.
    std::map<int, int> lastSliceForSource;

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
