#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class ModelTLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override;
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class ModelTAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ModelTAudioProcessorEditor (ModelTAudioProcessor&);
    ~ModelTAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ModelTAudioProcessor& processor;
    ModelTLookAndFeel lnf;

    juce::Slider brightSlider, normalSlider, bassSlider, trebleSlider;
    juce::Slider presenceSlider, masterSlider, inGainSlider, outGainSlider;
    juce::Label  brightLabel, normalLabel, bassLabel, trebleLabel;
    juce::Label  presenceLabel, masterLabel, inGainLabel, outGainLabel;
    juce::Label  titleLabel, subLabel;
    juce::ToggleButton jumpButton;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> brightAttach, normalAttach, bassAttach, trebleAttach;
    std::unique_ptr<Attachment> presenceAttach, masterAttach, inGainAttach, outGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> jumpAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModelTAudioProcessorEditor)
};
