#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class ParameterControl final : public juce::Component
{
public:
    ParameterControl (juce::RangedAudioParameter&, juce::AudioProcessorValueTreeState&,
                      juce::String caption = {}, bool stacked = false);
    void resized() override;

private:
    using State = juce::AudioProcessorValueTreeState;
    juce::Label label;
    bool stacked;
    std::unique_ptr<juce::Slider> value;
    std::unique_ptr<juce::ComboBox> choice;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<State::SliderAttachment> valueAttachment;
    std::unique_ptr<State::ComboBoxAttachment> choiceAttachment;
    std::unique_ptr<State::ButtonAttachment> toggleAttachment;
};
