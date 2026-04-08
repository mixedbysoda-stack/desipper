#pragma once
#include <juce_dsp/juce_dsp.h>
#include "SideChainFilter.h"
#include "EnergyDetector.h"
#include "CrossoverFilter.h"
#include <vector>

// Main de-esser engine following the Waves DeEsser internal design:
// "Signals entering DeEsser are fed into the audio path and SideChain.
//  In the SideChain, audio is filtered and translated into attenuation.
//  This attenuation is then applied to the audio path, giving the output."
class DeEsserEngine
{
public:
    enum class AudioMode { Wideband, Split };
    enum class MonitorMode { Audio, SideChain };

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    // Internal sidechain: filter the main input per PDF design
    void process (juce::AudioBuffer<float>& buffer);

    // External sidechain: use an external signal for energy detection
    // (Pro Tools key input / AAX sidechain bus)
    void processWithExternalSideChain (juce::AudioBuffer<float>& buffer,
                                        const float* externalScData, int numSamples);

    // Parameters
    void setThreshold (float thresholdDb);
    void setFrequency (float freqHz);
    void setAudioMode (AudioMode mode);
    void setSideChainFilterMode (SideChainFilter::Mode mode);
    void setMonitorMode (MonitorMode mode);

    // Meter readouts
    float getSideChainEnergyDb() const { return sideChainEnergyDb; }
    float getAttenuationDb() const { return currentAttenuationDb; }
    float getOutputLevelDbL() const { return outputLevelDbL; }
    float getOutputLevelDbR() const { return outputLevelDbR; }
    float getPeakAttenuationDb() const { return peakAttenuationDb; }
    void resetPeakAttenuation() { peakAttenuationDb = 0.0f; }

private:
    // Per PDF signal flow
    SideChainFilter sideChainFilterL, sideChainFilterR;
    EnergyDetector energyDetectorL, energyDetectorR;
    CrossoverFilter crossoverL, crossoverR;

    // Internal buffers
    std::vector<float> sideChainBuf;
    std::vector<float> lowBuf, highBuf;

    // State
    double currentSampleRate = 44100.0;
    float thresholdDb = -16.0f;
    float currentFreq = 5506.0f;
    AudioMode audioMode = AudioMode::Split;
    MonitorMode monitorMode = MonitorMode::Audio;

    // Meter values (updated per block)
    float sideChainEnergyDb = -100.0f;
    float currentAttenuationDb = 0.0f;
    float peakAttenuationDb = 0.0f;
    float outputLevelDbL = -100.0f;
    float outputLevelDbR = -100.0f;

    // Output level smoothing
    float outputEnvelopeL = 0.0f;
    float outputEnvelopeR = 0.0f;
    float outputAttackCoeff = 0.0f;
    float outputReleaseCoeff = 0.0f;

    // Per PDF: "hard knee compressor" — compute gain reduction from energy vs threshold
    float computeAttenuation (float energyDb) const;

    // Shared: apply attenuation to audio path + output metering
    void applyAttenuationAndMeter (float* channelData, int ch, int numSamples, float gainLinear);
};
