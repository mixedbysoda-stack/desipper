#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/DeEsserEngine.h"
#include "Licensing/LicenseManager.h"

class SipperProcessor : public juce::AudioProcessor
{
public:
    SipperProcessor();
    ~SipperProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // Bus layout support — needed for external sidechain
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Parameters
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Meter access for GUI
    DeEsserEngine& getEngine() { return engine; }

    // Licensing
    LicenseManager& getLicenseManager() { return licenseManager; }

    // External sidechain state for GUI indicator
    bool isExternalSideChainActive() const { return externalScActive; }

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    DeEsserEngine engine;
    LicenseManager licenseManager;
    bool externalScActive = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SipperProcessor)
};
