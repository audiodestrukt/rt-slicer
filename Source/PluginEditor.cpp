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
    
    int currentSlice = audioProcessor.getCurrentSliceIndex();

    for (int i = 0; i < AudioSlicerAudioProcessor::maxSlices; ++i)
    {
        auto sliceBounds = getSliceBounds(i);
        
        const auto& slice = audioProcessor.getSlice(i);
        
        // Highlight the slot being recorded into -- but not while capture is
        // blocked, when there is no such slot and the highlight would be
        // claiming something untrue.
        if (i == currentSlice && !audioProcessor.isCaptureBlocked())
        {
            g.setColour(juce::Colours::orange.withAlpha(0.3f));
            g.fillRect(sliceBounds);
        }
        
        drawSlice(g, slice, sliceBounds, i);
    }

    // Every pad frozen means capture has stopped. Say so structurally rather
    // than with text, which would not survive the smallest layouts: the grid
    // gets an orange-red frame, and no pad carries the orange recording
    // highlight, because there is no slot being recorded into.
    if (audioProcessor.isCaptureBlocked())
    {
        g.setColour(juce::Colour(0xffff3c00));
        g.drawRect(getLocalBounds(), juce::jmax(2, getHeight() / 60));
    }
}

void SliceGridDisplay::drawSlice(juce::Graphics& g, const AudioSlice& slice, 
                                  juce::Rectangle<int> bounds, int sliceNumber)
{
    bounds = bounds.reduced(juce::jlimit(1, 4, bounds.getHeight() / 12));

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

        // Component coordinates are logical POINTS, but a Retina panel has 2-3
        // physical pixels per point. Stepping the envelope one point at a time
        // would throw away two thirds of the resolution an iPad can actually
        // draw, so the envelope is computed per physical pixel and the path is
        // plotted back in points.
        const float pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();
        const int displayWidth = juce::jmax(1, juce::roundToInt(waveformBounds.getWidth() * pixelScale));
        const int samplesPerPixel = juce::jmax(1, numSamples / displayWidth);

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

            path.lineTo(waveformBounds.getX() + x / pixelScale, y);
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
    
    // A cell shrinks with the view. Below ~44px there is no room for both the
    // slot number and the MIDI note without them colliding, so the note -- the
    // less useful of the two while playing -- is dropped first.
    const bool showMidiNote = bounds.getHeight() >= 44;

    g.setFont(juce::jlimit(9.0f, 12.0f, bounds.getHeight() / 4.0f));
    g.drawText(text, bounds.getX() + 4, bounds.getY() + 2, 40, 16, juce::Justification::topLeft);

    if (showMidiNote)
    {
        g.setFont(10.0f);
        g.drawText(midiNote, bounds.getX() + 4, bounds.getBottom() - 18, bounds.getWidth() - 8, 14,
                   juce::Justification::left);
    }
    
    // Draw playback indicator
    if (slice.isPlaying)
    {
        float progress = (float)slice.playbackPosition / (float)slice.lengthSamples;
        int progressWidth = (int)(bounds.getWidth() * progress);
        
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.fillRect(bounds.getX(), bounds.getBottom() - 3, progressWidth, 3);
    }
    
    // Draw border. A frozen pad gets the brand lime at full strength and a
    // thicker stroke -- weight survives being small and glanced at, where a hue
    // change alone does not.
    if (slice.isFrozen.load())
    {
        const int thickness = juce::jmax(2, bounds.getHeight() / 22);
        g.setColour(juce::Colour(0xffc8ff00));
        g.drawRect(bounds, thickness);

        // Corner tab, so frozen is still distinguishable if the border is
        // clipped by a very small pad.
        const int tab = juce::jmax(5, bounds.getHeight() / 7);
        juce::Path corner;
        corner.startNewSubPath((float) bounds.getRight(), (float) bounds.getY());
        corner.lineTo((float) bounds.getRight(), (float) bounds.getY() + tab);
        corner.lineTo((float) bounds.getRight() - tab, (float) bounds.getY());
        corner.closeSubPath();
        g.fillPath(corner);
    }
    else
    {
        g.setColour(slice.isActive ? juce::Colours::white.withAlpha(0.5f) : juce::Colours::grey.withAlpha(0.3f));
        g.drawRect(bounds, 1);
    }
}

void SliceGridDisplay::resized()
{
}

void SliceGridDisplay::timerCallback()
{
    // Long presses are resolved here rather than with a timer per touch, since
    // this already runs for the repaint.
    const auto now = juce::Time::currentTimeMillis();
    for (auto& [source, hold] : holdForSource)
    {
        if (!hold.fired && now - hold.startMs >= holdToFreezeMs)
        {
            hold.fired = true;
            const_cast<AudioSlicerAudioProcessor&>(audioProcessor).toggleFreeze(hold.slice);
        }
    }

    repaint();
}

// The one place the grid is divided. Cell n of `divisions` spans
// [cellEdge(extent, n), cellEdge(extent, n + 1)), which tiles `extent` exactly
// with no truncation gap.
static int cellEdge(int extent, int index, int divisions)
{
    return extent * index / divisions;
}

// Inverse of cellEdge. Found by walking the same boundaries rather than by a
// closed-form division: x * divisions / extent is NOT an exact inverse, and the
// disagreement puts the boundary pixel of a cell in its neighbour. Walking is
// correct by construction, and with four columns it is free.
static int cellIndexAt(int extent, int position, int divisions)
{
    for (int i = divisions; --i > 0;)
        if (position >= cellEdge(extent, i, divisions))
            return i;

    return 0;
}

juce::Rectangle<int> SliceGridDisplay::getSliceBounds(int sliceIndex) const
{
    auto bounds = getLocalBounds();
    const int row = sliceIndex / gridColumns;
    const int col = sliceIndex % gridColumns;

    const int x1 = cellEdge(bounds.getWidth(),  col,     gridColumns);
    const int x2 = cellEdge(bounds.getWidth(),  col + 1, gridColumns);
    const int y1 = cellEdge(bounds.getHeight(), row,     gridRows);
    const int y2 = cellEdge(bounds.getHeight(), row + 1, gridRows);

    // The 2px inset is the gutter between pads; hit-testing deliberately does
    // not apply it, so the gutter still triggers the pad it belongs to rather
    // than being dead space under a fingertip.
    return juce::Rectangle<int>(x1, y1, x2 - x1 - 2, y2 - y1 - 2);
}

int SliceGridDisplay::sliceIndexAt(juce::Point<int> position) const
{
    auto bounds = getLocalBounds();

    if (bounds.isEmpty() || !bounds.contains(position))
        return -1;

    const int col = cellIndexAt(bounds.getWidth(),  position.x, gridColumns);
    const int row = cellIndexAt(bounds.getHeight(), position.y, gridRows);

    const int sliceIndex = row * gridColumns + col;

    return sliceIndex < AudioSlicerAudioProcessor::maxSlices ? sliceIndex : -1;
}

void SliceGridDisplay::triggerFromEvent(const juce::MouseEvent& event)
{
    const int sourceIndex = event.source.getIndex();
    const int sliceIndex = sliceIndexAt(event.getPosition());

    // Only fire when this touch moves onto a different pad, so holding a finger
    // still does not machine-gun the slice on every drag callback.
    const auto previous = lastSliceForSource.find(sourceIndex);
    if (previous != lastSliceForSource.end() && previous->second == sliceIndex)
        return;

    lastSliceForSource[sourceIndex] = sliceIndex;

    if (sliceIndex < 0)
        return;

    // Pressure-sensitive where the hardware reports it (3D Touch / Apple
    // Pencil); a plain mouse or a non-force touchscreen reports no pressure and
    // plays at full velocity.
    const float velocity = event.isPressureValid()
                             ? juce::jlimit(0.05f, 1.0f, event.pressure)
                             : 1.0f;

    const_cast<AudioSlicerAudioProcessor&>(audioProcessor).triggerSlice(sliceIndex, velocity);
}

void SliceGridDisplay::mouseDown(const juce::MouseEvent& event)
{
    const int source = event.source.getIndex();
    lastSliceForSource.erase(source);
    triggerFromEvent(event);

    // Arm the hold. The pad has already played -- you hear the slice, then keep
    // it by continuing to hold, which is the right order for deciding whether
    // it is worth keeping.
    const int sliceIndex = sliceIndexAt(event.getPosition());
    if (sliceIndex >= 0)
        holdForSource[source] = { sliceIndex, juce::Time::currentTimeMillis(), false };
}

void SliceGridDisplay::mouseDrag(const juce::MouseEvent& event)
{
    // Moving off the pad the touch started on means this is a drag, not a
    // hold.
    const int source = event.source.getIndex();
    auto held = holdForSource.find(source);
    if (held != holdForSource.end() && sliceIndexAt(event.getPosition()) != held->second.slice)
        holdForSource.erase(held);

    triggerFromEvent(event);
}

void SliceGridDisplay::mouseUp(const juce::MouseEvent& event)
{
    const int source = event.source.getIndex();
    lastSliceForSource.erase(source);
    holdForSource.erase(source);
}

//==============================================================================
AudioSlicerAudioProcessorEditor::AudioSlicerAudioProcessorEditor(AudioSlicerAudioProcessor& p)
    : AudioProcessorEditor(&p), 
      audioProcessor(p),
      waveformDisplay(p),
      sliceGridDisplay(p)
{
    // AUv3 hosts size the view themselves and will not honour a fixed size, so
    // the editor has to be resizable and lay out from whatever it is given.
    // The limits keep it usable rather than expressing a preference.
    setResizable(true, true);
    setResizeLimits(320, 240, 4096, 4096);
    setSize(800, 700);
    
    // Title
    titleLabel.setText("RipSlice", juce::dontSendNotification);
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

    // A fingertip needs a far larger target than a mouse pointer; 44pt is
    // Apple's minimum comfortable hit target.
   #if JUCE_IOS
    const int sliderHeight = 44;
   #else
    const int sliderHeight = 24;
   #endif
    const int spacing = 8;

    const int fullHeight = bounds.getHeight();

    // Reserve the controls from the BOTTOM first, then let the slice grid take
    // whatever is left. Laying out top-down with a fixed 400px grid meant that
    // in a short view -- which is exactly what an AUv3 host hands you -- the
    // grid ate the remaining space and the sliders fell off the bottom.
    //
    // The controls also never take more than 40% of the view: at their natural
    // height four sliders are half of a 240px host view, which starved the grid
    // down to an unusable strip. Below that they compress instead.
    const int naturalControls = 4 * sliderHeight + 3 * spacing;
    const int controlsHeight  = juce::jmin(naturalControls, fullHeight * 2 / 5);
    const int rowHeight       = juce::jmax(14, (controlsHeight - 3 * spacing) / 4);

    auto controls = bounds.removeFromBottom(controlsHeight);
    bounds.removeFromBottom(juce::jmin(15, fullHeight / 20));

    // Chrome drops progressively as the view shrinks, cheapest first, so the
    // grid keeps as much of the space as it can.
    if (fullHeight > 380)
    {
        titleLabel.setVisible(true);
        titleLabel.setBounds(bounds.removeFromTop(35));
        bounds.removeFromTop(5);
    }
    else
    {
        titleLabel.setVisible(false);
    }

    const bool showInstructions = fullHeight > 480;
    instructionsLabel.setVisible(showInstructions);
    if (showInstructions)
    {
        instructionsLabel.setBounds(bounds.removeFromTop(25));
        bounds.removeFromTop(10);
    }

    const bool showWaveform = fullHeight > 320;
    waveformDisplay.setVisible(showWaveform);
    if (showWaveform)
    {
        waveformDisplay.setBounds(bounds.removeFromTop(juce::jlimit(40, 100, bounds.getHeight() / 5)));
        bounds.removeFromTop(10);
    }

    // Everything that is left belongs to the grid -- it is the instrument.
    sliceGridDisplay.setBounds(bounds);

    // Labels are attached to the left of each slider, so the sliders are inset
    // to leave room for them. A narrow view cannot afford the full inset.
    const int labelWidth = bounds.getWidth() < 520 ? 64 : 90;
    auto layoutSlider = [&](juce::Slider& slider)
    {
        auto row = controls.removeFromTop(rowHeight);
        row.removeFromLeft(labelWidth);
        slider.setBounds(row);
        controls.removeFromTop(spacing);
    };

    layoutSlider(sensitivitySlider);
    layoutSlider(thresholdSlider);
    layoutSlider(minLengthSlider);
    layoutSlider(maxLengthSlider);
}
