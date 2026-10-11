#include "PluginProcessor.h"
#include "PluginEditor.h"

SipperProcessor::SipperProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                        // External sidechain input — used by Pro Tools (AAX) and supported hosts
                        // When active, the sidechain signal drives the energy detector
                        // instead of the internal filtered input
                        .withInput  ("Sidechain", juce::AudioChannelSet::mono(), false)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SipperProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Threshold: per PDF, controls level above which attenuation occurs
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -16.0f));

    // Frequency: sidechain filter frequency (1kHz–12kHz range)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "frequency", 1 }, "Frequency",
        juce::NormalisableRange<float> (1000.0f, 12000.0f, 1.0f, 0.3f), 5506.0f));

    // Audio mode: 0=Wideband, 1=Split
    // Per PDF: "Toggles between Wideband and Split compression modes"
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "split", 1 }, "Split", true));

    // SideChain filter: 0=HighPass, 1=BandPass
    // Per PDF: "Toggles between a HighPass and BandPass filter"
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bandpass", 1 }, "BandPass", false));

    // Monitor: 0=Audio, 1=SideChain
    // Per PDF: "Use this to listen to just signals in the SideChain"
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "monitor", 1 }, "Monitor SideChain", false));

    return { params.begin(), params.end() };
}

bool SipperProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Main input must be stereo
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Main output must be stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Sidechain input (bus index 1) — accept disabled, mono, or stereo
    auto scLayout = layouts.getChannelSet (true, 1);
    if (! scLayout.isDisabled()
        && scLayout != juce::AudioChannelSet::mono()
        && scLayout != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void SipperProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);

    scMonoBuf.resize (size_t (samplesPerBlock));
    scHoldSamples = int (sampleRate * scHoldSeconds);
    scHoldRemaining = 0;
    externalScActive.store (false, std::memory_order_relaxed);
}

void SipperProcessor::releaseResources()
{
    engine.reset();
    scHoldRemaining = 0;
    externalScActive.store (false, std::memory_order_relaxed);
}

void SipperProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Update engine from parameter state
    engine.setThreshold (*apvts.getRawParameterValue ("threshold"));
    engine.setFrequency (*apvts.getRawParameterValue ("frequency"));
    engine.setAudioMode (apvts.getRawParameterValue ("split")->load() > 0.5f
                             ? DeEsserEngine::AudioMode::Split
                             : DeEsserEngine::AudioMode::Wideband);
    engine.setSideChainFilterMode (apvts.getRawParameterValue ("bandpass")->load() > 0.5f
                                       ? SideChainFilter::Mode::BandPass
                                       : SideChainFilter::Mode::HighPass);
    engine.setMonitorMode (apvts.getRawParameterValue ("monitor")->load() > 0.5f
                               ? DeEsserEngine::MonitorMode::SideChain
                               : DeEsserEngine::MonitorMode::Audio);

    // Only the main bus is processed and written to the output; sidechain
    // channels (if any) live beyond it in the same buffer and are read-only.
    auto mainBuffer = getBusBuffer (buffer, false, 0);
    const int numSamples = buffer.getNumSamples();

    // Check for external sidechain input (bus index 1)
    // In Pro Tools (AAX), this is the key/sidechain input. Some hosts (Digital
    // Performer) enable this bus by default with nothing routed to it, so the
    // key is only used while it is actually carrying signal; otherwise the
    // internal sidechain (filtered main input) drives the detector.
    bool hasExternalSc = false;
    auto* scBus = getBus (true, 1);
    if (scBus != nullptr && scBus->isEnabled() && scBus->getNumberOfChannels() > 0
        && scBus->getChannelIndexInProcessBlockBuffer (0) + scBus->getNumberOfChannels()
               <= buffer.getNumChannels())
    {
        auto scBuffer = getBusBuffer (buffer, true, 1);
        const int numScChannels = juce::jmin (scBuffer.getNumChannels(), 2);

        if (scMonoBuf.size() < size_t (numSamples))
            scMonoBuf.resize (size_t (numSamples));

        // Mono key as-is; stereo key summed to mono (0.5 * (L + R))
        const float* scL = scBuffer.getReadPointer (0);
        const float* scR = scBuffer.getReadPointer (numScChannels - 1);
        float scPeak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            scPeak = juce::jmax (scPeak, std::abs (scL[i]), std::abs (scR[i]));
            scMonoBuf[size_t (i)] = numScChannels > 1 ? 0.5f * (scL[i] + scR[i]) : scL[i];
        }

        if (scPeak > scSilenceThreshold)
            scHoldRemaining = scHoldSamples;
        else
            scHoldRemaining = juce::jmax (0, scHoldRemaining - numSamples);

        hasExternalSc = scHoldRemaining > 0;
    }
    else
    {
        scHoldRemaining = 0;
    }
    externalScActive.store (hasExternalSc, std::memory_order_relaxed);

    // Demo mode: 60s play / 10s mute cycle when not activated
    if (! licenseManager.isActivated())
    {
        double sampleRate = getSampleRate();
        int totalSamples = buffer.getNumSamples();
        double cycleSeconds = 70.0; // 60s on + 10s off
        double currentTime = std::fmod (
            double (getTotalNumOutputChannels() > 0 ? getBlockSize() : 0) / sampleRate,
            cycleSeconds);

        // Simple approach: use a running sample counter (per instance)
        demoSampleCounter += totalSamples;
        double timeInCycle = std::fmod (double (demoSampleCounter) / sampleRate, cycleSeconds);

        if (timeInCycle >= 60.0)
        {
            // Mute during the 10s silent period
            buffer.clear();
            return;
        }
    }

    if (hasExternalSc)
    {
        // External sidechain: the (mono-summed) key drives the detector
        engine.processWithExternalSideChain (mainBuffer, scMonoBuf.data(), numSamples);
    }
    else
    {
        // Internal sidechain: filter the main input per the PDF design
        engine.process (mainBuffer);
    }
}

juce::AudioProcessorEditor* SipperProcessor::createEditor()
{
    return new SipperEditor (*this);
}

void SipperProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SipperProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SipperProcessor();
}
