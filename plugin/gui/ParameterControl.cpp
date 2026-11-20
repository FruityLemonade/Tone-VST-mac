#include "ParameterControl.h"

#include "HammerLookAndFeel.h"
#include "SampleView.h"

#include <utility>

using namespace juce;

ParameterControl::ParameterControl (RangedAudioParameter& parameter, State& state, String caption, bool stack, Style form)
    : style (form), stacked (stack || style == Style::knob)
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
    if (style == Style::knob) label.setJustificationType (Justification::centred);
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

    value = std::make_unique<Slider> (style == Style::knob ? Slider::RotaryHorizontalVerticalDrag : Slider::LinearBar,
                                      style == Style::knob ? Slider::TextBoxBelow : Slider::TextBoxLeft);
    value->setName (name);
    value->setTooltip (name + ". Drag to adjust or click the value to enter it.");
    value->setScrollWheelEnabled (false);
    addAndMakeVisible (*value);
    if (style != Style::slice)
    {
        valueAttachment = std::make_unique<State::SliderAttachment> (state, id, *value);
        return;
    }

    onlySlice = std::make_unique<Label> (String(), "1");
    onlySlice->setJustificationType (Justification::centred);
    onlySlice->setColour (Label::backgroundColourId, hammer::recess);
    onlySlice->setColour (Label::outlineColourId, hammer::edge);
    onlySlice->setTitle (name);
    addChildComponent (*onlySlice);
    sliceAttachment = std::make_unique<ParameterAttachment> (parameter, [this] (float index)
    {
        storedSlice = roundToInt (index);
        value->setValue (xt::wrapSliceIndex (storedSlice, sliceCount) + 1, dontSendNotification);
    });
    value->onDragStart = [this] { sliceGesture = true; sliceAttachment->beginGesture(); };
    value->onDragEnd = [this] { if (std::exchange (sliceGesture, false)) sliceAttachment->endGesture(); };
    value->onValueChange = [this]
    {
        if (sliceCount <= 1) return;
        const auto index = (float) value->getValue() - 1.0f;
        if (sliceGesture) sliceAttachment->setValueAsPartOfGesture (index);
        else sliceAttachment->setValueAsCompleteGesture (index);
    };
    setSliceCount (1);
    sliceAttachment->sendInitialUpdate();
}

ParameterControl::~ParameterControl()
{
    if (sliceGesture) sliceAttachment->endGesture();
}

void ParameterControl::setSliceCount (int count)
{
    if (sliceAttachment == nullptr || count == sliceCount) return;
    sliceCount = count;
    const bool multiple = count > 1;
    if (multiple) value->setRange (1.0, (double) count, 1.0);
    value->setValue (xt::wrapSliceIndex (storedSlice, count) + 1, dontSendNotification);
    value->setVisible (multiple);
    onlySlice->setVisible (! multiple);
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

    if (value != nullptr)
    {
        if (style == Style::knob) value->setTextBoxStyle (Slider::TextBoxBelow, false, jmin (92, r.getWidth()), 18);
        value->setBounds (r);
    }
    if (onlySlice != nullptr) onlySlice->setBounds (r);
    if (choice != nullptr) choice->setBounds (r);
}
