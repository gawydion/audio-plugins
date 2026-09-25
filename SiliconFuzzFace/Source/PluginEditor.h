#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class SiliconFuzzFaceLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
};

class SiliconFuzzFaceAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit SiliconFuzzFaceAudioProcessorEditor (SiliconFuzzFaceAudioProcessor&);
    ~SiliconFuzzFaceAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SiliconFuzzFaceAudioProcessor& processor;
    SiliconFuzzFaceLookAndFeel lnf;

    juce::Slider fuzzSlider, volumeSlider, inputSlider;
    juce::Label  fuzzLabel,  volumeLabel,  inputLabel, titleLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> fuzzAttach, volumeAttach, inputAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SiliconFuzzFaceAudioProcessorEditor)
};
