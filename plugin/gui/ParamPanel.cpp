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
                addAndMakeVisible (controls.add (new ParameterControl (*rp, state)));
    }

    void layoutFor (int width)
    {
        constexpr int ch = 35;
        const int cols = jmax (1, width / 258);
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
