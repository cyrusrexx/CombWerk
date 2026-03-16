/*
  ==============================================================================
    CombWerk – PluginEditor.h
    Minimal grouped UI for the drum FX chain.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  A small helper: labeled rotary slider bound to APVTS
// ─────────────────────────────────────────────────────────────────────────────
class LabeledKnob : public juce::Component
{
public:
    LabeledKnob (juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& paramID,
                 const juce::String& labelText)
        : attachment (apvts, paramID, slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
        slider.setColour (juce::Slider::rotarySliderFillColourId,
                          juce::Colour (0xFFE8A030));
        slider.setColour (juce::Slider::thumbColourId,
                          juce::Colour (0xFFF0C060));
        addAndMakeVisible (slider);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (16));
        slider.setBounds (b);
    }

private:
    juce::Slider slider;
    juce::Label  label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

// ─────────────────────────────────────────────────────────────────────────────
//  A helper: combo box bound to APVTS
// ─────────────────────────────────────────────────────────────────────────────
class LabeledCombo : public juce::Component
{
public:
    LabeledCombo (juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramID,
                  const juce::String& labelText)
        : attachment (apvts, paramID, combo)
    {
        auto* param = apvts.getParameter (paramID);
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*> (param))
        {
            int idx = 1;
            for (auto& choice : choiceParam->choices)
                combo.addItem (choice, idx++);
        }
        addAndMakeVisible (combo);

        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromTop (16));
        combo.setBounds (b.reduced (2));
    }

private:
    juce::ComboBox combo;
    juce::Label    label;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment attachment;
};

// ─────────────────────────────────────────────────────────────────────────────
//  A helper: toggle button bound to APVTS
// ─────────────────────────────────────────────────────────────────────────────
class LabeledToggle : public juce::Component
{
public:
    LabeledToggle (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramID,
                   const juce::String& labelText)
        : attachment (apvts, paramID, toggle)
    {
        toggle.setButtonText (labelText);
        toggle.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xFFE8A030));
        addAndMakeVisible (toggle);
    }

    void resized() override { toggle.setBounds (getLocalBounds()); }

private:
    juce::ToggleButton toggle;
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Section panel with title and background
// ─────────────────────────────────────────────────────────────────────────────
class SectionPanel : public juce::Component
{
public:
    SectionPanel (const juce::String& title) : sectionTitle (title) {}

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xFF2A2A2E));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (juce::Colour (0xFF3A3A3E));
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

        g.setFont (juce::Font (13.0f, juce::Font::bold));
        g.setColour (juce::Colour (0xFFE8A030));
        g.drawText (sectionTitle, b.removeFromTop (22.0f).reduced (8, 0),
                    juce::Justification::centredLeft);
    }

private:
    juce::String sectionTitle;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Main editor
// ─────────────────────────────────────────────────────────────────────────────
class CombWerkAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit CombWerkAudioProcessorEditor (CombWerkAudioProcessor&);
    ~CombWerkAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    CombWerkAudioProcessor& processorRef;

    // Section panels
    SectionPanel combPanel   { "COMB" };
    SectionPanel blurPanel   { "BLUR" };
    SectionPanel dirtPanel   { "DIRT" };
    SectionPanel pumpPanel   { "PUMP" };
    SectionPanel gluePanel   { "GLUE" };
    SectionPanel globalPanel { "GLOBAL" };

    // Stage 1
    LabeledCombo combModeCombo;
    LabeledKnob combFreqKnob, resonanceKnob, combToneKnob, mix1Knob;

    // Stage 2
    LabeledKnob blurAmountKnob, blurSizeKnob, blurFbKnob, blurToneKnob;

    // Stage 3
    LabeledKnob dirtDriveKnob, dirtToneKnob, dirtMixKnob;

    // Pump
    LabeledToggle pumpOnToggle;
    LabeledKnob   pumpAmountKnob, pumpSpeedKnob;

    // Glue
    LabeledToggle glueOnToggle;
    LabeledKnob   glueThreshKnob, glueRatioKnob, glueAttKnob, glueRelKnob, glueMakeupKnob;

    // Global
    LabeledCombo rangeModeCombo;
    LabeledKnob globalMixKnob, outputTrimKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CombWerkAudioProcessorEditor)
};
