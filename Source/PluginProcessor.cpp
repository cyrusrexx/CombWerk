/*
  ==============================================================================
    CombWerk – Multistage Comb Drum FX Plugin
    PluginProcessor.cpp

    Full DSP chain implementation.
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// Static member definitions
constexpr float CombWerkAudioProcessor::kBlurBaseDelaysMs[kBlurCombs];
constexpr int   CombWerkAudioProcessor::kBlurAPSizes[kBlurAllpass];

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter layout
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
CombWerkAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ---- Stage 1: Malström Comb ----
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::CombMode, 1 },
        "Comb Mode",
        juce::StringArray { "Comb+", "Comb\u2013" },  // plus / minus
        0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::CombFreq, 1 },
        "Comb Freq",
        juce::NormalisableRange<float> (100.0f, 8000.0f, 0.0f, 0.3f),  // log-ish skew
        1500.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::Resonance, 1 },
        "Resonance",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        55.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::CombTone, 1 },
        "Comb Tone",
        juce::NormalisableRange<float> (800.0f, 16000.0f, 1.0f, 0.35f),
        7000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::Mix1, 1 },
        "Comb Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        65.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // ---- Stage 2: Blur Verb ----
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::BlurAmount, 1 },
        "Blur Amount",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        45.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::BlurSize, 1 },
        "Blur Size",
        juce::NormalisableRange<float> (0.5f, 2.0f, 0.01f),
        1.0f,
        juce::AudioParameterFloatAttributes().withLabel ("x")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::BlurFeedback, 1 },
        "Blur Feedback",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::BlurTone, 1 },
        "Blur Tone",
        juce::NormalisableRange<float> (1000.0f, 16000.0f, 1.0f, 0.35f),
        7000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    // ---- Stage 3: Dirt ----
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::DirtDrive, 1 },
        "Dirt Drive",
        juce::NormalisableRange<float> (0.0f, 24.0f, 0.1f),
        6.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::DirtTone, 1 },
        "Dirt Tone",
        juce::NormalisableRange<float> (1000.0f, 16000.0f, 1.0f, 0.35f),
        7000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::DirtMix, 1 },
        "Dirt Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        45.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // ---- Sidechain Pump ----
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::PumpOn, 1 },
        "Pump On",
        false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::PumpAmount, 1 },
        "Pump Amount",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        35.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::PumpSpeed, 1 },
        "Pump Speed",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    // ---- Glue Compressor ----
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::GlueOn, 1 },
        "Glue On",
        true));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::GlueThreshold, 1 },
        "Glue Threshold",
        juce::NormalisableRange<float> (-30.0f, 0.0f, 0.1f),
        -18.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::GlueRatio, 1 },
        "Glue Ratio",
        juce::NormalisableRange<float> (1.5f, 4.0f, 0.1f),
        2.0f,
        juce::AudioParameterFloatAttributes().withLabel (":1")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::GlueAttack, 1 },
        "Glue Attack",
        juce::NormalisableRange<float> (0.3f, 30.0f, 0.1f, 0.5f),
        10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::GlueRelease, 1 },
        "Glue Release",
        juce::NormalisableRange<float> (50.0f, 1000.0f, 1.0f, 0.4f),
        250.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::GlueMakeup, 1 },
        "Glue Makeup",
        juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f),
        2.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // ---- Global ----
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::RangeMode, 1 },
        "Range Mode",
        juce::StringArray { "Normal", "Double", "Triple" },
        0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::GlobalMix, 1 },
        "Global Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::OutputTrim, 1 },
        "Output Trim",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return { params.begin(), params.end() };
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
CombWerkAudioProcessor::CombWerkAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CombWerkState", createParameterLayout())
{
}

// ─────────────────────────────────────────────────────────────────────────────
//  prepareToPlay
// ─────────────────────────────────────────────────────────────────────────────
void CombWerkAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;

    // Max comb delay ~= 1 / 20 Hz = 50 ms; allow generous headroom (100 ms)
    int maxCombSamples = static_cast<int> (sampleRate * 0.1) + 4;

    // Max blur delay: largest base (17.9 ms) * BlurSize max (2.0x) * sr scaling
    // At 192 kHz: 17.9ms * 2.0 * (192000/44100) ≈ 156 ms → need ~0.16 * sr
    int maxBlurSamples = static_cast<int> (sampleRate * 0.2) + 4;

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        // Stage 1
        stage1Comb[ch].prepare (maxCombSamples);
        stage1Comb[ch].clear();
        stage1LPF[ch].prepare (sampleRate);
        stage1LPF[ch].clear();
        stage1DelaySmoother[ch].prepare (sampleRate, 5.0f);  // 5 ms smooth for delay changes
        stage1DelaySmoother[ch].reset (freqToDelaySamples (1500.0f));

        // Stage 2
        for (int c = 0; c < kBlurCombs; ++c)
        {
            blurCombs[ch][c].prepare (maxBlurSamples);
            blurCombs[ch][c].clear();
            blurCombLPF[ch][c].prepare (sampleRate);
            blurCombLPF[ch][c].clear();
        }
        for (int a = 0; a < kBlurAllpass; ++a)
        {
            int apSize = static_cast<int> (kBlurAPSizes[a] * sampleRate / 44100.0);
            blurAP[ch][a].prepare (apSize);
            blurAP[ch][a].clear();
        }

        // Stage 3
        dirtPostLPF[ch].prepare (sampleRate);
        dirtPostLPF[ch].clear();
    }

    // Reset envelopes
    pumpEnvelope = 0.0f;
    glueEnvelope = 0.0f;

    // Prepare smoothers (5 ms default smoothing for most controls)
    float smoothMs = 5.0f;
    combFreqSmoother.prepare     (sampleRate, smoothMs);
    resonanceSmoother.prepare    (sampleRate, smoothMs);
    blurFeedbackSmoother.prepare (sampleRate, smoothMs);
    blurSizeSmoother.prepare     (sampleRate, smoothMs);
    dirtDriveSmoother.prepare    (sampleRate, smoothMs);
    globalMixSmoother.prepare    (sampleRate, smoothMs);
    outputTrimSmoother.prepare   (sampleRate, smoothMs);

    // Initialize smoothers to current parameter values
    combFreqSmoother.reset     (apvts.getRawParameterValue (ParamIDs::CombFreq)->load());
    resonanceSmoother.reset    (apvts.getRawParameterValue (ParamIDs::Resonance)->load());
    blurFeedbackSmoother.reset (apvts.getRawParameterValue (ParamIDs::BlurFeedback)->load());
    blurSizeSmoother.reset     (apvts.getRawParameterValue (ParamIDs::BlurSize)->load());
    dirtDriveSmoother.reset    (apvts.getRawParameterValue (ParamIDs::DirtDrive)->load());
    globalMixSmoother.reset    (apvts.getRawParameterValue (ParamIDs::GlobalMix)->load());
    outputTrimSmoother.reset   (apvts.getRawParameterValue (ParamIDs::OutputTrim)->load());
}

void CombWerkAudioProcessor::releaseResources() {}

// ─────────────────────────────────────────────────────────────────────────────
//  Utility: frequency → delay in samples
// ─────────────────────────────────────────────────────────────────────────────
float CombWerkAudioProcessor::freqToDelaySamples (float freqHz) const noexcept
{
    // delay = sampleRate / freq
    // Clamp freq to [20, Nyquist/2] to keep things safe
    float nyquist = static_cast<float> (currentSampleRate) * 0.5f;
    freqHz = juce::jlimit (20.0f, nyquist, freqHz);
    return static_cast<float> (currentSampleRate) / freqHz;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Utility: soft-clip waveshaper  (tanh approximation for efficiency)
// ─────────────────────────────────────────────────────────────────────────────
float CombWerkAudioProcessor::softClip (float x) noexcept
{
    // Fast tanh approximation (Pade 3/3, good up to ~|x|=4)
    if (x < -3.0f) return -1.0f;
    if (x >  3.0f) return  1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ─────────────────────────────────────────────────────────────────────────────
//  processBlock – main DSP
// ─────────────────────────────────────────────────────────────────────────────
void CombWerkAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), kNumChannels);

    if (numChannels < 2 || numSamples == 0)
        return;

    // ─── Read all parameter values (atomic loads, once per block) ────────────
    // Stage 1
    const int   combModeIdx     = static_cast<int> (apvts.getRawParameterValue (ParamIDs::CombMode)->load());
    const float combFreqParam   = apvts.getRawParameterValue (ParamIDs::CombFreq)->load();
    const float resonanceParam  = apvts.getRawParameterValue (ParamIDs::Resonance)->load();  // 0–100
    const float combToneParam   = apvts.getRawParameterValue (ParamIDs::CombTone)->load();
    const float mix1Param       = apvts.getRawParameterValue (ParamIDs::Mix1)->load() * 0.01f;

    // Stage 2
    const float blurAmountParam = apvts.getRawParameterValue (ParamIDs::BlurAmount)->load() * 0.01f;
    const float blurSizeParam   = apvts.getRawParameterValue (ParamIDs::BlurSize)->load();
    const float blurFbParam     = apvts.getRawParameterValue (ParamIDs::BlurFeedback)->load();  // 0–100
    const float blurToneParam   = apvts.getRawParameterValue (ParamIDs::BlurTone)->load();

    // Stage 3
    const float dirtDriveParam  = apvts.getRawParameterValue (ParamIDs::DirtDrive)->load();    // dB
    const float dirtToneParam   = apvts.getRawParameterValue (ParamIDs::DirtTone)->load();
    const float dirtMixParam    = apvts.getRawParameterValue (ParamIDs::DirtMix)->load() * 0.01f;

    // Pump
    const bool  pumpOn          = apvts.getRawParameterValue (ParamIDs::PumpOn)->load() > 0.5f;
    const float pumpAmountParam = apvts.getRawParameterValue (ParamIDs::PumpAmount)->load() * 0.01f; // 0–1
    const float pumpSpeedParam  = apvts.getRawParameterValue (ParamIDs::PumpSpeed)->load() * 0.01f;  // 0–1

    // Glue
    const bool  glueOn          = apvts.getRawParameterValue (ParamIDs::GlueOn)->load() > 0.5f;
    const float glueThreshDB    = apvts.getRawParameterValue (ParamIDs::GlueThreshold)->load();
    const float glueRatio       = apvts.getRawParameterValue (ParamIDs::GlueRatio)->load();
    const float glueAttackMs    = apvts.getRawParameterValue (ParamIDs::GlueAttack)->load();
    const float glueReleaseMs   = apvts.getRawParameterValue (ParamIDs::GlueRelease)->load();
    const float glueMakeupDB    = apvts.getRawParameterValue (ParamIDs::GlueMakeup)->load();

    // Global
    const int   rangeModeIdx    = static_cast<int> (apvts.getRawParameterValue (ParamIDs::RangeMode)->load());
    const float globalMixParam  = apvts.getRawParameterValue (ParamIDs::GlobalMix)->load() * 0.01f;
    const float outputTrimDB    = apvts.getRawParameterValue (ParamIDs::OutputTrim)->load();

    // ─── Compute RangeMode multiplier ────────────────────────────────────────
    // TWEAK: These multipliers define how far the plugin can be pushed.
    // Normal = 1x, Double = 2x, Triple = 3x
    const float rangeMult = static_cast<float> (rangeModeIdx + 1); // 1, 2, or 3

    // ─── Derive effective internal values with RangeMode ─────────────────────
    // CombFreq: multiply the effective frequency, clamped to Nyquist
    const float nyquist = static_cast<float> (currentSampleRate) * 0.5f;
    const float effectiveCombFreq = juce::jlimit (20.0f, nyquist * 0.95f,
                                                  combFreqParam * rangeMult);

    // Resonance: 0–100 → internal feedback coefficient
    // Normal max ~0.97, Double ~0.99, Triple ~0.995
    // TWEAK: maxFeedback controls how close to self-oscillation we can get.
    const float maxFeedback = juce::jlimit (0.9f, 0.995f,
                                            0.97f + (rangeMult - 1.0f) * 0.0125f);
    const float effectiveResonance = (resonanceParam * 0.01f) * maxFeedback;

    // BlurFeedback: similar scaling
    const float maxBlurFb = juce::jlimit (0.9f, 0.995f,
                                          0.95f + (rangeMult - 1.0f) * 0.02f);
    const float effectiveBlurFb = (blurFbParam * 0.01f) * maxBlurFb;

    // DirtDrive: add headroom in higher modes (dB)
    const float effectiveDriveDB = dirtDriveParam * rangeMult;
    const float driveGainLinear  = juce::Decibels::decibelsToGain (effectiveDriveDB);

    // PumpAmount: scale depth
    const float effectivePumpAmount = juce::jlimit (0.0f, 1.0f, pumpAmountParam * rangeMult);

    // ─── Pump envelope coefficients ──────────────────────────────────────────
    // PumpSpeed 0 = slow (long attack/release), 1 = fast (short)
    // Attack:  1 ms (fast) – 200 ms (slow)
    // Release: 20 ms (fast) – 1000 ms (slow)
    const float pumpAttackMs  = juce::jmap (pumpSpeedParam, 200.0f, 1.0f);
    const float pumpReleaseMs = juce::jmap (pumpSpeedParam, 1000.0f, 20.0f);
    const float pumpAttCoeff  = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * pumpAttackMs  * 0.001f));
    const float pumpRelCoeff  = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * pumpReleaseMs * 0.001f));

    // ─── Glue compressor coefficients ────────────────────────────────────────
    const float glueThreshLin = juce::Decibels::decibelsToGain (glueThreshDB);
    const float glueAttCoeff  = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * glueAttackMs  * 0.001f));
    const float glueRelCoeff  = std::exp (-1.0f / (static_cast<float> (currentSampleRate) * glueReleaseMs * 0.001f));
    const float glueMakeupLin = juce::Decibels::decibelsToGain (glueMakeupDB);

    const float outputTrimLin = juce::Decibels::decibelsToGain (outputTrimDB);

    // Comb mode: +1 for CombPlus, -1 for CombMinus (feedback polarity)
    const float combPolarity = (combModeIdx == 0) ? 1.0f : -1.0f;

    // Set tone filters (once per block is fine)
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        stage1LPF[ch].setCutoff (combToneParam);
        dirtPostLPF[ch].setCutoff (dirtToneParam);
        for (int c = 0; c < kBlurCombs; ++c)
            blurCombLPF[ch][c].setCutoff (blurToneParam);
    }

    // ─── Grab channel pointers ───────────────────────────────────────────────
    float* channelL = buffer.getWritePointer (0);
    float* channelR = buffer.getWritePointer (1);
    float* channels[kNumChannels] = { channelL, channelR };

    // ─── Per-sample DSP loop ─────────────────────────────────────────────────
    for (int i = 0; i < numSamples; ++i)
    {
        // ── Smooth key parameters ────────────────────────────────────────────
        const float smoothedCombFreq  = combFreqSmoother.process (effectiveCombFreq);
        const float smoothedResonance = resonanceSmoother.process (effectiveResonance);
        const float smoothedBlurFb    = blurFeedbackSmoother.process (effectiveBlurFb);
        const float smoothedBlurSize  = blurSizeSmoother.process (blurSizeParam);
        const float smoothedDriveGain = dirtDriveSmoother.process (driveGainLinear);
        const float smoothedGlobalMix = globalMixSmoother.process (globalMixParam);
        const float smoothedOutTrim   = outputTrimSmoother.process (outputTrimLin);

        // Compute Stage 1 delay time in samples (from smoothed freq)
        const float targetDelay = freqToDelaySamples (smoothedCombFreq);

        // ── Tap dry input for sidechain detector (mono sum) ──────────────────
        const float dryL = channels[0][i];
        const float dryR = channels[1][i];
        const float dryMono = (std::abs (dryL) + std::abs (dryR)) * 0.5f;

        // ── Pump envelope follower ───────────────────────────────────────────
        if (pumpOn)
        {
            if (dryMono > pumpEnvelope)
                pumpEnvelope = pumpAttCoeff * pumpEnvelope + (1.0f - pumpAttCoeff) * dryMono;
            else
                pumpEnvelope = pumpRelCoeff * pumpEnvelope + (1.0f - pumpRelCoeff) * dryMono;
        }

        // ── Pre-compute stereo-linked glue detector level ─────────────────
        // (Stereo-linked: use the max of both channels for the detector)
        float glueDetectorLevel = 0.0f;

        // ── Process each channel ─────────────────────────────────────────────
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            float dry = channels[ch][i];
            float sig = dry;

            // ─────────────────────────────────────────────────────────────────
            // STAGE 1: Malström-style Comb
            // ─────────────────────────────────────────────────────────────────
            {
                float smoothedDelay = stage1DelaySmoother[ch].process (targetDelay);

                // Read from comb delay line
                float delayed = stage1Comb[ch].read (smoothedDelay);

                // Apply LPF in feedback loop (damping)
                float dampedFb = stage1LPF[ch].process (delayed);

                // Feedback with polarity (Comb+ vs Comb–)
                float combInput = sig + combPolarity * smoothedResonance * dampedFb;

                // Soft-limit the comb input to prevent runaway feedback
                // TWEAK: This saturation keeps Triple mode stable
                combInput = softClip (combInput * 0.5f) * 2.0f;

                stage1Comb[ch].write (combInput);

                // For Comb–, we additionally invert the read signal
                // to shift notch positions relative to Comb+
                float combOut = (combModeIdx == 0) ? delayed : -delayed;

                // Local dry/wet mix for Stage 1
                sig = dry * (1.0f - mix1Param) + combOut * mix1Param;
            }

            // ─────────────────────────────────────────────────────────────────
            // STAGE 2: Blur Verb (parallel combs → allpass diffusion)
            // ─────────────────────────────────────────────────────────────────
            float blurOut = 0.0f;
            {
                float blurInput = sig;

                for (int c = 0; c < kBlurCombs; ++c)
                {
                    // Delay time = base * blurSize * (sampleRate / 44100)
                    float baseDelay = kBlurBaseDelaysMs[c] * 0.001f
                                    * static_cast<float> (currentSampleRate);
                    float scaledDelay = baseDelay * smoothedBlurSize;

                    float delayed = blurCombs[ch][c].read (scaledDelay);
                    float dampedFb = blurCombLPF[ch][c].process (delayed);

                    float combIn = blurInput + smoothedBlurFb * dampedFb;

                    // Safety saturation in the blur feedback loop
                    combIn = softClip (combIn * 0.33f) * 3.0f;

                    blurCombs[ch][c].write (combIn);
                    blurOut += delayed;
                }

                // Average the parallel combs
                blurOut *= (1.0f / static_cast<float> (kBlurCombs));

                // Allpass diffusion stages
                for (int a = 0; a < kBlurAllpass; ++a)
                    blurOut = blurAP[ch][a].process (blurOut, 0.5f);
            }

            // Mix blur into signal
            sig = sig * (1.0f - blurAmountParam) + blurOut * blurAmountParam;

            // ─────────────────────────────────────────────────────────────────
            // STAGE 3: Dirt / Overdrive
            // ─────────────────────────────────────────────────────────────────
            {
                float dirtInput = sig;
                float driven = sig * smoothedDriveGain;

                // Waveshaper (tanh-style soft clip)
                float clipped = softClip (driven);

                // Post-distortion tone control
                clipped = dirtPostLPF[ch].process (clipped);

                // Local dry/wet
                sig = dirtInput * (1.0f - dirtMixParam) + clipped * dirtMixParam;
            }

            // ─────────────────────────────────────────────────────────────────
            // SIDECHAIN PUMP (gain ducking from dry input)
            // ─────────────────────────────────────────────────────────────────
            if (pumpOn && effectivePumpAmount > 0.0f)
            {
                // Convert envelope to gain reduction
                // Higher envelope (louder drums) → more ducking
                // PumpAmount controls max gain reduction (0 = no duck, 1 = full silence)
                float reduction = 1.0f - effectivePumpAmount * juce::jlimit (0.0f, 1.0f, pumpEnvelope * 4.0f);
                reduction = juce::jmax (0.0f, reduction);
                sig *= reduction;
            }

            // ─────────────────────────────────────────────────────────────────
            // GLUE COMPRESSOR (stereo-linked)
            // ─────────────────────────────────────────────────────────────────
            // Track the max level across channels for the stereo-linked detector
            glueDetectorLevel = juce::jmax (glueDetectorLevel, std::abs (sig));

            if (glueOn)
            {
                // Stereo-linked: both channels use the same glueEnvelope
                // which is updated AFTER the channel loop (one-sample lookahead,
                // inaudible) so both L and R get identical gain reduction.

                // Apply gain reduction from the current envelope
                if (glueEnvelope > glueThreshLin && glueThreshLin > 0.0f)
                {
                    float envDB    = juce::Decibels::gainToDecibels (glueEnvelope, -80.0f);
                    float threshDB = juce::Decibels::gainToDecibels (glueThreshLin, -80.0f);
                    float overDB   = envDB - threshDB;

                    // Soft knee: smooth transition around threshold
                    // TWEAK: kneeWidth controls how gradual the compression onset is
                    constexpr float kneeWidth = 6.0f; // dB
                    float compGainDB = 0.0f;
                    if (overDB > kneeWidth)
                    {
                        compGainDB = -(overDB * (1.0f - 1.0f / glueRatio));
                    }
                    else if (overDB > 0.0f)
                    {
                        float kneeRatio = overDB / kneeWidth;
                        compGainDB = -(overDB * kneeRatio * (1.0f - 1.0f / glueRatio));
                    }

                    float compGainLin = juce::Decibels::decibelsToGain (compGainDB);
                    sig *= compGainLin;
                }

                // Apply makeup gain
                sig *= glueMakeupLin;
            }

            // ─────────────────────────────────────────────────────────────────
            // GLOBAL MIX + OUTPUT TRIM
            // ─────────────────────────────────────────────────────────────────
            float output = dry * (1.0f - smoothedGlobalMix) + sig * smoothedGlobalMix;
            output *= smoothedOutTrim;

            // Final safety limiter – prevent extreme levels
            output = juce::jlimit (-4.0f, 4.0f, output);

            channels[ch][i] = output;
        }

        // ── Update stereo-linked glue envelope AFTER both channels ───────
        if (glueOn)
        {
            if (glueDetectorLevel > glueEnvelope)
                glueEnvelope = glueAttCoeff * glueEnvelope + (1.0f - glueAttCoeff) * glueDetectorLevel;
            else
                glueEnvelope = glueRelCoeff * glueEnvelope + (1.0f - glueRelCoeff) * glueDetectorLevel;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  State save / restore
// ─────────────────────────────────────────────────────────────────────────────
void CombWerkAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CombWerkAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Editor factory
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorEditor* CombWerkAudioProcessor::createEditor()
{
    return new CombWerkAudioProcessorEditor (*this);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Plugin instantiation entry point
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CombWerkAudioProcessor();
}
