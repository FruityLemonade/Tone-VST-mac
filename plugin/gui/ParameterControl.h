#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class ParameterControl final : public juce::Component
{
public:
    enum class Style { bar, knob };
    ParameterControl (juce::RangedAudioParameter&, juce::AudioProcessorValueTreeState&,
                      juce::String caption = {}, bool stacked = false, Style style = Style::bar);
    int preferredHeight() const noexcept { return style == Style::knob ? 96 : 35; }
    void resized() override;

private:
    using State = juce::AudioProcessorValueTreeState;
    juce::Label label;
    Style style;
    bool stacked;
    std::unique_ptr<juce::Slider> value;
    std::unique_ptr<juce::ComboBox> choice;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<State::SliderAttachment> valueAttachment;
    std::unique_ptr<State::ComboBoxAttachment> choiceAttachment;
    std::unique_ptr<State::ButtonAttachment> toggleAttachment;
};
