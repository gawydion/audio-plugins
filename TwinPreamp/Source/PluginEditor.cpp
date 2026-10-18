#include "PluginEditor.h"

TwinPreampAudioProcessorEditor::TwinPreampAudioProcessorEditor (TwinPreampAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    auto setup = [this] (juce::Slider& s, juce::Label& l, const juce::String& name, const juce::String& param)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (l);
    };

    setup (inGainSlider,  inGainLabel,  "In Gain",  "inGain");
    setup (volumeSlider,  volumeLabel,  "Volume",   "volume");
    setup (bassSlider,    bassLabel,    "Bass",     "bass");
    setup (midSlider,     midLabel,     "Mid",      "mid");
    setup (trebleSlider,  trebleLabel,  "Treble",   "treble");
    setup (reverbSlider,  reverbLabel,  "Reverb",   "reverb");
    setup (outGainSlider, outGainLabel, "Out Gain", "outGain");

    inGainAtt  = std::make_unique<SliderAttachment> (processor.apvts, "inGain",  inGainSlider);
    volumeAtt  = std::make_unique<SliderAttachment> (processor.apvts, "volume",  volumeSlider);
    bassAtt    = std::make_unique<SliderAttachment> (processor.apvts, "bass",    bassSlider);
    midAtt     = std::make_unique<SliderAttachment> (processor.apvts, "mid",     midSlider);
    trebleAtt  = std::make_unique<SliderAttachment> (processor.apvts, "treble",  trebleSlider);
    reverbAtt  = std::make_unique<SliderAttachment> (processor.apvts, "reverb",  reverbSlider);
    outGainAtt = std::make_unique<SliderAttachment> (processor.apvts, "outGain", outGainSlider);

    setSize (640, 320);
}

void TwinPreampAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
    g.setColour (juce::Colours::white);
    g.setFont (22.0f);
    g.drawFittedText ("Twin Preamp", getLocalBounds().removeFromTop (40), juce::Justification::centred, 1);
    g.setFont (12.0f);
    g.setColour (juce::Colours::lightgrey);
    g.drawFittedText ("Blackface Twin style preamp + spring", getLocalBounds().removeFromTop (58).removeFromBottom (16), juce::Justification::centred, 1);
}

void TwinPreampAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20).removeFromBottom (240);
    const int w = area.getWidth() / 7;

    auto place = [&] (juce::Slider& s, juce::Label& l, int i)
    {
        auto col = area.removeFromLeft (w).reduced (4);
        l.setBounds (col.removeFromTop (20));
        s.setBounds (col);
    };

    place (inGainSlider,  inGainLabel,  0);
    place (volumeSlider,  volumeLabel,  1);
    place (bassSlider,    bassLabel,    2);
    place (midSlider,     midLabel,     3);
    place (trebleSlider,  trebleLabel,  4);
    place (reverbSlider,  reverbLabel,  5);
    place (outGainSlider, outGainLabel, 6);
}
