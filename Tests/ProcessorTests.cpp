#include "PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;
    OTOwareProcessor p;
    juce::MidiBuffer midi;
    auto set = [&](float value) { p.parameters.getParameter("ware")->setValueNotifyingHost(value); };
    auto* lowBoostParameter = p.parameters.getParameter("lowBoost");
    auto* lowBoostRaw = p.parameters.getRawParameterValue("lowBoost");
    if (lowBoostParameter == nullptr || lowBoostRaw == nullptr) return 11;
    if (std::abs(lowBoostRaw->load()) > 1e-6f) return 12;
    auto setLowBoostDb = [&](float db)
    {
        lowBoostParameter->setValueNotifyingHost(juce::jlimit(0.0f, 12.0f, db) / 12.0f);
    };
    for (double rate : {44100.0, 48000.0, 96000.0})
    {
        for (int channels : {1, 2})
        {
            auto layout = p.getBusesLayout();
            layout.inputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
            layout.outputBuses.set(0, juce::AudioChannelSet::canonicalChannelSet(channels));
            if (!p.setBusesLayout(layout)) return 1;
            set(0);
            setLowBoostDb(0);
            p.prepareToPlay(rate, 512);
            for (int size : {0, 1, 17, 127, 512, 2049})
            {
                juce::AudioBuffer<float> b(channels, size);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < size; ++i) b.setSample(c, i, std::sin(i * 0.1f) * 0.7f);
                juce::AudioBuffer<float> original; original.makeCopyOf(b);
                p.processBlock(b, midi);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < size; ++i)
                        if (b.getSample(c, i) != original.getSample(c, i)) return 2;
            }
            set(1);
            juce::AudioBuffer<float> b(channels, 512);
            for (int block = 0; block < 50; ++block)
            {
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < 512; ++i) b.setSample(c, i, c == 0 ? 0.7f * std::sin(i * 0.1f) : 0);
                p.processBlock(b, midi);
                for (int c = 0; c < channels; ++c)
                    for (int i = 0; i < 512; ++i)
                    {
                        float s = b.getSample(c, i);
                        if (!std::isfinite(s) || std::abs(s) > 1) return 3;
                        if (c == 1 && s != 0) return 4;
                    }
            }
            // Settled hard clipping should flatten several successive positive samples.
            if (std::abs(b.getSample(0, 10) - b.getSample(0, 11)) > 1e-6f) return 5;
            if (std::abs(b.getSample(0, 10) - 0.5011872f) > 1e-5f) return 6;
            b.clear();
            b.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
            b.setSample(0, 1, std::numeric_limits<float>::infinity());
            b.setSample(0, 2, std::numeric_limits<float>::max());
            p.processBlock(b, midi);
            for (int i = 0; i < 512; ++i) if (!std::isfinite(b.getSample(0, i))) return 7;
            set(0);
            for (int block = 0; block < 10; ++block) { b.clear(); p.processBlock(b, midi); }
            if (b.getMagnitude(0, 512) != 0) return 8;
        }
    }

    // With the drive knob off, the low-boost control should raise a low tone
    // while leaving a high tone close to its original level.  Render each tone
    // from a freshly prepared processor so the one-pole filter state cannot
    // leak between the measurements.  The 24-block warm-up also lets the
    // 20 ms parameter smoother and the 180 Hz filter settle.
    auto renderToneRms = [](float lowBoostDb, float frequency) -> float
    {
        OTOwareProcessor toneProcessor;
        auto layout = toneProcessor.getBusesLayout();
        layout.inputBuses.set(0, juce::AudioChannelSet::mono());
        layout.outputBuses.set(0, juce::AudioChannelSet::mono());
        if (!toneProcessor.setBusesLayout(layout))
            return std::numeric_limits<float>::quiet_NaN();

        toneProcessor.parameters.getParameter("ware")->setValueNotifyingHost(0.0f);
        toneProcessor.parameters.getParameter("lowBoost")->setValueNotifyingHost(lowBoostDb / 12.0f);
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;
        constexpr int warmupBlocks = 24;
        constexpr int measuredBlocks = 8;
        toneProcessor.prepareToPlay(sampleRate, blockSize);

        juce::AudioBuffer<float> buffer(1, blockSize);
        juce::MidiBuffer noMidi;
        double energy = 0.0;
        int sampleCount = 0;
        for (int block = 0; block < warmupBlocks + measuredBlocks; ++block)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const auto sampleIndex = block * blockSize + i;
                const auto phase = juce::MathConstants<double>::twoPi * frequency
                                 * static_cast<double>(sampleIndex) / sampleRate;
                buffer.setSample(0, i, 0.05f * static_cast<float>(std::sin(phase)));
            }
            toneProcessor.processBlock(buffer, noMidi);
            if (block >= warmupBlocks)
            {
                for (int i = 0; i < blockSize; ++i)
                {
                    const auto sample = buffer.getSample(0, i);
                    if (!std::isfinite(sample))
                        return std::numeric_limits<float>::quiet_NaN();
                    energy += static_cast<double>(sample) * static_cast<double>(sample);
                    ++sampleCount;
                }
            }
        }
        return sampleCount > 0 ? static_cast<float>(std::sqrt(energy / sampleCount))
                               : std::numeric_limits<float>::quiet_NaN();
    };

    const auto dryLowRms = renderToneRms(0.0f, 80.0f);
    const auto boostedLowRms = renderToneRms(12.0f, 80.0f);
    const auto dryHighRms = renderToneRms(0.0f, 6000.0f);
    const auto boostedHighRms = renderToneRms(12.0f, 6000.0f);
    if (!std::isfinite(dryLowRms) || !std::isfinite(boostedLowRms)
        || !std::isfinite(dryHighRms) || !std::isfinite(boostedHighRms)) return 13;
    if (!(boostedLowRms > dryLowRms * 1.2f)) return 14;
    if (!(boostedHighRms < dryHighRms * 1.3f)) return 15;
    if (!(boostedLowRms - dryLowRms > (boostedHighRms - dryHighRms) * 2.0f)) return 16;

    set(0.73f);
    setLowBoostDb(7.25f);
    juce::MemoryBlock state;
    p.getStateInformation(state);
    set(0);
    setLowBoostDb(0);
    p.setStateInformation(state.getData(), (int)state.getSize());
    if (std::abs(p.parameters.getRawParameterValue("ware")->load() - 73.0f) > 0.001f) return 9;
    if (std::abs(p.parameters.getRawParameterValue("lowBoost")->load() - 7.25f) > 0.001f) return 17;
    std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());
    bool hasLowBoostControl = false;
    for (auto* child : editor->getChildren())
        if (child->getName() == "LOW BOOST") hasLowBoostControl = true;
    if (!hasLowBoostControl) return 18;
    auto snapshot = editor->createComponentSnapshot(editor->getLocalBounds());
    auto file = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("OTO-ware-preview.png");
    if (auto stream = file.createOutputStream()) juce::PNGImageFormat().writeImageToStream(snapshot, *stream);
    else return 10;
    std::cout << "PASS: dry identity, low-frequency boost, variable blocks, mono/stereo isolation, 3 sample rates, clipping plateau, finite output, silence, state, editor.\n";
}
