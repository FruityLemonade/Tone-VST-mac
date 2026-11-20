#include "WaveformView.h"

#include "FaceplateArtwork.h"

#include "HammerLookAndFeel.h"

#include "../PluginProcessor.h"

#include <algorithm>
#include <array>
#include <utility>

using namespace juce;

WaveformView::WaveformView (XtoneProcessor& p) : proc (p)
{
    sensitivity.setRange (0, 100, 1);
    sensitivity.setValue (50, dontSendNotification);
    sensitivity.setTextValueSuffix (" %");
    sensitivity.setTextBoxStyle (Slider::TextBoxRight, false, 42, 23);
    sensitivity.setName ("Sensitivity");
    sensitivityLabel.setText ("Sensitivity", dontSendNotification);
    sensitivityLabel.setFont (hammer::font (12.5f));
    sensitivityLabel.setBorderSize (BorderSize<int> (0));

    sampleName.setFont (hammer::font (16.0f, Font::bold));
    sampleName.setBorderSize (BorderSize<int> (0, 9, 0, 9));
    sampleName.setColour (Label::backgroundColourId, hammer::recess);
    sampleName.setColour (Label::outlineColourId, hammer::edge);
    status.setFont (hammer::font (12.5f));
    status.setColour (Label::textColourId, hammer::muted);
    status.setBorderSize (BorderSize<int> (0));

    transientButton.setTooltip ("Detect transients in the entire sample and replace the slice markers.");
    zoomOutButton.setTooltip ("Zoom out");
    zoomInButton.setTooltip ("Zoom in");
    resetZoomButton.setTooltip ("Show the entire sample");
    zoomOutButton.setTitle ("Zoom out on waveform");
    zoomInButton.setTitle ("Zoom in on waveform");
    resetZoomButton.setTitle ("Show the entire sample");
    sliceButton.setTooltip ("Create evenly spaced slices or clear the slice markers");
    scrollbar.setAutoHide (true);
    scrollbar.setName ("Scroll waveform");
    scrollbar.addListener (this);

    for (auto* button : { &loadSample, &normalize, &transientButton, &sliceButton, &zoomOutButton, &zoomInButton, &resetZoomButton })
        addAndMakeVisible (*button);
    for (Component* component : std::array<Component*, 5> { &sampleName, &sensitivity, &sensitivityLabel, &status, &scrollbar })
        addAndMakeVisible (*component);

    loadSample.onClick = [this] { chooseSample(); };
    normalize.onClick = [this] { proc.normalizeSample(); };
    transientButton.onClick = [this] { startDetection(); };
    sliceButton.onClick = [this] { showSliceMenu(); };
    zoomInButton.onClick = [this] { updateAsset(); zoomAt (2.0, 0.5); };
    zoomOutButton.onClick = [this] { updateAsset(); zoomAt (0.5, 0.5); };
    resetZoomButton.onClick = [this]
    {
        updateAsset();
        if (asset != nullptr && asset->audio != nullptr) setViewport (0.0, (double) asset->audio->frames());
    };

    timerCallback();
    startTimerHz (30);
}

WaveformView::~WaveformView()
{
    if (sliceMenuOpen) PopupMenu::dismissAllActiveMenus();
    scrollbar.removeListener (this);
}

bool WaveformView::usesTwoToolbarRows() const { return getWidth() < 535; }

Rectangle<int> WaveformView::plotArea() const
{
    return getLocalBounds().reduced (13, 12).withTrimmedTop (41).withTrimmedBottom (usesTwoToolbarRows() ? 84 : 51);
}

Rectangle<int> WaveformView::waveArea() const { return plotArea().withTrimmedBottom (33); }

void WaveformView::resized()
{
    auto content = getLocalBounds().reduced (13, 12);
    auto sample = content.removeFromTop (32);
    normalize.setBounds (sample.removeFromRight (104));
    sample.removeFromRight (5);
    loadSample.setBounds (sample.removeFromRight (108));
    sampleName.setBounds (sample.withTrimmedRight (8));

    auto plot = plotArea();
    scrollbar.setBounds (plot.removeFromBottom (11));
    status.setBounds (content.removeFromBottom (17));
    auto toolbar = content.removeFromBottom (usesTwoToolbarRows() ? 63 : 30);
    auto primary = toolbar.removeFromTop (30);
    auto actions = usesTwoToolbarRows() ? toolbar.removeFromBottom (28) : primary.removeFromRight (181);
    transientButton.setBounds (primary.removeFromLeft (92));
    primary.removeFromLeft (7);
    sensitivityLabel.setBounds (primary.removeFromLeft (86));
    sensitivity.setBounds (primary.withTrimmedRight (7));
    sliceButton.setBounds (actions.removeFromLeft (62));
    actions.removeFromLeft (8);
    resetZoomButton.setBounds (actions.removeFromLeft (49));
    actions.removeFromLeft (3);
    zoomOutButton.setBounds (actions.removeFromLeft (27));
    actions.removeFromLeft (3);
    zoomInButton.setBounds (actions.removeFromLeft (27));
}

void WaveformView::chooseSample()
{
    if (chooser != nullptr) return;
    chooser = std::make_unique<FileChooser> ("Load Sample", File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                          [self = SafePointer<WaveformView> (this)] (const FileChooser& dialog)
    {
        if (self == nullptr) return;
        const auto file = dialog.getResult();
        self->chooser.reset();
        if (file.existsAsFile()) self->proc.loadSampleFile (file);
    });
}

void WaveformView::showSliceMenu()
{
    updateAsset();
    if (asset == nullptr || sliceMenuOpen || ! isShowing()) return;

    PopupMenu menu, equal;
    menu.setLookAndFeel (&getLookAndFeel());
    for (int first = 2; first <= 64;)
    {
        const int last = jmin (64, (first + 7) / 8 * 8);
        PopupMenu counts;
        for (int count = first; count <= last; ++count)
            counts.addItem (count, String (count) + " Slices");
        equal.addSubMenu (String (first) + String::fromUTF8 ("–") + String (last), counts);
        first = last + 1;
    }
    menu.addSubMenu ("Slice Evenly", equal);
    constexpr int clearMarkers = 1;
    menu.addItem (clearMarkers, "Clear All Markers", ! asset->slices.getMarkers().empty());
    sliceMenuOpen = true;
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&sliceButton),
                        [self = SafePointer<WaveformView> (this), target = asset] (int choice)
    {
        if (self == nullptr) return;
        self->sliceMenuOpen = false;
        if (choice == 0) return;
        if (choice == clearMarkers)
        {
            self->proc.setMarkers (*target, {});
        }
        else
        {
            auto slices = target->slices;
            slices.setEqual (choice);
            self->proc.setMarkers (*target, slices.getMarkers());
        }
    });
}

void WaveformView::updateAsset()
{
    auto current = proc.getCurrentAsset();
    if (current == asset) return;

    const auto previousAudio = asset != nullptr ? asset->audio : nullptr;
    asset = std::move (current);
    dragIndex = -1;
    selectOnRelease = false;
    if (asset == nullptr || asset->audio != previousAudio)
    {
        detectionMessage.clear();
        setViewport (0.0, asset != nullptr && asset->audio != nullptr ? (double) asset->audio->frames() : 0.0);
    }
    const bool hasAudio = asset != nullptr && asset->audio != nullptr;
    normalize.setEnabled (hasAudio && ! asset->normalized);
    sliceButton.setEnabled (hasAudio);
    sampleName.setText (hasAudio ? asset->name : "No Sample", dontSendNotification);
    sampleName.setTooltip (hasAudio ? asset->name : String());
    updateStatus();
    repaint();
}

void WaveformView::updateStatus()
{
    auto message = proc.getStatusMessage();
    if (message.isEmpty()) message = detectionMessage;
    if (message.isEmpty() && asset != nullptr)
    {
        if (asset->normalized) message = "Normalized";
        if (asset->fileBytes != nullptr)
        {
            if (message.isNotEmpty()) message << String::fromUTF8 ("  ·  ");
            message << "Embedded in project";
        }
    }
    status.setText (message, dontSendNotification);
    status.setTooltip (message);
}

void WaveformView::startDetection()
{
    updateAsset();
    if (asset == nullptr || asset->audio == nullptr) return;
    transientButton.setEnabled (false);
    sensitivity.setEnabled (false);
    detectionMessage = String::fromUTF8 ("Analyzing…");
    updateStatus();

    proc.detectTransients (asset, sensitivity.getValue() * 0.01,
                           [view = Component::SafePointer<WaveformView> (this)] (std::optional<int> count)
    {
        if (view == nullptr) return;
        view->updateAsset();
        view->transientButton.setEnabled (true);
        view->sensitivity.setEnabled (true);
        view->detectionMessage = ! count ? "Analysis discarded" : *count == 0 ? "No transients found" : "";
        view->updateStatus();
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
    g.setColour (hammer::accentFor (*this));

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

void WaveformView::paintTimeline (Graphics& g, Rectangle<int> w) const
{
    const auto& audio = *asset->audio;
    const int segments = jlimit (2, 5, w.getWidth() / 105);
    const double lengthSeconds = visibleLength / audio.sampleRate;
    g.setColour (hammer::muted);
    g.setFont (hammer::mono (11.0f));
    for (int i = 0; i <= segments; ++i)
    {
        const int labelWidth = w.getWidth() / segments;
        const double fraction = (double) i / (double) segments;
        const int x = w.getX() + roundToInt (fraction * w.getWidth());
        const int left = jlimit (w.getX(), w.getRight() - labelWidth, x - labelWidth / 2);
        const auto alignment = i == 0 ? Justification::centredLeft : i == segments ? Justification::centredRight : Justification::centred;
        const double time = visibleStart / audio.sampleRate + fraction * lengthSeconds;
        g.drawText (String (time, lengthSeconds < 0.02 ? 4 : 3) + " s", left, w.getBottom(), labelWidth, 20, alignment);
    }
}

void WaveformView::paint (Graphics& g)
{
    const auto skin = hammer::skinFor (*this);
    const auto accent = hammer::accentColour (skin);
    hammer::artwork::drawWaveformPanel (g, getLocalBounds(), skin);
    const auto w = waveArea();
    g.setColour (Colour (0xff101210));
    g.fillRoundedRectangle (w.toFloat(), 5.0f);
    g.setColour (hammer::edge);
    g.drawRoundedRectangle (w.toFloat().reduced (0.5f), 5.0f, 1.0f);
    if (visibleLength <= 0.0 || w.isEmpty())
    {
        g.setColour (hammer::muted);
        g.setFont (hammer::font (14.0f));
        g.drawText ("Drop an audio file here", w, Justification::centred);
        return;
    }

    paintTimeline (g, w);
    const Graphics::ScopedSaveState state (g);
    if (! g.reduceClipRegion (w.reduced (1))) return;

    const auto& markers = dragIndex >= 0 ? dragSlices.getMarkers() : asset->slices.getMarkers();
    const int numSlices = (int) markers.size() + 1;
    const int selected = plotState.selectedSlice % numSlices;
    const int playing = plotState.playingSlice;
    const bool isPlaying = isPositiveAndBelow (playing, numSlices);
    const int scanSlice = isPlaying ? playing : selected;
    const auto frames = (double) asset->audio->frames();
    const auto startOf = [&] (int i) { return i == 0 ? 0.0 : (double) markers[(size_t) i - 1]; };
    const auto endOf = [&] (int i) { return i == numSlices - 1 ? frames : (double) markers[(size_t) i]; };
    const double sliceStart = startOf (scanSlice), sliceEnd = endOf (scanSlice);
    const auto xInPlot = [this, w] (double frame) { return jlimit ((float) w.getX(), (float) w.getRight(), frameToX (frame)); };

    if (numSlices > 1)
    {
        const float x0 = xInPlot (startOf (selected)), x1 = xInPlot (endOf (selected));
        g.setColour (accent.withAlpha (0.15f));
        g.fillRect (x0, (float) w.getY(), x1 - x0, (float) w.getHeight());
        if (isPlaying)
        {
            const float activeLeft = xInPlot (startOf (playing)), activeRight = xInPlot (endOf (playing));
            g.setColour (hammer::text.withAlpha (0.85f));
            g.fillRect (activeLeft, (float) w.getY() + 1.0f, activeRight - activeLeft, 2.0f);
        }
    }

    g.setColour (hammer::edge.withAlpha (0.5f));
    for (int i = 1; i <= 3; ++i)
        g.drawHorizontalLine (w.getY() + i * w.getHeight() / 4, (float) w.getX(), (float) w.getRight());

    const double scan = plotState.scan * 0.01;
    const double length = plotState.length * 0.01 * (sliceEnd - sliceStart);
    const double windowStart = sliceStart + scan * (sliceEnd - sliceStart - length);
    const float sx0 = xInPlot (windowStart), sx1 = xInPlot (windowStart + length);
    g.setColour (accent.withAlpha (0.35f));
    g.fillRect (sx0, (float) w.getBottom() - 4.0f, sx1 - sx0, 3.0f);

    paintSignal (g, w);

    const double visibleEnd = visibleStart + visibleLength;
    constexpr float dashes[] { 4.0f, 3.0f };
    for (size_t i = 0; i < markers.size(); ++i)
    {
        const auto frame = (double) markers[i];
        if (frame < visibleStart || frame >= visibleEnd) continue;
        const float x = frameToX (frame);
        const auto handle = Rectangle<float> (12.0f, 15.0f).withCentre ({ x, (float) w.getBottom() - 10.0f });
        g.setColour (hammer::text.withAlpha (0.72f));
        g.drawDashedLine ({ x, (float) w.getY(), x, handle.getY() }, dashes, 2, 1.0f);
        g.setColour ((int) i == dragIndex ? accent : hammer::muted);
        g.fillRoundedRectangle (handle, 2.5f);
        g.setColour (hammer::ink);
        for (int offset : { -2, 0, 2 })
            g.drawVerticalLine (roundToInt (x) + offset, handle.getY() + 4.0f, handle.getBottom() - 4.0f);
    }

    const double position = plotState.position * frames;
    if (position >= visibleStart && position < visibleEnd)
    {
        g.setColour (hammer::text);
        const float x = frameToX (position);
        g.drawLine (x, (float) w.getY(), x, (float) w.getBottom(), 1.0f);
    }

    String sliceText = "SLICE " + String (selected + 1) + "/" + String (numSlices);
    const bool showPlaying = isPlaying && playing != selected;
    if (showPlaying) sliceText << "  /  PLAY " << playing + 1;
    const auto sliceReadout = w.reduced (8).removeFromTop (19).removeFromRight (showPlaying ? 190 : 115);
    g.setColour (hammer::recess.withAlpha (0.88f));
    g.fillRoundedRectangle (sliceReadout.toFloat(), 3.0f);
    g.setColour (accent);
    g.setFont (hammer::mono (12.0f, Font::bold));
    g.drawText (sliceText, sliceReadout, Justification::centred);
}

int WaveformView::markerAt (float x) const
{
    const double frame = xToFrame (x);
    const double tolerance = 6.0 * visibleLength / waveArea().getWidth();
    const int index = asset->slices.findMarker (std::llround (frame), (int64_t) std::ceil (tolerance));
    if (index < 0) return -1;
    const auto marker = (double) asset->slices.getMarkers()[(size_t) index];
    return marker >= visibleStart && marker < visibleStart + visibleLength && std::abs (marker - frame) <= tolerance ? index : -1;
}

void WaveformView::mouseMove (const MouseEvent& e)
{
    if (visibleLength <= 0.0 || ! waveArea().contains (e.getPosition()) || asset->slices.getMarkers().empty())
    {
        setMouseCursor (MouseCursor::NormalCursor);
        return;
    }
    setMouseCursor (markerAt (e.position.x) >= 0 ? MouseCursor::LeftRightResizeCursor : MouseCursor::PointingHandCursor);
}

void WaveformView::mouseDown (const MouseEvent& e)
{
    updateAsset();
    selectOnRelease = false;
    if (visibleLength <= 0.0 || ! waveArea().contains (e.getPosition())) return;
    selectOnRelease = e.mods.isLeftButtonDown() && ! e.mods.isPopupMenu();
    const int index = markerAt (e.position.x);
    if (index < 0) return;
    if (e.mods.isPopupMenu())
    {
        auto slices = asset->slices;
        slices.remove (index);
        proc.setMarkers (*asset, slices.getMarkers());
    }
    else if (selectOnRelease)
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

void WaveformView::mouseUp (const MouseEvent& e)
{
    updateAsset();
    const bool select = std::exchange (selectOnRelease, false) && waveArea().contains (e.getPosition())
                     && ! e.mouseWasDraggedSinceMouseDown() && e.getNumberOfClicks() == 1;
    if (const int marker = std::exchange (dragIndex, -1); marker >= 0)
    {
        if (dragSlices.getMarkers() != asset->slices.getMarkers())
            proc.setMarkers (*asset, dragSlices.getMarkers());
        else if (select)
            selectSlice (marker + 1);
        repaint();
    }
    else if (select)
    {
        const auto& markers = asset->slices.getMarkers();
        const auto frame = (int64_t) std::floor (xToFrame (e.position.x));
        selectSlice ((int) std::distance (markers.begin(), std::upper_bound (markers.begin(), markers.end(), frame)));
    }
}

void WaveformView::selectSlice (int index)
{
    auto* parameter = proc.apvts.getParameter ("slice");
    const float value = parameter->convertTo0to1 ((float) index);
    if (approximatelyEqual (value, parameter->getValue())) return;
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (value);
    parameter->endChangeGesture();
    plotState.selectedSlice = index;
    repaint (waveArea());
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
    updateStatus();
    const auto& telemetry = proc.telemetry();
    const auto raw = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    const auto active = telemetry.active.load();
    const PlotState next { active ? telemetry.playPos.load() : -1.0f, raw ("scan"), raw ("length"),
                           (int) raw ("slice"), active ? telemetry.slice.load() : -1 };
    if (next.selectedSlice != plotState.selectedSlice || next.playingSlice != plotState.playingSlice
        || ! approximatelyEqual (next.position, plotState.position)
        || ! approximatelyEqual (next.scan, plotState.scan) || ! approximatelyEqual (next.length, plotState.length))
    {
        plotState = next;
        repaint (waveArea());
    }
}
