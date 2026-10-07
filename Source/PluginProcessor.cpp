#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

OTOwareProcessor::OTOwareProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "OTOwareState", {
          std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ware", 1}, "WARE",
              juce::NormalisableRange<float>(0, 100, 0.01f), 0.0f),
          std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"lowBoost", 1}, "LOW BOOST",
              juce::NormalisableRange<float>(0, 12, 0.01f), 0.0f) })
{
    ware = parameters.getRawParameterValue("ware");
    lowBoost = parameters.getRawParameterValue("lowBoost");
}
void OTOwareProcessor::prepareToPlay(double rate, int)
{
    amount.reset(rate, 0.02);
    lowAmountDb.reset(rate, 0.02);
    const auto safeRate = juce::jmax(1.0, rate);
    lowAlpha = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 180.0f / static_cast<float>(safeRate));
    reset();
}
void OTOwareProcessor::reset()
{
    amount.setCurrentAndTargetValue(ware->load() * 0.01f);
    lowAmountDb.setCurrentAndTargetValue(lowBoost->load());
    lowState.fill(0.0f);
}
bool OTOwareProcessor::isBusesLayoutSupported(const BusesLayout& layout) const
{
    const auto output = layout.getMainOutputChannelSet();
    return (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo())
        && output == layout.getMainInputChannelSet();
}
void OTOwareProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    for (int ch = getTotalNumInputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());
    amount.setTargetValue(juce::jlimit(0.0f, 1.0f, ware->load() * 0.01f));
    lowAmountDb.setTargetValue(juce::jlimit(0.0f, 12.0f, lowBoost->load()));
    const int channels = juce::jmin(getTotalNumInputChannels(), buffer.getNumChannels());
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // One shared smoother step per frame preserves stereo balance.
        const float a = amount.getNextValue();
        const float lowDb = lowAmountDb.getNextValue();
        const float drive = std::pow(10.0f, 1.8f * a); // up to +36 dB
        const float trim = std::pow(10.0f, -0.3f * a); // up to -6 dB
        const float mix = juce::jmin(1.0f, a * 4.0f);
        const float lowGainMinusOne = std::pow(10.0f, lowDb * 0.05f) - 1.0f;
        const bool lowIsActive = lowGainMinusOne > 1.0e-7f;
        for (int ch = 0; ch < channels; ++ch)
        {
            auto& sample = buffer.getWritePointer(ch)[i];
            if (!std::isfinite(sample)) { sample = 0; continue; }
            if (a == 0.0f && lowDb == 0.0f) continue; // bit-exact dry at rest

            // A first-order low-pass in the feedback-free path provides a
            // real-time-safe low-shelf boost without allocating coefficients.
            const auto channelIndex = static_cast<std::size_t>(ch);
            const float filterInput = juce::jlimit(-1.0f, 1.0f, sample);
            lowState[channelIndex] += lowAlpha * (filterInput - lowState[channelIndex]);
            const float safeBase = juce::jlimit(-16.0f, 16.0f, sample);
            const float boosted = lowIsActive
                ? juce::jlimit(-16.0f, 16.0f, safeBase + lowState[channelIndex] * lowGainMinusOne)
                : sample;
            if (a == 0.0f)
            {
                sample = boosted;
                continue;
            }
            // Clamp before multiplication to avoid overflow for extreme host input.
            const float wet = juce::jlimit(-1.0f / drive, 1.0f / drive, boosted) * drive * trim;
            sample = (1.0f - mix) * boosted + mix * wet;
        }
    }
}
juce::AudioProcessorEditor* OTOwareProcessor::createEditor() { return new OTOwareEditor(*this); }
void OTOwareProcessor::getStateInformation(juce::MemoryBlock& data)
{
    copyXmlToBinary(*parameters.copyState().createXml(), data);
}
void OTOwareProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OTOwareProcessor(); }
