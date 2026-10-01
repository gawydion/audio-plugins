#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class RussianBigMuffLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
};

class RussianBigMuffAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit RussianBigMuffAudioProcessorEditor (RussianBigMuffAudioProcessor&);
    ~RussianBigMuffAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    RussianBigMuffAudioProcessor& processor;
    RussianBigMuffLookAndFeel lnf;

    juce::Slider sustainSlider, toneSlider, volumeSlider, inputSlider;
    juce::Label  sustainLabel,  toneLabel,  volumeLabel,  inputLabel;
    juce::Label  titleLabel, subLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> sustainAttach, toneAttach, volumeAttach, inputAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RussianBigMuffAudioProcessorEditor)
};
