#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class BrownEyeODAudioProcessor final : public juce::AudioProcessor
{
public:
    BrownEyeODAudioProcessor();
    ~BrownEyeODAudioProcessor() override = default;

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
    juce::dsp::IIR::Filter<float> tightHpf;
    juce::dsp::IIR::Filter<float> stageLpf;
    juce::dsp::IIR::Filter<float> presenceLpf;
    juce::dsp::IIR::Filter<float> trebleLpf;
    juce::dsp::IIR::Filter<float> bassShelf;
    juce::dsp::IIR::Filter<float> outputHpf;

    std::atomic<float>* gainParam     = nullptr;
    std::atomic<float>* volumeParam   = nullptr;
    std::atomic<float>* tightParam    = nullptr;
    std::atomic<float>* bassParam     = nullptr;
    std::atomic<float>* trebleParam   = nullptr;
    std::atomic<float>* presenceParam = nullptr;
    std::atomic<float>* trimParam     = nullptr;

    float lastTight    = -1.0f;
    float lastPresence = -1.0f;
    float lastTreble   = -1.0f;
    float lastBass     = -1.0f;

    double currentSampleRate = 44100.0;
    double osRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrownEyeODAudioProcessor)
};
