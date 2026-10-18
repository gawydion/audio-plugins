#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class TwinPreampAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit TwinPreampAudioProcessorEditor (TwinPreampAudioProcessor&);
    ~TwinPreampAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    TwinPreampAudioProcessor& processor;

    juce::Slider inGainSlider, volumeSlider, bassSlider, midSlider, trebleSlider, reverbSlider, outGainSlider;
    juce::Label  inGainLabel,  volumeLabel,  bassLabel,  midLabel,  trebleLabel,  reverbLabel,  outGainLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> inGainAtt, volumeAtt, bassAtt, midAtt, trebleAtt, reverbAtt, outGainAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TwinPreampAudioProcessorEditor)
};
