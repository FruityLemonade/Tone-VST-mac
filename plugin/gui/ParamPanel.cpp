#include "ParamPanel.h"

#include "HammerLookAndFeel.h"

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
        label.setFont (hammer::font (13.0f));
        label.setBorderSize (BorderSize<int> (1, 1, 1, 1));
        addAndMakeVisible (label);

        const String id = p.getParameterID();
        if (auto* choice = dynamic_cast<AudioParameterChoice*> (&p))
        {
            combo = std::make_unique<ComboBox>();
            combo->addItemList (choice->choices, 1);
            combo->setTitle (label.getText());
            addAndMakeVisible (*combo);
            comboAtt = std::make_unique<APVTS::ComboBoxAttachment> (state, id, *combo);
        }
        else if (dynamic_cast<AudioParameterBool*> (&p) != nullptr)
        {
            toggle = std::make_unique<ToggleButton> ("On");
            toggle->setTitle (label.getText());
            addAndMakeVisible (*toggle);
            buttonAtt = std::make_unique<APVTS::ButtonAttachment> (state, id, *toggle);
        }
        else
        {
            slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
            slider->setTextBoxStyle (Slider::TextBoxBelow, false, 90, 22);
            slider->setName (label.getText());
            slider->setPopupDisplayEnabled (false, false, nullptr);
            addAndMakeVisible (*slider);
            sliderAtt = std::make_unique<APVTS::SliderAttachment> (state, id, *slider);
            slider->setDoubleClickReturnValue (true, (double) p.convertFrom0to1 (p.getDefaultValue()));
            slider->setTooltip (p.getName (128));
        }
    }

    void paint (Graphics& g) override
    {
        const auto card = getLocalBounds().toFloat().reduced (2.5f);
        g.setColour (hammer::recess);
        g.fillRoundedRectangle (card, 5.0f);
        g.setColour (hammer::edge);
        g.drawRoundedRectangle (card, 5.0f, 1.0f);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (8, 6);
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
        constexpr int ch = 117;
        const int cols = jlimit (1, 5, width / 103);
        const int rows = (controls.size() + cols - 1) / cols;
        setSize (width, jmax (ch, rows * ch));
        for (int i = 0; i < controls.size(); ++i)
        {
            const int col = i % cols;
            const int x = col * width / cols;
            controls[i]->setBounds (x, (i / cols) * ch, (col + 1) * width / cols - x, ch);
        }
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
    const auto groups = p.getParameterTree().getSubgroups (false);
    for (int index = 1; index < groups.size(); ++index)
        addChildComponent (pages.add (new ScrollingPage (*groups[index], state)));
}

void ParamPanel::showGroup (int index)
{
    const int next = index - 1;
    if (next == selected) return;
    if (auto* current = pages[selected]) current->setVisible (false);
    selected = next;
    if (auto* page = pages[selected]) page->setVisible (true);
    resized();
}

void ParamPanel::paint (Graphics& g)
{
    hammer::drawRecess (g, getLocalBounds().toFloat());
}

void ParamPanel::resized()
{
    if (auto* page = pages[selected])
        page->setBounds (getLocalBounds().reduced (8));
}
