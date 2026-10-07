#include "PluginEditor.h"

OTOwareEditor::OTOwareEditor(OTOwareProcessor& p)
    : AudioProcessorEditor(p), attachment(p.parameters, "ware", knob), lowAttachment(p.parameters, "lowBoost", lowKnob)
{
    knob.setName("WARE");
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 30);
    knob.setTextValueSuffix(" %");
    knob.setDoubleClickReturnValue(true, 0);
    knob.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff3eadc));
    knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    knob.setLookAndFeel(this);
    addAndMakeVisible(knob);
    lowKnob.setName("LOW BOOST");
    lowKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    lowKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 24);
    lowKnob.setTextValueSuffix(" dB");
    lowKnob.setDoubleClickReturnValue(true, 0);
    lowKnob.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff3eadc));
    lowKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lowKnob.setLookAndFeel(this);
    addAndMakeVisible(lowKnob);
    setSize(400, 460);
}
OTOwareEditor::~OTOwareEditor()
{
    knob.setLookAndFeel(nullptr);
    lowKnob.setLookAndFeel(nullptr);
}
void OTOwareEditor::resized()
{
    knob.setBounds(28, 120, 270, 250);
    lowKnob.setBounds(306, 176, 86, 145);
}
void OTOwareEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff141616));
    g.setColour(juce::Colour(0xff363936));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12), 12, 1);
    g.setColour(juce::Colour(0xfff3eadc));
    g.setFont(juce::FontOptions(38.0f, juce::Font::bold));
    g.drawText("OTO-ware", 28, 35, 344, 50, juce::Justification::centred);
    g.setColour(juce::Colour(0xfffa7146));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("ONE KNOB. HARD CLIP.", 28, 88, 344, 24, juce::Justification::centred);
    g.setColour(juce::Colour(0xffb7b8ac));
    g.drawText("CLEAN", 32, 350, 64, 22, juce::Justification::centred);
    g.drawText("BROKEN", 208, 350, 78, 22, juce::Justification::centred);
    g.setColour(juce::Colour(0xff56b8d4));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("LOW", 306, 335, 86, 20, juce::Justification::centred);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("BASS BOOST", 300, 352, 98, 18, juce::Justification::centred);
    g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xfff3eadc));
    g.drawText("WARE", 95, 397, 138, 26, juce::Justification::centred);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("YJ AUDIO   /   PROTOTYPE 0.2", 30, 430, 340, 16, juce::Justification::centred);
}
void OTOwareEditor::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                    float position, float start, float end, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(20);
    const auto centre = bounds.getCentre();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float angle = start + position * (end - start);
    juce::Path track, active;
    track.addCentredArc(centre.x, centre.y, radius, radius, 0, start, end, true);
    active.addCentredArc(centre.x, centre.y, radius, radius, 0, start, angle, true);
    g.setColour(juce::Colour(0xff343936));
    g.strokePath(track, juce::PathStrokeType(6));
    const bool isLow = slider.getName() == "LOW BOOST";
    g.setColour(isLow ? juce::Colour(0xff56b8d4) : juce::Colour(0xfffa7146));
    g.strokePath(active, juce::PathStrokeType(6));
    g.setColour(juce::Colour(0xff282c2b));
    g.fillEllipse(centre.x - radius + 14, centre.y - radius + 14, 2 * radius - 28, 2 * radius - 28);
    g.setColour(juce::Colour(0xfff3eadc));
    g.drawLine(centre.x + std::sin(angle) * (radius * 0.48f), centre.y - std::cos(angle) * (radius * 0.48f),
               centre.x + std::sin(angle) * (radius - 23), centre.y - std::cos(angle) * (radius - 23), 4);
}
