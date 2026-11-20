#include "PluginEditor.h"

#include "../PluginProcessor.h"

using namespace juce;

XtoneEditor::XtoneEditor (XtoneProcessor& p)
    : AudioProcessorEditor (p), proc (p), header (p), waveform (p), rate (p), params (p, p.apvts)
{
    addAndMakeVisible (header);
    addAndMakeVisible (waveform);
    addAndMakeVisible (rate);
    addChildComponent (params);

    const auto groups = p.getParameterTree().getSubgroups (false);
    for (int i = 0; i < groups.size(); ++i)
    {
        auto* key = keys.add (new TextButton (groups[i]->getName()));
        key->getProperties().set ("key", true);
        key->setRadioGroupId (2, dontSendNotification);
        key->setClickingTogglesState (true);
        key->onClick = [this, key, i]
        {
            if (key->getToggleState()) selectGroup (i);
        };
        addAndMakeVisible (key);
    }

    setLookAndFeel (&theme);
    selectGroup (0);
    setResizable (true, true);
    setResizeLimits (760, 560, 2400, 1800);
    setSize (1000, 720);
}

XtoneEditor::~XtoneEditor()
{
    setLookAndFeel (nullptr);
}

void XtoneEditor::selectGroup (int index)
{
    keys[index]->setToggleState (true, dontSendNotification);
    rate.setVisible (index == 0);
    params.setVisible (index != 0);
    params.showGroup (index);
}

void XtoneEditor::paint (Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll (hammer::ink);
    const auto outer = bounds.reduced (1.0f);
    g.setGradientFill (ColourGradient (Colour (0xff55564e), outer.getTopLeft(), hammer::recess, outer.getBottomRight(), false));
    g.fillPath (hammer::cutCorners (outer, 19.0f));

    hammer::drawMoulding (g, bounds.reduced (8.0f), 20.0f);

    for (const bool right : { false, true })
    {
        const float x = right ? bounds.getRight() - 17.0f : bounds.getX() + 17.0f;
        hammer::drawFastener (g, { x, 28.0f }, 7.2f);
        hammer::drawFastener (g, { x, bounds.getBottom() - 29.0f }, 7.2f);

        const auto grip = Rectangle<float> (25.0f, jmin (bounds.getHeight() * 0.53f, 470.0f))
                              .withCentre ({ right ? bounds.getRight() - 11.0f : bounds.getX() + 11.0f, bounds.getCentreY() });
        hammer::drawGrip (g, grip, right);
    }

    hammer::drawRecess (g, keyWell.toFloat());
}

void XtoneEditor::resized()
{
    auto content = getLocalBounds().reduced (28, 20);
    if (content.isEmpty()) return;
    header.setBounds (content.removeFromTop (57));
    content.removeFromTop (10);
    keyWell = content.removeFromBottom (45);
    auto keyRow = keyWell.reduced (7, 7);
    constexpr auto gap = 3;
    int totalWeight = 0;
    for (auto* key : keys) totalWeight += key->getButtonText().length() + 5;
    int space = keyRow.getWidth() - (keys.size() - 1) * gap;
    for (auto* key : keys)
    {
        const int weight = key->getButtonText().length() + 5;
        const int width = space * weight / totalWeight;
        key->setBounds (keyRow.removeFromLeft (width));
        keyRow.removeFromLeft (gap);
        space -= width;
        totalWeight -= weight;
    }

    content.removeFromBottom (10);
    auto left = content.removeFromLeft (content.getWidth() * 61 / 100);
    waveform.setBounds (left);
    content.removeFromLeft (10);
    rate.setBounds (content);
    params.setBounds (content);
}

bool XtoneEditor::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) return true;
    return false;
}

void XtoneEditor::filesDropped (const StringArray& files, int, int)
{
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3"))
        {
            proc.loadSampleFile (File (f));
            return;
        }
}
