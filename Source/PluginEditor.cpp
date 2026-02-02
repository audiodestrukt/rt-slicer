#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WaveformDisplay::WaveformDisplay(AudioSlicerAudioProcessor& p)
    : audioProcessor(p)
{
    displayBuffer.setSize(2, 4096);
    displayBuffer.clear();
    startTimerHz(30); // 30 FPS refresh
}

WaveformDisplay::~WaveformDisplay()
{
    stopTimer();
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    
    auto bounds = getLocalBounds().toFloat();
    
    // Draw waveform
    if (displayBuffer.getNumSamples() > 0)
    {
        g.setColour(juce::Colours::green);
        
        auto waveform = juce::Path();
        auto ratio = bounds.getWidth() / (float)displayBuffer.getNumSamples();
        auto* data = displayBuffer.getReadPointer(0);
        
        waveform.startNewSubPath(0, bounds.getCentreY());
        
        for (int i = 0; i < displayBuffer.getNumSamples(); ++i)
        {
            auto x = i * ratio;
            auto y = juce::jmap(data[i], -1.0f, 1.0f, 
                               bounds.getBottom(), bounds.getY());
            waveform.lineTo(x, y);
        }
        
        g.strokePath(waveform, juce::PathStrokeType(1.0f));
    }
    
    // Draw center line
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawLine(0, bounds.getCentreY(), bounds.getWidth(), bounds.getCentreY(), 1.0f);
    
    // Draw border
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.drawRect(bounds, 1.0f);
}

void WaveformDisplay::resized()
{
}

void WaveformDisplay::timerCallback()
{
    audioProcessor.getIncomingAudioBuffer(displayBuffer);
    repaint();
}

//==============================================================================
SliceGridDisplay::SliceGridDisplay(AudioSlicerAudioProcessor& p)
    : audioProcessor(p)
{
    startTimerHz(30); // 30 FPS refresh
}

SliceGridDisplay::~SliceGridDisplay()
{
    stopTimer();
}

void SliceGridDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    auto bounds = getLocalBounds();
    int sliceWidth = bounds.getWidth() / 4;
    int sliceHeight = bounds.getHeight() / 4;
    
    int currentSlice = audioProcessor.getCurrentSliceIndex();
    
    for (int i = 0; i < AudioSlicerAudioProcessor::maxSlices; ++i)
    {
        int row = i / 4;
        int col = i % 4;
        
        auto sliceBounds = juce::Rectangle<int>(
            col * sliceWidth, 
            row * sliceHeight, 
            sliceWidth - 2, 
            sliceHeight - 2
        );
        
        const auto& slice = audioProcessor.getSlice(i);
        
        // Highlight current recording slice
        if (i == currentSlice)
        {
            g.setColour(juce::Colours::orange.withAlpha(0.3f));
            g.fillRect(sliceBounds);
        }
        
        drawSlice(g, slice, sliceBounds, i);
    }
}

void SliceGridDisplay::drawSlice(juce::Graphics& g, const AudioSlice& slice, 
                                  juce::Rectangle<int> bounds, int sliceNumber)
{
    bounds = bounds.reduced(4);
    
    // Draw background based on state
    if (slice.isPlaying)
    {
        g.setColour(juce::Colours::green.withAlpha(0.5f));
        g.fillRect(bounds);
    }
    else if (slice.isActive)
    {
        g.setColour(juce::Colours::blue.withAlpha(0.3f));
        g.fillRect(bounds);
    }
    else
    {
        g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
        g.fillRect(bounds);
    }
    
    // Draw waveform if slice is active
    if (slice.isActive && slice.lengthSamples > 0)
    {
        g.setColour(juce::Colours::cyan);
        
        auto waveformBounds = bounds.reduced(2).toFloat();
        int numSamples = slice.lengthSamples;
        auto* data = slice.buffer.getReadPointer(0);
        
        // Downsample for display
        int displayWidth = waveformBounds.getWidth();
        int samplesPerPixel = juce::jmax(1, numSamples / displayWidth);
        
        auto path = juce::Path();
        path.startNewSubPath(waveformBounds.getX(), waveformBounds.getCentreY());
        
        for (int x = 0; x < displayWidth; ++x)
        {
            int sampleIndex = juce::jmin(x * samplesPerPixel, numSamples - 1);
            
            float max = 0.0f;
            for (int i = 0; i < samplesPerPixel && (sampleIndex + i) < numSamples; ++i)
            {
                max = juce::jmax(max, std::abs(data[sampleIndex + i]));
            }
            
            float y = juce::jmap(max, 0.0f, 1.0f,
                               waveformBounds.getCentreY(),
                               waveformBounds.getY());
            
            path.lineTo(waveformBounds.getX() + x, y);
        }
        
        // Mirror for bottom half
        for (int x = displayWidth - 1; x >= 0; --x)
        {
            int sampleIndex = juce::jmin(x * samplesPerPixel, numSamples - 1);
            
            float max = 0.0f;
            for (int i = 0; i < samplesPerPixel && (sampleIndex + i) < numSamples; ++i)
            {
                max = juce::jmax(max, std::abs(data[sampleIndex + i]));
            }
            
            float y = juce::jmap(max, 0.0f, 1.0f,
                               waveformBounds.getCentreY(),
                               waveformBounds.getBottom());
            
            path.lineTo(waveformBounds.getX() + x, y);
        }
        
        path.closeSubPath();
        g.fillPath(path);
    }
    
    // Draw slice number and MIDI note
    g.setColour(juce::Colours::white);
    g.setFont(12.0f);
    
    juce::String text = juce::String(sliceNumber + 1);
    juce::String midiNote = "C" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    
    if (sliceNumber % 12 == 1) midiNote = "C#" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 2) midiNote = "D" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 3) midiNote = "D#" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 4) midiNote = "E" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 5) midiNote = "F" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 6) midiNote = "F#" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 7) midiNote = "G" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 8) midiNote = "G#" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 9) midiNote = "A" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 10) midiNote = "A#" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    else if (sliceNumber % 12 == 11) midiNote = "B" + juce::String((sliceNumber / 12) + 3) + " (" + juce::String(sliceNumber + 60) + ")";
    
    g.drawText(text, bounds.getX() + 4, bounds.getY() + 4, 40, 16, juce::Justification::left);
    g.setFont(10.0f);
    g.drawText(midiNote, bounds.getX() + 4, bounds.getBottom() - 18, bounds.getWidth() - 8, 14, 
               juce::Justification::left);
    
    // Draw playback indicator
    if (slice.isPlaying)
    {
        float progress = (float)slice.playbackPosition / (float)slice.lengthSamples;
        int progressWidth = (int)(bounds.getWidth() * progress);
        
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.fillRect(bounds.getX(), bounds.getBottom() - 3, progressWidth, 3);
    }
    
    // Draw border
    g.setColour(slice.isActive ? juce::Colours::white.withAlpha(0.5f) : juce::Colours::grey.withAlpha(0.3f));
    g.drawRect(bounds, 1);
}

void SliceGridDisplay::resized()
{
}

void SliceGridDisplay::timerCallback()
{
    repaint();
}

void SliceGridDisplay::mouseDown(const juce::MouseEvent& event)
{
    // Calculate which slice was clicked
    auto bounds = getLocalBounds();
    int sliceWidth = bounds.getWidth() / 4;
    int sliceHeight = bounds.getHeight() / 4;
    
    int col = event.x / sliceWidth;
    int row = event.y / sliceHeight;
    int sliceIndex = row * 4 + col;
    
    if (sliceIndex >= 0 && sliceIndex < AudioSlicerAudioProcessor::maxSlices)
    {
        // Trigger the slice with full velocity
        const_cast<AudioSlicerAudioProcessor&>(audioProcessor).triggerSlice(sliceIndex, 1.0f);
    }
}

//==============================================================================
AudioSlicerAudioProcessorEditor::AudioSlicerAudioProcessorEditor(AudioSlicerAudioProcessor& p)
    : AudioProcessorEditor(&p), 
      audioProcessor(p),
      waveformDisplay(p),
      sliceGridDisplay(p)
{
    setSize(800, 700);
    
    // Title
    titleLabel.setText("Audio Slicer VST", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);
    
    // Instructions
    instructionsLabel.setText("Live audio is sliced automatically. Trigger slices with MIDI notes C3-D#4 (60-75) or click slots.", 
                             juce::dontSendNotification);
    instructionsLabel.setFont(juce::Font(12.0f));
    instructionsLabel.setJustificationType(juce::Justification::centred);
    instructionsLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(instructionsLabel);
    
    // Waveform display
    addAndMakeVisible(waveformDisplay);
    
    // Slice grid
    addAndMakeVisible(sliceGridDisplay);
    
    // Sensitivity slider
    sensitivitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    sensitivitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    addAndMakeVisible(sensitivitySlider);
    
    sensitivityLabel.setText("Sensitivity:", juce::dontSendNotification);
    sensitivityLabel.setJustificationType(juce::Justification::right);
    sensitivityLabel.attachToComponent(&sensitivitySlider, true);
    addAndMakeVisible(sensitivityLabel);
    
    sensitivityAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *audioProcessor.sensitivityParam, sensitivitySlider);
    
    // Threshold slider
    thresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    thresholdSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(thresholdSlider);
    
    thresholdLabel.setText("Threshold:", juce::dontSendNotification);
    thresholdLabel.setJustificationType(juce::Justification::right);
    thresholdLabel.attachToComponent(&thresholdSlider, true);
    addAndMakeVisible(thresholdLabel);
    
    thresholdAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *audioProcessor.thresholdParam, thresholdSlider);
    
    // Min length slider
    minLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    minLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    minLengthSlider.setTextValueSuffix(" s");
    addAndMakeVisible(minLengthSlider);
    
    minLengthLabel.setText("Min Length:", juce::dontSendNotification);
    minLengthLabel.setJustificationType(juce::Justification::right);
    minLengthLabel.attachToComponent(&minLengthSlider, true);
    addAndMakeVisible(minLengthLabel);
    
    minLengthAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *audioProcessor.minSliceLengthParam, minLengthSlider);
    
    // Max length slider
    maxLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    maxLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    maxLengthSlider.setTextValueSuffix(" s");
    addAndMakeVisible(maxLengthSlider);
    
    maxLengthLabel.setText("Max Length:", juce::dontSendNotification);
    maxLengthLabel.setJustificationType(juce::Justification::right);
    maxLengthLabel.attachToComponent(&maxLengthSlider, true);
    addAndMakeVisible(maxLengthLabel);
    
    maxLengthAttachment = std::make_unique<juce::SliderParameterAttachment>(
        *audioProcessor.maxSliceLengthParam, maxLengthSlider);
}

AudioSlicerAudioProcessorEditor::~AudioSlicerAudioProcessorEditor()
{
}

//==============================================================================
void AudioSlicerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2d2d2d));
}

void AudioSlicerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(35));
    bounds.removeFromTop(5);
    
    // Instructions
    instructionsLabel.setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);
    
    // Waveform display
    waveformDisplay.setBounds(bounds.removeFromTop(100));
    bounds.removeFromTop(10);
    
    // Slice grid (main area)
    sliceGridDisplay.setBounds(bounds.removeFromTop(400));
    bounds.removeFromTop(15);
    
    // Parameter controls
    int labelWidth = 90;
    int sliderHeight = 24;
    int spacing = 8;
    
    auto sliderBounds = bounds.removeFromTop(sliderHeight);
    sliderBounds.removeFromLeft(labelWidth);
    sensitivitySlider.setBounds(sliderBounds);
    
    bounds.removeFromTop(spacing);
    sliderBounds = bounds.removeFromTop(sliderHeight);
    sliderBounds.removeFromLeft(labelWidth);
    thresholdSlider.setBounds(sliderBounds);
    
    bounds.removeFromTop(spacing);
    sliderBounds = bounds.removeFromTop(sliderHeight);
    sliderBounds.removeFromLeft(labelWidth);
    minLengthSlider.setBounds(sliderBounds);
    
    bounds.removeFromTop(spacing);
    sliderBounds = bounds.removeFromTop(sliderHeight);
    sliderBounds.removeFromLeft(labelWidth);
    maxLengthSlider.setBounds(sliderBounds);
}
