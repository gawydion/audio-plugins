#include "PluginEditor.h"

void BrownEyeODLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                              juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (5.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colour (0xff0c0c0c));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour (juce::Colour (0xffe4d4b4));
    g.fillEllipse (centre.x - radius + 3.5f, centre.y - radius + 3.5f,
                   radius * 2.0f - 7.0f, radius * 2.0f - 7.0f);

    g.setColour (juce::Colour (0xff3a3226).withAlpha (0.30f));
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
    g.setColour (juce::Colour (0xff1c1610));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

    g.setColour (juce::Colour (0xff8a1a14));
    g.fillEllipse (centre.x - 4.5f, centre.y - 4.5f, 9.0f, 9.0f);
}

BrownEyeODAudioProcessorEditor::BrownEyeODAudioProcessorEditor (BrownEyeODAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);
    setSize (520, 360);

    auto setupKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& name)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (juce::degreesToRadians (225.0f),
                               juce::degreesToRadians (495.0f), true);
        addAndMakeVisible (s);

        l.setText (name, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, juce::Colour (0xffefe4cc));
        l.setFont (juce::Font (juce::FontOptions (13.0f)).boldened());
        addAndMakeVisible (l);
    };

    setupKnob (bassSlider,     bassLabel,     "BASS");
    setupKnob (trebleSlider,   trebleLabel,   "TREBLE");
    setupKnob (presenceSlider, presenceLabel, "PRES");
    setupKnob (trimSlider,     trimLabel,     "TRIM");
    setupKnob (gainSlider,     gainLabel,     "GAIN");
    setupKnob (volumeSlider,   volumeLabel,   "VOL");
    setupKnob (tightSlider,    tightLabel,    "TIGHT");

    titleLabel.setText ("BROWN EYE OD", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xfff6ecd8));
    titleLabel.setFont (juce::Font (juce::FontOptions (22.0f)).boldened());
    addAndMakeVisible (titleLabel);

    subLabel.setText ("BE-100  ·  Grok Audio", juce::dontSendNotification);
    subLabel.setJustificationType (juce::Justification::centred);
    subLabel.setColour (juce::Label::textColourId, juce::Colour (0xffc9b48a));
    subLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (subLabel);

    bassAttach     = std::make_unique<Attachment> (processor.apvts, "bass",     bassSlider);
    trebleAttach   = std::make_unique<Attachment> (processor.apvts, "treble",   trebleSlider);
    presenceAttach = std::make_unique<Attachment> (processor.apvts, "presence", presenceSlider);
    trimAttach     = std::make_unique<Attachment> (processor.apvts, "trim",     trimSlider);
    gainAttach     = std::make_unique<Attachment> (processor.apvts, "gain",     gainSlider);
    volumeAttach   = std::make_unique<Attachment> (processor.apvts, "volume",   volumeSlider);
    tightAttach    = std::make_unique<Attachment> (processor.apvts, "tight",    tightSlider);
}

BrownEyeODAudioProcessorEditor::~BrownEyeODAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void BrownEyeODAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour (juce::Colour (0xff161614));
    g.fillRoundedRectangle (bounds, 14.0f);

    g.setColour (juce::Colour (0xff6b1210));
    g.fillRoundedRectangle (bounds.reduced (10.0f).removeFromTop (58.0f), 8.0f);

    g.setColour (juce::Colour (0xffc9a227).withAlpha (0.55f));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 14.0f, 1.6f);
}

void BrownEyeODAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    titleLabel.setBounds (r.removeFromTop (32));
    subLabel.setBounds (r.removeFromTop (18));
    r.removeFromTop (10);

    const int knob = 96;
    const int labelH = 20;

    auto placeRow = [&] (juce::Slider* sliders[], juce::Label* labels[], int count)
    {
        auto row = r.removeFromTop (knob + labelH);
        r.removeFromTop (14);
        const int gap = (row.getWidth() - knob * count) / (count + 1);

        for (int i = 0; i < count; ++i)
        {
            row.removeFromLeft (gap);
            auto cell = row.removeFromLeft (knob);
            sliders[i]->setBounds (cell.removeFromTop (knob));
            labels[i]->setBounds (cell);
        }
    };

    juce::Slider* topS[] = { &bassSlider, &trebleSlider, &presenceSlider, &trimSlider };
    juce::Label*  topL[] = { &bassLabel,  &trebleLabel,  &presenceLabel,  &trimLabel };
    placeRow (topS, topL, 4);

    juce::Slider* botS[] = { &gainSlider, &volumeSlider, &tightSlider };
    juce::Label*  botL[] = { &gainLabel,  &volumeLabel,  &tightLabel };
    placeRow (botS, botL, 3);
}
