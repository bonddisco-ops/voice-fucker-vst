#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include "DSP.h"
#include "CDsp.h"
#include "Params.h"
#include "PresetManager.h"

// Voice Fucker 2.1 by Primal Rage Lab
//
//  IN -> Pre EQ (FX: only the processing / All: the clean voice too) -> Carrier + Vocoder (or the voice itself when off)
//     -> Harmonize (Dry: what came in, Wet: the harmony voices) -> Dirt (Drive, Crunch, Destroy) -> Comp -> Width
//     -> Post EQ (FX) -> Wet Gain  +  Dry Gain (clean voice)  -> Post EQ (All) -> Clip -> Output -> OUT
class VoiceFuckerProcessor : public juce::AudioProcessor
{
public:
    static constexpr int maxBands = 100;
    static constexpr int maxVoices = 8;

    VoiceFuckerProcessor();
    ~VoiceFuckerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Voice Fucker"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets { apvts };

    // ---- data for the display (written by the audio thread, read by the editor)
    struct Scope
    {
        static constexpr int size = 65536;            // 1 ms buckets
        float hi[size] {}, lo[size] {}, in[size] {}, preClip[size] {};
        float gr[size] {};                            // Glue gain reduction in dB (0 or below)
        std::atomic<int> writeCount { 0 };
        std::atomic<int> beatCount { 0 };
        std::atomic<double> beatAt { 0.0 };
        std::atomic<double> bpm { 120.0 };
    };
    Scope scope;

    struct Spectrum
    {
        static constexpr int size = 8192;
        float in[size] {}, out[size] {};
        std::atomic<int> writePos { 0 };
    };
    Spectrum spectrum;

    std::atomic<float> inPeak[2] {}, outPeak[2] {};
    std::atomic<double> currentSampleRate { 44100.0 };
    std::array<std::atomic<float>, maxBands> bandLevel;
    std::atomic<int> numBandsUi { 28 };
    std::array<std::atomic<int>, maxVoices> uiNotes;          // MIDI notes sounding (-1 = none)
    std::atomic<float> uiAutoHz { 0 };
    std::array<std::atomic<int>, Tables::numIntervals> uiInterval;   // interval that really sounds

    // compressor display: Glue gain reduction and its input level, OTT gain (+ up / - down) and level per band
    std::atomic<float> uiGlueGr { 0 }, uiGlueIn { -100 };
    std::array<std::atomic<float>, 3> uiOttGain, uiOttLevel;

    // editor size, kept with the plugin instance so reopening the window keeps it
    std::atomic<int> editorW { 0 }, editorH { 0 };

private:
    struct Voice
    {
        bool active = false;
        int note = -1;
        float logTarget = 0, logFreq = 0, amp = 0;
        float phase[7] {};
        float hPhase[Tables::numIntervals][3] {};
        float harmRatio[Tables::numIntervals] {};
        uint32_t age = 0;
    };
    struct Band
    {
        vf::Biquad m1, m2, cl1, cl2, cr1, cr2, hl1, hl2, hr1, hr2;
        float envM = 0, envCL = 0, envCR = 0, envHL = 0, envHR = 0;
        bool enabled = true;
    };

    std::atomic<float>* raw (const juce::String& id) { return apvts.getRawParameterValue (id); }
    void updateBands (int bands, float formant);
    void computeHarmonyLayout (int voiceIndex, float baseNote);
    void handleMidi (const juce::MidiMessage&);
    float harmPanFor (int interval, int panMode, float spread) const;

    struct FilterParams { std::atomic<float> *on, *lc, *hc, *lcSlope, *hcSlope, *pkF, *pkG, *pkQ, *target; };
    FilterParams preP {}, postP {};
    void bindFilter (FilterParams&, bool post);

    std::atomic<float> *pInGain, *pOutGain, *pSrc, *pPitch, *pNote, *pGlide, *pDetune, *pSpread, *pNoise,
                       *pVocOn, *pBands, *pFormant, *pAttack, *pRelease, *pSib, *pGate,
                       *pHarmOn, *pScale, *pRoot, *pMode, *pPan, *pHarmDry, *pHarmWet, *pHarmDetune, *pHarmSpread,
                       *pDirtOn, *pDirtMode, *pDrive, *pCrunch, *pDestroy,
                       *pCompOn, *pCompMode, *pOttDepth, *pOttTime, *pOttUp, *pOttDown,
                       *pGlueThresh, *pGlueRatio, *pGlueAttack, *pGlueRelease, *pCompMix,
                       *pWidthOn, *pWidthMode, *pWidth, *pMono,
                       *pDryGain, *pWetGain, *pAutoGain, *pClipOn, *pClipMode, *pCeiling;
    std::array<std::atomic<float>*, Tables::numIntervals> pIvOn, pIvDb, pIvPan;

    double sr = 44100.0;

    std::array<Voice, maxVoices> voices;
    uint32_t voiceCounter = 0;
    float lastLogFreq = -1;
    int lastPitchMode = -1;

    vf::PitchDetector pitch;
    float autoLogTarget = std::log2 (110.f), detectedHz = 0, detectedPeriod = 0.008f;
    vf::PitchShifter shifter;

    std::array<Band, maxBands> bands;
    int curBands = -1;
    float curFormant = 1000.f;

    vf::Biquad sib1, sib2;
    vf::Noise noiseA { 0x9e3779b9u }, noiseB { 0x7f4a7c15u }, noiseC { 0x2545f491u };
    float gateDet = 0, gateGain = 1;
    bool gateOpen = true;
    float dcX[2] {}, dcY[2] {};
    static constexpr int numWideBands = 12;
    struct WideBand { vf::Biquad b1, b2; } wide[numWideBands];
    static constexpr int numCrunchBands = 6;
    struct CrunchBand { vf::Biquad l1, l2, r1, r2; } crunchBands[numCrunchBands];

    float holdL = 0, holdR = 0, holdPhase = 0;
    vf::DelayLine dlL, dlR;
    float chorusPhase = 0;
    vf::Biquad sideHp1, sideHp2;
    float curMonoHz = -1;

    cdsp::FilterBank preFilter, postFilter;
    cdsp::Ott ott;
    cdsp::Glue glue;
    cdsp::AutoGain agComp;

    float rmsDry = 0, rmsWet = 0, autoGain = 1;
    juce::SmoothedValue<float> sInGain, sOutGain, sDirtOn, sWidthOn, sClipOn, sDryGain, sWetGain, sDrive, sCrunch, sDestroy, sWidth,
                               sHaas, sVocOn, sHarmDry, sHarmWet, sCeiling, sCompOn, sCompMix, sOttDepth, sPreOn, sPostOn, sPreAll, sPostAll;

    juce::AudioBuffer<float> scBuffer;

    // scope decimation and beat clock
    int bucketLen = 48, bucketPos = 0;
    float bucketHi = 0, bucketLo = 0, bucketIn = 0, bucketPre = 0, bucketGr = 0;
    double beatClock = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceFuckerProcessor)
};
