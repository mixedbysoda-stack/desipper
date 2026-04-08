#include "CrossoverFilter.h"

void CrossoverFilter::prepare (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, 0, 1 };
    lpf1.prepare (spec);
    lpf2.prepare (spec);
    hpf1.prepare (spec);
    hpf2.prepare (spec);
    updateCoefficients();
}

void CrossoverFilter::reset()
{
    lpf1.reset();
    lpf2.reset();
    hpf1.reset();
    hpf2.reset();
}

void CrossoverFilter::setFrequency (float freqHz)
{
    if (currentFreq != freqHz)
    {
        currentFreq = freqHz;
        updateCoefficients();
    }
}

void CrossoverFilter::process (const float* input, float* lowOut, float* highOut, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = input[i];
        // Cascaded LP for LR4
        float lp = lpf1.processSample (sample);
        lp = lpf2.processSample (lp);
        lowOut[i] = lp;

        // Cascaded HP for LR4
        float hp = hpf1.processSample (sample);
        hp = hpf2.processSample (hp);
        highOut[i] = hp;
    }
}

void CrossoverFilter::updateCoefficients()
{
    // Butterworth Q=0.707 cascaded twice = Linkwitz-Riley 4th order
    auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, currentFreq, 0.707f);
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, currentFreq, 0.707f);

    lpf1.coefficients = lpCoeffs;
    lpf2.coefficients = lpCoeffs;
    hpf1.coefficients = hpCoeffs;
    hpf2.coefficients = hpCoeffs;
}
