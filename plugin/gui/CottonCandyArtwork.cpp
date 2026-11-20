#include "CottonCandyArtwork.h"

#include "XtoneFaceplateData.h"

using namespace juce;

namespace hammer::cottonCandy
{
namespace
{
const Colour pearl { 0xfffffcff };
const Colour blush { 0xffffe5f1 };
const Colour seam { 0xffd8a1bc };

const Image& heart()
{
    static const auto image = ImageFileFormat::loadFrom (XtoneFaceplateData::CottonCandyHeart_png,
                                                        XtoneFaceplateData::CottonCandyHeart_pngSize);
    return image;
}

void drawHeart (Graphics& g, Rectangle<float> area, float rotation = 0.0f)
{
    const auto& image = heart();
    const auto centre = area.getCentre();
    const auto transform = RectanglePlacement (RectanglePlacement::centred)
                               .getTransformToFit (image.getBounds().toFloat(), area)
                               .rotated (rotation, centre.x, centre.y);
    g.setColour (berry.withAlpha (0.22f));
    g.drawImageTransformed (image, transform.translated (0.0f, area.getHeight() * 0.025f), true);
    g.setOpacity (1.0f);
    g.drawImageTransformed (image, transform);
}

void drawStone (Graphics& g, Point<float> centre, float radius)
{
    const auto area = Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
    g.setColour (berry.withAlpha (0.32f));
    g.fillEllipse (area.translated (0.0f, 1.0f));

    Path stone;
    stone.addPolygon (centre, 8, radius, MathConstants<float>::pi / 8.0f);
    g.setGradientFill (ColourGradient (Colours::white, area.getTopLeft(),
                                       Colour (0xffa8b5e5), area.getBottomRight(), false));
    g.fillPath (stone);
    g.setColour (Colours::white.withAlpha (0.85f));
    g.drawLine (centre.x - radius * 0.58f, centre.y + radius * 0.15f,
                centre.x + radius * 0.47f, centre.y - radius * 0.62f, 0.9f);
}

void drawSparkle (Graphics& g, Point<float> c, float r)
{
    Path sparkle;
    sparkle.startNewSubPath (c.x, c.y - r);
    sparkle.quadraticTo (c.x + 0.14f * r, c.y - 0.12f * r, c.x + 0.72f * r, c.y);
    sparkle.quadraticTo (c.x + 0.14f * r, c.y + 0.12f * r, c.x, c.y + r);
    sparkle.quadraticTo (c.x - 0.14f * r, c.y + 0.12f * r, c.x - 0.72f * r, c.y);
    sparkle.quadraticTo (c.x - 0.14f * r, c.y - 0.12f * r, c.x, c.y - r);
    sparkle.closeSubPath();
    g.setColour (Colours::white.withAlpha (0.93f));
    g.fillPath (sparkle);
}
} // namespace

Rectangle<int> wordmarkBounds (int headerHeight, int dockX)
{
    return { 28, (headerHeight - 47) / 2, jmin (284, dockX - 44), 57 };
}

void drawFrame (Graphics& g, Rectangle<int> bounds, int headerHeight, int dockX, Rectangle<int> keys)
{
    static const auto fur = ImageFileFormat::loadFrom (XtoneFaceplateData::CottonCandyFur_jpg,
                                                      XtoneFaceplateData::CottonCandyFur_jpgSize);
    g.fillAll (Colour (0xfff4a5c9));
    {
        const Graphics::ScopedSaveState state (g);
        Path shell;
        shell.addRoundedRectangle (bounds.toFloat(), 13.0f);
        g.reduceClipRegion (shell);
        // Crop the edges with a single scale factor so the pile keeps its shape at any aspect ratio.
        g.drawImage (fur, bounds.toFloat(), RectanglePlacement::fillDestination);
    }
    g.setColour (pearl.withAlpha (0.58f));
    g.drawRoundedRectangle (bounds.toFloat().reduced (1.5f), 13.0f, 1.0f);

    const auto badge = wordmarkBounds (headerHeight, dockX).toFloat();
    drawPearl (g, badge, pearl, blush, 24.0f);
    drawHeart (g, Rectangle<float> (39.0f, 39.0f).withCentre (badge.getCentre().withX (badge.getX() + 31.0f)), -0.18f);
    const float textX = badge.getX() + 57.0f;
    const auto title = Rectangle<float> (textX, badge.getY() + 2.0f, badge.getWidth() - 64.0f, 29.0f);
    g.setColour (pearl);
    g.setFont (FontOptions ("Arial Rounded MT Bold", 31.0f, Font::bold));
    g.drawText ("XTONE", title.translated (0.0f, 2.0f), Justification::centredLeft);
    g.setColour (rose);
    g.drawText ("XTONE", title, Justification::centredLeft);
    g.setColour (berry);
    auto descriptor = Font (FontOptions (9.5f, Font::bold)).withExtraKerningFactor (0.065f);
    g.setFont (descriptor);
    g.drawFittedText ("EXTRATONE RETRIGGER SAMPLER", Rectangle<float> (textX, badge.getY() + 33.0f,
                       badge.getWidth() - 67.0f, 14.0f).toNearestInt(), Justification::centredLeft, 1);

    const auto y = (float) headerHeight * 0.5f;
    const auto charmX = (badge.getRight() + (float) dockX) * 0.5f;
    if ((float) dockX - badge.getRight() >= 68.0f)
    {
        drawStone (g, { charmX - 15.0f, y + 7.0f }, 5.0f);
        drawStone (g, { charmX, y - 3.0f }, 7.0f);
        drawStone (g, { charmX + 15.0f, y + 7.0f }, 5.0f);
        drawSparkle (g, { charmX + 4.0f, y - 9.0f }, 8.0f);
    }

    const auto footerY = (float) keys.getCentreY();
    for (const auto x : { (float) keys.getX() * 0.5f, ((float) keys.getRight() + (float) bounds.getRight()) * 0.5f })
    {
        const float tilt = x < (float) bounds.getCentreX() ? -0.23f : 0.23f;
        drawHeart (g, Rectangle<float> (31.0f, 31.0f).withCentre ({ x, footerY + 1.0f }), tilt);
        drawSparkle (g, { x + 12.0f, footerY - 12.0f }, 6.0f);
    }
}

void drawPearl (Graphics& g, Rectangle<float> area, Colour top, Colour bottom, float radius)
{
    if (area.isEmpty()) return;
    g.setColour (berry.withAlpha (0.24f));
    g.fillRoundedRectangle (area.translated (0.0f, 1.8f), radius);
    g.setGradientFill (ColourGradient (top, area.getTopLeft(), bottom, area.getBottomLeft(), false));
    g.fillRoundedRectangle (area, radius);
    g.setColour (seam);
    g.drawRoundedRectangle (area.reduced (0.5f), radius, 1.0f);
    g.setColour (Colours::white.withAlpha (0.82f));
    g.drawRoundedRectangle (area.reduced (1.5f), jmax (1.0f, radius - 1.0f), 1.0f);
}

void drawButton (Graphics& g, Rectangle<int> target, bool selected)
{
    const auto area = target.toFloat().reduced (0.5f);
    const auto top = selected ? Colour (0xfff0faff) : pearl;
    const auto bottom = selected ? blue : blush;
    drawPearl (g, area, top, bottom, 7.0f);
    if (selected)
    {
        g.setColour (Colour (0xff608bb4));
        g.drawRoundedRectangle (area.reduced (0.5f), 7.0f, 1.0f);
    }
}

void drawSlider (Graphics& g, Rectangle<int> track, Rectangle<float> grip, bool enabled, bool focused)
{
    const auto area = track.toFloat().reduced (0.5f);
    const float radius = area.getHeight() * 0.5f;
    g.setColour (blush);
    g.fillRoundedRectangle (area, radius);
    {
        const Graphics::ScopedSaveState state (g);
        Path rail;
        rail.addRoundedRectangle (area, radius);
        g.reduceClipRegion (rail);
        g.setColour (enabled ? Colour (0xffe16da5) : seam);
        g.fillRect (area.withRight (jlimit (area.getX(), area.getRight(), grip.getCentreX())));
    }
    g.setColour (focused && enabled ? rose : seam);
    g.drawRoundedRectangle (area, radius, 1.0f);
    drawStone (g, grip.getCentre(), grip.getWidth() * 0.49f);
}

void drawKnob (Graphics& g, Rectangle<float> dial, float angle, float start, float end, bool focused)
{
    const auto centre = dial.getCentre();
    const auto radius = dial.getWidth() * 0.5f;
    const auto ring = dial.withSizeKeepingCentre (radius * 2.04f, radius * 2.04f);
    g.setGradientFill (ColourGradient (blush, ring.getTopLeft(), pearl, ring.getBottomRight(), false));
    g.fillEllipse (ring);
    g.setColour (seam);
    g.drawEllipse (ring, 1.0f);

    const float arcRadius = radius * 1.13f;
    Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, start, end, true);
    const float stroke = jlimit (1.6f, 3.5f, radius * 0.047f);
    const PathStrokeType line (stroke, PathStrokeType::curved, PathStrokeType::rounded);
    g.setColour (seam);
    g.strokePath (track, line);
    if (angle > start)
    {
        Path progress;
        progress.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, start, angle, true);
        g.setColour (rose);
        g.strokePath (progress, line);
    }

    drawHeart (g, dial.reduced (radius * 0.14f));
    drawStone (g, centre.getPointOnCircumference (arcRadius, angle), jlimit (2.5f, 5.0f, radius * 0.06f));
    if (focused)
    {
        g.setColour (rose);
        g.drawEllipse (dial.expanded (radius * 0.22f), 1.1f);
    }
}
} // namespace hammer::cottonCandy
