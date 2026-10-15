#include "PluginProcessor.h"
#include "PluginEditor.h"

ModelTAudioProcessor::ModelTAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    mt_init (&core);
    brightParam   = apvts.getRawParameterValue ("bright");
    normalParam   = apvts.getRawParameterValue ("normal");
    bassParam     = apvts.getRawParameterValue ("bass");
    trebleParam   = apvts.getRawParameterValue ("treble");
    presenceParam = apvts.getRawParameterValue ("presence");
    masterParam   = apvts.getRawParameterValue ("master");
    jumpParam     = apvts.getRawParameterValue ("jump");
    inGainParam   = apvts.getRawParameterValue ("inGain");
    outGainParam  = apvts.getRawParameterValue ("outGain");
}

juce::AudioProcessorValueTreeState::ParameterLayout
ModelTAudioProcessor::createParameterLayout()
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
        juce::ParameterID { "bright", 1 }, "Bright", range, 0.60f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "normal", 1 }, "Normal", range, 0.50f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "bass", 1 }, "Bass", range, 0.55f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "treble", 1 }, "Treble", range, 0.48f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "presence", 1 }, "Presence", range, 0.42f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "master", 1 }, "Master", range, 0.32f, attr (Cat::outputGain)));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "jump", 1 }, "Jump", true));

    const auto dbRange = juce::NormalisableRange<float> (-15.0f, 15.0f, 0.1f);
    const auto dbText = [] (float v, int)
    {
        return juce::String (v, 1) + " dB";
    };
    const auto dbParse = [] (const juce::String& t)
    {
        return t.getFloatValue();
    };

    auto dbAttr = [&] (juce::AudioProcessorParameter::Category cat)
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel ("dB")
            .withCategory (cat)
            .withStringFromValueFunction (dbText)
            .withValueFromStringFunction (dbParse);
    };

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "inGain", 1 }, "Input Gain", dbRange, 0.0f, dbAttr (Cat::inputGain)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "outGain", 1 }, "Output Gain", dbRange, 0.0f, dbAttr (Cat::outputGain)));

    return { params.begin(), params.end() };
}

void ModelTAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    oversampling.initProcessing ((size_t) samplesPerBlock);

    const double osRate = sampleRate * (double) oversampling.getOversamplingFactor();
    mt_prepare (&core, (float) osRate);

    setLatencySamples ((int) oversampling.getLatencyInSamples());
}

void ModelTAudioProcessor::releaseResources()
{
    oversampling.reset();
    mt_reset (&core);
}

bool ModelTAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn  = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void ModelTAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
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

    const float bright   = brightParam->load   (std::memory_order_relaxed);
    const float normal   = normalParam->load   (std::memory_order_relaxed);
    const float bass     = bassParam->load     (std::memory_order_relaxed);
    const float treble   = trebleParam->load   (std::memory_order_relaxed);
    const float presence = presenceParam->load (std::memory_order_relaxed);
    const float master   = masterParam->load   (std::memory_order_relaxed);
    const int   jump     = jumpParam->load     (std::memory_order_relaxed) >= 0.5f ? 1 : 0;
    const float inGainDb  = inGainParam->load  (std::memory_order_relaxed);
    const float outGainDb = outGainParam->load (std::memory_order_relaxed);

    const float inLin  = juce::Decibels::decibelsToGain (inGainDb);
    const float outLin = juce::Decibels::decibelsToGain (outGainDb);

    if (inLin != 1.0f)
    {
        auto* left = buffer.getWritePointer (0);
        for (int i = 0; i < numSamples; ++i)
            left[i] *= inLin;
    }

    juce::dsp::AudioBlock<float> block (buffer);
    auto monoBlock = block.getSingleChannelBlock (0);
    auto osBlock = oversampling.processSamplesUp (monoBlock);

    auto* samples = osBlock.getChannelPointer (0);
    const auto osCount = osBlock.getNumSamples();

    for (size_t i = 0; i < osCount; ++i)
        samples[i] = mt_process (&core, samples[i],
                                 bright, normal, bass, treble, presence,
                                 master, jump);

    oversampling.processSamplesDown (monoBlock);

    if (outLin != 1.0f)
    {
        auto* left = buffer.getWritePointer (0);
        for (int i = 0; i < numSamples; ++i)
            left[i] *= outLin;
    }

    auto* mono = buffer.getReadPointer (0);
    for (int ch = 1; ch < numCh; ++ch)
        buffer.copyFrom (ch, 0, mono, numSamples);
}

void ModelTAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ModelTAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* ModelTAudioProcessor::createEditor()
{
    return new ModelTAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ModelTAudioProcessor();
}
