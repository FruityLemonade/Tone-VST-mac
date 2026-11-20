#include "HammerLookAndFeel.h"

#include "FaceplateArtwork.h"

using namespace juce;

namespace hammer
{
Colour accentColour (xtp::EditorSkin skin)
{
    switch (skin)
    {
        case xtp::EditorSkin::hammerDrill: return yellow;
        case xtp::EditorSkin::makita:     return Colour (0xff58eeee);
        case xtp::EditorSkin::festool:   return Colour (0xffb9ef00);
        case xtp::EditorSkin::hilti:     return Colour (0xffff4937);
    }
    return yellow;
}

Colour ratePanelText (xtp::EditorSkin skin) { return skin == xtp::EditorSkin::hammerDrill ? ink : text; }

xtp::EditorSkin skinFor (const Component& component)
{
    if (const auto* look = dynamic_cast<const LookAndFeel*> (&component.getLookAndFeel()))
        return look->getSkin();
    return xtp::EditorSkin::hammerDrill;
}

Colour accentFor (const Component& component) { return accentColour (skinFor (component)); }

Font font (float height, int style) { return FontOptions (height, style); }
Font mono (float height, int style) { return FontOptions (Font::getDefaultMonospacedFontName(), height, style); }

Path cutCorners (Rectangle<float> r, float cut)
{
    Path p;
    p.startNewSubPath (r.getX() + cut, r.getY());
    p.lineTo (r.getRight() - cut, r.getY());
    p.lineTo (r.getRight(), r.getY() + cut);
    p.lineTo (r.getRight(), r.getBottom() - cut);
    p.lineTo (r.getRight() - cut, r.getBottom());
    p.lineTo (r.getX() + cut, r.getBottom());
    p.lineTo (r.getX(), r.getBottom() - cut);
    p.lineTo (r.getX(), r.getY() + cut);
    p.closeSubPath();
    return p;
}

namespace
{
const Image& mouldGrain()
{
    static const auto tile = []
    {
        constexpr int size = 128;
        Image grain (Image::ARGB, size, size, true);
        Image::BitmapData pixels (grain, Image::BitmapData::writeOnly);
        Random random (48769);
        float height[size][size];
        for (auto& row : height)
            for (auto& sample : row)
                sample = random.nextFloat();

        const auto sample = [&] (int x, int y) { return height[(y + size) % size][(x + size) % size]; };
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const float light = 0.23f * (sample (x, y) - sample (x - 1, y - 1))
                                  + 0.14f * (sample (x + 1, y) - sample (x - 1, y))
                                  + 0.10f * (sample (x, y + 1) - sample (x - 2, y - 1));
                pixels.setPixelColour (x, y, (light >= 0.0f ? Colour (0xffffedbd) : Colours::black)
                                                 .withAlpha (std::abs (light)));
            }
        return grain;
    }();
    return tile;
}

void stipple (Graphics& g, const Path& surface, float opacity)
{
    g.setTiledImageFill (mouldGrain(), 0, 0, opacity);
    g.fillPath (surface);
}

void fillSlope (Graphics& g, const Path& shape, Rectangle<float> r, Colour top, Colour bottom)
{
    g.setGradientFill (ColourGradient (top, r.getTopLeft(), bottom, r.getBottomLeft(), false));
    g.fillPath (shape);
}

} // namespace

void drawRecess (Graphics& g, Rectangle<float> r)
{
    const auto rim = cutCorners (r, 11.0f);
    fillSlope (g, rim, r, Colour (0xff10110f), Colour (0xff626158));
    const auto slope = r.reduced (1.0f);
    fillSlope (g, cutCorners (slope, 10.0f), slope, Colour (0xff191a18), Colour (0xff353633));
    const auto face = slope.reduced (3.0f);
    const auto surface = cutCorners (face, 8.0f);
    fillSlope (g, surface, face, Colour (0xff2c2e2a), Colour (0xff191a18));
    stipple (g, surface, 0.17f);
    g.setColour (Colours::black.withAlpha (0.36f));
    g.strokePath (surface, PathStrokeType (1.5f));
}

LookAndFeel::LookAndFeel()
{
    setSkin (xtp::EditorSkin::hammerDrill);
}

void LookAndFeel::setSkin (xtp::EditorSkin next)
{
    skin = next;
    accent = accentColour (skin);
    const auto selectedText = skin == xtp::EditorSkin::hilti ? text : ink;
    const auto selection = skin == xtp::EditorSkin::hilti ? Colour (0xffbb2316) : accent;
    setColour (ResizableWindow::backgroundColourId, recess);
    setColour (Label::textColourId, text);
    setColour (TextEditor::textColourId, text);
    setColour (TextEditor::backgroundColourId, recess);
    setColour (TextEditor::outlineColourId, edge);
    setColour (TextEditor::focusedOutlineColourId, accent);
    setColour (TextEditor::highlightColourId, selection);
    setColour (TextEditor::highlightedTextColourId, selectedText);
    setColour (CaretComponent::caretColourId, accent);
    setColour (TextButton::buttonColourId, rubber);
    setColour (TextButton::buttonOnColourId, selection);
    setColour (TextButton::textColourOffId, text);
    setColour (TextButton::textColourOnId, selectedText);
    setColour (ToggleButton::textColourId, text);
    setColour (ToggleButton::tickColourId, accent);
    setColour (ToggleButton::tickDisabledColourId, edge);
    setColour (ComboBox::textColourId, text);
    setColour (ComboBox::backgroundColourId, recess);
    setColour (ComboBox::outlineColourId, edge);
    setColour (ComboBox::arrowColourId, accent);
    setColour (PopupMenu::backgroundColourId, rubber);
    setColour (PopupMenu::textColourId, text);
    setColour (PopupMenu::highlightedBackgroundColourId, selection);
    setColour (PopupMenu::highlightedTextColourId, selectedText);
    setColour (Slider::textBoxTextColourId, text);
    setColour (Slider::textBoxBackgroundColourId, recess);
    setColour (Slider::textBoxOutlineColourId, edge);
    setColour (Slider::textBoxHighlightColourId, accent.withAlpha (0.4f));
    setColour (Slider::trackColourId, accent);
    setColour (Slider::thumbColourId, accent);
    setColour (Slider::rotarySliderFillColourId, accent);
    setColour (Slider::rotarySliderOutlineColourId, edge);
    setColour (ScrollBar::thumbColourId, Colour (0xff77796c));
    setColour (TooltipWindow::backgroundColourId, recess);
    setColour (TooltipWindow::textColourId, text);
    setColour (TooltipWindow::outlineColourId, edge);
}

Font LookAndFeel::getTextButtonFont (TextButton& button, int height)
{
    const auto size = button.getProperties().contains ("key") ? 12.5f : 13.5f;
    return font (jmin (size, height * 0.6f), Font::bold);
}

Font LookAndFeel::getComboBoxFont (ComboBox&) { return font (14.0f, Font::bold); }
Font LookAndFeel::getPopupMenuFont() { return font (15.0f); }

void LookAndFeel::drawButtonBackground (Graphics& g, Button& button, const Colour&, bool over, bool down)
{
    if (button.getProperties().contains ("wordmark"))
    {
        if (over || down || button.hasKeyboardFocus (true))
        {
            const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (Colours::black.withAlpha (0.65f));
            g.drawRoundedRectangle (bounds, 3.0f, 2.0f);
            g.setColour (Colours::white.withAlpha (0.85f));
            g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
        }
        return;
    }

    const auto well = button.getLocalBounds().reduced (0, 1);
    const auto cap = well.withTrimmedBottom (1).translated (0, down ? 1 : 0);
    const bool selected = button.getToggleState();
    g.setColour (Colours::black.withAlpha (0.70f));
    g.fillRoundedRectangle (well.toFloat(), 4.5f);

    const Graphics::ScopedSaveState saved (g);
    Path edgeShape;
    edgeShape.addRoundedRectangle (cap.toFloat(), 4.5f);
    g.reduceClipRegion (edgeShape);
    g.setOpacity (1.0f);
    artwork::drawButton (g, cap, selected, button.getProperties().contains ("key"), skin);

    if (! button.isEnabled() || down || over)
    {
        const auto shade = ! button.isEnabled() ? Colours::black.withAlpha (0.35f)
                         : down ? Colours::black.withAlpha (0.20f) : Colours::white.withAlpha (0.10f);
        g.fillAll (shade);
    }

    if (button.hasKeyboardFocus (true))
    {
        g.setColour (selected ? findColour (TextButton::textColourOnId) : accent);
        g.drawRoundedRectangle (cap.toFloat().reduced (1.5f), 3.0f, 1.0f);
    }
}

void LookAndFeel::drawButtonText (Graphics& g, TextButton& button, bool over, bool down)
{
    if (button.getProperties().contains ("wordmark")) return;
    const Graphics::ScopedSaveState saved (g);
    if (down) g.addTransform (AffineTransform::translation (0.0f, 1.0f));
    LookAndFeel_V4::drawButtonText (g, button, over, down);
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool down, int, int, int, int, ComboBox& box)
{
    const auto r = Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    {
        const Graphics::ScopedSaveState saved (g);
        Path edgeShape;
        edgeShape.addRoundedRectangle (r, 4.0f);
        g.reduceClipRegion (edgeShape);
        artwork::drawButton (g, Rectangle<int> (w, h), false, false);
        if (down) g.fillAll (Colours::black.withAlpha (0.20f));
    }
    if (box.hasKeyboardFocus (true))
    {
        g.setColour (accent);
        g.drawRoundedRectangle (r.reduced (1.0f), 3.0f, 1.0f);
    }
    auto centre = Point<float> (r.getRight() - 13.0f, r.getCentreY());
    Path arrow;
    arrow.addTriangle (centre.translated (-4.0f, -2.0f), centre.translated (4.0f, -2.0f), centre.translated (0.0f, 3.0f));
    g.setColour (box.findColour (ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void LookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float min, float max,
                                    Slider::SliderStyle style, Slider& slider)
{
    if (style == Slider::LinearBar)
    {
        const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const bool bipolar = slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0;
        const float start = bipolar ? slider.getPositionOfValue (0.0) : bounds.getX();
        const auto from = jlimit (bounds.getX(), bounds.getRight(), jmin (start, pos));
        const auto to = jlimit (bounds.getX(), bounds.getRight(), jmax (start, pos));
        g.setColour (recess);
        g.fillRoundedRectangle (bounds, 3.0f);
        {
            const Graphics::ScopedSaveState saved (g);
            Path clip;
            clip.addRoundedRectangle (bounds.reduced (0.5f), 3.0f);
            g.reduceClipRegion (clip);
            g.setColour (accent.withAlpha (slider.isEnabled() ? 0.26f : 0.12f));
            g.fillRect (bounds.withX (from).withRight (to));
            if (bipolar)
            {
                g.setColour (muted.withAlpha (0.6f));
                g.drawVerticalLine (roundToInt (start), bounds.getY() + 1.0f, bounds.getY() + 4.0f);
                g.drawVerticalLine (roundToInt (start), bounds.getBottom() - 4.0f, bounds.getBottom() - 1.0f);
            }
        }
        g.setColour (slider.hasKeyboardFocus (true) ? accent : edge);
        g.drawRoundedRectangle (bounds, 3.0f, 1.0f);
        return;
    }

    if (style != Slider::LinearHorizontal || w < 12 || h < 14)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, min, max, style, slider);
        return;
    }

    const int centreY = y + h / 2;
    const auto track = Rectangle<int> (x - 5, centreY - 5, w + 10, 10);
    const float gripHeight = jmin (22.0f, (float) h - 3.0f);
    const auto grip = Rectangle<float> (gripHeight * (24.0f / 33.0f), gripHeight)
                          .withCentre ({ pos, (float) centreY });
    artwork::drawSlider (g, track, grip, slider.isEnabled(), slider.hasKeyboardFocus (true), skin);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end,
                                    Slider& slider)
{
    const bool chuck = slider.getProperties().contains ("chuck");
    const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto centre = b.getCentre();
    const float radius = jmin (b.getWidth(), b.getHeight()) * (chuck ? 0.435f : 0.40f);
    const auto point = [centre] (float distance, float angle) { return centre.getPointOnCircumference (distance, angle); };
    const float angle = jmap (pos, start, end);
    const auto rim = Rectangle<float> (2.0f * radius, 2.0f * radius).withCentre (centre);
    const int ticks = chuck ? 11 : 7;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = jmap ((float) i / (float) (ticks - 1), start, end);
        g.setColour (chuck ? ratePanelText (skin).withAlpha (0.85f) : a <= angle ? accent : edge);
        g.drawLine ({ point (radius * 1.05f, a), point (radius * 1.14f, a) }, chuck ? 2.0f : 1.2f);
    }

    if (! chuck)
    {
        g.setColour (ink.withAlpha (0.65f));
        g.fillEllipse (rim.expanded (2.0f).translated (0.0f, 2.5f));
    }

    artwork::drawKnob (g, rim, angle, skin);
    if (slider.hasKeyboardFocus (true))
    {
        g.setColour (chuck ? ratePanelText (skin) : accent);
        g.drawEllipse (rim.expanded (2.5f), 1.2f);
    }
}
} // namespace hammer
