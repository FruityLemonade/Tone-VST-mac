#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "HammerLookAndFeel.h"
#include "HeaderBar.h"
#include "ParamPanel.h"
#include "RatePanel.h"
#include "WaveformView.h"

class XtoneProcessor;

class XtoneEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget
{
public:
    explicit XtoneEditor (XtoneProcessor&);
    ~XtoneEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray&, int x, int y) override;

private:
    void selectGroup (int);

    XtoneProcessor& proc;
    hammer::LookAndFeel theme;
    HeaderBar header;
    WaveformView waveform;
    RatePanel rate;
    ParamPanel params;
    juce::OwnedArray<juce::TextButton> keys;
    juce::Rectangle<int> keyWell;
    juce::TooltipWindow tooltips { this };
};
