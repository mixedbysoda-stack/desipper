#include "DeEsserEngine.h"
#include <algorithm>
#include <cmath>

void DeEsserEngine::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    sideChainFilterL.prepare (sampleRate, samplesPerBlock);
    sideChainFilterR.prepare (sampleRate, samplesPerBlock);
    energyDetectorL.prepare (sampleRate);
    energyDetectorR.prepare (sampleRate);
    crossoverL.prepare (sampleRate, samplesPerBlock);
    crossoverR.prepare (sampleRate, samplesPerBlock);

    sideChainBuf.resize (size_t (samplesPerBlock));
    lowBuf.resize (size_t (samplesPerBlock));
    highBuf.resize (size_t (samplesPerBlock));

    // Output meter smoothing
    outputAttackCoeff  = 1.0f - std::exp (-1.0f / (float (sampleRate) * 0.001f));
    outputReleaseCoeff = 1.0f - std::exp (-1.0f / (float (sampleRate) * 0.300f));

    reset();
}

void DeEsserEngine::reset()
{
    sideChainFilterL.reset();
    sideChainFilterR.reset();
    energyDetectorL.reset();
    energyDetectorR.reset();
    crossoverL.reset();
    crossoverR.reset();
    outputEnvelopeL = 0.0f;
    outputEnvelopeR = 0.0f;
    sideChainEnergyDb = -100.0f;
    currentAttenuationDb = 0.0f;
    peakAttenuationDb = 0.0f;
    outputLevelDbL = -100.0f;
    outputLevelDbR = -100.0f;
}

void DeEsserEngine::setThreshold (float db)         { thresholdDb = db; }
void DeEsserEngine::setFrequency (float freqHz)
{
    currentFreq = freqHz;
    sideChainFilterL.setFrequency (freqHz);
    sideChainFilterR.setFrequency (freqHz);
    crossoverL.setFrequency (freqHz);
    crossoverR.setFrequency (freqHz);
}

void DeEsserEngine::setAudioMode (AudioMode mode)   { audioMode = mode; }

void DeEsserEngine::setSideChainFilterMode (SideChainFilter::Mode mode)
{
    sideChainFilterL.setMode (mode);
    sideChainFilterR.setMode (mode);
}

void DeEsserEngine::setMonitorMode (MonitorMode mode) { monitorMode = mode; }

float DeEsserEngine::computeAttenuation (float energyDb) const
{
    // Per PDF: "When the SideChain energy is below threshold, the compressor
    // generates no attenuation. When energy exceeds the threshold, attenuation
    // is generated – more energy above threshold means more attenuation."
    // Hard knee: 1:1 ratio below threshold, infinite ratio above
    if (energyDb <= thresholdDb)
        return 0.0f;

    return -(energyDb - thresholdDb);
}

void DeEsserEngine::applyAttenuationAndMeter (float* channelData, int ch, int numSamples, float gainLinear)
{
    auto& crossover = (ch == 0) ? crossoverL : crossoverR;

    if (monitorMode == MonitorMode::SideChain)
    {
        // Per PDF: "Use this to listen to just signals in the SideChain."
        for (int i = 0; i < numSamples; ++i)
            channelData[i] = sideChainBuf[size_t (i)];
    }
    else if (audioMode == AudioMode::Split)
    {
        // Per PDF: "In Split mode, audio is split into high and low frequencies.
        // Attenuation is only applied to the high frequencies."
        crossover.process (channelData, lowBuf.data(), highBuf.data(), numSamples);
        for (int i = 0; i < numSamples; ++i)
            channelData[i] = lowBuf[size_t (i)] + highBuf[size_t (i)] * gainLinear;
    }
    else
    {
        // Per PDF: "In Wideband mode, attenuation is applied to the entire audio."
        for (int i = 0; i < numSamples; ++i)
            channelData[i] *= gainLinear;
    }

    // Output metering
    float& outEnv = (ch == 0) ? outputEnvelopeL : outputEnvelopeR;
    for (int i = 0; i < numSamples; ++i)
    {
        float absVal = std::abs (channelData[i]);
        float coeff = (absVal > outEnv) ? outputAttackCoeff : outputReleaseCoeff;
        outEnv += coeff * (absVal - outEnv);
    }

    float& outDb = (ch == 0) ? outputLevelDbL : outputLevelDbR;
    outDb = (outEnv > 1e-10f) ? 20.0f * std::log10 (outEnv) : -100.0f;
}

// ============================================================================
// Internal sidechain: per PDF, filter the main input to detect energy
// ============================================================================
void DeEsserEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0) return;

    if (sideChainBuf.size() < size_t (numSamples))
    {
        sideChainBuf.resize (size_t (numSamples));
        lowBuf.resize (size_t (numSamples));
        highBuf.resize (size_t (numSamples));
    }

    for (int ch = 0; ch < juce::jmin (numChannels, 2); ++ch)
    {
        float* channelData = buffer.getWritePointer (ch);
        auto& scFilter = (ch == 0) ? sideChainFilterL : sideChainFilterR;
        auto& detector = (ch == 0) ? energyDetectorL : energyDetectorR;

        // Step 1: SideChain — filter the input
        scFilter.process (channelData, sideChainBuf.data(), numSamples);

        // Step 2: Measure energy of filtered sidechain signal
        float energyDb = detector.process (sideChainBuf.data(), numSamples);

        // Step 3: Compute attenuation from hard-knee compressor
        float attenDb = computeAttenuation (energyDb);
        float gainLinear = std::pow (10.0f, attenDb / 20.0f);

        // Update meter values from left channel
        if (ch == 0)
        {
            sideChainEnergyDb = energyDb;
            currentAttenuationDb = attenDb;
            if (std::abs (attenDb) > std::abs (peakAttenuationDb))
                peakAttenuationDb = attenDb;
        }

        // Step 4: Apply attenuation to audio path + meter output
        applyAttenuationAndMeter (channelData, ch, numSamples, gainLinear);
    }
}

// ============================================================================
// External sidechain: use an external key signal for energy detection
// (Pro Tools AAX sidechain bus / VST3 sidechain)
// The sidechain filter is still applied to the external signal — this lets
// the user still control HP/BP mode and frequency to shape detection.
// ============================================================================
void DeEsserEngine::processWithExternalSideChain (juce::AudioBuffer<float>& buffer,
                                                    const float* externalScData, int numSamples)
{
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);

    if (numSamples == 0) return;

    if (sideChainBuf.size() < size_t (numSamples))
    {
        sideChainBuf.resize (size_t (numSamples));
        lowBuf.resize (size_t (numSamples));
        highBuf.resize (size_t (numSamples));
    }

    // Filter the external sidechain signal (same HP/BP + Freq controls apply)
    sideChainFilterL.process (externalScData, sideChainBuf.data(), numSamples);

    // Detect energy from the filtered external signal
    float energyDb = energyDetectorL.process (sideChainBuf.data(), numSamples);
    float attenDb = computeAttenuation (energyDb);
    float gainLinear = std::pow (10.0f, attenDb / 20.0f);

    // Update meters
    sideChainEnergyDb = energyDb;
    currentAttenuationDb = attenDb;
    if (std::abs (attenDb) > std::abs (peakAttenuationDb))
        peakAttenuationDb = attenDb;

    // Apply the same attenuation to both main audio channels
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer (ch);
        applyAttenuationAndMeter (channelData, ch, numSamples, gainLinear);
    }
}
