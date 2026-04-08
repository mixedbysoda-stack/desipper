#include "EnergyDetector.h"
#include <algorithm>

void EnergyDetector::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    // Fast attack (~1ms), moderate release (~50ms) for sibilance detection
    attackCoeff  = 1.0f - std::exp (-1.0f / (float (sampleRate) * 0.001f));
    releaseCoeff = 1.0f - std::exp (-1.0f / (float (sampleRate) * 0.050f));
    reset();
}

void EnergyDetector::reset()
{
    envelope = 0.0f;
    currentLevelDb = -100.0f;
}

float EnergyDetector::process (const float* input, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float absVal = std::abs (input[i]);
        float coeff = (absVal > envelope) ? attackCoeff : releaseCoeff;
        envelope += coeff * (absVal - envelope);
    }

    // Convert to dB
    if (envelope > 1e-10f)
        currentLevelDb = 20.0f * std::log10 (envelope);
    else
        currentLevelDb = -100.0f;

    return currentLevelDb;
}
