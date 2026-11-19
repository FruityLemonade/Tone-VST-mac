#include "WaveformView.h"

#include "../PluginProcessor.h"

using namespace juce;

WaveformView::WaveformView (XtoneProcessor& p) : proc (p)
{
    equalCount.setRange (2, 64, 1);
    equalCount.setValue (8, dontSendNotification);
    equalCount.setTextBoxStyle (Slider::TextBoxLeft, false, 40, 22);
    addAndMakeVisible (equalCount);
    addAndMakeVisible (equalButton);
    addAndMakeVisible (clearButton);

    equalButton.onClick = [this]
    {
        if (auto a = proc.getCurrentAsset())
        {
            xt::SliceModel m;
            m.setLength (a->audio ? a->audio->frames() : 0);
            m.setEqual ((int) equalCount.getValue());
            proc.setMarkers (m.getMarkers());
        }
    };
    clearButton.onClick = [this] { proc.setMarkers ({}); };
    startTimerHz (30);
}

Rectangle<int> WaveformView::waveArea() const { return getLocalBounds().withTrimmedRight (170).reduced (2); }

void WaveformView::resized()
{
    auto r = getLocalBounds().removeFromRight (165).reduced (4);
    auto row = r.removeFromTop (26);
    equalButton.setBounds (row.removeFromLeft (90));
    equalCount.setBounds (row.withTrimmedLeft (4));
    r.removeFromTop (6);
    clearButton.setBounds (r.removeFromTop (26));
}

double WaveformView::xToFrame (float x) const
{
    auto a = proc.getCurrentAsset();
    if (a == nullptr || a->audio == nullptr) return 0.0;
    const auto w = waveArea();
    return jlimit (0.0, (double) a->audio->frames(), (double) (x - (float) w.getX()) / (double) w.getWidth() * (double) a->audio->frames());
}

float WaveformView::frameToX (double frame) const
{
    auto a = proc.getCurrentAsset();
    const auto w = waveArea();
    if (a == nullptr || a->audio == nullptr || a->audio->frames() == 0) return (float) w.getX();
    return (float) w.getX() + (float) (frame / (double) a->audio->frames()) * (float) w.getWidth();
}

void WaveformView::paint (Graphics& g)
{
    const auto w = waveArea();
    g.setColour (Colours::black.withAlpha (0.6f));
    g.fillRect (w);

    auto a = proc.getCurrentAsset();
    if (a == nullptr || a->audio == nullptr || a->peaks == nullptr || a->audio->frames() == 0) return;

    const auto& tel = proc.telemetry();
    const auto& markers = dragIndex >= 0 ? dragMarkers : a->slices.getMarkers();
    std::vector<int64_t> bounds { 0 };
    bounds.insert (bounds.end(), markers.begin(), markers.end());
    bounds.push_back (a->audio->frames());
    const int numSlices = (int) bounds.size() - 1;

    // current slice + scan window
    const int cur = jlimit (0, numSlices - 1, tel.slice.load());
    const float x0 = frameToX ((double) bounds[(size_t) cur]), x1 = frameToX ((double) bounds[(size_t) cur + 1]);
    g.setColour (Colours::orange.withAlpha (0.12f));
    g.fillRect (x0, (float) w.getY(), x1 - x0, (float) w.getHeight());
    {
        const double scan = proc.apvts.getRawParameterValue ("scan")->load() * 0.01;
        const double len = proc.apvts.getRawParameterValue ("length")->load() * 0.01;
        const double sl = (double) (bounds[(size_t) cur + 1] - bounds[(size_t) cur]);
        const double ws = (double) bounds[(size_t) cur] + scan * (sl - len * sl);
        const float sx0 = frameToX (ws), sx1 = frameToX (ws + len * sl);
        g.setColour (Colours::orange.withAlpha (0.18f));
        g.fillRect (sx0, (float) w.getY() + (float) w.getHeight() * 0.85f, jmax (1.0f, sx1 - sx0), (float) w.getHeight() * 0.15f);
    }

    // peaks
    const auto& pk = *a->peaks;
    const int numPeaks = (int) pk.size() / 2;
    const float midY = (float) w.getCentreY(), halfH = (float) w.getHeight() * 0.48f;
    g.setColour (Colours::lightgreen);
    for (int px = 0; px < w.getWidth(); ++px)
    {
        const int i0 = (int) ((int64) px * numPeaks / w.getWidth());
        const int i1 = jmax (i0 + 1, (int) ((int64) (px + 1) * numPeaks / w.getWidth()));
        float lo = 0.0f, hi = 0.0f;
        for (int i = i0; i < jmin (i1, numPeaks); ++i)
        {
            lo = jmin (lo, pk[(size_t) i * 2]);
            hi = jmax (hi, pk[(size_t) i * 2 + 1]);
        }
        g.drawVerticalLine (w.getX() + px, midY - hi * halfH, midY - lo * halfH + 1.0f);
    }

    // markers
    g.setColour (Colours::yellow);
    for (size_t i = 0; i < markers.size(); ++i)
    {
        const float x = frameToX ((double) markers[i]);
        g.drawLine (x, (float) w.getY(), x, (float) w.getBottom(), 1.5f);
        g.drawText (String ((int) i + 1), (int) x + 2, w.getY() + 2, 30, 14, Justification::left);
    }

    // play position
    const float pos = tel.playPos.load();
    if (pos >= 0.0f)
    {
        g.setColour (Colours::white);
        const float x = (float) w.getX() + pos * (float) w.getWidth();
        g.drawLine (x, (float) w.getY(), x, (float) w.getBottom(), 1.0f);
    }

    g.setColour (Colours::white.withAlpha (0.8f));
    g.setFont (FontOptions (13.0f));
    g.drawText (a->name + "  (" + String (numSlices) + " Slices)", w.reduced (6), Justification::bottomLeft);
}

void WaveformView::mouseDown (const MouseEvent& e)
{
    auto a = proc.getCurrentAsset();
    if (a == nullptr || ! waveArea().contains (e.getPosition())) return;
    const int64 tol = (int64) (xToFrame ((float) e.x + 4.0f) - xToFrame ((float) e.x)) + 1;
    const int idx = a->slices.findMarker ((int64) xToFrame ((float) e.x), tol);
    if (idx < 0) return;
    if (e.mods.isPopupMenu())
    {
        auto m = a->slices.getMarkers();
        m.erase (m.begin() + idx);
        proc.setMarkers (std::move (m));
        return;
    }
    dragIndex = idx;
    dragMarkers = a->slices.getMarkers();
}

void WaveformView::mouseDrag (const MouseEvent& e)
{
    if (dragIndex < 0) return;
    auto a = proc.getCurrentAsset();
    if (a == nullptr) return;
    xt::SliceModel m;
    m.setLength (a->audio->frames());
    m.setMarkers (dragMarkers);
    m.move (dragIndex, (int64) xToFrame ((float) e.x));
    dragMarkers = m.getMarkers();
    repaint();
}

void WaveformView::mouseUp (const MouseEvent&)
{
    if (dragIndex < 0) return;
    dragIndex = -1;
    proc.setMarkers (dragMarkers);
}

void WaveformView::mouseDoubleClick (const MouseEvent& e)
{
    auto a = proc.getCurrentAsset();
    if (a == nullptr || ! waveArea().contains (e.getPosition())) return;
    xt::SliceModel m;
    m.setLength (a->audio->frames());
    m.setMarkers (a->slices.getMarkers());
    if (m.add ((int64) xToFrame ((float) e.x))) proc.setMarkers (m.getMarkers());
}

void WaveformView::timerCallback()
{
    const float pos = proc.telemetry().playPos.load();
    const int v = proc.getAssetVersion();
    if (std::abs (pos - lastPos) > 1.0e-6f || v != lastVersion || proc.telemetry().active.load())
    {
        lastPos = pos;
        lastVersion = v;
        repaint();
    }
}
