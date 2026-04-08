#include "ThresholdMeter.h"

ThresholdMeter::ThresholdMeter() {}

void ThresholdMeter::setThresholdDb (float db)
{
    db = juce::jlimit (minDb, maxDb, db);
    if (thresholdDb != db)
    {
        thresholdDb = db;
        repaint();
    }
}

void ThresholdMeter::setEnergyDb (float db)
{
    energyDb = db;
    repaint();
}

juce::Rectangle<float> ThresholdMeter::getMeterTrackBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    float labelH = 20.0f;
    float lcdH = 30.0f;
    float topPad = labelH + 4.0f;
    float botPad = lcdH + 4.0f;
    auto trackArea = bounds.withTrimmedTop (topPad).withTrimmedBottom (botPad);

    // Meter track on the right side, 22px wide — offset enough for fader handle overhang
    float trackW = 22.0f;
    float trackX = bounds.getRight() - trackW - 30.0f;
    return juce::Rectangle<float> (trackX, trackArea.getY(), trackW, trackArea.getHeight());
}

float ThresholdMeter::dbToY (float db) const
{
    auto track = getMeterTrackBounds();
    float normalized = (db - minDb) / (maxDb - minDb); // 0=bottom(-80), 1=top(0)
    return track.getBottom() - normalized * track.getHeight();
}

float ThresholdMeter::yToDb (float y) const
{
    auto track = getMeterTrackBounds();
    float normalized = (track.getBottom() - y) / track.getHeight();
    normalized = juce::jlimit (0.0f, 1.0f, normalized);
    return minDb + normalized * (maxDb - minDb);
}

void ThresholdMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto laf = dynamic_cast<SipperLookAndFeel*> (&getLookAndFeel());

    // --- THRESHOLD label at top ---
    g.setColour (SipperColors::textBright);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText ("THRESHOLD", bounds.removeFromTop (20.0f), juce::Justification::centredTop);
    bounds.removeFromTop (4.0f);

    // --- LCD readout at bottom ---
    auto lcdArea = bounds.removeFromBottom (30.0f);
    auto lcdRect = lcdArea.reduced (4.0f, 0.0f);
    g.setColour (SipperColors::insetBg);
    g.fillRoundedRectangle (lcdRect, 3.0f);
    g.setColour (SipperColors::borderDark);
    g.drawRoundedRectangle (lcdRect, 3.0f, 1.0f);
    // Inner shadow on LCD
    juce::ColourGradient lcdShadow (juce::Colours::black.withAlpha (0.6f),
                                     lcdRect.getX(), lcdRect.getY(),
                                     juce::Colours::transparentBlack,
                                     lcdRect.getX(), lcdRect.getY() + 4.0f, false);
    g.setGradientFill (lcdShadow);
    g.fillRoundedRectangle (lcdRect.reduced (1.0f), 2.0f);

    if (laf)
        g.setFont (laf->getLCDFont (15.0f));
    else
        g.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 15.0f, juce::Font::plain));
    // Red glow layer
    g.setColour (SipperColors::lcdRed.withAlpha (0.35f));
    g.drawText (juce::String (thresholdDb, 1), lcdRect.expanded (1.0f), juce::Justification::centred);
    g.setColour (SipperColors::lcdRed);
    g.drawText (juce::String (thresholdDb, 1), lcdRect, juce::Justification::centred);

    bounds.removeFromBottom (4.0f);

    // --- Meter track ---
    auto track = getMeterTrackBounds();

    // Scale ticks and labels on the LEFT of the track
    g.setFont (juce::Font (9.0f, juce::Font::plain));
    for (int i = 0; i < numTicks; ++i)
    {
        float y = dbToY (scaleTicks[i]);

        // Tick label
        g.setColour (SipperColors::textDim);
        auto labelRect = juce::Rectangle<float> (track.getX() - 40.0f, y - 5.5f, 30.0f, 11.0f);
        g.drawText (juce::String (int (scaleTicks[i])), labelRect, juce::Justification::centredRight);

        // Tick mark — horizontal line from label area to track edge
        float tickLen = (i % 2 == 0) ? 8.0f : 5.0f;
        float alpha = (i % 2 == 0) ? 0.5f : 0.25f;
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.drawHorizontalLine (int (y), track.getX() - tickLen - 1.0f, track.getX() - 1.0f);
    }

    // Meter track background — deep black inset
    g.setColour (SipperColors::insetBg);
    g.fillRoundedRectangle (track, 2.0f);
    // Inset shadow on track
    juce::ColourGradient trackShadow (juce::Colours::black.withAlpha (0.7f),
                                       track.getX(), track.getY(),
                                       juce::Colours::transparentBlack,
                                       track.getX(), track.getY() + 6.0f, false);
    g.setGradientFill (trackShadow);
    g.fillRoundedRectangle (track.reduced (0.5f), 2.0f);
    g.setColour (SipperColors::borderDark);
    g.drawRoundedRectangle (track, 2.0f, 1.0f);

    // --- Blue energy fill (from bottom up to energy level) ---
    float energyY = dbToY (juce::jlimit (minDb, maxDb, energyDb));
    auto fillTrack = track.reduced (1.5f);
    if (energyY < fillTrack.getBottom())
    {
        auto fillRect = fillTrack.withTop (juce::jmax (energyY, fillTrack.getY()));

        // Solid bright blue fill
        g.setColour (SipperColors::meterBlue);
        g.fillRect (fillRect);

        // Subtle blue glow on edges
        g.setColour (SipperColors::meterBlue.withAlpha (0.4f));
        g.fillRect (fillRect.withLeft (fillRect.getX() - 2.0f).withWidth (2.0f));
        g.fillRect (fillRect.withLeft (fillRect.getRight()).withWidth (2.0f));
    }

    // --- Fader handle at threshold position ---
    float faderY = dbToY (thresholdDb);
    float handleW = 54.0f;
    float handleH = 16.0f;
    auto faderBounds = juce::Rectangle<float> (handleW, handleH)
                           .withCentre ({ track.getCentreX(), faderY });

    // Handle shadow
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (faderBounds.translated (0.0f, 2.0f), 3.0f);

    // Handle body — metallic gradient (light top → dark bottom)
    juce::ColourGradient handleGrad (juce::Colour (0xffc0c0c4), faderBounds.getX(), faderBounds.getY(),
                                      juce::Colour (0xff606068), faderBounds.getX(), faderBounds.getBottom(), false);
    g.setGradientFill (handleGrad);
    g.fillRoundedRectangle (faderBounds, 3.0f);

    // Handle top highlight
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawHorizontalLine (int (faderBounds.getY() + 1.0f),
                          faderBounds.getX() + 3.0f, faderBounds.getRight() - 3.0f);

    // Handle groove (dark line through center)
    auto grooveRect = juce::Rectangle<float> (handleW * 0.6f, 2.0f)
                          .withCentre (faderBounds.getCentre());
    g.setColour (juce::Colour (0xff1a1a1e));
    g.fillRoundedRectangle (grooveRect, 1.0f);
    // Groove inner shadow
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillRect (juce::Rectangle<float> (grooveRect.getWidth(), 1.0f)
                    .withPosition (grooveRect.getX(), grooveRect.getY()));

    // Handle border
    g.setColour (juce::Colour (0xff404048));
    g.drawRoundedRectangle (faderBounds, 3.0f, 0.75f);
}

void ThresholdMeter::mouseDown (const juce::MouseEvent& e)
{
    float db = yToDb (float (e.y));
    thresholdDb = juce::jlimit (minDb, maxDb, db);
    repaint();
    if (onThresholdChanged)
        onThresholdChanged (thresholdDb);
}

void ThresholdMeter::mouseDrag (const juce::MouseEvent& e)
{
    float db = yToDb (float (e.y));
    thresholdDb = juce::jlimit (minDb, maxDb, db);
    repaint();
    if (onThresholdChanged)
        onThresholdChanged (thresholdDb);
}
