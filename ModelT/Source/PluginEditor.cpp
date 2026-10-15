#include "PluginEditor.h"

void ModelTLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                          juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (5.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colour (0xff14110e));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colour (0xffd7a15a));
    g.fillEllipse (centre.x - radius + 3.5f, centre.y - radius + 3.5f,
                   radius * 2.0f - 7.0f, radius * 2.0f - 7.0f);

    g.setColour (juce::Colour (0xff3a2c1c).withAlpha (0.35f));
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
    g.setColour (juce::Colour (0xff1c1612));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    g.setColour (juce::Colour (0xff8a3a1c));
    g.fillEllipse (centre.x - 4.5f, centre.y - 4.5f, 9.0f, 9.0f);
}

void ModelTLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted, bool)
{
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();

    g.setColour (on ? juce::Colour (0xffc46a2a) : juce::Colour (0xff2a241c));
    g.fillRoundedRectangle (r, 5.0f);

    g.setColour (on ? juce::Colour (0xfff0d8a8) : juce::Colour (0xff8a7a64));
    g.drawRoundedRectangle (r, 5.0f, shouldDrawButtonAsHighlighted ? 1.6f : 1.0f);

    g.setColour (on ? juce::Colour (0xff1c120c) : juce::Colour (0xffe8dcc4));
    g.setFont (juce::Font (juce::FontOptions (12.0f)).boldened());
    g.drawText (on ? "JUMP ON" : "JUMP", r, juce::Justification::centred);
}

ModelTAudioProcessorEditor::ModelTAudioProcessorEditor (ModelTAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    setSize (520, 500);

    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::degreesToRadians (225.0f),
                               juce::degreesToRadians (495.0f), true);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, juce::Colour (0xfff0e2c4));
        l.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
        addAndMakeVisible (l);
    };

    setupKnob (brightSlider,   brightLabel,   "BRIGHT");
    setupKnob (normalSlider,   normalLabel,   "NORMAL");
    setupKnob (bassSlider,     bassLabel,     "BASS");
    setupKnob (trebleSlider,   trebleLabel,   "TREBLE");
    setupKnob (presenceSlider, presenceLabel, "PRESENCE");
    setupKnob (masterSlider,   masterLabel,   "MASTER");
    setupKnob (inGainSlider,   inGainLabel,   "IN GAIN");
    setupKnob (outGainSlider,  outGainLabel,  "OUT GAIN");

    jumpButton.setClickingTogglesState (true);
    jumpButton.setColour (juce::ToggleButton::textColourId, juce::Colour (0xfff0e2c4));
    addAndMakeVisible (jumpButton);

    titleLabel.setText ("MODEL T", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff6ead0));
    titleLabel.setFont (juce::Font (juce::FontOptions (22.0f)).boldened());
    addAndMakeVisible (titleLabel);

    subLabel.setText ("High-headroom amp  -  Grok Audio", juce::dontSendNotification);
    subLabel.setJustificationType (juce::Justification::centred);
    subLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc4a06a));
    subLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (subLabel);

    brightAttach   = std::make_unique<Attachment> (processor.apvts, "bright",   brightSlider);
    normalAttach   = std::make_unique<Attachment> (processor.apvts, "normal",   normalSlider);
    bassAttach     = std::make_unique<Attachment> (processor.apvts, "bass",     bassSlider);
    trebleAttach   = std::make_unique<Attachment> (processor.apvts, "treble",   trebleSlider);
    presenceAttach = std::make_unique<Attachment> (processor.apvts, "presence", presenceSlider);
    masterAttach   = std::make_unique<Attachment> (processor.apvts, "master",   masterSlider);
    inGainAttach   = std::make_unique<Attachment> (processor.apvts, "inGain",   inGainSlider);
    outGainAttach  = std::make_unique<Attachment> (processor.apvts, "outGain",  outGainSlider);
    jumpAttach     = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
                         processor.apvts, "jump", jumpButton);
}

ModelTAudioProcessorEditor::~ModelTAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ModelTAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff1c1612));
    g.fillRoundedRectangle (bounds, 14.0f);

    g.setColour (juce::Colour (0xff2e241c));
    g.fillRoundedRectangle (bounds.reduced (10.0f).removeFromTop (58.0f), 8.0f);

    g.setColour (juce::Colour (0xffc46a2a).withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 14.0f, 1.6f);
}

void ModelTAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    titleLabel.setBounds (r.removeFromTop (32));
    subLabel.setBounds (r.removeFromTop (18));
    r.removeFromTop (8);

    {
        auto bar = r.removeFromTop (28);
        jumpButton.setBounds (bar.withSizeKeepingCentre (110, 26));
    }
    r.removeFromTop (8);

    const int knob = 100;
    const int labelH = 20;
    const int rowH = knob + labelH;

    auto placeRow = [&] (juce::Slider* sliders[], juce::Label* labels[], int count)
    {
        auto row = r.removeFromTop (rowH);
        const int gap = (row.getWidth() - knob * count) / (count + 1);
        for (int i = 0; i < count; ++i)
        {
            row.removeFromLeft (gap);
            auto cell = row.removeFromLeft (knob);
            sliders[i]->setBounds (cell.removeFromTop (knob));
            labels[i]->setBounds (cell);
        }
    };

    {
        juce::Slider* s[] = { &brightSlider, &normalSlider, &bassSlider, &trebleSlider };
        juce::Label*  l[] = { &brightLabel,  &normalLabel,  &bassLabel,  &trebleLabel };
        placeRow (s, l, 4);
    }

    r.removeFromTop (10);

    {
        juce::Slider* s[] = { &presenceSlider, &masterSlider, &inGainSlider, &outGainSlider };
        juce::Label*  l[] = { &presenceLabel,  &masterLabel,  &inGainLabel,  &outGainLabel };
        placeRow (s, l, 4);
    }
}
