#include "SideChainFilter.h"

void SideChainFilter::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, 0, 1 };
    filter.prepare (spec);
    updateCoefficients();
}

void SideChainFilter::reset()
{
    filter.reset();
}

void SideChainFilter::setFrequency (float freqHz)
{
    if (currentFreq != freqHz)
    {
        currentFreq = freqHz;
        updateCoefficients();
    }
}

void SideChainFilter::setMode (Mode mode)
{
    if (currentMode != mode)
    {
        currentMode = mode;
        updateCoefficients();
    }
}

void SideChainFilter::process (const float* input, float* output, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        output[i] = filter.processSample (input[i]);
}

void SideChainFilter::updateCoefficients()
{
    if (currentMode == Mode::HighPass)
    {
        // 2nd-order Butterworth high-pass
        filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (
            currentSampleRate, currentFreq, 0.707f);
    }
    else
    {
        // Narrow bandpass (Q=4 for tight isolation of specific ess frequency)
        filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass (
            currentSampleRate, currentFreq, 4.0f);
    }
}
