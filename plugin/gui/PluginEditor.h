#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include "HeaderBar.h"
#include "ParamPanel.h"
#include "WaveformView.h"

class XtoneProcessor;

class XtoneEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget
{
public:
    explicit XtoneEditor (XtoneProcessor& p);

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    XtoneProcessor& proc;
    HeaderBar header;
    WaveformView waveform;
    ParamPanel params;
    juce::TooltipWindow tooltips { this };
};
