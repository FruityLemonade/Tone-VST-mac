#include "HammerLookAndFeel.h"

using namespace juce;

namespace hammer
{
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

const Image& machinedCap()
{
    static const auto cap = []
    {
        constexpr int size = 384;
        Image metal (Image::ARGB, size, size, true);
        Image::BitmapData pixels (metal, Image::BitmapData::writeOnly);
        Random random (63719);
        float grooves[size * 2];
        for (auto& groove : grooves)
            groove = random.nextFloat() - 0.5f;

        const auto centre = (float) size * 0.5f;
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
            {
                const float dx = (float) x + 0.5f - centre;
                const float dy = (float) y + 0.5f - centre;
                const float distance = std::hypot (dx, dy);
                if (distance >= centre) continue;

                const float radius = distance / centre;
                const float angle = std::atan2 (dy, dx);
                const float specular = std::pow (std::abs (std::sin (angle - 0.23f)), 10.0f);
                const float brushing = grooves[(int) (distance * 2.9f)] * (5.0f + specular * 10.0f);
                const float light = 23.0f + 83.0f * specular + 13.0f * radius + brushing;
                const auto grey = (uint8) roundToInt (light);
                pixels.setPixelColour (x, y, Colour (grey, grey, (uint8) (grey - 2))
                                                 .withAlpha (jlimit (0.0f, 1.0f, centre - distance)));
            }
        return metal;
    }();
    return cap;
}

void drawChuck (Graphics& g, Point<float> centre, float radius, float angle)
{
    const auto point = [centre] (float r, float a) { return centre.getPointOnCircumference (r, a); };
    const auto disc = [centre] (float r) { return Rectangle<float> (2.0f * r, 2.0f * r).withCentre (centre); };
    const auto rim = disc (radius);

    for (int i = 3; i > 0; --i)
    {
        g.setColour (Colour (0xff422b09).withAlpha (0.20f));
        g.fillEllipse (rim.expanded ((float) i).translated (0.0f, (float) i * 1.3f));
    }

    auto steel = ColourGradient (Colour (0xffe5e1cb), rim.getTopLeft(), Colour (0xffa6a296), rim.getBottomRight(), false);
    steel.addColour (0.25, Colour (0xffa8a79a));
    steel.addColour (0.54, Colour (0xff403f36));
    steel.addColour (0.75, Colour (0xffd2cec0));
    steel.addColour (0.89, Colour (0xff7c7b70));
    g.setGradientFill (steel);
    g.fillEllipse (rim);
    g.setColour (Colour (0xfff7f2e3).withAlpha (0.72f));
    g.drawEllipse (rim.reduced (0.6f), 1.0f);
    g.setColour (Colour (0xff0c0d0c));
    g.fillEllipse (disc (radius * 0.957f));

    constexpr int flutes = 11;
    constexpr auto step = MathConstants<float>::twoPi / (float) flutes;
    const float valley = radius * 0.855f;
    const float crown = radius * 0.941f;
    Path grip;
    grip.startNewSubPath (point (valley, angle - step * 0.5f));
    for (int i = 0; i < flutes; ++i)
    {
        const float a = angle + step * (float) i;
        grip.cubicTo (point (valley, a - step * 0.34f), point (crown, a - step * 0.32f), point (crown, a));
        grip.cubicTo (point (crown, a + step * 0.32f), point (valley, a + step * 0.34f), point (valley, a + step * 0.5f));
    }
    grip.closeSubPath();
    fillSlope (g, grip, rim, Colour (0xff56554c), Colour (0xff292925));

    for (int i = 0; i < flutes; ++i)
    {
        const float a = angle + step * (float) i;
        const float shoulder = radius * 0.66f;
        const auto inner = point (shoulder, a);
        Path flute;
        flute.startNewSubPath (point (valley, a - step * 0.5f));
        flute.cubicTo (point (valley, a - step * 0.34f), point (crown, a - step * 0.32f), point (crown, a));
        flute.cubicTo (point (crown, a + step * 0.32f), point (valley, a + step * 0.34f), point (valley, a + step * 0.5f));
        flute.quadraticTo (point (shoulder, a + step * 0.29f), inner);
        flute.quadraticTo (point (shoulder, a - step * 0.29f), point (valley, a - step * 0.5f));
        flute.closeSubPath();

        const float exposure = 0.5f + 0.5f * std::cos (a + 0.70f);
        g.setGradientFill (ColourGradient (Colour (0xff100f0d).interpolatedWith (Colour (0xff696962), exposure),
                                          point (crown, a), Colour (0xff2e2e2b), inner, false));
        g.fillPath (flute);
    }
    stipple (g, grip, 0.27f);

    g.setColour (Colour (0xff0c0c0b));
    g.fillEllipse (disc (radius * 0.492f));
    const auto cap = disc (radius * 0.465f);
    g.drawImage (machinedCap(), cap);
    g.setColour (Colour (0xffb9b8a9).withAlpha (0.62f));
    g.drawEllipse (cap.reduced (0.45f), 0.9f);
    g.setColour (Colours::white.withAlpha (0.15f));
    g.drawEllipse (cap.reduced (1.5f), 0.6f);
}

} // namespace

void drawMoulding (Graphics& g, Rectangle<float> r, float cut, bool inset)
{
    const auto outline = cutCorners (r, cut);
    fillSlope (g, outline, r, Colour (0xff785009), Colour (0xffffdd61));
    auto slope = r.reduced (1.0f);
    const auto rim = cutCorners (slope, cut - 1.0f);
    auto finish = ColourGradient (Colour (inset ? 0xff75430b : 0xffffeda1), slope.getTopLeft(),
                                  Colour (inset ? 0xffffed8e : 0xff7f4504), slope.getBottomLeft(), false);
    finish.addColour (0.45, Colour (inset ? 0xffba7509 : 0xffe7a511));
    g.setGradientFill (finish);
    g.fillPath (rim);
    stipple (g, rim, 0.29f);

    const auto face = slope.reduced (inset ? 4.0f : 3.0f);
    const auto faceShape = cutCorners (face, cut - 3.0f);
    auto light = ColourGradient (Colour (inset ? 0xfff8b913 : 0xffffd52c), face.getTopLeft(),
                                 Colour (inset ? 0xffe9a105 : 0xffeba407), face.getBottomRight(), false);
    light.addColour (0.50, Colour (inset ? 0xffffca1d : 0xfffbc013));
    g.setGradientFill (light);
    g.fillPath (faceShape);
    stipple (g, faceShape, 0.48f);
}

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

void drawGrip (Graphics& g, Rectangle<float> r, bool right)
{
    const auto shadow = cutCorners (r.expanded (1.5f).translated (right ? -1.3f : 1.3f, 1.2f), 8.0f);
    g.setColour (Colours::black.withAlpha (0.70f));
    g.fillPath (shadow);
    const auto body = cutCorners (r, 8.0f);
    auto rubberLight = ColourGradient (Colour (0xff51524e), r.getTopLeft(), Colour (0xff141514), r.getTopRight(), false);
    rubberLight.addColour (0.36, Colour (0xff2d2e2b));
    g.setGradientFill (rubberLight);
    g.fillPath (body);
    stipple (g, body, 0.30f);

    for (float y = r.getY() + 18.0f; y < r.getBottom() - 13.0f; y += 17.0f)
    {
        const auto left = Point<float> (r.getX() + 2.5f, y + (right ? -4.0f : 4.0f));
        const auto end = Point<float> (r.getRight() - 2.5f, y + (right ? 4.0f : -4.0f));
        g.setColour (Colour (0xff090a09));
        g.drawLine ({ left, end }, 4.5f);
        g.setColour (Colour (0xff666761).withAlpha (0.65f));
        g.drawLine ({ left.translated (0.0f, 2.6f), end.translated (0.0f, 2.6f) }, 1.0f);
    }
}

void drawVents (Graphics& g, Rectangle<float> r)
{
    for (int i = 0; i < 3; ++i)
    {
        const float x = r.getX() + (float) i * 11.0f;
        Path groove;
        groove.addQuadrilateral (x + 9, r.getY(), x + 15, r.getY(),
                                x + 6, r.getBottom(), x, r.getBottom());
        g.setColour (Colour (0xffffe18a));
        g.fillPath (groove, AffineTransform::translation (0.8f, 1.1f));
        fillSlope (g, groove, r, Colour (0xff754608), Colour (0xffd5970d));
    }
}

void drawPresetBezel (Graphics& g, Rectangle<float> r)
{
    const auto shadow = cutCorners (r.translated (0.0f, 1.5f), 8.0f);
    g.setColour (Colour (0xff734b0b));
    g.fillPath (shadow);
    fillSlope (g, cutCorners (r, 8.0f), r, Colour (0xffe4dbba), Colour (0xff36332b));
    const auto face = r.reduced (2.0f);
    const auto plate = cutCorners (face, 6.0f);
    fillSlope (g, plate, face, Colour (0xffbab6a6), Colour (0xff73736c));
    const Graphics::ScopedSaveState saved (g);
    g.reduceClipRegion (plate);
    for (int y = roundToInt (face.getY()) + 1; y < (int) face.getBottom(); ++y)
    {
        g.setColour (((y % 3) == 0 ? Colours::white : Colours::black).withAlpha (0.07f));
        g.drawHorizontalLine (y, face.getX(), face.getRight());
    }
}

void drawFastener (Graphics& g, Point<float> centre, float radius)
{
    const auto washer = Rectangle<float> (2.0f * radius, 2.0f * radius).withCentre (centre);
    g.setColour (Colour (0xff784b08));
    g.fillEllipse (washer.expanded (2.0f).translated (0.0f, 1.0f));
    g.setGradientFill (ColourGradient (Colour (0xff7a470a), washer.getTopLeft(),
                                       Colour (0xffffdf74), washer.getBottomRight(), false));
    g.fillEllipse (washer.expanded (1.4f));
    g.setColour (Colour (0xff1e1d17));
    g.fillEllipse (washer);
    const auto head = washer.reduced (0.9f);
    auto steel = ColourGradient (Colour (0xffeeeade), head.getTopLeft(), Colour (0xff68685c), head.getBottomRight(), false);
    steel.addColour (0.43, Colour (0xff8a897f));
    steel.addColour (0.74, Colour (0xffc9c8bd));
    g.setGradientFill (steel);
    g.fillEllipse (head);

    Path socket;
    socket.addStar (centre, 6, radius * 0.32f, radius * 0.59f, 0.15f);
    const auto roundedSocket = socket.createPathWithRoundedCorners (0.85f);
    g.setColour (Colour (0xffd9d6c9));
    g.fillPath (roundedSocket, AffineTransform::translation (0.0f, 0.6f));
    fillSlope (g, roundedSocket, head, Colour (0xff10100e), Colour (0xff383a34));
}

LookAndFeel::LookAndFeel()
{
    setColour (ResizableWindow::backgroundColourId, recess);
    setColour (Label::textColourId, text);
    setColour (TextEditor::textColourId, text);
    setColour (TextEditor::backgroundColourId, recess);
    setColour (TextEditor::outlineColourId, edge);
    setColour (TextEditor::focusedOutlineColourId, yellow);
    setColour (TextEditor::highlightColourId, yellow);
    setColour (TextEditor::highlightedTextColourId, ink);
    setColour (CaretComponent::caretColourId, yellow);
    setColour (TextButton::buttonColourId, rubber);
    setColour (TextButton::buttonOnColourId, yellow);
    setColour (TextButton::textColourOffId, text);
    setColour (TextButton::textColourOnId, ink);
    setColour (ToggleButton::textColourId, text);
    setColour (ToggleButton::tickColourId, yellow);
    setColour (ToggleButton::tickDisabledColourId, edge);
    setColour (ComboBox::textColourId, text);
    setColour (ComboBox::backgroundColourId, recess);
    setColour (ComboBox::outlineColourId, edge);
    setColour (ComboBox::arrowColourId, yellow);
    setColour (PopupMenu::backgroundColourId, rubber);
    setColour (PopupMenu::textColourId, text);
    setColour (PopupMenu::highlightedBackgroundColourId, yellow);
    setColour (PopupMenu::highlightedTextColourId, ink);
    setColour (Slider::textBoxTextColourId, text);
    setColour (Slider::textBoxBackgroundColourId, recess);
    setColour (Slider::textBoxOutlineColourId, edge);
    setColour (Slider::textBoxHighlightColourId, yellow.withAlpha (0.4f));
    setColour (Slider::trackColourId, yellow);
    setColour (Slider::thumbColourId, yellow);
    setColour (Slider::rotarySliderFillColourId, yellow);
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

void LookAndFeel::drawButtonBackground (Graphics& g, Button& button, const Colour& background, bool over, bool down)
{
    auto b = button.getLocalBounds().toFloat().reduced (0.75f);
    const auto fill = ! button.isEnabled() ? background.darker (0.35f)
                    : down ? background.darker (0.25f)
                    : over ? background.brighter (0.16f) : background;
    g.setColour (ink.withAlpha (0.75f));
    g.fillRoundedRectangle (b.translated (0.0f, 1.0f), 4.0f);
    g.setGradientFill (ColourGradient (fill.brighter (0.17f), b.getTopLeft(), fill.darker (0.22f), b.getBottomLeft(), false));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (button.hasKeyboardFocus (true) ? (button.getToggleState() ? ink : yellow) : fill.brighter (0.32f));
    g.drawRoundedRectangle (b, 4.0f, 1.0f);
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool down, int, int, int, int, ComboBox& box)
{
    const auto r = Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (down ? rubber : box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.hasKeyboardFocus (true) ? yellow : box.findColour (ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    auto centre = Point<float> (r.getRight() - 13.0f, r.getCentreY());
    Path arrow;
    arrow.addTriangle (centre.translated (-4.0f, -2.0f), centre.translated (4.0f, -2.0f), centre.translated (0.0f, 3.0f));
    g.setColour (box.findColour (ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void LookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float min, float max,
                                    Slider::SliderStyle style, Slider& slider)
{
    if (style != Slider::LinearHorizontal || w < 12)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, min, max, style, slider);
        return;
    }

    const auto cy = (float) y + (float) h * 0.5f;
    auto track = Rectangle<float> ((float) x, cy - 3.5f, (float) w, 7.0f);
    g.setColour (recess);
    g.fillRoundedRectangle (track, 3.5f);
    g.setColour (slider.isEnabled() ? slider.findColour (Slider::trackColourId) : edge);
    g.fillRoundedRectangle (track.withRight (jlimit (track.getX(), track.getRight(), pos)), 3.5f);
    g.setColour (slider.hasKeyboardFocus (true) ? yellow : ink.withAlpha (0.65f));
    g.drawRoundedRectangle (track, 3.5f, 1.0f);

    const auto grip = Rectangle<float> (10.0f, 16.0f).withCentre ({ pos, cy });
    g.setColour (ink);
    g.fillRoundedRectangle (grip.expanded (1.0f), 3.0f);
    g.setGradientFill (ColourGradient (muted, grip.getTopLeft(), edge, grip.getBottomRight(), false));
    g.fillRoundedRectangle (grip, 2.0f);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end,
                                    Slider& slider)
{
    const bool chuck = slider.getProperties().contains ("chuck");
    const auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto centre = b.getCentre();
    const float radius = jmin (b.getWidth(), b.getHeight()) * (chuck ? 0.435f : 0.40f);
    const auto point = [centre] (float distance, float angle) { return centre.getPointOnCircumference (distance, angle); };
    const auto disc = [centre] (float r) { return Rectangle<float> (2.0f * r, 2.0f * r).withCentre (centre); };
    const float angle = jmap (pos, start, end);
    const auto rim = disc (radius);
    const int ticks = chuck ? 11 : 7;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = jmap ((float) i / (float) (ticks - 1), start, end);
        g.setColour (chuck ? ink.withAlpha (0.85f) : a <= angle ? yellow : edge);
        g.drawLine ({ point (radius * 1.05f, a), point (radius * 1.14f, a) }, chuck ? 2.0f : 1.2f);
    }

    if (chuck)
    {
        drawChuck (g, centre, radius, angle);
        Path index;
        index.startNewSubPath (point (radius * 0.67f, angle));
        index.lineTo (point (radius * 0.92f, angle));
        g.setColour (text);
        g.strokePath (index, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
        if (slider.hasKeyboardFocus (true))
        {
            g.setColour (ink);
            g.drawEllipse (rim.expanded (2.5f), 1.2f);
        }
        return;
    }

    g.setColour (ink.withAlpha (0.65f));
    g.fillEllipse (rim.expanded (2.0f).translated (0.0f, 2.5f));
    g.setGradientFill (ColourGradient (Colour (0xffc0beb0), rim.getTopLeft(), Colour (0xff53564e), rim.getBottomRight(), false));
    g.fillEllipse (rim);
    g.setColour (recess);
    g.fillEllipse (disc (radius * 0.965f));

    Path fluting;
    constexpr int flutes = 8;
    for (int i = 0; i < flutes; ++i)
    {
        const float a = angle + MathConstants<float>::twoPi * (float) i / (float) flutes;
        Path flute;
        flute.startNewSubPath (point (radius * 0.49f, a - 0.19f));
        flute.lineTo (point (radius * 0.90f, a - 0.22f));
        flute.quadraticTo (point (radius * 0.99f, a), point (radius * 0.90f, a + 0.22f));
        flute.lineTo (point (radius * 0.49f, a + 0.19f));
        flute.closeSubPath();
        fluting.addPath (flute);
    }
    g.setGradientFill (ColourGradient (Colour (0xff5d6057), rim.getTopLeft(), Colour (0xff252724), rim.getBottomRight(), false));
    g.fillPath (fluting);
    g.setColour (ink);
    g.fillEllipse (disc (radius * 0.55f));
    g.setGradientFill (ColourGradient (Colour (0xff50534d), centre.translated (-radius, -radius),
                                       Colour (0xff202220), centre.translated (radius * 0.36f, radius * 0.35f), false));
    g.fillEllipse (disc (radius * 0.51f));
    g.setColour (muted.withAlpha (0.28f));
    g.drawEllipse (disc (radius * 0.51f), 1.0f);
    g.setColour (yellow);
    g.drawLine ({ point (radius * 0.67f, angle), point (radius * 0.87f, angle) }, 2.0f);
    if (slider.hasKeyboardFocus (true))
    {
        g.setColour (yellow);
        g.drawEllipse (rim.expanded (2.5f), 1.2f);
    }
}
} // namespace hammer
