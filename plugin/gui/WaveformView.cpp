#include "WaveformView.h"

#include "../PluginProcessor.h"

using namespace juce;

WaveformView::WaveformView (XtoneProcessor& p) : proc (p)
{
    equalCount.setRange (2, 64, 1);
    equalCount.setValue (8, dontSendNotification);
    equalCount.setTextBoxStyle (Slider::TextBoxLeft, false, 40, 22);
    equalCount.setName ("Slice-Anzahl");

    sensitivity.setRange (0, 100, 1);
    sensitivity.setValue (50, dontSendNotification);
    sensitivity.setTextValueSuffix (" %");
    sensitivity.setTextBoxStyle (Slider::TextBoxRight, false, 48, 22);
    sensitivity.setName ("Empfindlichkeit");
    sensitivityLabel.setText ("Empfindlichkeit", dontSendNotification);
    detectionStatus.setFont (FontOptions (12.0f));
    transientButton.setTooltip ("Erkennt Transienten im ganzen Sample und ersetzt die Slice-Marker.");

    zoomOutButton.setTooltip ("Verkleinern");
    zoomInButton.setTooltip (String::fromUTF8 ("Vergrößern"));
    resetZoomButton.setTooltip ("Ganzes Sample anzeigen");
    scrollbar.setAutoHide (false);
    scrollbar.setName ("Wellenform scrollen");
    scrollbar.addListener (this);

    for (auto* button : { &equalButton, &transientButton, &clearButton, &zoomOutButton, &zoomInButton, &resetZoomButton })
        addAndMakeVisible (*button);
    addAndMakeVisible (equalCount);
    addAndMakeVisible (sensitivity);
    addAndMakeVisible (sensitivityLabel);
    addAndMakeVisible (detectionStatus);
    addAndMakeVisible (scrollbar);

    equalButton.onClick = [this]
    {
        updateAsset();
        if (asset == nullptr) return;
        auto slices = asset->slices;
        slices.setEqual ((int) equalCount.getValue());
        proc.setMarkers (*asset, slices.getMarkers());
    };
    transientButton.onClick = [this] { startDetection(); };
    clearButton.onClick = [this]
    {
        updateAsset();
        if (asset != nullptr) proc.setMarkers (*asset, {});
    };
    zoomInButton.onClick = [this] { updateAsset(); zoomAt (2.0, 0.5); };
    zoomOutButton.onClick = [this] { updateAsset(); zoomAt (0.5, 0.5); };
    resetZoomButton.onClick = [this]
    {
        updateAsset();
        if (asset != nullptr && asset->audio != nullptr) setViewport (0.0, (double) asset->audio->frames());
    };

    updateAsset();
    startTimerHz (30);
}

Rectangle<int> WaveformView::plotArea() const { return getLocalBounds().withTrimmedRight (170).reduced (2); }
Rectangle<int> WaveformView::waveArea() const { return plotArea().withTrimmedBottom (12); }

void WaveformView::resized()
{
    auto plot = plotArea();
    scrollbar.setBounds (plot.removeFromBottom (12));
    auto panel = getLocalBounds().removeFromRight (165).reduced (4);
    auto equalRow = panel.removeFromTop (26);
    equalButton.setBounds (equalRow.removeFromLeft (90));
    equalCount.setBounds (equalRow.withTrimmedLeft (4));
    panel.removeFromTop (4);
    transientButton.setBounds (panel.removeFromTop (26));
    sensitivityLabel.setBounds (panel.removeFromTop (17));
    sensitivity.setBounds (panel.removeFromTop (23));
    detectionStatus.setBounds (panel.removeFromTop (18));
    clearButton.setBounds (panel.removeFromTop (26));
    panel.removeFromTop (4);
    auto zoomRow = panel.removeFromTop (26);
    zoomOutButton.setBounds (zoomRow.removeFromLeft (36));
    zoomRow.removeFromLeft (4);
    zoomInButton.setBounds (zoomRow.removeFromLeft (36));
    resetZoomButton.setBounds (zoomRow.withTrimmedLeft (4));
}

void WaveformView::updateAsset()
{
    auto current = proc.getCurrentAsset();
    if (current == asset) return;

    const auto previousAudio = asset != nullptr ? asset->audio : nullptr;
    asset = std::move (current);
    dragIndex = -1;
    if (asset == nullptr || asset->audio != previousAudio)
    {
        detectionStatus.setText ({}, dontSendNotification);
        setViewport (0.0, asset != nullptr && asset->audio != nullptr ? (double) asset->audio->frames() : 0.0);
    }
    repaint();
}

void WaveformView::startDetection()
{
    updateAsset();
    if (asset == nullptr || asset->audio == nullptr) return;
    transientButton.setEnabled (false);
    sensitivity.setEnabled (false);
    detectionStatus.setText ("Analysiere ...", dontSendNotification);

    proc.detectTransients (asset, sensitivity.getValue() * 0.01,
                           [view = Component::SafePointer<WaveformView> (this)] (std::optional<int> count)
    {
        if (view == nullptr) return;
        view->updateAsset();
        view->transientButton.setEnabled (true);
        view->sensitivity.setEnabled (true);
        const String status = ! count ? "Analyse verworfen" : *count == 0 ? "Keine Transienten" : "";
        view->detectionStatus.setText (status, dontSendNotification);
    });
}

void WaveformView::setViewport (double start, double length)
{
    const double frames = asset != nullptr && asset->audio != nullptr ? (double) asset->audio->frames() : 0.0;
    const double minimumLength = jmin (frames, 64.0);
    visibleLength = jlimit (minimumLength, frames, length);
    visibleStart = jlimit (0.0, frames - visibleLength, start);
    scrollbar.setRangeLimits (0.0, jmax (1.0, frames), dontSendNotification);
    scrollbar.setCurrentRange (visibleStart, visibleLength, dontSendNotification);
    scrollbar.setSingleStepSize (visibleLength * 0.1);
    scrollbar.setEnabled (visibleLength < frames);
    zoomOutButton.setEnabled (visibleLength < frames);
    resetZoomButton.setEnabled (visibleLength < frames);
    zoomInButton.setEnabled (visibleLength > minimumLength);
    repaint();
}

void WaveformView::zoomAt (double factor, double anchor)
{
    if (asset == nullptr || asset->audio == nullptr) return;
    const double frames = (double) asset->audio->frames();
    const double nextLength = jlimit (jmin (frames, 64.0), frames, visibleLength / factor);
    setViewport (visibleStart + anchor * (visibleLength - nextLength), nextLength);
}

void WaveformView::scrollBarMoved (ScrollBar*, double start)
{
    updateAsset();
    setViewport (start, visibleLength);
}

double WaveformView::xToFrame (float x) const
{
    const auto w = waveArea();
    const double fraction = jlimit (0.0, 1.0, (x - (double) w.getX()) / jmax (1, w.getWidth()));
    return visibleStart + fraction * visibleLength;
}

float WaveformView::frameToX (double frame) const
{
    const auto w = waveArea();
    return (float) (w.getX() + (frame - visibleStart) * w.getWidth() / visibleLength);
}

void WaveformView::paintSignal (Graphics& g, Rectangle<int> w) const
{
    const auto& audio = *asset->audio;
    const float* mono = audio.buffer.getReadPointer (2);
    const int frames = audio.buffer.getNumSamples();
    const double framesPerPixel = visibleLength / w.getWidth();
    const float midY = (float) w.getCentreY(), halfH = (float) w.getHeight() * 0.48f;
    g.setColour (Colours::lightgreen);

    if (framesPerPixel < 1.0)
    {
        Path trace;
        const int first = (int) std::floor (visibleStart);
        const int last = jmin (frames - 1, (int) std::ceil (visibleStart + visibleLength));
        trace.startNewSubPath (frameToX (first), midY - mono[first] * halfH);
        for (int frame = first + 1; frame <= last; ++frame)
            trace.lineTo (frameToX (frame), midY - mono[frame] * halfH);
        if (last == frames - 1) trace.lineTo (frameToX (frames), midY - mono[last] * halfH);
        g.strokePath (trace, PathStrokeType (1.0f));
        return;
    }

    constexpr int blockSize = xtp::SampleAsset::kPeakBlock;
    const auto* peaks = asset->peaks.get();
    for (int px = 0; px < w.getWidth(); ++px)
    {
        const int first = (int) std::floor (visibleStart + px * framesPerPixel);
        const int end = jmin (frames, (int) std::ceil (visibleStart + (px + 1) * framesPerPixel));
        float lo = 0.0f, hi = 0.0f;
        if (peaks != nullptr && framesPerPixel >= blockSize)
        {
            for (int block = first / blockSize; block * blockSize < end; ++block)
            {
                lo = jmin (lo, (*peaks)[(size_t) block * 2]);
                hi = jmax (hi, (*peaks)[(size_t) block * 2 + 1]);
            }
        }
        else
        {
            for (int frame = first; frame < end; ++frame)
            {
                lo = jmin (lo, mono[frame]);
                hi = jmax (hi, mono[frame]);
            }
        }
        g.drawVerticalLine (w.getX() + px, midY - hi * halfH, midY - lo * halfH + 1.0f);
    }
}

void WaveformView::paint (Graphics& g)
{
    const auto w = waveArea();
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRect (w);
    const Graphics::ScopedSaveState state (g);
    if (visibleLength <= 0.0 || ! g.reduceClipRegion (w)) return;

    const auto& tel = proc.telemetry();
    const auto& markers = dragIndex >= 0 ? dragSlices.getMarkers() : asset->slices.getMarkers();
    const int numSlices = (int) markers.size() + 1;
    const int currentSlice = jlimit (0, numSlices - 1, tel.slice.load());
    const auto frames = (double) asset->audio->frames();
    const double sliceStart = currentSlice == 0 ? 0.0 : (double) markers[(size_t) currentSlice - 1];
    const double sliceEnd = currentSlice == numSlices - 1 ? frames : (double) markers[(size_t) currentSlice];
    const auto xInPlot = [this, w] (double frame) { return jlimit ((float) w.getX(), (float) w.getRight(), frameToX (frame)); };

    const float x0 = xInPlot (sliceStart), x1 = xInPlot (sliceEnd);
    g.setColour (Colours::orange.withAlpha (0.12f));
    g.fillRect (x0, (float) w.getY(), x1 - x0, (float) w.getHeight());

    const double scan = proc.apvts.getRawParameterValue ("scan")->load() * 0.01;
    const double length = proc.apvts.getRawParameterValue ("length")->load() * 0.01 * (sliceEnd - sliceStart);
    const double windowStart = sliceStart + scan * (sliceEnd - sliceStart - length);
    const float sx0 = xInPlot (windowStart), sx1 = xInPlot (windowStart + length);
    g.setColour (Colours::orange.withAlpha (0.18f));
    g.fillRect (sx0, (float) w.getY() + (float) w.getHeight() * 0.85f, sx1 - sx0, (float) w.getHeight() * 0.15f);

    paintSignal (g, w);

    const double visibleEnd = visibleStart + visibleLength;
    g.setColour (Colours::yellow);
    for (size_t i = 0; i < markers.size(); ++i)
    {
        const auto frame = (double) markers[i];
        if (frame < visibleStart || frame >= visibleEnd) continue;
        const float x = frameToX (frame);
        g.drawLine (x, (float) w.getY(), x, (float) w.getBottom(), 1.5f);
        g.drawText (String ((int) i + 1), (int) x + 2, w.getY() + 2, 30, 14, Justification::left);
    }

    const double position = tel.playPos.load() * frames;
    if (position >= visibleStart && position < visibleEnd)
    {
        g.setColour (Colours::white);
        const float x = frameToX (position);
        g.drawLine (x, (float) w.getY(), x, (float) w.getBottom(), 1.0f);
    }

    g.setColour (Colours::white.withAlpha (0.8f));
    g.setFont (FontOptions (13.0f));
    g.drawText (asset->name + "  (" + String (numSlices) + " Slices)", w.reduced (6), Justification::bottomLeft);
}

void WaveformView::mouseDown (const MouseEvent& e)
{
    updateAsset();
    if (visibleLength <= 0.0 || ! waveArea().contains (e.getPosition())) return;
    const double frame = xToFrame (e.position.x);
    const double tolerance = 4.0 * visibleLength / waveArea().getWidth();
    const int index = asset->slices.findMarker (std::llround (frame), (int64_t) std::ceil (tolerance));
    if (index < 0) return;
    const auto marker = (double) asset->slices.getMarkers()[(size_t) index];
    if (marker < visibleStart || marker >= visibleStart + visibleLength || std::abs (marker - frame) > tolerance) return;
    if (e.mods.isPopupMenu())
    {
        auto slices = asset->slices;
        slices.remove (index);
        proc.setMarkers (*asset, slices.getMarkers());
    }
    else if (e.mods.isLeftButtonDown())
    {
        dragIndex = index;
        dragSlices = asset->slices;
    }
}

void WaveformView::mouseDrag (const MouseEvent& e)
{
    updateAsset();
    if (dragIndex < 0) return;
    dragSlices.move (dragIndex, std::llround (xToFrame (e.position.x)));
    repaint();
}

void WaveformView::mouseUp (const MouseEvent&)
{
    updateAsset();
    if (dragIndex < 0) return;
    dragIndex = -1;
    if (dragSlices.getMarkers() != asset->slices.getMarkers())
        proc.setMarkers (*asset, dragSlices.getMarkers());
    repaint();
}

void WaveformView::mouseDoubleClick (const MouseEvent& e)
{
    updateAsset();
    if (visibleLength <= 0.0 || e.mods.isPopupMenu() || ! waveArea().contains (e.getPosition())) return;
    auto slices = asset->slices;
    if (slices.add (std::llround (xToFrame (e.position.x)))) proc.setMarkers (*asset, slices.getMarkers());
}

void WaveformView::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wheel)
{
    updateAsset();
    const auto w = waveArea();
    if (visibleLength <= 0.0 || dragIndex >= 0 || ! w.contains (e.getPosition())) return;
    if (e.mods.isShiftDown() || std::abs (wheel.deltaX) > std::abs (wheel.deltaY))
    {
        const double delta = wheel.deltaX != 0.0f ? wheel.deltaX : wheel.deltaY;
        setViewport (visibleStart - delta * visibleLength, visibleLength);
    }
    else if (wheel.deltaY != 0.0f)
    {
        const double anchor = (xToFrame (e.position.x) - visibleStart) / visibleLength;
        zoomAt (std::exp2 (4.0 * wheel.deltaY), anchor);
    }
}

void WaveformView::timerCallback()
{
    updateAsset();
    const float pos = proc.telemetry().playPos.load();
    if (std::abs (pos - lastPos) > 1.0e-6f || proc.telemetry().active.load())
    {
        lastPos = pos;
        repaint();
    }
}
