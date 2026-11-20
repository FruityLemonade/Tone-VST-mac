#include "ParameterControl.h"

#include "HammerLookAndFeel.h"

using namespace juce;

ParameterControl::ParameterControl (RangedAudioParameter& parameter, State& state, String caption, bool stack)
    : stacked (stack)
{
    const auto name = parameter.getName (128);
    if (caption.isEmpty()) caption = name;
    const auto id = parameter.getParameterID();

    if (dynamic_cast<AudioParameterBool*> (&parameter) != nullptr)
    {
        toggle = std::make_unique<ToggleButton> (caption);
        toggle->setTitle (name);
        toggle->setTooltip (name);
        addAndMakeVisible (*toggle);
        toggleAttachment = std::make_unique<State::ButtonAttachment> (state, id, *toggle);
        return;
    }

    label.setText (caption, dontSendNotification);
    label.setFont (hammer::font (12.5f, Font::bold));
    label.setMinimumHorizontalScale (0.78f);
    label.setBorderSize (BorderSize<int> (0));
    label.setTooltip (name);
    addAndMakeVisible (label);

    if (auto* p = dynamic_cast<AudioParameterChoice*> (&parameter))
    {
        choice = std::make_unique<ComboBox>();
        choice->addItemList (p->choices, 1);
        choice->setTitle (name);
        choice->setTooltip (name);
        addAndMakeVisible (*choice);
        choiceAttachment = std::make_unique<State::ComboBoxAttachment> (state, id, *choice);
        return;
    }

    value = std::make_unique<Slider> (Slider::LinearBar, Slider::TextBoxLeft);
    value->setName (name);
    value->setTooltip (name + ". Drag to adjust or click the value to enter it.");
    value->setScrollWheelEnabled (false);
    addAndMakeVisible (*value);
    valueAttachment = std::make_unique<State::SliderAttachment> (state, id, *value);
}

void ParameterControl::resized()
{
    auto r = getLocalBounds().reduced (4, 2);
    if (toggle != nullptr)
    {
        toggle->setBounds (r);
        return;
    }

    if (stacked)
        label.setBounds (r.removeFromTop (15));
    else
        label.setBounds (r.removeFromLeft (r.getWidth() * 47 / 100).withTrimmedRight (6));

    if (value != nullptr) value->setBounds (r);
    if (choice != nullptr) choice->setBounds (r);
}
