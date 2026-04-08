#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SipperLookAndFeel.h"

// Dual amber/orange vertical bar meters for stereo output
// Scale: 0 to -30 dBFS
// Matches Magic Patterns: amber-500 fill, dual bars, LCD readouts per channel
class StereoOutputMeter : public juce::Component
{
public:
    StereoOutputMeter();

    void setLevels (float leftDb, float rightDb);
    void paint (juce::Graphics& g) override;
    void resized() override {}

private:
    float leftDb = -100.0f;
    float rightDb = -100.0f;

    static constexpr float minDb = -30.0f;
    static constexpr float maxDb = 0.0f;
    static constexpr float scaleTicks[] = { 0.0f, -3.0f, -6.0f, -10.0f, -15.0f, -20.0f, -25.0f, -30.0f };
    static constexpr int numTicks = 8;

    juce::Rectangle<float> getMeterArea() const;
    float dbToFillHeight (float db) const;
};
