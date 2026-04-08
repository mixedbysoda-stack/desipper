#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SipperLookAndFeel.h"
#include "ThresholdMeter.h"
#include "AttenuationMeter.h"
#include "StereoOutputMeter.h"
#include "LCDDisplay.h"
#include "SkeuoButton.h"
#include "FilterDropdown.h"
#include "../DSP/DeEsserEngine.h"

// Main plugin panel: 3-column layout matching Magic Patterns SIPPER design
// Header + Left controls + Center meters + Right output
class PluginPanel : public juce::Component, public juce::Timer
{
public:
    PluginPanel (juce::AudioProcessorValueTreeState& apvts, DeEsserEngine& engine);
    ~PluginPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    DeEsserEngine& engine;

    // Left column controls
    SkeuoButton splitButton { "Split" };
    LCDDisplay freqDisplay { "Freq" };
    FilterDropdown filterDropdown;
    SkeuoButton audioMonButton { "Audio" };
    SkeuoButton sChainMonButton { "S Chain" };

    // Center meters
    ThresholdMeter thresholdMeter;
    AttenuationMeter attenuationMeter;

    // Right meters
    StereoOutputMeter outputMeter;

    // Layout areas for label painting
    juce::Rectangle<int> filterLabelArea;
    juce::Rectangle<int> monitorLabelArea;

    void drawHeader (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawLeftColumnLabels (juce::Graphics& g);
};
