#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/// Scrollable pages for the parameter groups without a dedicated editor.
class ParamPanel : public juce::Component
{
public:
    ParamPanel (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&);
    void showGroup (int index);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::OwnedArray<juce::Component> pages;
    int selected = -1;
};
