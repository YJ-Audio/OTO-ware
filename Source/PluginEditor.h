#pragma once
#include "PluginProcessor.h"

class OTOwareEditor final : public juce::AudioProcessorEditor, private juce::LookAndFeel_V4
{
public:
    explicit OTOwareEditor(OTOwareProcessor&);
    ~OTOwareEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    juce::Slider knob;
    juce::Slider lowKnob;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment lowAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OTOwareEditor)
};
