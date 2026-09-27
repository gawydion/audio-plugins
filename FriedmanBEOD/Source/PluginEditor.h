#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class BrownEyeODLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
};

class BrownEyeODAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit BrownEyeODAudioProcessorEditor (BrownEyeODAudioProcessor&);
    ~BrownEyeODAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BrownEyeODAudioProcessor& processor;
    BrownEyeODLookAndFeel lnf;

    juce::Slider gainSlider, volumeSlider, tightSlider;
    juce::Slider bassSlider, trebleSlider, presenceSlider, trimSlider;

    juce::Label gainLabel, volumeLabel, tightLabel;
    juce::Label bassLabel, trebleLabel, presenceLabel, trimLabel;
    juce::Label titleLabel, subLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> gainAttach, volumeAttach, tightAttach;
    std::unique_ptr<Attachment> bassAttach, trebleAttach, presenceAttach, trimAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BrownEyeODAudioProcessorEditor)
};
