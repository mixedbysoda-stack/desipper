#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SipperLookAndFeel.h"

// Blue-filled vertical meter with draggable fader handle
// Scale: 0 to -80 dB, labeled "Threshold"
// Matches Magic Patterns: blue-500 fill, fader handle with zinc gradient
class ThresholdMeter : public juce::Component
{
public:
    ThresholdMeter();

    void setThresholdDb (float db);
    void setEnergyDb (float db);
    void paint (juce::Graphics& g) override;
    void resized() override {}

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

    std::function<void (float)> onThresholdChanged;

private:
    float thresholdDb = -16.0f;
    float energyDb = -100.0f;

    static constexpr float minDb = -80.0f;
    static constexpr float maxDb = 0.0f;

    juce::Rectangle<float> getMeterTrackBounds() const;
    float dbToY (float db) const;
    float yToDb (float y) const;

    // Scale ticks matching Magic Patterns: [0, -10, -20, -30, -40, -50, -60, -80]
    static constexpr float scaleTicks[] = { 0.0f, -10.0f, -20.0f, -30.0f, -40.0f, -50.0f, -60.0f, -80.0f };
    static constexpr int numTicks = 8;
};
