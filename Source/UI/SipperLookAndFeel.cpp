#include "SipperLookAndFeel.h"

SipperLookAndFeel::SipperLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, SipperColors::background);
}

juce::Font SipperLookAndFeel::getLCDFont (float height) const
{
    return juce::Font (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain);
}

juce::Font SipperLookAndFeel::getLabelFont (float height) const
{
    return juce::Font (height, juce::Font::bold);
}

juce::Font SipperLookAndFeel::getTitleFont (float height) const
{
    return juce::Font (height, juce::Font::bold);
}

void SipperLookAndFeel::drawInsetPanel (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Inset shadow effect matching CSS: inset 0 2px 6px rgba(0,0,0,0.9)
    g.setColour (SipperColors::panelBg.withAlpha (0.4f));
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (SipperColors::borderLight.withAlpha (0.3f));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    // Inner shadow (top)
    juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.5f), bounds.getX(), bounds.getY(),
                                  juce::Colours::transparentBlack, bounds.getX(), bounds.getY() + 8.0f, false);
    g.setGradientFill (shadow);
    g.fillRoundedRectangle (bounds.reduced (1.0f), 5.0f);
}

void SipperLookAndFeel::drawOutsetPanel (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // 3D raised look
    juce::ColourGradient grad (SipperColors::buttonFace, bounds.getX(), bounds.getY(),
                                SipperColors::buttonFaceDark, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, 4.0f);

    // Highlight on top edge
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (int (bounds.getY() + 1), bounds.getX() + 2, bounds.getRight() - 2);

    g.setColour (SipperColors::borderDark);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

void SipperLookAndFeel::drawScrew (juce::Graphics& g, juce::Point<float> center, float size, float rotation)
{
    auto half = size * 0.5f;
    auto screwBounds = juce::Rectangle<float> (size, size).withCentre (center);

    // Screw body
    juce::ColourGradient grad (SipperColors::screwLight, center.x - half, center.y - half,
                                SipperColors::screwDark, center.x + half, center.y + half, false);
    g.setGradientFill (grad);
    g.fillEllipse (screwBounds);

    // Border
    g.setColour (SipperColors::borderDark);
    g.drawEllipse (screwBounds, 0.5f);

    // Slot
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (rotation, center.x, center.y));
    g.setColour (SipperColors::borderDark.withAlpha (0.9f));
    g.fillRect (juce::Rectangle<float> (size * 0.8f, 1.5f).withCentre (center));
    g.restoreState();
}

void SipperLookAndFeel::drawBrushedMetalOverlay (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Subtle vertical lines for brushed metal texture
    g.setColour (juce::Colours::white.withAlpha (0.02f));
    for (int x = bounds.getX(); x < bounds.getRight(); x += 3)
        g.drawVerticalLine (x, float (bounds.getY()), float (bounds.getBottom()));

    // Radial highlight from top center
    juce::ColourGradient highlight (juce::Colours::white.withAlpha (0.08f),
                                     float (bounds.getCentreX()), float (bounds.getY()),
                                     juce::Colours::transparentWhite,
                                     float (bounds.getCentreX()), float (bounds.getY() + bounds.getHeight() * 0.7f), true);
    g.setGradientFill (highlight);
    g.fillRect (bounds);
}
