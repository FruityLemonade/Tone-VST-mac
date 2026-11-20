#include "ParamPanel.h"

#include "HammerLookAndFeel.h"
#include "ModulationPanels.h"
#include "ParameterControl.h"
#include "../PluginProcessor.h"

using namespace juce;

namespace
{
using APVTS = AudioProcessorValueTreeState;

class GroupPage : public Component
{
public:
    GroupPage (const AudioProcessorParameterGroup& g, APVTS& state)
    {
        for (auto* p : g.getParameters (true))
            if (auto* rp = dynamic_cast<RangedAudioParameter*> (p))
            {
                using Style = ParameterControl::Style;
                const auto id = rp->getParameterID();
                const bool knob = id == "transpose" || id == "fine" || id == "s_pitch" || id == "penv_amt"
                               || id == "ab_pitch" || id == "drive" || id == "out_gain";
                addAndMakeVisible (controls.add (new ParameterControl (*rp, state, {}, false, knob ? Style::knob : Style::bar)));
            }
    }

    void layoutFor (int width)
    {
        const int cols = jmax (1, width / 258);
        int y = 0;
        for (int first = 0; first < controls.size(); first += cols)
        {
            int height = 35;
            const int count = jmin (cols, controls.size() - first);
            for (int col = 0; col < count; ++col)
                height = jmax (height, controls[first + col]->preferredHeight());

            for (int col = 0; col < count; ++col)
            {
                const int x = col * width / cols;
                auto* control = controls[first + col];
                const int controlHeight = control->preferredHeight();
                control->setBounds (x, y + (height - controlHeight) / 2, (col + 1) * width / cols - x, controlHeight);
            }
            y += height;
        }
        setSize (width, y);
    }

private:
    OwnedArray<ParameterControl> controls;
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

ParamPanel::ParamPanel (XtoneProcessor& p)
{
    const auto groups = p.getParameterTree().getSubgroups (false);
    for (int index = 1; index < groups.size(); ++index)
    {
        const auto& group = *groups[index];
        const auto id = group.getID();
        if (id == "env")
            addChildComponent (pages.add (hammer::makeEnvelopePanel (p).release()));
        else if (id == "lfo1" || id == "lfo2")
            addChildComponent (pages.add (hammer::makeLfoPanel (p.apvts, id).release()));
        else
            addChildComponent (pages.add (new ScrollingPage (group, p.apvts)));
    }
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
