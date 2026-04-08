#include "SkeuoButton.h"

SkeuoButton::SkeuoButton (const juce::String& text)
    : label (text)
{
}

void SkeuoButton::setToggled (bool toggled)
{
    if (active != toggled)
    {
        active = toggled;
        repaint();
    }
}

void SkeuoButton::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);

    if (active)
    {
        // Pressed/active state: inset shadow
        g.setColour (SipperColors::buttonFaceDark.darker (0.3f));
        g.fillRoundedRectangle (bounds, 4.0f);

        // Inset shadow
        juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.5f),
                                      bounds.getX(), bounds.getY(),
                                      juce::Colours::transparentBlack,
                                      bounds.getX(), bounds.getY() + 5.0f, false);
        g.setGradientFill (shadow);
        g.fillRoundedRectangle (bounds, 4.0f);

        // LED indicator
        auto ledCenter = juce::Point<float> (bounds.getCentreX(), bounds.getY() + 5.0f);
        g.setColour (SipperColors::ledYellow.withAlpha (0.7f));
        g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (ledCenter));
        g.setColour (SipperColors::ledYellow);
        g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (ledCenter));
    }
    else
    {
        // Raised/outset state
        juce::ColourGradient grad (SipperColors::buttonFace, bounds.getX(), bounds.getY(),
                                    SipperColors::buttonFaceDark, bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, 4.0f);

        // Top highlight
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawHorizontalLine (int (bounds.getY() + 1), bounds.getX() + 3, bounds.getRight() - 3);
    }

    g.setColour (SipperColors::borderDark);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    // Label
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (active ? SipperColors::ledYellow : SipperColors::textBright.withAlpha (0.8f));
    g.drawText (label.toUpperCase(), bounds, juce::Justification::centred);
}

void SkeuoButton::mouseDown (const juce::MouseEvent&)
{
    active = !active;
    repaint();
    if (onClick)
        onClick();
}
