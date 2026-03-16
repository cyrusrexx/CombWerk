/*
  ==============================================================================
    CombWerk – Multistage Comb Drum FX Plugin
    PluginProcessor.h

    Signal flow:
      Dry input → Stage 1 (Malström Comb) → Stage 2 (Blur Verb) →
      Stage 3 (Dirt) → Sidechain Pump → Glue Compressor →
      GlobalMix → OutputTrim → Output
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter ID strings  (kept in one place for easy reference)
// ─────────────────────────────────────────────────────────────────────────────
namespace ParamIDs
{
    // Stage 1 – Malström Comb
    inline constexpr const char* CombMode       = "combMode";
    inline constexpr const char* CombFreq       = "combFreq";
    inline constexpr const char* Resonance      = "resonance";
    inline constexpr const char* CombTone       = "combTone";
    inline constexpr const char* Mix1           = "mix1";

    // Stage 2 – Blur Verb
    inline constexpr const char* BlurAmount     = "blurAmount";
    inline constexpr const char* BlurSize       = "blurSize";
    inline constexpr const char* BlurFeedback   = "blurFeedback";
    inline constexpr const char* BlurTone       = "blurTone";

    // Stage 3 – Dirt
    inline constexpr const char* DirtDrive      = "dirtDrive";
    inline constexpr const char* DirtTone       = "dirtTone";
    inline constexpr const char* DirtMix        = "dirtMix";

    // Pump (internal sidechain)
    inline constexpr const char* PumpOn         = "pumpOn";
    inline constexpr const char* PumpAmount     = "pumpAmount";
    inline constexpr const char* PumpSpeed      = "pumpSpeed";

    // Glue Compressor
    inline constexpr const char* GlueOn         = "glueOn";
    inline constexpr const char* GlueThreshold  = "glueThreshold";
    inline constexpr const char* GlueRatio      = "glueRatio";
    inline constexpr const char* GlueAttack     = "glueAttack";
    inline constexpr const char* GlueRelease    = "glueRelease";
    inline constexpr const char* GlueMakeup     = "glueMakeup";

    // Global
    inline constexpr const char* RangeMode      = "rangeMode";
    inline constexpr const char* GlobalMix      = "globalMix";
    inline constexpr const char* OutputTrim     = "outputTrim";
}

// ─────────────────────────────────────────────────────────────────────────────
//  DSP helper: simple one-pole smoother for parameter values
// ─────────────────────────────────────────────────────────────────────────────
class OnePoleSmoother
{
public:
    void reset (float value) noexcept   { z = value; }
    void setCoeff (float coeff) noexcept { a = coeff; b = 1.0f - coeff; }

    /** Prepare smoother for a given smoothing time and sample rate. */
    void prepare (double sampleRate, float smoothTimeMs) noexcept
    {
        if (smoothTimeMs <= 0.0f || sampleRate <= 0.0)
        {
            a = 0.0f; b = 1.0f;
            return;
        }
        float numSamples = static_cast<float> (sampleRate * smoothTimeMs * 0.001);
        a = std::exp (-1.0f / numSamples);
        b = 1.0f - a;
    }

    float process (float target) noexcept { z = a * z + b * target; return z; }
    float current() const noexcept { return z; }

private:
    float a = 0.0f, b = 1.0f, z = 0.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  DSP helper: fractional-delay comb filter (linear interpolation)
// ─────────────────────────────────────────────────────────────────────────────
class FractionalComb
{
public:
    void prepare (int maxSamples)
    {
        buffer.resize (static_cast<size_t> (maxSamples + 2), 0.0f);
        maxDelay = maxSamples;
        writePos = 0;
    }

    void clear()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    /** Read with linear-interpolated fractional delay (in samples). */
    float read (float delaySamples) const noexcept
    {
        float clampedDelay = juce::jlimit (0.5f, static_cast<float> (maxDelay), delaySamples);
        float readF  = static_cast<float> (writePos) - clampedDelay;
        if (readF < 0.0f) readF += static_cast<float> (buffer.size());
        int   idx0   = static_cast<int> (readF);
        float frac   = readF - static_cast<float> (idx0);
        int   idx1   = idx0 + 1;
        if (idx0 >= static_cast<int> (buffer.size())) idx0 -= static_cast<int> (buffer.size());
        if (idx1 >= static_cast<int> (buffer.size())) idx1 -= static_cast<int> (buffer.size());
        return buffer[static_cast<size_t> (idx0)] * (1.0f - frac)
             + buffer[static_cast<size_t> (idx1)] * frac;
    }

    void write (float value) noexcept
    {
        buffer[static_cast<size_t> (writePos)] = value;
        writePos++;
        if (writePos >= static_cast<int> (buffer.size())) writePos = 0;
    }

private:
    std::vector<float> buffer;
    int writePos = 0;
    int maxDelay = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
//  DSP helper: simple one-pole low-pass filter
// ─────────────────────────────────────────────────────────────────────────────
class OnePoleLPF
{
public:
    void prepare (double sampleRate) { sr = sampleRate; updateCoeff(); }
    void setCutoff (float freqHz) { cutoff = freqHz; updateCoeff(); }
    void clear() { z = 0.0f; }

    float process (float x) noexcept
    {
        z += g * (x - z);
        return z;
    }

private:
    void updateCoeff()
    {
        float w = juce::MathConstants<float>::twoPi * cutoff / static_cast<float> (sr);
        g = w / (1.0f + w);
    }
    double sr = 44100.0;
    float cutoff = 8000.0f;
    float g = 0.5f;
    float z = 0.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  DSP helper: simple allpass for diffusion
// ─────────────────────────────────────────────────────────────────────────────
class DiffuserAllpass
{
public:
    void prepare (int delaySamples)
    {
        delay.prepare (delaySamples + 2);
        delaySmp = static_cast<float> (delaySamples);
    }

    void clear() { delay.clear(); }

    float process (float x, float coeff) noexcept
    {
        float delayed = delay.read (delaySmp);
        float v = x - coeff * delayed;
        delay.write (v);
        return delayed + coeff * v;
    }

private:
    FractionalComb delay;
    float delaySmp = 1.0f;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Main processor
// ─────────────────────────────────────────────────────────────────────────────
class CombWerkAudioProcessor : public juce::AudioProcessor
{
public:
    CombWerkAudioProcessor();
    ~CombWerkAudioProcessor() override = default;

    // --- Standard AudioProcessor overrides ------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Parameter tree (public so editor can attach) -------------------------
    juce::AudioProcessorValueTreeState apvts;

private:
    // --- Parameter layout builder ---------------------------------------------
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // --- Internal sample rate cache -------------------------------------------
    double currentSampleRate = 44100.0;

    // === STAGE 1: Malström Comb (per channel) =================================
    static constexpr int kNumChannels = 2;
    FractionalComb   stage1Comb[kNumChannels];
    OnePoleLPF       stage1LPF[kNumChannels];       // LPF in the feedback loop
    OnePoleSmoother  stage1DelaySmoother[kNumChannels];

    // === STAGE 2: Blur Verb (4 parallel combs + 2 allpass diffusers per ch) ===
    static constexpr int kBlurCombs    = 4;
    static constexpr int kBlurAllpass  = 2;
    FractionalComb   blurCombs[kNumChannels][kBlurCombs];
    OnePoleLPF       blurCombLPF[kNumChannels][kBlurCombs];
    DiffuserAllpass  blurAP[kNumChannels][kBlurAllpass];

    // Base delay times in samples for blur combs (at 44100 Hz reference)
    // Mutually-prime-ish values avoid strong periodicity.
    static constexpr float kBlurBaseDelaysMs[kBlurCombs] = { 7.3f, 9.7f, 13.1f, 17.9f };
    static constexpr int   kBlurAPSizes[kBlurAllpass]    = { 113, 199 };

    // === STAGE 3: Dirt / Overdrive ============================================
    OnePoleLPF dirtPostLPF[kNumChannels];

    // === SIDECHAIN PUMP =======================================================
    float pumpEnvelope = 0.0f;   // shared mono envelope from dry input

    // === GLUE COMPRESSOR ======================================================
    float glueEnvelope = 0.0f;   // RMS-ish envelope for compression

    // === Smoothers for key parameters =========================================
    OnePoleSmoother combFreqSmoother;
    OnePoleSmoother resonanceSmoother;
    OnePoleSmoother blurFeedbackSmoother;
    OnePoleSmoother blurSizeSmoother;
    OnePoleSmoother dirtDriveSmoother;
    OnePoleSmoother globalMixSmoother;
    OnePoleSmoother outputTrimSmoother;

    // === Helpers ==============================================================
    /** Convert a "comb frequency" in Hz to a delay time in samples. */
    float freqToDelaySamples (float freqHz) const noexcept;

    /** Soft-clip waveshaper (tanh-based). */
    static float softClip (float x) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CombWerkAudioProcessor)
};
