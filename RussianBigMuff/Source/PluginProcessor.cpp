#include "PluginProcessor.h"
#include "PluginEditor.h"

RussianBigMuffAudioProcessor::RussianBigMuffAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    rbm_init (&core);
    sustainParam = apvts.getRawParameterValue ("sustain");
    toneParam    = apvts.getRawParameterValue ("tone");
    volumeParam  = apvts.getRawParameterValue ("volume");
    inputParam   = apvts.getRawParameterValue ("input");
}

juce::AudioProcessorValueTreeState::ParameterLayout
RussianBigMuffAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    const auto range = juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f);

    const auto percentText = [] (float v, int)
    {
        return juce::String (juce::roundToInt (v * 100.0f)) + "%";
    };
    const auto percentParse = [] (const juce::String& t)
    {
        return t.getFloatValue() / 100.0f;
    };

    auto attr = [&] (juce::AudioProcessorParameter::Category cat)
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withCategory (cat)
            .withStringFromValueFunction (percentText)
            .withValueFromStringFunction (percentParse);
    };

    using Cat = juce::AudioProcessorParameter;
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sustain", 1 }, "Sustain", range, 0.62f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tone", 1 }, "Tone", range, 0.48f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume", range, 0.52f, attr (Cat::outputGain)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "input", 1 }, "Input", range, 0.70f, attr (Cat::inputGain)));

    return { params.begin(), params.end() };
}

void RussianBigMuffAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    oversampling.initProcessing ((size_t) samplesPerBlock);

    const double osRate = sampleRate * (double) oversampling.getOversamplingFactor();
    rbm_prepare (&core, (float) osRate);

    setLatencySamples ((int) oversampling.getLatencyInSamples());
}

void RussianBigMuffAudioProcessor::releaseResources()
{
    oversampling.reset();
    rbm_reset (&core);
}

bool RussianBigMuffAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn  = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void RussianBigMuffAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numCh <= 0 || numSamples <= 0)
        return;

    if (numCh > 1)
    {
        auto* left = buffer.getWritePointer (0);
        for (int ch = 1; ch < numCh; ++ch)
        {
            auto* src = buffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
                left[i] += src[i];
        }
        const float norm = 1.0f / (float) numCh;
        for (int i = 0; i < numSamples; ++i)
            left[i] *= norm;
    }

    const float sustain = sustainParam->load (std::memory_order_relaxed);
    const float tone    = toneParam->load    (std::memory_order_relaxed);
    const float volume  = volumeParam->load  (std::memory_order_relaxed);
    const float input   = inputParam->load   (std::memory_order_relaxed);

    juce::dsp::AudioBlock<float> block (buffer);
    auto monoBlock = block.getSingleChannelBlock (0);
    auto osBlock = oversampling.processSamplesUp (monoBlock);

    auto* samples = osBlock.getChannelPointer (0);
    const auto osCount = osBlock.getNumSamples();

    for (size_t i = 0; i < osCount; ++i)
        samples[i] = rbm_process (&core, samples[i], sustain, tone, volume, input);

    oversampling.processSamplesDown (monoBlock);

    auto* mono = buffer.getReadPointer (0);
    for (int ch = 1; ch < numCh; ++ch)
        buffer.copyFrom (ch, 0, mono, numSamples);
}

void RussianBigMuffAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RussianBigMuffAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* RussianBigMuffAudioProcessor::createEditor()
{
    return new RussianBigMuffAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RussianBigMuffAudioProcessor();
}
