#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace hammer
{
inline const juce::Colour yellow { 0xffffca18 };
inline const juce::Colour ink { 0xff171813 };
inline const juce::Colour rubber { 0xff242624 };
inline const juce::Colour recess { 0xff121413 };
inline const juce::Colour edge { 0xff464941 };
inline const juce::Colour text { 0xfff4f2e8 };
inline const juce::Colour muted { 0xffb3b6aa };

juce::Font font (float height, int style = juce::Font::plain);
juce::Font mono (float height, int style = juce::Font::plain);
juce::Path cutCorners (juce::Rectangle<float>, float cut);
void drawMoulding (juce::Graphics&, juce::Rectangle<float>, float cut, bool inset = false);
void drawGrip (juce::Graphics&, juce::Rectangle<float>, bool right);
void drawVents (juce::Graphics&, juce::Rectangle<float>);
void drawPresetBezel (juce::Graphics&, juce::Rectangle<float>);
void drawRecess (juce::Graphics&, juce::Rectangle<float>);
void drawFastener (juce::Graphics&, juce::Point<float>, float radius);

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    juce::Font getTextButtonFont (juce::TextButton&, int height) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float min, float max,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end,
                           juce::Slider&) override;
};
} // namespace hammer
