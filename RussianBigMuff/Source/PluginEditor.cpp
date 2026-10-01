#include "PluginEditor.h"

void RussianBigMuffLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                  float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                  juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (5.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colour (0xff1a1c14));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colour (0xffc4b896));
    g.fillEllipse (centre.x - radius + 3.5f, centre.y - radius + 3.5f,
                   radius * 2.0f - 7.0f, radius * 2.0f - 7.0f);

    g.setColour (juce::Colour (0xff3a3c28).withAlpha (0.32f));
    for (int i = 0; i < 10; ++i)
    {
        const float a = (float) i / 10.0f * juce::MathConstants<float>::twoPi;
        g.drawLine (centre.x, centre.y,
                    centre.x + std::cos (a) * (radius - 7.0f),
                    centre.y + std::sin (a) * (radius - 7.0f), 1.0f);
    }

    juce::Path pointer;
    const float pointerLen = radius - 9.0f;
    pointer.addRoundedRectangle (-1.8f, -pointerLen, 3.6f, pointerLen * 0.70f, 1.2f);
    g.setColour (juce::Colour (0xff1c1e14));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    g.setColour (juce::Colour (0xff6b8f3a));
    g.fillEllipse (centre.x - 4.5f, centre.y - 4.5f, 9.0f, 9.0f);
}

RussianBigMuffAudioProcessorEditor::RussianBigMuffAudioProcessorEditor (RussianBigMuffAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    setSize (480, 280);

    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::degreesToRadians (225.0f),
                               juce::degreesToRadians (495.0f), true);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, juce::Colour (0xffe8e0c4));
        l.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
        addAndMakeVisible (l);
    };

    setupKnob (sustainSlider, sustainLabel, "SUSTAIN");
    setupKnob (toneSlider,    toneLabel,    "TONE");
    setupKnob (volumeSlider,  volumeLabel,  "VOL");
    setupKnob (inputSlider,   inputLabel,   "INPUT");

    titleLabel.setText ("RUSSIAN BIG MUFF", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xffefe6c8));
    titleLabel.setFont (juce::Font (juce::FontOptions (22.0f)).boldened());
    addAndMakeVisible (titleLabel);

    subLabel.setText ("Sovtek Green Russian  ·  Grok Audio", juce::dontSendNotification);
    subLabel.setJustificationType (juce::Justification::centred);
    subLabel.setColour (juce::Label::textColourId, juce::Colour (0xffa8b07a));
    subLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (subLabel);

    sustainAttach = std::make_unique<Attachment> (processor.apvts, "sustain", sustainSlider);
    toneAttach    = std::make_unique<Attachment> (processor.apvts, "tone",    toneSlider);
    volumeAttach  = std::make_unique<Attachment> (processor.apvts, "volume",  volumeSlider);
    inputAttach   = std::make_unique<Attachment> (processor.apvts, "input",   inputSlider);
}

RussianBigMuffAudioProcessorEditor::~RussianBigMuffAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void RussianBigMuffAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff2a3320));
    g.fillRoundedRectangle (bounds, 14.0f);

    g.setColour (juce::Colour (0xff3d4a28));
    g.fillRoundedRectangle (bounds.reduced (10.0f).removeFromTop (58.0f), 8.0f);

    g.setColour (juce::Colour (0xff8a9a4a).withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 14.0f, 1.6f);
}

void RussianBigMuffAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    titleLabel.setBounds (r.removeFromTop (32));
    subLabel.setBounds (r.removeFromTop (18));
    r.removeFromTop (10);

    const int knob = 96;
    const int labelH = 20;
    auto row = r.removeFromTop (knob + labelH);
    const int count = 4;
    const int gap = (row.getWidth() - knob * count) / (count + 1);

    juce::Slider* sliders[] = { &sustainSlider, &toneSlider, &volumeSlider, &inputSlider };
    juce::Label*  labels[]  = { &sustainLabel,  &toneLabel,  &volumeLabel,  &inputLabel };

    for (int i = 0; i < count; ++i)
    {
        row.removeFromLeft (gap);
        auto cell = row.removeFromLeft (knob);
        sliders[i]->setBounds (cell.removeFromTop (knob));
        labels[i]->setBounds (cell);
    }
}
