#include "PluginEditor.h"

#include "FaceplateArtwork.h"

#include "../PluginProcessor.h"

using namespace juce;

class XtoneEditor::AboutOverlay : public Component
{
public:
    explicit AboutOverlay (XtoneEditor& owner) : editor (owner)
    {
        setTitle ("About XTONE");
        setDescription ("Extratone Retrigger Sampler. Version " JucePlugin_VersionString ". " JucePlugin_Manufacturer);
        setWantsKeyboardFocus (true);
        setFocusContainerType (FocusContainerType::keyboardFocusContainer);
        close.onClick = [this] { dismiss(); };
        addAndMakeVisible (close);

        for (size_t i = 0; i < xtp::editorSkins.size(); ++i)
            skins.addItem (xtp::editorSkins[i].name, (int) i + 1);
        skins.setTitle ("Skin");
        skins.onChange = [this]
        {
            const int index = skins.getSelectedItemIndex();
            if (! isPositiveAndBelow (index, (int) xtp::editorSkins.size())) return;
            const auto skin = xtp::editorSkins[(size_t) index].skin;
            if (skin == editor.proc.getEditorSkin()) return;

            editor.proc.setEditorSkin (skin);
            editor.updateSkin();
            editor.proc.updateHostDisplay (AudioProcessorListener::ChangeDetails().withNonParameterStateChanged (true));
        };
        addAndMakeVisible (skins);
    }

    void show()
    {
        syncSelection();
        setVisible (true);
        toFront (true);
        skins.grabKeyboardFocus();
    }

    void syncSelection()
    {
        for (size_t i = 0; i < xtp::editorSkins.size(); ++i)
            if (xtp::editorSkins[i].skin == editor.theme.getSkin())
                skins.setSelectedItemIndex ((int) i, dontSendNotification);
    }

    void resized() override
    {
        card = getLocalBounds().withSizeKeepingCentre (352, 168);
        close.setBounds (card.getRight() - 77, card.getY() + 15, 61, 26);
        skins.setBounds (card.getX() + 81, card.getBottom() - 47, card.getWidth() - 99, 29);
    }

    void paint (Graphics& g) override
    {
        g.fillAll (Colours::black.withAlpha (0.60f));
        g.setColour (hammer::rubber);
        g.fillRoundedRectangle (card.toFloat(), 10.0f);
        g.setColour (hammer::edge);
        g.drawRoundedRectangle (card.toFloat().reduced (0.5f), 10.0f, 1.0f);

        const int x = card.getX() + 18;
        const int y = card.getY() + 13;
        g.setColour (hammer::accentFor (*this));
        g.setFont (hammer::font (26.0f, Font::bold));
        g.drawText ("XTONE", x, y, 220, 30, Justification::centredLeft);
        g.setColour (hammer::text);
        g.setFont (hammer::font (15.0f));
        g.drawText ("Extratone Retrigger Sampler", x, y + 32, 250, 22, Justification::centredLeft);
        g.setColour (hammer::muted);
        g.setFont (hammer::font (13.5f));
        g.drawText ("Version " JucePlugin_VersionString "  " JucePlugin_Manufacturer,
                    x, y + 55, 250, 19, Justification::centredLeft);
        g.setColour (hammer::text);
        g.setFont (hammer::font (14.0f, Font::bold));
        g.drawText ("Skin", x, skins.getY(), 57, skins.getHeight(), Justification::centredLeft);
    }

    void mouseDown (const MouseEvent& event) override
    {
        if (! card.contains (event.getPosition())) dismiss();
    }

    bool keyPressed (const KeyPress& key) override
    {
        if (key != KeyPress::escapeKey) return false;
        dismiss();
        return true;
    }

private:
    void dismiss()
    {
        setVisible (false);
        editor.wordmark.grabKeyboardFocus();
    }

    XtoneEditor& editor;
    TextButton close { "Close" };
    ComboBox skins;
    Rectangle<int> card;
};

XtoneEditor::XtoneEditor (XtoneProcessor& p)
    : AudioProcessorEditor (p), proc (p), header (p), waveform (p), rate (p), params (p)
{
    addAndMakeVisible (header);
    addAndMakeVisible (waveform);
    addAndMakeVisible (rate);
    addChildComponent (params);
    wordmark.getProperties().set ("wordmark", true);
    wordmark.setTitle ("About XTONE and skin settings");
    wordmark.setTooltip ("About XTONE and skin settings");
    wordmark.setMouseCursor (MouseCursor::PointingHandCursor);
    wordmark.onClick = [this] { about->show(); };
    addAndMakeVisible (wordmark);

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

    theme.setSkin (p.getEditorSkin());
    setLookAndFeel (&theme);
    selectGroup (0);
    setResizable (true, true);
    setResizeLimits (760, 560, 2400, 1800);
    about = std::make_unique<AboutOverlay> (*this);
    addChildComponent (*about);
    setSize (1000, 720);
    startTimerHz (12);
}

XtoneEditor::~XtoneEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void XtoneEditor::timerCallback() { updateSkin(); }

void XtoneEditor::updateSkin()
{
    const auto skin = proc.getEditorSkin();
    if (theme.getSkin() == skin) return;
    theme.setSkin (skin);
    sendLookAndFeelChange();
    about->syncSelection();
    const auto dockX = header.getX() + header.presetDockBounds().getX();
    wordmark.setBounds (hammer::artwork::wordmarkBounds (waveform.getY(), dockX, skin));
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
    const auto dock = header.presetDockBounds().translated (header.getX(), header.getY());
    const auto skin = theme.getSkin();
    hammer::artwork::drawFrame (g, getLocalBounds(), waveform.getBounds(), rate.getBounds(), dock, skin);
    if (params.isVisible()) hammer::artwork::drawRatePanel (g, params.getBounds(), skin);
    hammer::artwork::drawKeyWell (g, keyWell);
}

void XtoneEditor::resized()
{
    auto content = getLocalBounds().reduced (28, 20);
    if (content.isEmpty()) return;
    header.setBounds (content.removeFromTop (57));
    content.removeFromTop (10);
    content.removeFromBottom (74);
    keyWell = hammer::artwork::keyArea (getLocalBounds(), content.getBottom());
    auto keyRow = keyWell.reduced (2, 1);
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

    auto left = content.removeFromLeft (content.getWidth() * 61 / 100);
    waveform.setBounds (left);
    content.removeFromLeft (10);
    rate.setBounds (content);
    params.setBounds (content);
    const auto dockX = header.getX() + header.presetDockBounds().getX();
    wordmark.setBounds (hammer::artwork::wordmarkBounds (waveform.getY(), dockX, theme.getSkin()));
    if (about != nullptr) about->setBounds (getLocalBounds());
}

bool XtoneEditor::isInterestedInFileDrag (const StringArray& files)
{
    if (about->isVisible()) return false;
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3")) return true;
    return false;
}

void XtoneEditor::filesDropped (const StringArray& files, int, int)
{
    if (about->isVisible()) return;
    for (auto& f : files)
        if (File (f).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3"))
        {
            proc.loadSampleFile (File (f));
            return;
        }
}
