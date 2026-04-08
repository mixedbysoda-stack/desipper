#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SipperLookAndFeel.h"

// Red/orange attenuation meter — fills from TOP down (reverse)
// Scale: 0, 3, 6, 9, 12, 18, 24, INF (dB of gain reduction)
// Matches Magic Patterns: gradient red-500→orange-500, reverse fill
class AttenuationMeter : public juce::Component
{
public:
    AttenuationMeter();

    void setAttenuationDb (float db);  // negative value = gain reduction
    void paint (juce::Graphics& g) override;
    void resized() override {}

private:
    float attenuationDb = 0.0f;  // stored as negative (e.g., -16 = 16dB reduction)

    // Scale: 0 to INF (mapped as 0 to ~40dB range)
    static constexpr float scaleValues[] = { 0.0f, 3.0f, 6.0f, 9.0f, 12.0f, 18.0f, 24.0f, 40.0f };
    static const char* const scaleLabels[];
    static constexpr int numTicks = 8;
    static constexpr float maxAttenDisplay = 40.0f;

    juce::Rectangle<float> getMeterTrackBounds() const;
};
