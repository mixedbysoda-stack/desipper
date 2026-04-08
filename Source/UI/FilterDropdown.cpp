#include "FilterDropdown.h"

FilterDropdown::FilterDropdown() {}

void FilterDropdown::setIsBandPass (bool bp)
{
    if (isBandPass != bp)
    {
        isBandPass = bp;
        repaint();
    }
}

void FilterDropdown::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    // Outset panel background (gradient zinc-600 → zinc-700)
    juce::ColourGradient grad (SipperColors::buttonFace, bounds.getX(), bounds.getY(),
                                SipperColors::buttonFaceDark, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, 4.0f);

    // Top highlight
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (int (bounds.getY() + 1), bounds.getX() + 3, bounds.getRight() - 3);

    // Border
    g.setColour (SipperColors::borderDark);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    // Filter curve icon on the left
    auto iconArea = bounds.withWidth (bounds.getWidth() - 20.0f).reduced (6.0f, 4.0f);
    drawFilterIcon (g, iconArea);

    // Dropdown triangle on the right
    float triX = bounds.getRight() - 14.0f;
    float triY = bounds.getCentreY() - 2.0f;
    juce::Path triangle;
    triangle.addTriangle (triX - 4.0f, triY, triX + 4.0f, triY, triX, triY + 5.0f);
    g.setColour (SipperColors::textBright);
    g.fillPath (triangle);
}

void FilterDropdown::drawFilterIcon (juce::Graphics& g, juce::Rectangle<float> area)
{
    // Draw a simple filter frequency response curve
    juce::Path curve;
    float midY = area.getCentreY();
    float left = area.getX();
    float right = area.getRight();
    float width = area.getWidth();

    if (isBandPass)
    {
        // BandPass: peak in the middle
        curve.startNewSubPath (left, midY + 4.0f);
        curve.quadraticTo (left + width * 0.3f, midY + 4.0f, left + width * 0.4f, midY - 5.0f);
        curve.quadraticTo (left + width * 0.5f, midY - 8.0f, left + width * 0.6f, midY - 5.0f);
        curve.quadraticTo (left + width * 0.7f, midY + 4.0f, right, midY + 4.0f);
    }
    else
    {
        // HighPass: rising curve from left to right
        curve.startNewSubPath (left, midY + 4.0f);
        curve.quadraticTo (left + width * 0.4f, midY + 4.0f, left + width * 0.5f, midY - 2.0f);
        curve.quadraticTo (left + width * 0.6f, midY - 6.0f, left + width * 0.7f, midY - 6.0f);
        curve.lineTo (right, midY - 6.0f);
    }

    g.setColour (SipperColors::textBright);
    g.strokePath (curve, juce::PathStrokeType (1.5f));
}

void FilterDropdown::mouseDown (const juce::MouseEvent&)
{
    isBandPass = !isBandPass;
    repaint();
    if (onClick)
        onClick();
}
