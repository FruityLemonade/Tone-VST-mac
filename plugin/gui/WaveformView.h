#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

#include "../SampleLoader.h"

class XtoneProcessor;

/// Waveform with slice markers, highlighted current slice, scan window and play position.
/// Double click: add marker, drag: move, right click: delete.
class WaveformView : public juce::Component, private juce::Timer, private juce::ScrollBar::Listener
{
public:
    explicit WaveformView (XtoneProcessor&);
    ~WaveformView() override;

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
    void updateStatus();
    void chooseSample();
    void showSliceMenu();
    void startDetection();
    void setViewport (double start, double length);
    void zoomAt (double factor, double anchor);
    void paintSignal (juce::Graphics&, juce::Rectangle<int>) const;
    void paintTimeline (juce::Graphics&, juce::Rectangle<int>) const;
    juce::Rectangle<int> plotArea() const;
    juce::Rectangle<int> waveArea() const;
    bool usesTwoToolbarRows() const;
    double xToFrame (float x) const;
    float frameToX (double frame) const;

    XtoneProcessor& proc;
    juce::TextButton loadSample { juce::String::fromUTF8 ("Load Sample…") }, normalize { "Normalize" };
    juce::TextButton transientButton { "Transients" }, sliceButton { juce::String::fromUTF8 ("Slices ▾") };
    juce::TextButton zoomOutButton { juce::String::fromUTF8 ("−") }, zoomInButton { "+" }, resetZoomButton { "Fit" };
    juce::Slider sensitivity { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Label sampleName, sensitivityLabel, status;
    juce::ScrollBar scrollbar { false };
    std::unique_ptr<juce::FileChooser> chooser;

    struct PlotState
    {
        float position = -1.0f, scan = 0.0f, length = 100.0f;
        int slice = 0;
    };

    xtp::SampleAsset::Ptr asset;
    PlotState plotState;
    juce::String detectionMessage;
    bool sliceMenuOpen = false;
    double visibleStart { 0.0 }, visibleLength { 0.0 };
    int dragIndex { -1 };
    xt::SliceModel dragSlices;
};
