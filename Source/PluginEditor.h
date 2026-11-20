#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// -----------------------------------------------------------------------------
// Knob al neon (rosa/arancio tramonto)
// -----------------------------------------------------------------------------
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
};

// -----------------------------------------------------------------------------
// Le 5 stelle stile HUD: cliccabili, si riempiono in modo continuo
// -----------------------------------------------------------------------------
class StarsComponent : public juce::Component
{
public:
    explicit StarsComponent (juce::RangedAudioParameter& p) : param (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    float level   = 0.0f;
    bool  blinkOn = true;

private:
    juce::RangedAudioParameter& param;
};

// -----------------------------------------------------------------------------
// Editor
// -----------------------------------------------------------------------------
class WantedLevelEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit WantedLevelEditor (WantedLevelProcessor&);
    ~WantedLevelEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildBackdrop();
    void setupSmallKnob (juce::Slider&, juce::Label&, const juce::String& name);

    WantedLevelProcessor& proc;
    NeonLookAndFeel lnf;

    juce::Slider levelKnob, mixKnob, outKnob;
    juce::Label  mixLabel, outLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAtt, mixAtt, outAtt;

    StarsComponent stars;

    juce::Image backdrop;
    juce::Image statusImages[6];
    float animTime = 0.0f;
    float level    = 1.0f;
    float meter    = 0.0f;
    juce::Point<float> shake;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WantedLevelEditor)
};
