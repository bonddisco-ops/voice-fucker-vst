#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include "DSP.h"
#include "Params.h"

// Voice Fucker: a dirty vocoder. The vocal is the modulator; the carrier is an
// internal synth (MIDI / auto-tracked / fixed pitch), a sidechain input, or both.
class VoiceFuckerProcessor : public juce::AudioProcessor,
                             private juce::AudioProcessorValueTreeState::Listener
{
public:
    static constexpr int maxBands = 100;
    static constexpr int maxVoices = 8;
    static constexpr int waveSize = 8192;

    VoiceFuckerProcessor();
    ~VoiceFuckerProcessor() override;

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

    // ---- presets
    juce::StringArray getFactoryPresetNames() const;
    void loadFactoryPreset (int index);
    juce::Array<juce::File> getUserPresets() const;
    juce::File getUserPresetFolder() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    juce::String getPresetName() const { return presetName; }
    bool isPresetDirty() const { return presetDirty.load(); }

    // ---- UI settings stored with the session
    float uiScale = 1.0f;
    int screenMode = 0;

    // ---- data for the UI (written by the audio thread, read by the UI)
    std::atomic<float> inPeak[2], outPeak[2];
    std::array<std::atomic<float>, maxBands> bandLevel;
    std::atomic<int> numBandsUi { 28 };
    std::array<float, waveSize> wave {};
    std::atomic<int> waveWrite { 0 };
    std::array<std::atomic<int>, maxVoices> uiNotes;   // MIDI notes currently sounding (-1 = none)
    std::atomic<float> uiAutoHz { 0 };

private:
    void parameterChanged (const juce::String&, float) override;
    void updateBands (int bands, float formant);
    void computeHarmonyLayout (int voiceIndex, float baseNote);
    void handleMidi (const juce::MidiMessage& m);
    void resetToDefaults();

    template <typename T> T get (std::atomic<float>* p) const { return (T) p->load(); }

    // parameter pointers
    std::atomic<float> *pInGain, *pOutGain, *pSrc, *pPitch, *pNote, *pGlide, *pDetune, *pSpread, *pNoise,
                       *pVocOn, *pBands, *pFormant, *pAttack, *pRelease, *pSib, *pGate,
                       *pHarmOn, *pScale, *pKey, *pPan, *pHarmLevel, *pHarmDetune, *pHarmSpread,
                       *pDirtOn, *pDirtMode, *pDrive, *pCrunch, *pDestroy,
                       *pWidthOn, *pWidthMode, *pWidth, *pMono,
                       *pMix, *pAutoGain, *pClipOn, *pClipMode, *pCeiling;
    std::array<std::atomic<float>*, Params::numIntervals> pIvOn, pIvDb;

    double sr = 44100.0;

    // ---- voices
    struct Voice
    {
        bool active = false;
        int note = -1;
        float logTarget = 0, logFreq = 0;   // log2(Hz)
        float amp = 0;
        float phase[7] {};
        float hPhase[Params::numIntervals][3] {};
        float harmRatio[Params::numIntervals] {};
        uint32_t age = 0;
    };
    std::array<Voice, maxVoices> voices;
    uint32_t voiceCounter = 0;
    float lastLogFreq = -1;
    int lastPitchMode = -1;

    vf::PitchDetector pitch;
    float autoLogTarget = std::log2 (110.f);

    // ---- vocoder
    struct Band
    {
        vf::Biquad m1, m2, cl1, cl2, cr1, cr2, ch1, ch2;
        float envM = 0, envCL = 0, envCR = 0, envCH = 0, peak = 0;
        bool enabled = true;
        float panL = 1, panR = 1;
    };
    std::array<Band, maxBands> bands;
    int curBands = -1;
    float curFormant = 1000.f;

    vf::Biquad sib1, sib2;
    vf::Noise noiseA { 0x9e3779b9u }, noiseB { 0x7f4a7c15u }, noiseC { 0x2545f491u };

    float gateDet = 0, gateGain = 1;
    bool gateOpen = true;

    // ---- post
    float holdL = 0, holdR = 0, holdPhase = 0;
    vf::DelayLine dlL, dlR;
    float chorusPhase = 0;
    vf::Biquad sideHp1, sideHp2;
    float curMonoHz = -1;

    float rmsDry = 0, rmsWet = 0, autoGain = 1;

    juce::SmoothedValue<float> sInGain, sOutGain, sDirtOn, sWidthOn, sClipOn, sMix, sDrive, sCrunch,
                               sDestroy, sWidth, sHaas, sVocOn, sHarmLevel, sCeiling;

    juce::AudioBuffer<float> scBuffer;

    // ---- presets
    juce::String presetName { "Init" };
    std::atomic<bool> presetDirty { false };
    bool loadingPreset = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceFuckerProcessor)
};
