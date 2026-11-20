#include "HeaderBar.h"

#include "HammerLookAndFeel.h"
#include "../PluginProcessor.h"

using namespace juce;

HeaderBar::HeaderBar (XtoneProcessor& p) : proc (p)
{
    addAndMakeVisible (presets);
    addAndMakeVisible (load);
    addAndMakeVisible (save);
    presets.setTitle ("Factory Preset");
    load.setTitle ("Load preset from file");
    save.setTitle ("Save preset to file");
    load.setTooltip ("Load preset from file");
    save.setTooltip ("Save preset to file");
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presets.addItem (proc.getProgramName (i), i + 1);
    timerCallback();
    presets.onChange = [this]
    {
        const int index = presets.getSelectedItemIndex();
        if (index >= 0 && index != proc.getCurrentProgram())
            proc.setCurrentProgram (index);
    };
    load.onClick = [this] { choosePresetFile (false); };
    save.onClick = [this] { choosePresetFile (true); };
    startTimerHz (12);
}

void HeaderBar::choosePresetFile (bool saving)
{
    if (chooser != nullptr) return;
    chooser = std::make_unique<FileChooser> (saving ? "Save Preset" : "Load Preset", File(), "*.xtone");
    const auto flags = saving ? FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting
                              : FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
    chooser->launchAsync (flags, [self = SafePointer<HeaderBar> (this), saving] (const FileChooser& dialog)
    {
        if (self == nullptr) return;
        const auto file = dialog.getResult();
        self->chooser.reset();
        if (saving && file != File())
            self->proc.saveStateToFile (file.withFileExtension ("xtone"));
        else if (! saving && file.existsAsFile())
            self->proc.loadStateFromFile (file);
    });
}

void HeaderBar::timerCallback()
{
    const int current = proc.getCurrentProgram();
    if (presets.getSelectedItemIndex() != current)
        presets.setSelectedItemIndex (current, dontSendNotification);
}

void HeaderBar::resized()
{
    auto r = getLocalBounds();
    auto dock = r.removeFromRight (jlimit (340, 450, getWidth() / 2)).reduced (8, 10);
    dock.removeFromLeft (58);
    save.setBounds (dock.removeFromRight (88));
    dock.removeFromRight (4);
    load.setBounds (dock.removeFromRight (66));
    dock.removeFromRight (4);
    presets.setBounds (dock);
}

void HeaderBar::paint (Graphics& g)
{
    auto left = getLocalBounds().withTrimmedRight (jlimit (340, 450, getWidth() / 2));
    const bool stacked = left.getWidth() < 455;
    auto logo = stacked ? left.removeFromTop (34) : left.removeFromLeft (160);
    g.setColour (hammer::ink);
    g.setFont (hammer::font (stacked ? 34.0f : 45.0f, Font::bold | Font::italic).withHorizontalScale (1.13f));
    g.drawFittedText ("XTONE", logo.reduced (2, 0), Justification::centredLeft, 1, 0.75f);
    g.setFont (hammer::font (13.0f, Font::bold));
    g.drawText ("EXTRATONE RETRIGGER SAMPLER", left, Justification::centredLeft);

    const auto dock = getLocalBounds().removeFromRight (jlimit (340, 450, getWidth() / 2)).reduced (0, 5);
    if (! stacked)
        hammer::drawVents (g, Rectangle<float> (38.0f, 22.0f).withCentre ({ (float) dock.getX() - 36.0f, (float) dock.getCentreY() }));
    hammer::drawPresetBezel (g, dock.toFloat());
    g.setColour (hammer::ink);
    g.setFont (hammer::font (11.0f, Font::bold));
    g.drawText ("PRESET", dock.withTrimmedLeft (14), Justification::centredLeft);
}
