#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

extern "C"
{
    #include "rbm_core.h"
}

// JUCE wrapper only. All DSP lives in rbm_core.c; this class owns the
// oversampler, the parameters, the editor and the saved state.
class RussianBigMuffAudioProcessor final : public juce::AudioProcessor
{
public:
    RussianBigMuffAudioProcessor();
    ~RussianBigMuffAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    RbmCore core;

    juce::dsp::Oversampling<float> oversampling { 1, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };

    std::atomic<float>* sustainParam = nullptr;
    std::atomic<float>* toneParam    = nullptr;
    std::atomic<float>* volumeParam  = nullptr;
    std::atomic<float>* inputParam   = nullptr;

    double osRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RussianBigMuffAudioProcessor)
};
