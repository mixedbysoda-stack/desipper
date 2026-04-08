#include "LCDDisplay.h"

LCDDisplay::LCDDisplay (const juce::String& labelText)
    : label (labelText)
{
}

void LCDDisplay::setValue (const juce::String& text)
{
    if (value != text)
    {
        value = text;
        numericValue = text.getFloatValue();
        repaint();
    }
}

void LCDDisplay::setDraggable (float minVal, float maxVal, float step)
{
    isDraggable = true;
    dragMin = minVal;
    dragMax = maxVal;
    dragStep = step;
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void LCDDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    float labelHeight = 0.0f;
    if (label.isNotEmpty())
    {
        labelHeight = 14.0f;
        g.setColour (SipperColors::textDim);
        g.setFont (juce::Font (10.0f, juce::Font::bold));
        g.drawText (label.toUpperCase(), bounds.removeFromTop (labelHeight),
                    juce::Justification::centred);
        bounds.removeFromTop (2.0f);
    }

    // Inset LCD panel
    auto lcdBounds = bounds;
    g.setColour (SipperColors::insetBg);
    g.fillRoundedRectangle (lcdBounds, 3.0f);

    // Inset shadow
    g.setColour (SipperColors::borderDark);
    g.drawRoundedRectangle (lcdBounds, 3.0f, 1.0f);
    juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.6f),
                                  lcdBounds.getX(), lcdBounds.getY(),
                                  juce::Colours::transparentBlack,
                                  lcdBounds.getX(), lcdBounds.getY() + 4.0f, false);
    g.setGradientFill (shadow);
    g.fillRoundedRectangle (lcdBounds.reduced (1.0f), 2.0f);

    // Glowing red text
    auto laf = dynamic_cast<SipperLookAndFeel*> (&getLookAndFeel());
    if (laf != nullptr)
        g.setFont (laf->getLCDFont (14.0f));
    else
        g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::plain));

    // Glow effect
    g.setColour (SipperColors::lcdRed.withAlpha (0.4f));
    g.drawText (value, lcdBounds.expanded (1.0f), juce::Justification::centred);
    g.setColour (SipperColors::lcdRed);
    g.drawText (value, lcdBounds, juce::Justification::centred);
}

void LCDDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (! isDraggable) return;
    dragStartY = e.y;
    dragStartValue = numericValue;
}

void LCDDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (! isDraggable) return;

    // Drag up = increase, drag down = decrease
    float deltaPixels = float (dragStartY - e.y);
    float range = dragMax - dragMin;
    float sensitivity = range / 200.0f; // full range over 200px drag
    float newVal = dragStartValue + deltaPixels * sensitivity;

    // Snap to step
    newVal = std::round (newVal / dragStep) * dragStep;
    newVal = juce::jlimit (dragMin, dragMax, newVal);

    if (newVal != numericValue)
    {
        numericValue = newVal;
        value = juce::String (int (numericValue));
        repaint();
        if (onValueChanged)
            onValueChanged (numericValue);
    }
}

void LCDDisplay::mouseUp (const juce::MouseEvent&)
{
    // nothing needed
}
