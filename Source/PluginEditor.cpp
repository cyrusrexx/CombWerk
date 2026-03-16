/*
  ==============================================================================
    CombWerk – PluginEditor.cpp
    Layout: 6 sections arranged in 2 rows × 3 columns.
    Row 1: COMB | BLUR | DIRT
    Row 2: PUMP | GLUE | GLOBAL
  ==============================================================================
*/

#include "PluginEditor.h"

// Window dimensions
static constexpr int kWindowWidth  = 780;
static constexpr int kWindowHeight = 500;
static constexpr int kMargin       = 8;
static constexpr int kSectionGap   = 6;
static constexpr int kHeaderHeight = 26;

// ─────────────────────────────────────────────────────────────────────────────
CombWerkAudioProcessorEditor::CombWerkAudioProcessorEditor (CombWerkAudioProcessor& p)
    : AudioProcessorEditor (p),
      processorRef (p),
      // Stage 1
      combModeCombo  (p.apvts, ParamIDs::CombMode,    "Mode"),
      combFreqKnob   (p.apvts, ParamIDs::CombFreq,    "Freq"),
      resonanceKnob  (p.apvts, ParamIDs::Resonance,    "Reso"),
      combToneKnob   (p.apvts, ParamIDs::CombTone,     "Tone"),
      mix1Knob       (p.apvts, ParamIDs::Mix1,         "Mix"),
      // Stage 2
      blurAmountKnob (p.apvts, ParamIDs::BlurAmount,   "Amount"),
      blurSizeKnob   (p.apvts, ParamIDs::BlurSize,     "Size"),
      blurFbKnob     (p.apvts, ParamIDs::BlurFeedback,  "Feedback"),
      blurToneKnob   (p.apvts, ParamIDs::BlurTone,     "Tone"),
      // Stage 3
      dirtDriveKnob  (p.apvts, ParamIDs::DirtDrive,    "Drive"),
      dirtToneKnob   (p.apvts, ParamIDs::DirtTone,     "Tone"),
      dirtMixKnob    (p.apvts, ParamIDs::DirtMix,      "Mix"),
      // Pump
      pumpOnToggle   (p.apvts, ParamIDs::PumpOn,       "On"),
      pumpAmountKnob (p.apvts, ParamIDs::PumpAmount,   "Amount"),
      pumpSpeedKnob  (p.apvts, ParamIDs::PumpSpeed,    "Speed"),
      // Glue
      glueOnToggle   (p.apvts, ParamIDs::GlueOn,       "On"),
      glueThreshKnob (p.apvts, ParamIDs::GlueThreshold,"Thresh"),
      glueRatioKnob  (p.apvts, ParamIDs::GlueRatio,    "Ratio"),
      glueAttKnob    (p.apvts, ParamIDs::GlueAttack,   "Attack"),
      glueRelKnob    (p.apvts, ParamIDs::GlueRelease,  "Release"),
      glueMakeupKnob (p.apvts, ParamIDs::GlueMakeup,   "Makeup"),
      // Global
      rangeModeCombo (p.apvts, ParamIDs::RangeMode,    "Range"),
      globalMixKnob  (p.apvts, ParamIDs::GlobalMix,    "Mix"),
      outputTrimKnob (p.apvts, ParamIDs::OutputTrim,   "Trim")
{
    setSize (kWindowWidth, kWindowHeight);

    // Add section panels
    addAndMakeVisible (combPanel);
    addAndMakeVisible (blurPanel);
    addAndMakeVisible (dirtPanel);
    addAndMakeVisible (pumpPanel);
    addAndMakeVisible (gluePanel);
    addAndMakeVisible (globalPanel);

    // Stage 1 controls → on top of combPanel
    combPanel.addAndMakeVisible (combModeCombo);
    combPanel.addAndMakeVisible (combFreqKnob);
    combPanel.addAndMakeVisible (resonanceKnob);
    combPanel.addAndMakeVisible (combToneKnob);
    combPanel.addAndMakeVisible (mix1Knob);

    // Stage 2
    blurPanel.addAndMakeVisible (blurAmountKnob);
    blurPanel.addAndMakeVisible (blurSizeKnob);
    blurPanel.addAndMakeVisible (blurFbKnob);
    blurPanel.addAndMakeVisible (blurToneKnob);

    // Stage 3
    dirtPanel.addAndMakeVisible (dirtDriveKnob);
    dirtPanel.addAndMakeVisible (dirtToneKnob);
    dirtPanel.addAndMakeVisible (dirtMixKnob);

    // Pump
    pumpPanel.addAndMakeVisible (pumpOnToggle);
    pumpPanel.addAndMakeVisible (pumpAmountKnob);
    pumpPanel.addAndMakeVisible (pumpSpeedKnob);

    // Glue
    gluePanel.addAndMakeVisible (glueOnToggle);
    gluePanel.addAndMakeVisible (glueThreshKnob);
    gluePanel.addAndMakeVisible (glueRatioKnob);
    gluePanel.addAndMakeVisible (glueAttKnob);
    gluePanel.addAndMakeVisible (glueRelKnob);
    gluePanel.addAndMakeVisible (glueMakeupKnob);

    // Global
    globalPanel.addAndMakeVisible (rangeModeCombo);
    globalPanel.addAndMakeVisible (globalMixKnob);
    globalPanel.addAndMakeVisible (outputTrimKnob);
}

// ─────────────────────────────────────────────────────────────────────────────
void CombWerkAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark background
    g.fillAll (juce::Colour (0xFF1A1A1E));

    // Title
    g.setFont (juce::Font (18.0f, juce::Font::bold));
    g.setColour (juce::Colour (0xFFE8A030));
    g.drawText ("CombWerk", getLocalBounds().removeFromTop (30),
                juce::Justification::centred);
}

// ─────────────────────────────────────────────────────────────────────────────
void CombWerkAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (kMargin);
    area.removeFromTop (30); // title area

    const int rowHeight = (area.getHeight() - kSectionGap) / 2;

    // Row 1: COMB (wide) | BLUR | DIRT
    auto row1 = area.removeFromTop (rowHeight);
    area.removeFromTop (kSectionGap);

    const int col3w = (row1.getWidth() - 2 * kSectionGap) / 3;

    auto combArea = row1.removeFromLeft (col3w + 30);  // wider for 5 controls
    row1.removeFromLeft (kSectionGap);
    auto blurArea = row1.removeFromLeft (col3w);
    row1.removeFromLeft (kSectionGap);
    auto dirtArea = row1;

    combPanel.setBounds (combArea);
    blurPanel.setBounds (blurArea);
    dirtPanel.setBounds (dirtArea);

    // Row 2: PUMP | GLUE (wide) | GLOBAL
    auto row2 = area;
    auto pumpArea   = row2.removeFromLeft (col3w - 20);
    row2.removeFromLeft (kSectionGap);
    auto glueArea   = row2.removeFromLeft (col3w + 60);
    row2.removeFromLeft (kSectionGap);
    auto globalArea = row2;

    pumpPanel.setBounds (pumpArea);
    gluePanel.setBounds (glueArea);
    globalPanel.setBounds (globalArea);

    // ── Layout controls inside each section panel ────────────────────────────

    auto layoutKnobRow = [] (juce::Rectangle<int> area, int headerH,
                             std::initializer_list<juce::Component*> components)
    {
        auto r = area.reduced (4);
        r.removeFromTop (headerH);
        int knobW = r.getWidth() / static_cast<int> (components.size());
        for (auto* comp : components)
            comp->setBounds (r.removeFromLeft (knobW));
    };

    // Stage 1: combo on top-right, then knobs row
    {
        auto r = combPanel.getLocalBounds().reduced (4);
        r.removeFromTop (kHeaderHeight);
        combModeCombo.setBounds (r.removeFromTop (36));
        int knobW = r.getWidth() / 4;
        combFreqKnob.setBounds  (r.removeFromLeft (knobW));
        resonanceKnob.setBounds (r.removeFromLeft (knobW));
        combToneKnob.setBounds  (r.removeFromLeft (knobW));
        mix1Knob.setBounds      (r);
    }

    // Stage 2
    layoutKnobRow (blurPanel.getLocalBounds(), kHeaderHeight,
                   { &blurAmountKnob, &blurSizeKnob, &blurFbKnob, &blurToneKnob });

    // Stage 3
    layoutKnobRow (dirtPanel.getLocalBounds(), kHeaderHeight,
                   { &dirtDriveKnob, &dirtToneKnob, &dirtMixKnob });

    // Pump
    {
        auto r = pumpPanel.getLocalBounds().reduced (4);
        r.removeFromTop (kHeaderHeight);
        pumpOnToggle.setBounds (r.removeFromTop (24));
        int knobW = r.getWidth() / 2;
        pumpAmountKnob.setBounds (r.removeFromLeft (knobW));
        pumpSpeedKnob.setBounds  (r);
    }

    // Glue
    {
        auto r = gluePanel.getLocalBounds().reduced (4);
        r.removeFromTop (kHeaderHeight);
        glueOnToggle.setBounds (r.removeFromTop (24));
        int knobW = r.getWidth() / 5;
        glueThreshKnob.setBounds (r.removeFromLeft (knobW));
        glueRatioKnob.setBounds  (r.removeFromLeft (knobW));
        glueAttKnob.setBounds    (r.removeFromLeft (knobW));
        glueRelKnob.setBounds    (r.removeFromLeft (knobW));
        glueMakeupKnob.setBounds (r);
    }

    // Global
    {
        auto r = globalPanel.getLocalBounds().reduced (4);
        r.removeFromTop (kHeaderHeight);
        rangeModeCombo.setBounds (r.removeFromTop (36));
        int knobW = r.getWidth() / 2;
        globalMixKnob.setBounds  (r.removeFromLeft (knobW));
        outputTrimKnob.setBounds (r);
    }
}
