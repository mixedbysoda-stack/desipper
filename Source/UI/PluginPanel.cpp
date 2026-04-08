#include "PluginPanel.h"

PluginPanel::PluginPanel (juce::AudioProcessorValueTreeState& vts, DeEsserEngine& eng)
    : apvts (vts), engine (eng)
{
    addAndMakeVisible (splitButton);
    addAndMakeVisible (freqDisplay);
    addAndMakeVisible (filterDropdown);
    addAndMakeVisible (audioMonButton);
    addAndMakeVisible (sChainMonButton);
    addAndMakeVisible (thresholdMeter);
    addAndMakeVisible (attenuationMeter);
    addAndMakeVisible (outputMeter);

    // Initialize button states from parameters
    splitButton.setToggled (apvts.getRawParameterValue ("split")->load() > 0.5f);
    filterDropdown.setIsBandPass (apvts.getRawParameterValue ("bandpass")->load() > 0.5f);
    audioMonButton.setToggled (apvts.getRawParameterValue ("monitor")->load() < 0.5f);
    sChainMonButton.setToggled (apvts.getRawParameterValue ("monitor")->load() > 0.5f);

    // Button callbacks
    splitButton.setClickCallback ([this]
    {
        auto param = apvts.getParameter ("split");
        param->setValueNotifyingHost (splitButton.isToggled() ? 1.0f : 0.0f);
    });

    filterDropdown.setClickCallback ([this]
    {
        auto param = apvts.getParameter ("bandpass");
        param->setValueNotifyingHost (filterDropdown.getIsBandPass() ? 1.0f : 0.0f);
    });

    audioMonButton.setClickCallback ([this]
    {
        audioMonButton.setToggled (true);
        sChainMonButton.setToggled (false);
        apvts.getParameter ("monitor")->setValueNotifyingHost (0.0f);
    });

    sChainMonButton.setClickCallback ([this]
    {
        sChainMonButton.setToggled (true);
        audioMonButton.setToggled (false);
        apvts.getParameter ("monitor")->setValueNotifyingHost (1.0f);
    });

    // Threshold fader callback
    thresholdMeter.onThresholdChanged = [this] (float db)
    {
        auto param = apvts.getParameter ("threshold");
        auto range = param->getNormalisableRange();
        param->setValueNotifyingHost (range.convertTo0to1 (db));
    };

    // Frequency display: draggable 1000–12000 Hz
    freqDisplay.setDraggable (1000.0f, 12000.0f, 1.0f);
    freqDisplay.setValue (juce::String (int (apvts.getRawParameterValue ("frequency")->load())));
    freqDisplay.onValueChanged = [this] (float freq)
    {
        auto param = apvts.getParameter ("frequency");
        auto range = param->getNormalisableRange();
        param->setValueNotifyingHost (range.convertTo0to1 (freq));
    };

    startTimerHz (30); // 30fps meter updates
}

PluginPanel::~PluginPanel()
{
    stopTimer();
}

void PluginPanel::drawHeader (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Header bar: gradient zinc-800 → zinc-700 → zinc-800
    juce::ColourGradient headerGrad (SipperColors::headerGradEnd, bounds.getX(), bounds.getCentreY(),
                                      SipperColors::headerGradStart, bounds.getCentreX(), bounds.getCentreY(), false);
    headerGrad.addColour (0.5, SipperColors::headerGradStart);
    headerGrad.addColour (1.0, SipperColors::headerGradEnd);
    g.setGradientFill (headerGrad);
    g.fillRoundedRectangle (bounds, 5.0f);

    // Border
    g.setColour (SipperColors::borderLight);
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

    // Top highlight
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawHorizontalLine (int (bounds.getY() + 1), bounds.getX() + 4, bounds.getRight() - 4);

    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawHorizontalLine (int (bounds.getBottom() + 1), bounds.getX(), bounds.getRight());

    // Left screw
    SipperLookAndFeel::drawScrew (g, { bounds.getX() + 20.0f, bounds.getCentreY() }, 12.0f, 0.785f);

    // "SIPPER" title — wide letter-spacing matching Magic Patterns tracking-[0.2em]
    g.setColour (SipperColors::textBright);
    g.setFont (juce::Font ("Impact", 36.0f, juce::Font::plain));
    auto titleArea = bounds.withTrimmedLeft (40.0f).withWidth (320.0f);
    // Draw each letter with spacing to match tracking-[0.2em]
    juce::String title = "D E - S I P P E R";
    g.drawText (title, titleArea, juce::Justification::centredLeft);

    // Right side: "Carbonated Audio" + "Professional De-Esser"
    auto rightText = bounds.withTrimmedRight (40.0f).removeFromRight (220.0f);
    g.setColour (SipperColors::textBright);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    // Wide letter-spacing matching tracking-[0.3em]
    g.drawText ("C A R B O N A T E D   A U D I O", rightText.removeFromTop (rightText.getHeight() * 0.55f),
                juce::Justification::centredRight);
    g.setColour (SipperColors::textDim);
    g.setFont (juce::Font (9.0f, juce::Font::plain));
    g.drawText ("PROFESSIONAL DE-ESSER", rightText, juce::Justification::centredRight);

    // Right screw
    SipperLookAndFeel::drawScrew (g, { bounds.getRight() - 20.0f, bounds.getCentreY() }, 12.0f, -0.2f);
}

void PluginPanel::drawLeftColumnLabels (juce::Graphics& g)
{
    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (SipperColors::textDim);

    // "SIDECHAIN" label — per PDF: toggles between HighPass and BandPass sidechain filter
    if (! filterLabelArea.isEmpty())
        g.drawText ("SIDECHAIN", filterLabelArea.toFloat(), juce::Justification::centred);

    // "MONITOR" label
    if (! monitorLabelArea.isEmpty())
        g.drawText ("MONITOR", monitorLabelArea.toFloat(), juce::Justification::centred);
}

void PluginPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Main background
    g.fillAll (SipperColors::background);

    // Brushed metal overlay
    SipperLookAndFeel::drawBrushedMetalOverlay (g, bounds);

    // Main panel border
    auto panelBounds = bounds.toFloat().reduced (4.0f);
    g.setColour (SipperColors::borderLight.withAlpha (0.5f));
    g.drawRoundedRectangle (panelBounds, 8.0f, 1.0f);

    // Inner shadow
    juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.3f),
                                  panelBounds.getX(), panelBounds.getY(),
                                  juce::Colours::transparentBlack,
                                  panelBounds.getX(), panelBounds.getY() + 15.0f, false);
    g.setGradientFill (shadow);
    g.fillRoundedRectangle (panelBounds, 8.0f);

    // Header
    auto headerBounds = bounds.toFloat().reduced (20.0f, 0).withHeight (50.0f).translated (0, 16.0f);
    drawHeader (g, headerBounds);

    // Center panel inset background
    auto centerArea = bounds.reduced (20, 0).withTrimmedTop (80).withTrimmedBottom (12);
    auto leftW = 110;
    auto rightW = 160;
    auto centerBounds = centerArea.withTrimmedLeft (leftW + 12).withTrimmedRight (rightW + 12).toFloat();
    SipperLookAndFeel::drawInsetPanel (g, centerBounds);

    // Right panel inset background
    auto rightArea2 = centerArea;
    auto rightBounds = rightArea2.removeFromRight (rightW).toFloat();
    SipperLookAndFeel::drawInsetPanel (g, rightBounds);

    // Left panel inset background
    auto leftBounds = rightArea2.removeFromLeft (leftW).toFloat();
    SipperLookAndFeel::drawInsetPanel (g, leftBounds);

    // Draw left column labels
    drawLeftColumnLabels (g);
}

void PluginPanel::resized()
{
    auto bounds = getLocalBounds().reduced (20, 0);

    // Skip header area
    bounds.removeFromTop (80);
    bounds.removeFromBottom (12);

    // 3-column layout
    int leftW = 110;
    int rightW = 160;
    int gap = 12;

    auto leftArea = bounds.removeFromLeft (leftW);
    bounds.removeFromLeft (gap);
    auto rightArea = bounds.removeFromRight (rightW);
    bounds.removeFromRight (gap);
    auto centerArea = bounds;

    // Left column controls — matching Magic Patterns layout:
    // Split button → Freq display → Filter label + dropdown → Monitor label + Audio + S Chain
    auto leftInner = leftArea.reduced (10, 16);

    // Split button at top
    splitButton.setBounds (leftInner.removeFromTop (32));
    leftInner.removeFromTop (24);

    // Freq LCD display
    freqDisplay.setBounds (leftInner.removeFromTop (42));
    leftInner.removeFromTop (24);

    // "FILTER" label
    filterLabelArea = leftInner.removeFromTop (14);
    leftInner.removeFromTop (4);

    // Filter dropdown
    filterDropdown.setBounds (leftInner.removeFromTop (32));

    // Monitor section at bottom
    auto monitorArea = leftArea.reduced (10, 0);
    auto bottomSection = monitorArea.removeFromBottom (100).translated (0, -16);

    // "MONITOR" label
    monitorLabelArea = bottomSection.removeFromTop (14);
    bottomSection.removeFromTop (6);

    // Audio button
    audioMonButton.setBounds (bottomSection.removeFromTop (30));
    bottomSection.removeFromTop (8);

    // S Chain button
    sChainMonButton.setBounds (bottomSection.removeFromTop (30));

    // Center column: Threshold + Attenuation meters
    auto centerInner = centerArea.reduced (16, 8);
    int meterW = centerInner.getWidth() / 2;
    thresholdMeter.setBounds (centerInner.removeFromLeft (meterW));
    attenuationMeter.setBounds (centerInner);

    // Right column: Stereo output
    outputMeter.setBounds (rightArea.reduced (8, 8));
}

void PluginPanel::timerCallback()
{
    // Update meters from engine
    thresholdMeter.setEnergyDb (engine.getSideChainEnergyDb());
    thresholdMeter.setThresholdDb (*apvts.getRawParameterValue ("threshold"));
    attenuationMeter.setAttenuationDb (engine.getAttenuationDb());
    outputMeter.setLevels (engine.getOutputLevelDbL(), engine.getOutputLevelDbR());
    freqDisplay.setValue (juce::String (int (*apvts.getRawParameterValue ("frequency"))));
}
