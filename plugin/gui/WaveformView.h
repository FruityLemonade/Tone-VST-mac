#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "../SampleLoader.h"

class XtoneProcessor;

/// Waveform with slice markers, highlighted current slice, scan window and play position.
/// Double click: add marker, drag: move, right click: delete.
class WaveformView : public juce::Component, private juce::Timer, private juce::ScrollBar::Listener
{
public:
    explicit WaveformView (XtoneProcessor& p);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    void timerCallback() override;
    void scrollBarMoved (juce::ScrollBar*, double start) override;
    void updateAsset();
    void startDetection();
    void setViewport (double start, double length);
    void zoomAt (double factor, double anchor);
    void paintSignal (juce::Graphics&, juce::Rectangle<int>) const;
    juce::Rectangle<int> plotArea() const;
    juce::Rectangle<int> waveArea() const;
    double xToFrame (float x) const;
    float frameToX (double frame) const;

    XtoneProcessor& proc;
    juce::TextButton equalButton { juce::String::fromUTF8 ("Gleichmäßig") };
    juce::TextButton transientButton { "Transienten" }, clearButton { juce::String::fromUTF8 ("Marker löschen") };
    juce::TextButton zoomOutButton { "-" }, zoomInButton { "+" }, resetZoomButton { "Alles" };
    juce::Slider equalCount { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::Slider sensitivity { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label sensitivityLabel, detectionStatus;
    juce::ScrollBar scrollbar { false };

    xtp::SampleAsset::Ptr asset;
    double visibleStart { 0.0 }, visibleLength { 0.0 };
    int dragIndex { -1 };
    xt::SliceModel dragSlices;
    float lastPos { -1.0f };
};
