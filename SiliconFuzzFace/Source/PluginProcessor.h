#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class SiliconFuzzFaceAudioProcessor final : public juce::AudioProcessor
{
public:
    SiliconFuzzFaceAudioProcessor();
    ~SiliconFuzzFaceAudioProcessor() override = default;

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
    void processMonoSample (float& sample) noexcept;
    void updateFilters (double oversampledRate);

    juce::dsp::Oversampling<float> oversampling { 1, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };

    juce::dsp::IIR::Filter<float> inputHpf;
    juce::dsp::IIR::Filter<float> pickupLpf;
    juce::dsp::IIR::Filter<float> outputHpf;
    juce::dsp::IIR::Filter<float> collectorLpf;

    std::atomic<float>* fuzzParam   = nullptr;
    std::atomic<float>* volumeParam = nullptr;
    std::atomic<float>* inputParam  = nullptr;

    float sagEnv = 0.0f;
    float sagCoeff = 0.0f;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SiliconFuzzFaceAudioProcessor)
};
