#include "PluginEditor.h"

void SiliconFuzzFaceLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                   float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                   juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colour (0xff1a1a18));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colour (0xffd9c7a2));
    g.fillEllipse (centre.x - radius + 4.0f, centre.y - radius + 4.0f,
                   radius * 2.0f - 8.0f, radius * 2.0f - 8.0f);

    g.setColour (juce::Colour (0xff3a3328).withAlpha (0.35f));
    for (int i = 0; i < 8; ++i)
    {
        const float a = (float) i / 8.0f * juce::MathConstants<float>::twoPi;
        g.drawLine (centre.x, centre.y,
                    centre.x + std::cos (a) * (radius - 8.0f),
                    centre.y + std::sin (a) * (radius - 8.0f), 1.0f);
    }

    juce::Path pointer;
    const float pointerLen = radius - 10.0f;
    pointer.addRoundedRectangle (-2.0f, -pointerLen, 4.0f, pointerLen * 0.72f, 1.5f);
    g.setColour (juce::Colour (0xff2b241c));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    g.setColour (juce::Colour (0xff8b1e1e));
    g.fillEllipse (centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);
}

SiliconFuzzFaceAudioProcessorEditor::SiliconFuzzFaceAudioProcessorEditor (SiliconFuzzFaceAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    setSize (420, 280);

    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::degreesToRadians (225.0f),
                               juce::degreesToRadians (495.0f), true);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, juce::Colour (0xfff0e6d2));
        l.setFont (juce::Font (juce::FontOptions (14.0f)).boldened());
        addAndMakeVisible (l);
    };

    setupKnob (fuzzSlider,   fuzzLabel,   "FUZZ");
    setupKnob (volumeSlider, volumeLabel, "VOL");
    setupKnob (inputSlider,  inputLabel,  "INPUT");

    titleLabel.setText ("SILICON FUZZ FACE", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff4ead8));
    titleLabel.setFont (juce::Font (juce::FontOptions (22.0f)).boldened());
    addAndMakeVisible (titleLabel);

    fuzzAttach   = std::make_unique<Attachment> (processor.apvts, "fuzz",   fuzzSlider);
    volumeAttach = std::make_unique<Attachment> (processor.apvts, "volume", volumeSlider);
    inputAttach  = std::make_unique<Attachment> (processor.apvts, "input",  inputSlider);
}

SiliconFuzzFaceAudioProcessorEditor::~SiliconFuzzFaceAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SiliconFuzzFaceAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff2c2c28));
    g.fillRoundedRectangle (bounds, 16.0f);

    g.setColour (juce::Colour (0xff7a1515));
    g.fillRoundedRectangle (bounds.reduced (10.0f, 10.0f).removeFromTop (52.0f), 8.0f);

    g.setColour (juce::Colour (0xff1f1f1c));
    g.drawRoundedRectangle (bounds.reduced (1.5f), 16.0f, 3.0f);
}

void SiliconFuzzFaceAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (18);
    titleLabel.setBounds (r.removeFromTop (44));
    r.removeFromTop (12);

    const int knob = 128;
    auto row = r.removeFromTop (knob + 24);
    const int gap = (row.getWidth() - knob * 3) / 4;

    auto place = [&] (juce::Slider& s, juce::Label& l)
    {
        row.removeFromLeft (gap);
        auto cell = row.removeFromLeft (knob);
        s.setBounds (cell.removeFromTop (knob));
        l.setBounds (cell);
    };

    place (fuzzSlider, fuzzLabel);
    place (volumeSlider, volumeLabel);
    place (inputSlider, inputLabel);
}
