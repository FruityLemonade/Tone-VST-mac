#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class XtoneProcessor;

/// Waveform with slice markers, highlighted current slice, scan window and play position.
/// Double click: add marker, drag: move, right click: delete.
class WaveformView : public juce::Component, private juce::Timer
{
public:
    explicit WaveformView (XtoneProcessor& p);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    juce::Rectangle<int> waveArea() const;
    double xToFrame (float x) const;
    float frameToX (double frame) const;

    XtoneProcessor& proc;
    juce::TextButton equalButton { juce::String::fromUTF8 ("Gleichmäßig") }, clearButton { juce::String::fromUTF8 ("Marker löschen") };
    juce::Slider equalCount { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };

    int dragIndex { -1 };
    std::vector<int64_t> dragMarkers;
    float lastPos { -1.0f };
    int lastVersion { -1 };
};
