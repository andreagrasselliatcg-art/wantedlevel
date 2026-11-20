#include "PluginProcessor.h"
#include "PluginEditor.h"

WantedLevelProcessor::WantedLevelProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "WANTED_LEVEL", createLayout())
{
    levelParam  = apvts.getRawParameterValue ("level");
    mixParam    = apvts.getRawParameterValue ("mix");
    outputParam = apvts.getRawParameterValue ("output");
}

juce::AudioProcessorValueTreeState::ParameterLayout WantedLevelProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "level", 1 }, "Wanted Level",
        NormalisableRange<float> (0.0f, 5.0f, 0.01f), 1.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (v, 2) + " stars"; })
            .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue(); })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "mix", 1 }, "Mix",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; })
            .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue(); })));

    params.push_back (std::make_unique<AudioParameterFloat> (
        ParameterID { "output", 1 }, "Output",
        NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; })
            .withValueFromStringFunction ([] (const String& s) { return s.getFloatValue(); })));

    return { params.begin(), params.end() };
}

void WantedLevelProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
}

bool WantedLevelProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void WantedLevelProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    for (int c = numIn; c < numOut; ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    const int numCh = juce::jmin (numIn, numOut, buffer.getNumChannels());

    engine.process (buffer.getArrayOfWritePointers(), numCh, buffer.getNumSamples(),
                    levelParam->load(),
                    mixParam->load() * 0.01f,
                    juce::Decibels::decibelsToGain (outputParam->load()));

    // meter per la GUI (picco con decadimento)
    float peak = 0.0f;
    for (int c = 0; c < numCh; ++c)
        peak = juce::jmax (peak, buffer.getMagnitude (c, 0, buffer.getNumSamples()));
    const float prev = outputMeter.load();
    outputMeter.store (juce::jmax (peak, prev * 0.85f));
}

juce::AudioProcessorEditor* WantedLevelProcessor::createEditor()
{
    return new WantedLevelEditor (*this);
}

void WantedLevelProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void WantedLevelProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WantedLevelProcessor();
}
