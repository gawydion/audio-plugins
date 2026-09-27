#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Schematic values from the NPN silicon Fuzz Face:
    // Cin 2.2uF, Cout 0.01uF, Fuzz 1kB + 20uF, Volume 500kA,
    // Rc1 33k, Rc2 8k2, Rfb 100k, Rtop 330.
    constexpr float kInputCapHz    = 12.0f;   // 2.2uF into low Q1 base Z
    constexpr float kOutputCapHz   = 32.0f;   // 0.01uF into 500k volume pot
    constexpr float kPickupLoadHz  = 6200.0f; // guitar + low input impedance
    constexpr float kCollectorHz   = 9800.0f; // 330 + 8k2 + Miller rolloff
}

SiliconFuzzFaceAudioProcessor::SiliconFuzzFaceAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    fuzzParam   = apvts.getRawParameterValue ("fuzz");
    volumeParam = apvts.getRawParameterValue ("volume");
    inputParam  = apvts.getRawParameterValue ("input");
}

juce::AudioProcessorValueTreeState::ParameterLayout
SiliconFuzzFaceAudioProcessor::createParameterLayout()
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

    // Names + categories are what Logic / GarageBand read when they
    // list AU parameters (automation, generic view). Smart Controls
    // on a guitar track still use Apple's fixed skin.
    auto fuzzAttr = juce::AudioParameterFloatAttributes()
        .withLabel ("%")
        .withCategory (juce::AudioProcessorParameter::genericParameter)
        .withStringFromValueFunction (percentText)
        .withValueFromStringFunction (percentParse);

    auto volAttr = juce::AudioParameterFloatAttributes()
        .withLabel ("%")
        .withCategory (juce::AudioProcessorParameter::outputGain)
        .withStringFromValueFunction (percentText)
        .withValueFromStringFunction (percentParse);

    auto inAttr = juce::AudioParameterFloatAttributes()
        .withLabel ("%")
        .withCategory (juce::AudioProcessorParameter::inputGain)
        .withStringFromValueFunction (percentText)
        .withValueFromStringFunction (percentParse);

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "fuzz", 1 }, "Fuzz", range, 0.65f, fuzzAttr));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume", range, 0.55f, volAttr));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "input", 1 }, "Input", range, 0.7f, inAttr));

    return { params.begin(), params.end() };
}

void SiliconFuzzFaceAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 1;

    oversampling.initProcessing ((size_t) samplesPerBlock);

    const double osRate = sampleRate * (double) oversampling.getOversamplingFactor();
    spec.sampleRate = osRate;
    spec.maximumBlockSize = (juce::uint32) (samplesPerBlock * (int) oversampling.getOversamplingFactor());

    inputHpf.prepare (spec);
    pickupLpf.prepare (spec);
    outputHpf.prepare (spec);
    collectorLpf.prepare (spec);

    updateFilters (osRate);

    inputHpf.reset();
    pickupLpf.reset();
    outputHpf.reset();
    collectorLpf.reset();

    sagEnv = 0.0f;
    // 20uF on the 1k fuzz pot ~ 20 ms; use that as sag time constant
    sagCoeff = 1.0f - std::exp (-1.0f / (float) (osRate * 0.02));

    setLatencySamples ((int) oversampling.getLatencyInSamples());
}

void SiliconFuzzFaceAudioProcessor::updateFilters (double oversampledRate)
{
    *inputHpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (oversampledRate, kInputCapHz);
    *pickupLpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (oversampledRate, kPickupLoadHz);
    *outputHpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (oversampledRate, kOutputCapHz);
    *collectorLpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (oversampledRate, kCollectorHz);
}

void SiliconFuzzFaceAudioProcessor::releaseResources()
{
    oversampling.reset();
}

bool SiliconFuzzFaceAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn  = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void SiliconFuzzFaceAudioProcessor::processMonoSample (float& x) noexcept
{
    const float fuzz   = fuzzParam->load (std::memory_order_relaxed);
    const float volume = volumeParam->load (std::memory_order_relaxed);
    const float input  = inputParam->load (std::memory_order_relaxed);

    // Guitar level into the 2.2uF input cap
    x *= juce::jmap (input, 0.15f, 1.35f);

    x = inputHpf.processSample (x);
    x = pickupLpf.processSample (x);

    // Q1: first NPN stage (33k collector, 100k feedback). High gain, already compressing.
    const float q1Gain = juce::jmap (fuzz, 18.0f, 42.0f);
    float q1 = std::tanh (x * q1Gain);

    // Sag from the 9V rail + 20uF emitter network
    sagEnv += sagCoeff * (std::abs (q1) - sagEnv);
    const float sag = 1.0f / (1.0f + sagEnv * juce::jmap (fuzz, 0.35f, 1.6f));
    q1 *= sag;

    // Q2 emitter degeneration: Fuzz pot 1kB. High fuzz = less degeneration = smash.
    const float q2Gain = juce::jmap (fuzz, 6.0f, 36.0f);

    // Silicon NPN asymmetry (even harmonics) + hard clip into 8k2 collector
    float pre = q1 * q2Gain + 0.08f;
    pre += 0.18f * pre * pre;

    float q2 = std::tanh (pre);

    // Silicon gating: quiet notes die sooner than germanium
    const float gate = juce::jmap (fuzz, 0.02f, 0.09f);
    const float absQ1 = std::abs (q1);
    if (absQ1 < gate)
        q2 *= absQ1 / gate;

    q2 = collectorLpf.processSample (q2);
    q2 = outputHpf.processSample (q2);

    // Volume 500k audio taper (approx x^2)
    const float volLin = volume * volume;
    x = q2 * volLin * 0.85f;
}

void SiliconFuzzFaceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                  juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numCh <= 0 || numSamples <= 0)
        return;

    // One circuit, like the real pedal: sum to mono, then copy out
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

    juce::dsp::AudioBlock<float> block (buffer);
    auto monoBlock = block.getSingleChannelBlock (0);
    auto osBlock = oversampling.processSamplesUp (monoBlock);

    auto* samples = osBlock.getChannelPointer (0);
    const auto osCount = osBlock.getNumSamples();

    for (size_t i = 0; i < osCount; ++i)
        processMonoSample (samples[i]);

    oversampling.processSamplesDown (monoBlock);

    auto* mono = buffer.getReadPointer (0);
    for (int ch = 1; ch < numCh; ++ch)
        buffer.copyFrom (ch, 0, mono, numSamples);
}

void SiliconFuzzFaceAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SiliconFuzzFaceAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* SiliconFuzzFaceAudioProcessor::createEditor()
{
    return new SiliconFuzzFaceAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SiliconFuzzFaceAudioProcessor();
}
