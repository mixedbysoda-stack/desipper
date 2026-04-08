#include "StereoOutputMeter.h"

StereoOutputMeter::StereoOutputMeter() {}

void StereoOutputMeter::setLevels (float l, float r)
{
    leftDb = l;
    rightDb = r;
    repaint();
}

juce::Rectangle<float> StereoOutputMeter::getMeterArea() const
{
    auto bounds = getLocalBounds().toFloat();
    float labelH = 18.0f;
    float lcdH = 28.0f;
    return bounds.reduced (0, 2.0f).withTrimmedTop (labelH).withTrimmedBottom (lcdH).reduced (0, 4.0f);
}

float StereoOutputMeter::dbToFillHeight (float db) const
{
    float clamped = juce::jlimit (minDb, maxDb, db);
    float normalized = (clamped - minDb) / (maxDb - minDb);
    return normalized * getMeterArea().getHeight();
}

void StereoOutputMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto laf = dynamic_cast<SipperLookAndFeel*> (&getLookAndFeel());

    // Label
    g.setColour (SipperColors::textBright);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("OUTPUT", bounds.removeFromTop (18.0f), juce::Justification::centredTop);

    // LCD readouts at bottom
    auto lcdArea = bounds.removeFromBottom (28.0f);
    float lcdW = 52.0f;
    float gap = 4.0f;
    float totalW = lcdW * 2 + gap;
    float lcdX = lcdArea.getCentreX() - totalW * 0.5f;

    auto drawLCD = [&] (float x, float db)
    {
        auto rect = juce::Rectangle<float> (x, lcdArea.getY(), lcdW, lcdArea.getHeight());
        g.setColour (SipperColors::insetBg);
        g.fillRoundedRectangle (rect, 3.0f);
        g.setColour (SipperColors::borderDark);
        g.drawRoundedRectangle (rect, 3.0f, 1.0f);

        if (laf)
            g.setFont (laf->getLCDFont (12.0f));
        g.setColour (SipperColors::lcdRed);
        g.drawText (juce::String (db, 1), rect, juce::Justification::centred);
    };

    drawLCD (lcdX, leftDb);
    drawLCD (lcdX + lcdW + gap, rightDb);

    auto meterArea = getMeterArea();

    // Scale ticks (left side)
    g.setFont (juce::Font (9.0f));
    for (int i = 0; i < numTicks; ++i)
    {
        float normalized = (scaleTicks[i] - minDb) / (maxDb - minDb);
        float y = meterArea.getBottom() - normalized * meterArea.getHeight();

        g.setColour (SipperColors::textDim);
        g.drawText (juce::String (int (scaleTicks[i])),
                    juce::Rectangle<float> (meterArea.getX(), y - 5.0f, 28.0f, 10.0f),
                    juce::Justification::centredRight);

        float tickLen = (i % 2 == 0) ? 10.0f : 6.0f;
        float alpha = (i % 2 == 0) ? 0.6f : 0.3f;
        g.setColour (juce::Colours::white.withAlpha (alpha));
        float tickX = meterArea.getX() + 32.0f;
        g.drawHorizontalLine (int (y), tickX - tickLen, tickX);
    }

    // Meter bars
    float barW = 16.0f;
    float barGap = 6.0f;
    float barsX = meterArea.getRight() - barW * 2 - barGap;

    auto drawBar = [&] (float x, float db)
    {
        auto barTrack = juce::Rectangle<float> (x, meterArea.getY(), barW, meterArea.getHeight());
        g.setColour (SipperColors::insetBg);
        g.fillRoundedRectangle (barTrack, 2.0f);
        g.setColour (SipperColors::borderDark);
        g.drawRoundedRectangle (barTrack, 2.0f, 1.0f);

        float fillH = dbToFillHeight (db);
        if (fillH > 0.0f)
        {
            auto fillRect = barTrack.reduced (1.0f);
            fillRect = fillRect.withTop (fillRect.getBottom() - fillH);
            g.setColour (SipperColors::meterAmber.withAlpha (0.9f));
            g.fillRect (fillRect);

            // Glow
            g.setColour (SipperColors::meterAmber.withAlpha (0.3f));
            g.fillRect (fillRect.expanded (2.0f, 0.0f));
        }
    };

    drawBar (barsX, leftDb);
    drawBar (barsX + barW + barGap, rightDb);
}
