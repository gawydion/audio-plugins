#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Soft clip approximating anti-parallel diodes in an op-amp loop.
    // thresh is the conduction voltage in the internal gain-staged units.
    inline float diodeClip (float x, float thresh) noexcept
    {
        const float s = thresh > 1.0e-6f ? (x / thresh) : x;
        return thresh * std::tanh (s);
    }

    // Hard clip with a small knee (shunt LEDs to Vref after Presence).
    inline float ledHardClip (float x, float thresh) noexcept
    {
        const float a = std::abs (x);
        if (a <= thresh)
            return x;

        const float sign = x >= 0.0f ? 1.0f : -1.0f;
        const float over = a - thresh;
        return sign * (thresh + thresh * std::tanh (over / thresh) * 0.22f);
    }
}

BrownEyeODAudioProcessor::BrownEyeODAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    gainParam     = apvts.getRawParameterValue ("gain");
    volumeParam   = apvts.getRawParameterValue ("volume");
    tightParam    = apvts.getRawParameterValue ("tight");
    bassParam     = apvts.getRawParameterValue ("bass");
    trebleParam   = apvts.getRawParameterValue ("treble");
    presenceParam = apvts.getRawParameterValue ("presence");
    trimParam     = apvts.getRawParameterValue ("trim");
}

juce::AudioProcessorValueTreeState::ParameterLayout
BrownEyeODAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto norm = juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f);

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
        juce::ParameterID { "gain", 1 }, "Gain", norm, 0.55f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume", norm, 0.48f, attr (Cat::outputGain)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "tight", 1 }, "Tight", norm, 0.42f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "bass", 1 }, "Bass", norm, 0.50f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "treble", 1 }, "Treble", norm, 0.55f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "presence", 1 }, "Presence", norm, 0.40f, attr (Cat::genericParameter)));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "trim", 1 }, "Trim", norm, 0.45f, attr (Cat::genericParameter)));

    return { params.begin(), params.end() };
}

void BrownEyeODAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 1;

    oversampling.initProcessing ((size_t) samplesPerBlock);

    osRate = sampleRate * (double) oversampling.getOversamplingFactor();
    spec.sampleRate = osRate;
    spec.maximumBlockSize = (juce::uint32) (samplesPerBlock * (int) oversampling.getOversamplingFactor());

    inputHpf.prepare (spec);
    tightHpf.prepare (spec);
    stageLpf.prepare (spec);
    presenceLpf.prepare (spec);
    trebleLpf.prepare (spec);
    bassShelf.prepare (spec);
    outputHpf.prepare (spec);

    lastTight = lastPresence = lastTreble = lastBass = -1.0f;
    updateFilters (osRate);

    inputHpf.reset();
    tightHpf.reset();
    stageLpf.reset();
    presenceLpf.reset();
    trebleLpf.reset();
    bassShelf.reset();
    outputHpf.reset();

    setLatencySamples ((int) oversampling.getLatencyInSamples());
}

void BrownEyeODAudioProcessor::updateFilters (double oversampledRate)
{
    const float tight    = tightParam->load (std::memory_order_relaxed);
    const float presence = presenceParam->load (std::memory_order_relaxed);
    const float treble   = trebleParam->load (std::memory_order_relaxed);
    const float bass     = bassParam->load (std::memory_order_relaxed);

    // 22 nF into ~1 M input: ~7 Hz coupling
    *inputHpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (oversampledRate, 7.2f);

    // Tight: 152 Hz → 3388 Hz (C100k pot after the input cap network)
    const float tightHz = 152.0f * std::pow (3388.0f / 152.0f, tight);
    *tightHpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (oversampledRate, tightHz);

    // R8 + C4 after Tight, ~16 kHz
    *stageLpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (oversampledRate, 16000.0f);

    // Presence: 1305 Hz → 7238 Hz into the hard clipper
    const float presHz = 1305.0f * std::pow (7238.0f / 1305.0f, presence);
    *presenceLpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (oversampledRate, presHz);

    // Treble after hard clip. Fully down is a severe cut (~340 Hz in the
    // schematic); fully up leaves the 8–9 kHz air the Presence did not eat.
    const float trebHz = 340.0f * std::pow (9000.0f / 340.0f, treble);
    *trebleLpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass (oversampledRate, trebHz);

    // Bass is an active low shelf centred near 80 Hz. Noon is almost flat.
    const float bassGainDb = juce::jmap (bass, -7.0f, 9.0f);
    *bassShelf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeLowShelf (oversampledRate, 80.0f, 0.72f, juce::Decibels::decibelsToGain (bassGainDb));

    // Output coupling
    *outputHpf.coefficients =
        *juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (oversampledRate, 12.0f);

    lastTight    = tight;
    lastPresence = presence;
    lastTreble   = treble;
    lastBass     = bass;
}

void BrownEyeODAudioProcessor::releaseResources()
{
    oversampling.reset();
}

bool BrownEyeODAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainIn  = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainOut == juce::AudioChannelSet::mono()
        || mainOut == juce::AudioChannelSet::stereo();
}

void BrownEyeODAudioProcessor::processMonoSample (float& x) noexcept
{
    const float gain   = gainParam->load (std::memory_order_relaxed);
    const float volume = volumeParam->load (std::memory_order_relaxed);
    const float trim   = trimParam->load (std::memory_order_relaxed);

    x = inputHpf.processSample (x);
    x = tightHpf.processSample (x);
    x = stageLpf.processSample (x);

    // Stage 1 — non-inverting, red LED pair in the feedback loop.
    // Fixed modest gain, high clip threshold (LED ~1.7 V).
    {
        const float s1 = x * 3.4f;
        x = diodeClip (s1, 1.55f);
    }

    // Stage 2 — non-inverting, Gain pot in the feedback, red LEDs.
    // 47 nF-ish recovery HPF is already covered by Tight + input.
    {
        const float g2 = juce::jmap (gain * gain, 2.2f, 14.0f);
        x = diodeClip (x * g2, 1.45f);
    }

    // Stage 3 — inverting, 1N4148 / BAV99 pair + 22k knee resistor.
    // Lower threshold, sharper than the LEDs. Gain about 10.
    {
        x = diodeClip (x * -9.5f, 0.68f);
    }

    // Stage 4 — inverting, 10k + 100k trim. Gain 1 → ~11.
    {
        const float g4 = juce::jmap (trim, 1.15f, 10.5f);
        x *= -g4;
    }

    // Presence sits in front of the shunt LED hard clipper.
    x = presenceLpf.processSample (x);
    x = ledHardClip (x, 1.62f);

    x = trebleLpf.processSample (x);
    x = bassShelf.processSample (x);
    x = outputHpf.processSample (x);

    // Volume A50k, audio-ish taper.
    const float volLin = volume * volume;
    x *= volLin * 0.62f;
}

void BrownEyeODAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
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

    const float tight    = tightParam->load (std::memory_order_relaxed);
    const float presence = presenceParam->load (std::memory_order_relaxed);
    const float treble   = trebleParam->load (std::memory_order_relaxed);
    const float bass     = bassParam->load (std::memory_order_relaxed);

    if (std::abs (tight - lastTight) > 0.0015f
        || std::abs (presence - lastPresence) > 0.0015f
        || std::abs (treble - lastTreble) > 0.0015f
        || std::abs (bass - lastBass) > 0.0015f)
    {
        updateFilters (osRate);
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

void BrownEyeODAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BrownEyeODAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* BrownEyeODAudioProcessor::createEditor()
{
    return new BrownEyeODAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BrownEyeODAudioProcessor();
}
