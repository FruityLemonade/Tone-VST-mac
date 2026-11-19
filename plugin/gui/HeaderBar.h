#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class XtoneProcessor;

/// Load / normalise / presets + live rate display.
class HeaderBar : public juce::Component, private juce::Timer
{
public:
    explicit HeaderBar (XtoneProcessor& p);
    void resized() override;
    void paint (juce::Graphics&) override;

    juce::String buildLiveText() const;

private:
    void timerCallback() override;
    void refreshPresets();

    XtoneProcessor& proc;
    juce::TextButton loadButton { juce::String::fromUTF8 ("Laden…") }, normButton { "Normalisieren" };
    juce::ComboBox presetBox;
    juce::TextButton savePresetButton { juce::String::fromUTF8 ("Speichern…") }, loadPresetButton { juce::String::fromUTF8 ("Preset laden…") };
    juce::Label fileLabel, statusLabel, liveLabel;
    std::unique_ptr<juce::FileChooser> chooser;
};
