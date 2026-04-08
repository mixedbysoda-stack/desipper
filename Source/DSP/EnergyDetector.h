#pragma once
#include <cmath>

// Per PDF: "The energy in the filtered signal is measured and translated
// into attenuation by a hard knee compressor according to the threshold."
class EnergyDetector
{
public:
    void prepare (double sampleRate);
    void reset();

    // Returns energy level in dB (below 0 dBFS)
    float process (const float* input, int numSamples);

    // Current smoothed energy in dB
    float getCurrentLevelDb() const { return currentLevelDb; }

private:
    double currentSampleRate = 44100.0;
    float envelope = 0.0f;
    float currentLevelDb = -100.0f;

    // Attack/release for envelope follower
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
};
