#include "ParamPanel.h"

using namespace juce;

namespace
{
using APVTS = AudioProcessorValueTreeState;

/// Label + matching control + attachment for one parameter.
class ParamControl : public Component
{
public:
    ParamControl (RangedAudioParameter& p, APVTS& state)
    {
        label.setText (p.getName (64), dontSendNotification);
        label.setJustificationType (Justification::centred);
        label.setFont (FontOptions (13.0f));
        addAndMakeVisible (label);

        const String id = p.getParameterID();
        if (auto* choice = dynamic_cast<AudioParameterChoice*> (&p))
        {
            combo = std::make_unique<ComboBox>();
            combo->addItemList (choice->choices, 1);
            addAndMakeVisible (*combo);
            comboAtt = std::make_unique<APVTS::ComboBoxAttachment> (state, id, *combo);
        }
        else if (dynamic_cast<AudioParameterBool*> (&p) != nullptr)
        {
            toggle = std::make_unique<ToggleButton> ("an");
            addAndMakeVisible (*toggle);
            buttonAtt = std::make_unique<APVTS::ButtonAttachment> (state, id, *toggle);
        }
        else
        {
            slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
            slider->setTextBoxStyle (Slider::TextBoxBelow, false, 110, 18);
            slider->setPopupDisplayEnabled (false, false, nullptr);
            addAndMakeVisible (*slider);
            sliderAtt = std::make_unique<APVTS::SliderAttachment> (state, id, *slider);
            slider->setDoubleClickReturnValue (true, (double) p.convertFrom0to1 (p.getDefaultValue()));
            slider->setTooltip (p.getName (128));
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (4);
        label.setBounds (r.removeFromTop (30));
        if (slider) slider->setBounds (r);
        if (combo) combo->setBounds (r.withSizeKeepingCentre (r.getWidth(), 24));
        if (toggle) toggle->setBounds (r.withSizeKeepingCentre (60, 24));
    }

private:
    Label label;
    std::unique_ptr<Slider> slider;
    std::unique_ptr<ComboBox> combo;
    std::unique_ptr<ToggleButton> toggle;
    std::unique_ptr<APVTS::SliderAttachment> sliderAtt;
    std::unique_ptr<APVTS::ComboBoxAttachment> comboAtt;
    std::unique_ptr<APVTS::ButtonAttachment> buttonAtt;
};

class GroupPage : public Component
{
public:
    GroupPage (const AudioProcessorParameterGroup& g, APVTS& state)
    {
        for (auto* p : g.getParameters (true))
            if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
                addAndMakeVisible (controls.add (new ParamControl (*rp, state)));
    }

    void layoutFor (int width)
    {
        const int cw = 120, ch = 120;
        const int cols = jmax (1, width / cw);
        const int rows = (controls.size() + cols - 1) / cols;
        setSize (width, jmax (ch, rows * ch));
        for (int i = 0; i < controls.size(); ++i)
            controls[i]->setBounds ((i % cols) * cw, (i / cols) * ch, cw, ch);
    }

private:
    OwnedArray<ParamControl> controls;
};

class ScrollingPage : public Component
{
public:
    ScrollingPage (const AudioProcessorParameterGroup& g, APVTS& state) : page (g, state)
    {
        view.setViewedComponent (&page, false);
        view.setScrollBarsShown (true, false);
        addAndMakeVisible (view);
    }
    void resized() override
    {
        view.setBounds (getLocalBounds());
        page.layoutFor (getWidth() - view.getScrollBarThickness());
    }

private:
    GroupPage page;
    Viewport view;
};
} // namespace

ParamPanel::ParamPanel (AudioProcessor& p, APVTS& state)
{
    const auto bg = getLookAndFeel().findColour (ResizableWindow::backgroundColourId);
    for (auto* g : p.getParameterTree().getSubgroups (false))
        tabs.addTab (g->getName(), bg, new ScrollingPage (*g, state), true);
    addAndMakeVisible (tabs);
}

void ParamPanel::resized() { tabs.setBounds (getLocalBounds()); }
