#include "AttenuationMeter.h"

const char* const AttenuationMeter::scaleLabels[] = { "0", "3", "6", "9", "12", "18", "24", "INF" };

AttenuationMeter::AttenuationMeter() {}

void AttenuationMeter::setAttenuationDb (float db)
{
    attenuationDb = db;
    repaint();
}

juce::Rectangle<float> AttenuationMeter::getMeterTrackBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    float labelH = 18.0f;
    float lcdH = 28.0f;
    auto trackArea = bounds.reduced (0, 2.0f).withTrimmedTop (labelH).withTrimmedBottom (lcdH);
    return trackArea.removeFromRight (20.0f).reduced (0, 4.0f);
}

void AttenuationMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto laf = dynamic_cast<SipperLookAndFeel*> (&getLookAndFeel());

    // Label
    g.setColour (SipperColors::textBright);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("ATTEN", bounds.removeFromTop (18.0f), juce::Justification::centredTop);

    // LCD readout
    auto lcdArea = bounds.removeFromBottom (28.0f);
    g.setColour (SipperColors::insetBg);
    g.fillRoundedRectangle (lcdArea.reduced (8.0f, 0.0f), 3.0f);
    g.setColour (SipperColors::borderDark);
    g.drawRoundedRectangle (lcdArea.reduced (8.0f, 0.0f), 3.0f, 1.0f);

    if (laf)
        g.setFont (laf->getLCDFont (14.0f));
    g.setColour (SipperColors::lcdRed);
    float displayVal = std::abs (attenuationDb);
    g.drawText (juce::String (displayVal, 1), lcdArea.reduced (8.0f, 0.0f), juce::Justification::centred);

    auto trackArea = bounds.reduced (0, 4.0f);

    // Scale ticks
    auto track = getMeterTrackBounds();
    g.setFont (juce::Font (9.0f));
    for (int i = 0; i < numTicks; ++i)
    {
        float normalized = scaleValues[i] / maxAttenDisplay;
        normalized = juce::jlimit (0.0f, 1.0f, normalized);
        float y = track.getY() + normalized * track.getHeight();

        g.setColour (SipperColors::textDim);
        g.drawText (scaleLabels[i],
                    juce::Rectangle<float> (trackArea.getX(), y - 5.0f, trackArea.getWidth() - 24.0f, 10.0f),
                    juce::Justification::centredRight);

        float tickLen = (i % 2 == 0) ? 10.0f : 6.0f;
        float alpha = (i % 2 == 0) ? 0.6f : 0.3f;
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.drawHorizontalLine (int (y), track.getX() - tickLen - 2.0f, track.getX() - 2.0f);
    }

    // Meter track background
    g.setColour (SipperColors::insetBg);
    g.fillRoundedRectangle (track, 2.0f);
    g.setColour (SipperColors::borderDark);
    g.drawRoundedRectangle (track, 2.0f, 1.0f);

    // Red→Orange fill from TOP (reverse direction per Magic Patterns)
    float absAtten = std::abs (attenuationDb);
    float fillNorm = juce::jlimit (0.0f, 1.0f, absAtten / maxAttenDisplay);
    if (fillNorm > 0.0f)
    {
        auto fillRect = track.reduced (1.0f);
        float fillH = fillNorm * fillRect.getHeight();
        fillRect = fillRect.withHeight (fillH);

        // Gradient: red at top → orange at bottom
        juce::ColourGradient grad (SipperColors::meterRed, fillRect.getX(), fillRect.getY(),
                                    SipperColors::meterOrange, fillRect.getX(), fillRect.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRect (fillRect);

        // Glow
        g.setColour (SipperColors::meterRed.withAlpha (0.3f));
        g.fillRect (fillRect.expanded (2.0f, 0.0f));
    }
}
