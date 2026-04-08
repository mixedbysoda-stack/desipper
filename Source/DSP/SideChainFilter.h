#pragma once
#include <juce_dsp/juce_dsp.h>

// SideChain filter: HighPass or BandPass mode
// Per PDF: "The filter has two modes of operation: HighPass and BandPass."
// "HighPass mode is useful for attenuating several different 'ess' sounds."
// "BandPass mode is more suitable for attenuating a specific high frequency."
class SideChainFilter
{
public:
    enum class Mode { HighPass, BandPass };

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();
    void setFrequency (float freqHz);
    void setMode (Mode mode);
    void process (const float* input, float* output, int numSamples);

private:
    void updateCoefficients();

    juce::dsp::IIR::Filter<float> filter;
    double currentSampleRate = 44100.0;
    float currentFreq = 5506.0f;
    Mode currentMode = Mode::HighPass;
};
