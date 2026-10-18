#include "PluginProcessor.h"
#include "PluginEditor.h"

TwinPreampAudioProcessor::TwinPreampAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    volumeParam  = apvts.getRawParameterValue ("volume");
    bassParam    = apvts.getRawParameterValue ("bass");
    midParam     = apvts.getRawParameterValue ("mid");
    trebleParam  = apvts.getRawParameterValue ("treble");
    reverbParam  = apvts.getRawParameterValue ("reverb");
    inGainParam  = apvts.getRawParameterValue ("inGain");
    outGainParam = apvts.getRawParameterValue ("outGain");

    twp_init (&core);
}

void TwinPreampAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    twp_prepare (&core, (float) sampleRate);
}

void TwinPreampAudioProcessor::releaseResources()
{
    twp_reset (&core);
}

bool TwinPreampAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void TwinPreampAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    const float vol   = volumeParam->load();
    const float bass  = bassParam->load();
    const float mid   = midParam->load();
    const float treb  = trebleParam->load();
    const float rev   = reverbParam->load();
    const float inDb  = inGainParam->load();
    const float outDb = outGainParam->load();

    const float inGain  = juce::Decibels::decibelsToGain (inDb);
    const float outGain = juce::Decibels::decibelsToGain (outDb);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float x = data[i] * inGain;
            x = twp_process (&core, x, vol, bass, mid, treb, rev);
            data[i] = x * outGain;
        }
    }
}

juce::AudioProcessorEditor* TwinPreampAudioProcessor::createEditor()
{
    return new TwinPreampAudioProcessorEditor (*this);
}

void TwinPreampAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void TwinPreampAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout
TwinPreampAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "inGain", "Input Gain",
        juce::NormalisableRange<float> (-15.0f, 15.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "volume", "Volume",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "bass", "Bass",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mid", "Mid",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "treble", "Treble",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "reverb", "Reverb",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "outGain", "Output Gain",
        juce::NormalisableRange<float> (-15.0f, 15.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TwinPreampAudioProcessor();
}
