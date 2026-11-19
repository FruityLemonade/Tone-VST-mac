#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

/// Generic controls for all parameters, one tab per parameter group.
class ParamPanel : public juce::Component
{
public:
    ParamPanel (juce::AudioProcessor& p, juce::AudioProcessorValueTreeState& state);
    void resized() override;

private:
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };
};
