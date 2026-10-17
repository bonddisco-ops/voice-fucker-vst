#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace vf;

namespace
{
    // unison layouts: detune position (-1..1) for each oscillator
    constexpr float uni7[7] = { -1.f, -0.62f, -0.28f, 0.f, 0.28f, 0.62f, 1.f };
    constexpr float uni3[3] = { -1.f, 0.f, 1.f };

    inline void panGains (float pos, float& l, float& r)   // pos -1..1, constant power
    {
        const float a = (juce::jlimit (-1.f, 1.f, pos) + 1.f) * (pi * 0.25f);
        l = std::min (1.f, std::cos (a) * 1.41421356f);
        r = std::min (1.f, std::sin (a) * 1.41421356f);
    }

    bool inScale (int note, int key)
    {
        static constexpr int minor[7] = { 0, 2, 3, 5, 7, 8, 10 };
        static constexpr int major[7] = { 0, 2, 4, 5, 7, 9, 11 };
        const int root = key % 12;
        const int* sc = key < 12 ? minor : major;
        const int pc = ((note - root) % 12 + 12) % 12;
        for (int i = 0; i < 7; ++i) if (sc[i] == pc) return true;
        return false;
    }

    int snapInterval (int base, int iv, bool useScale, int key)
    {
        const int a = std::abs (iv), sign = iv < 0 ? -1 : 1;
        if (! useScale || a == 12 || a == 24) return iv;
        std::vector<int> cand;
        if (a == 3) cand = { 3, 4 };
        else if (a == 5) cand = { 5, 6 };
        else cand = { 7, 6, 8 };
        for (int c : cand)
            if (inScale (base + sign * c, key)) return sign * c;
        return iv;
    }

    struct FactoryPreset { const char* name; std::vector<std::pair<const char*, float>> values; };

    const std::vector<FactoryPreset>& factoryPresets()
    {
        static const std::vector<FactoryPreset> list {
            { "Init", {} },
            { "Demon Choir", { { "voc_formant", -6.f }, { "voc_bands", 32 }, { "harm_iv0_on", 1 }, { "harm_iv1_on", 1 },
                               { "harm_iv7_on", 1 }, { "harm_pan", 1 }, { "dirt_drive", 45 }, { "dirt_crunch", 55 },
                               { "width_amt", 70 }, { "car_src", 2 } } },
            { "Daft Robot", { { "voc_bands", 16 }, { "car_src", 0 }, { "car_detune", 10 }, { "dirt_drive", 10 },
                              { "dirt_crunch", 5 }, { "voc_release", 30 }, { "width_amt", 30 } } },
            { "Crunch Beast", { { "voc_bands", 24 }, { "dirt_mode", 1 }, { "dirt_drive", 70 }, { "dirt_crunch", 90 },
                                { "dirt_destroy", 20 }, { "voc_formant", -3 }, { "harm_iv1_on", 1 }, { "clip_mode", 1 } } },
            { "Ghost Whisper", { { "car_src", 3 }, { "voc_bands", 40 }, { "voc_release", 120 }, { "width_mode", 2 },
                                 { "width_amt", 80 }, { "dirt_drive", 0 }, { "dirt_crunch", 10 }, { "voc_sib", 70 } } },
            { "Goblin Squad", { { "voc_formant", 7 }, { "harm_iv6_on", 1 }, { "harm_iv8_on", 1 }, { "harm_iv5_on", 1 },
                                { "harm_pan", 0 }, { "harm_spread", 90 }, { "dirt_crunch", 50 }, { "dirt_mode", 2 } } },
            { "Radio Grunt", { { "voc_bands", 8 }, { "dirt_mode", 2 }, { "dirt_destroy", 60 }, { "dirt_crunch", 70 },
                               { "width_on", 0 }, { "voc_formant", -2 } } },
            { "Wide Wall", { { "voc_bands", 48 }, { "harm_iv1_on", 1 }, { "harm_iv3_on", 1 }, { "harm_iv7_on", 1 },
                             { "harm_iv8_on", 1 }, { "harm_pan", 2 }, { "width_mode", 0 }, { "width_amt", 90 },
                             { "dirt_crunch", 30 } } },
        };
        return list;
    }
}

//==============================================================================
VoiceFuckerProcessor::VoiceFuckerProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", Params::createLayout())
{
    auto P = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    pInGain = P ("in_gain"); pOutGain = P ("out_gain");
    pSrc = P ("car_src"); pPitch = P ("car_pitch"); pNote = P ("car_note"); pGlide = P ("car_glide");
    pDetune = P ("car_detune"); pSpread = P ("car_spread"); pNoise = P ("car_noise");
    pVocOn = P ("voc_on"); pBands = P ("voc_bands"); pFormant = P ("voc_formant"); pAttack = P ("voc_attack");
    pRelease = P ("voc_release"); pSib = P ("voc_sib"); pGate = P ("voc_gate");
    pHarmOn = P ("harm_on"); pScale = P ("harm_scale"); pKey = P ("harm_key"); pPan = P ("harm_pan");
    pHarmLevel = P ("harm_level"); pHarmDetune = P ("harm_detune"); pHarmSpread = P ("harm_spread");
    pDirtOn = P ("dirt_on"); pDirtMode = P ("dirt_mode"); pDrive = P ("dirt_drive"); pCrunch = P ("dirt_crunch");
    pDestroy = P ("dirt_destroy");
    pWidthOn = P ("width_on"); pWidthMode = P ("width_mode"); pWidth = P ("width_amt"); pMono = P ("width_mono");
    pMix = P ("mix"); pAutoGain = P ("autogain");
    pClipOn = P ("clip_on"); pClipMode = P ("clip_mode"); pCeiling = P ("clip_ceiling");
    for (int i = 0; i < Params::numIntervals; ++i) { pIvOn[(size_t) i] = P (Params::ivOn (i)); pIvDb[(size_t) i] = P (Params::ivDb (i)); }

    for (auto& b : bandLevel) b.store (0.f);
    for (auto& n : uiNotes) n.store (-1);
    for (int c = 0; c < 2; ++c) { inPeak[c].store (0.f); outPeak[c].store (0.f); }

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.addParameterListener (rp->getParameterID(), this);
}

VoiceFuckerProcessor::~VoiceFuckerProcessor()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            apvts.removeParameterListener (rp->getParameterID(), this);
}

void VoiceFuckerProcessor::parameterChanged (const juce::String&, float)
{
    if (! loadingPreset) presetDirty = true;
}

bool VoiceFuckerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    const auto in = layouts.getMainInputChannelSet();
    if (in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono()) return false;
    }
    return true;
}

//==============================================================================
void VoiceFuckerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    pitch.prepare (sampleRate);
    curBands = -1;
    for (auto& b : bands) b = Band();
    for (auto& v : voices) v = Voice();
    lastLogFreq = -1;

    sib1.setHighPass ((float) sr, 4500.f); sib2 = sib1;
    sib1.reset(); sib2.reset();

    dlL.prepare ((int) (sr * 0.05)); dlR.prepare ((int) (sr * 0.05));
    sideHp1.reset(); sideHp2.reset(); curMonoHz = -1;
    gateDet = 0; gateGain = 1; gateOpen = true;
    rmsDry = rmsWet = 0; autoGain = 1;
    holdL = holdR = holdPhase = 0;

    auto prep = [&] (juce::SmoothedValue<float>& s, float v, double t = 0.03)
    { s.reset (sampleRate, t); s.setCurrentAndTargetValue (v); };
    prep (sInGain, dbToGain (*pInGain)); prep (sOutGain, dbToGain (*pOutGain));
    prep (sDirtOn, *pDirtOn > 0.5f); prep (sWidthOn, *pWidthOn > 0.5f); prep (sClipOn, *pClipOn > 0.5f);
    prep (sVocOn, *pVocOn > 0.5f);
    prep (sMix, *pMix * 0.01f); prep (sDrive, *pDrive * 0.01f); prep (sCrunch, *pCrunch * 0.01f);
    prep (sDestroy, *pDestroy * 0.01f); prep (sWidth, *pWidth * 0.01f); prep (sHaas, 0.f, 0.08);
    prep (sHarmLevel, *pHarmLevel * 0.01f); prep (sCeiling, dbToGain (*pCeiling));

    scBuffer.setSize (2, std::max (1, samplesPerBlock));
}

void VoiceFuckerProcessor::updateBands (int n, float formant)
{
    n = juce::jlimit (4, maxBands, n);
    const float fs = (float) sr;
    const float fLo = 90.f, fHi = std::min (11000.f, fs * 0.42f);
    const float bwOct = std::log2 (fHi / fLo) / (float) n;
    const float r = std::pow (2.f, bwOct);
    float q = std::sqrt (r) / (r - 1.f);
    q = juce::jlimit (0.5f, 40.f, q * 0.8f);
    const float shift = std::pow (2.f, formant / 12.f);

    for (int i = 0; i < n; ++i)
    {
        auto& b = bands[(size_t) i];
        const float fc = fLo * std::pow (2.f, bwOct * ((float) i + 0.5f));
        auto keep = [] (Biquad& dst, const Biquad& src) { const float z1 = dst.z1, z2 = dst.z2; dst = src; dst.z1 = z1; dst.z2 = z2; };
        Biquad m; m.setBandPass (fs, fc, q);
        keep (b.m1, m); keep (b.m2, m);
        const float fcC = fc * shift;
        b.enabled = fcC < fs * 0.45f && fcC > 25.f;
        Biquad c; c.setBandPass (fs, juce::jlimit (25.f, fs * 0.45f, fcC), q);
        keep (b.cl1, c); keep (b.cl2, c); keep (b.cr1, c); keep (b.cr2, c); keep (b.ch1, c); keep (b.ch2, c);
    }
    if (n != curBands)
        for (int i = curBands < 0 ? 0 : std::min (curBands, n); i < maxBands; ++i)
        {
            auto& b = bands[(size_t) i];
            b.m1.reset(); b.m2.reset(); b.cl1.reset(); b.cl2.reset(); b.cr1.reset(); b.cr2.reset();
            b.ch1.reset(); b.ch2.reset(); b.envM = b.envCL = b.envCR = b.envCH = 0;
        }
    curBands = n;
    curFormant = formant;
    numBandsUi = n;
}

void VoiceFuckerProcessor::computeHarmonyLayout (int vi, float baseNote)
{
    auto& v = voices[(size_t) vi];
    const bool useScale = *pScale > 0.5f;
    const int key = (int) *pKey;
    const int base = juce::roundToInt (baseNote);
    for (int j = 0; j < Params::numIntervals; ++j)
    {
        const int semis = snapInterval (base, Params::intervals[j], useScale, key);
        v.harmRatio[j] = std::pow (2.f, (float) semis / 12.f);
    }
}

void VoiceFuckerProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        int slot = -1;
        for (int i = 0; i < maxVoices; ++i) if (voices[(size_t) i].active && voices[(size_t) i].note == note) slot = i;
        if (slot < 0) for (int i = 0; i < maxVoices; ++i) if (! voices[(size_t) i].active && voices[(size_t) i].amp < 1.0e-4f) { slot = i; break; }
        if (slot < 0) for (int i = 0; i < maxVoices; ++i) if (! voices[(size_t) i].active) { slot = i; break; }
        if (slot < 0)
        {
            uint32_t oldest = UINT32_MAX;
            for (int i = 0; i < maxVoices; ++i) if (voices[(size_t) i].age < oldest) { oldest = voices[(size_t) i].age; slot = i; }
        }
        auto& v = voices[(size_t) slot];
        const float target = std::log2 (midiToHz ((float) note));
        const bool wasSilent = v.amp < 1.0e-4f;
        v.active = true; v.note = note; v.logTarget = target; v.age = ++voiceCounter;
        if (wasSilent) v.logFreq = (*pGlide > 0.5f && lastLogFreq > 0) ? lastLogFreq : target;
        lastLogFreq = target;
        computeHarmonyLayout (slot, (float) note);
    }
    else if (m.isNoteOff())
    {
        for (auto& v : voices) if (v.active && v.note == m.getNoteNumber()) v.active = false;
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        for (auto& v : voices) v.active = false;
    }
}

//==============================================================================
void VoiceFuckerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const float fs = (float) sr;

    auto mainIn = getBusBuffer (buffer, true, 0);
    auto out = getBusBuffer (buffer, false, 0);
    const int inCh = mainIn.getNumChannels(), outCh = out.getNumChannels();
    if (n == 0 || outCh == 0) return;

    // sidechain copy
    bool haveSc = false;
    if (getBusCount (true) > 1)
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0 && getBus (true, 1)->isEnabled())
        {
            if (scBuffer.getNumSamples() < n) scBuffer.setSize (2, n, false, false, true);
            scBuffer.copyFrom (0, 0, sc, 0, 0, n);
            scBuffer.copyFrom (1, 0, sc, sc.getNumChannels() > 1 ? 1 : 0, 0, n);
            haveSc = true;
        }
    }

    // ---------------- block parameters
    const int src = (int) *pSrc;
    const int pitchMode = (int) *pPitch;
    const bool synthOn = src <= 2 || src == 5;
    const bool scOn = (src == 4 || src == 5) && haveSc;
    const int osc = src == 5 ? 2 : src;                     // Synth+SC uses the supersaw
    const int nUni = osc == 2 ? 7 : (osc == 3 ? 0 : 3);
    const float detCents = (osc == 2 ? 45.f : 14.f) * (*pDetune * 0.01f);
    const float spread = *pSpread * 0.01f;
    const float noiseAmt = *pNoise * 0.01f;
    const float glideMs = *pGlide;
    const float glideCoef = glideMs < 0.5f ? 0.f : std::exp (-1.f / (0.001f * glideMs * 0.35f * fs));

    float uniRatio[7], uniL[7], uniR[7];
    for (int k = 0; k < 7; ++k)
    {
        const float pos = nUni == 7 ? uni7[k] : (k < 3 ? uni3[k] : 0.f);
        uniRatio[k] = std::pow (2.f, pos * detCents / 1200.f);
        panGains (pos * spread * (k % 2 ? -1.f : 1.f), uniL[k], uniR[k]);
    }
    const float uniNorm = nUni > 0 ? 1.f / std::sqrt ((float) nUni) : 1.f;

    // harmonies
    const bool harmOn = *pHarmOn > 0.5f;
    const int panMode = (int) *pPan;
    const float hSpread = *pHarmSpread * 0.01f;
    const float hDetCents = 30.f * (*pHarmDetune * 0.01f);
    int hList[Params::numIntervals]; int nH = 0;
    float hGain[Params::numIntervals], hL[Params::numIntervals], hR[Params::numIntervals];
    for (int j = 0; j < Params::numIntervals; ++j)
        if (harmOn && *pIvOn[(size_t) j] > 0.5f) hList[nH++] = j;
    for (int k = 0; k < nH; ++k)
    {
        const int j = hList[k];
        hGain[j] = dbToGain (*pIvDb[(size_t) j]);
        float pos = 0;
        if (panMode == 0) pos = (k % 2 ? 1.f : -1.f) * hSpread * (0.5f + 0.5f * (float) (k + 1) / (float) nH);
        else if (panMode == 1) pos = nH > 1 ? (-1.f + 2.f * (float) k / (float) (nH - 1)) * hSpread : 0.f;
        panGains (pos, hL[j], hR[j]);
    }
    const bool hSpectral = panMode == 2 && nH > 0;
    const float hDet[3] = { std::pow (2.f, -hDetCents / 1200.f), 1.f, std::pow (2.f, hDetCents / 1200.f) };
    const int hWave = osc == 3 ? 0 : osc;

    // voices for auto / fixed modes
    if (pitchMode != lastPitchMode)
    {
        for (auto& v : voices) { v.active = false; }
        lastPitchMode = pitchMode;
    }
    if (pitchMode == 2)
    {
        auto& v = voices[0];
        const float t = std::log2 (midiToHz (*pNote));
        if (! v.active || v.note != (int) *pNote) { v.active = true; v.note = (int) *pNote; v.logTarget = t; if (v.amp < 1.0e-4f) v.logFreq = t; }
        computeHarmonyLayout (0, *pNote);
    }
    else if (pitchMode == 1)
    {
        auto& v = voices[0];
        if (! v.active) { v.active = true; v.note = -1; v.logTarget = autoLogTarget; v.logFreq = autoLogTarget; }
    }

    // vocoder settings
    const int nb = (int) *pBands;
    const float formant = *pFormant;
    if (nb != curBands || std::abs (formant - curFormant) > 0.005f) updateBands (nb, formant);
    const float atk = onePoleCoef (*pAttack, fs), rel = onePoleCoef (*pRelease, fs);
    const float cAtk = onePoleCoef (0.6f, fs), cRel = onePoleCoef (25.f, fs);
    const float sibAmt = *pSib * 0.01f;
    const float gateThr = *pGate <= -79.9f ? -1.f : dbToGain (*pGate);
    const float gAtk = onePoleCoef (1.f, fs), gRel = onePoleCoef (60.f, fs);
    const float detRel = onePoleCoef (40.f, fs);

    const int dirtMode = (int) *pDirtMode;
    const int widthMode = (int) *pWidthMode;
    const bool clipHard = *pClipMode > 0.5f;
    const bool autoGainOn = *pAutoGain > 0.5f;

    const float monoHz = *pMono;
    if (std::abs (monoHz - curMonoHz) > 0.5f)
    {
        sideHp1.setHighPass (fs, monoHz, 0.7071f);
        { const float z1 = sideHp2.z1, z2 = sideHp2.z2; sideHp2 = sideHp1; sideHp2.z1 = z1; sideHp2.z2 = z2; }
        curMonoHz = monoHz;
    }

    sInGain.setTargetValue (dbToGain (*pInGain)); sOutGain.setTargetValue (dbToGain (*pOutGain));
    sDirtOn.setTargetValue (*pDirtOn > 0.5f ? 1.f : 0.f); sWidthOn.setTargetValue (*pWidthOn > 0.5f ? 1.f : 0.f);
    sClipOn.setTargetValue (*pClipOn > 0.5f ? 1.f : 0.f); sVocOn.setTargetValue (*pVocOn > 0.5f ? 1.f : 0.f);
    sMix.setTargetValue (*pMix * 0.01f); sDrive.setTargetValue (*pDrive * 0.01f); sCrunch.setTargetValue (*pCrunch * 0.01f);
    sDestroy.setTargetValue (*pDestroy * 0.01f); sWidth.setTargetValue (*pWidth * 0.01f);
    sHaas.setTargetValue (widthMode == 1 ? 1.f : 0.f);
    sHarmLevel.setTargetValue (*pHarmLevel * 0.01f); sCeiling.setTargetValue (dbToGain (*pCeiling));

    const float vAtk = onePoleCoef (3.f, fs), vRel = onePoleCoef (70.f, fs);
    const float rmsCoef = onePoleCoef (300.f, fs), agCoef = onePoleCoef (80.f, fs);

    float peakIn[2] = { 0, 0 }, peakOut[2] = { 0, 0 };
    std::array<float, maxBands> bandPk {};

    auto midiIt = midi.cbegin();
    const auto midiEnd = midi.cend();

    const float* inL = mainIn.getReadPointer (0);
    const float* inR = mainIn.getReadPointer (inCh > 1 ? 1 : 0);
    float* oL = out.getWritePointer (0);
    float* oR = out.getWritePointer (outCh > 1 ? 1 : 0);

    int wpos = waveWrite.load();

    for (int s = 0; s < n; ++s)
    {
        // ---- MIDI
        while (midiIt != midiEnd && (*midiIt).samplePosition <= s)
        {
            if (pitchMode == 0) handleMidi ((*midiIt).getMessage());
            ++midiIt;
        }

        const float gIn = sInGain.getNextValue();
        const float xL = inL[s] * gIn, xR = inR[s] * gIn;
        peakIn[0] = std::max (peakIn[0], std::abs (xL));
        peakIn[1] = std::max (peakIn[1], std::abs (xR));
        const float mod = 0.5f * (xL + xR);

        // ---- auto pitch
        if (pitchMode == 1 && pitch.push (mod))
        {
            if (pitch.freq > 50.f && pitch.freq < 1100.f && pitch.confidence > 0.6f)
            {
                autoLogTarget = std::log2 (pitch.freq);
                voices[0].logTarget = autoLogTarget;
                uiAutoHz = pitch.freq;
                computeHarmonyLayout (0, hzToMidi (pitch.freq));
            }
        }

        const float dOn = sDirtOn.getNextValue();
        const float drive = sDrive.getNextValue() * dOn;
        const float crunch = sCrunch.getNextValue() * dOn;
        const float destroy = sDestroy.getNextValue() * dOn;
        const float wOn = sWidthOn.getNextValue();
        const float width = sWidth.getNextValue() * wOn;
        const float vocOn = sVocOn.getNextValue();
        const float harmLevel = sHarmLevel.getNextValue();

        // ---- carrier
        float cL = 0, cR = 0, cH = 0;
        if (synthOn)
        {
            if (osc == 3)
            {
                const float a = noiseA.next(), b = noiseB.next();
                cL = a; cR = a + (b - a) * spread;
            }
            else
            {
                for (int vi = 0; vi < maxVoices; ++vi)
                {
                    auto& v = voices[(size_t) vi];
                    v.amp += ((v.active ? 1.f : 0.f) - v.amp) * (1.f - (v.active ? vAtk : vRel));
                    if (v.amp < 1.0e-5f && ! v.active) continue;
                    v.logFreq = v.logTarget + (v.logFreq - v.logTarget) * glideCoef;
                    const float f = std::exp2 (v.logFreq);
                    const float amp = v.amp;

                    for (int k = 0; k < nUni; ++k)
                    {
                        const float dt = std::min (0.45f, f * uniRatio[k] / fs);
                        float& ph = v.phase[k];
                        const float o = osc == 1 ? squareBlep (ph, dt) : sawBlep (ph, dt);
                        ph += dt; if (ph >= 1.f) ph -= 1.f;
                        cL += o * uniL[k] * uniNorm * amp;
                        cR += o * uniR[k] * uniNorm * amp;
                    }
                    for (int k = 0; k < nH; ++k)
                    {
                        const int j = hList[k];
                        const float fh = f * v.harmRatio[j];
                        const float g = hGain[j] * harmLevel * amp * 0.577f;
                        float sum = 0;
                        for (int u = 0; u < 3; ++u)
                        {
                            const float dt = std::min (0.45f, fh * hDet[u] / fs);
                            float& ph = v.hPhase[j][u];
                            sum += hWave == 1 ? squareBlep (ph, dt) : sawBlep (ph, dt);
                            ph += dt; if (ph >= 1.f) ph -= 1.f;
                        }
                        if (hSpectral) cH += sum * g;
                        else { cL += sum * g * hL[j]; cR += sum * g * hR[j]; }
                    }
                }
            }
        }
        if (scOn) { cL += scBuffer.getSample (0, s); cR += scBuffer.getSample (1, s); }
        if (noiseAmt > 0)
        {
            const float a = noiseA.next(), b = noiseC.next();
            cL += a * noiseAmt * 0.6f; cR += (a + (b - a) * spread) * noiseAmt * 0.6f;
        }
        if (drive > 0.001f)
        {
            const float k = dbToGain (drive * 32.f);
            const float mixD = std::min (1.f, drive * 6.f);
            cL += (shape (dirtMode, cL * k) - cL) * mixD;
            cR += (shape (dirtMode, cR * k) - cR) * mixD;
            cH += (shape (dirtMode, cH * k) - cH) * mixD;
        }

        // ---- vocoder
        float vL = 0, vR = 0;
        const float crK = dbToGain (crunch * 38.f);
        const float crMix = std::min (1.f, crunch * 5.f);
        const float crNorm = 1.f / std::sqrt (crK);
        if (vocOn > 0.0001f)
        {
            for (int i = 0; i < curBands; ++i)
            {
                auto& b = bands[(size_t) i];
                const float mb = b.m2.process (b.m1.process (mod));
                const float am = std::abs (mb);
                b.envM = am + (b.envM - am) * (am > b.envM ? atk : rel);
                if (! b.enabled) continue;

                const float bl = b.cl2.process (b.cl1.process (cL));
                const float br = b.cr2.process (b.cr1.process (cR));
                const float al = std::abs (bl), ar = std::abs (br);
                b.envCL = al + (b.envCL - al) * (al > b.envCL ? cAtk : cRel);
                b.envCR = ar + (b.envCR - ar) * (ar > b.envCR ? cAtk : cRel);
                float yl = bl * b.envM / (b.envCL + 1.0e-4f);
                float yr = br * b.envM / (b.envCR + 1.0e-4f);

                if (hSpectral)
                {
                    const float bh = b.ch2.process (b.ch1.process (cH));
                    const float ah = std::abs (bh);
                    b.envCH = ah + (b.envCH - ah) * (ah > b.envCH ? cAtk : cRel);
                    const float yh = bh * b.envM / (b.envCH + 1.0e-4f) * std::min (1.5f, b.envCH * 8.f);
                    const float side = (i % 2 ? 1.f : -1.f) * hSpread;
                    yl += yh * (1.f - std::max (0.f, side));
                    yr += yh * (1.f + std::min (0.f, side));
                }

                if (crMix > 0)
                {
                    yl += (shape (dirtMode, yl * crK) * crNorm - yl) * crMix;
                    yr += (shape (dirtMode, yr * crK) * crNorm - yr) * crMix;
                }

                // spectral width: alternate bands lean left / right
                const float w = widthMode == 0 ? width : 0.f;
                const float gl = (i % 2 == 0) ? 1.f : 1.f - w, gr = (i % 2 == 0) ? 1.f - w : 1.f;
                vL += yl * gl;
                vR += yr * gr;
                b.peak = std::max (b.peak, std::abs (yl) + std::abs (yr));
                bandPk[(size_t) i] = std::max (bandPk[(size_t) i], b.envM);
            }
            const float sib = sib2.process (sib1.process (mod)) * sibAmt * 1.6f;
            vL += sib; vR += sib;

            // gate
            const float am = std::abs (mod);
            gateDet = am > gateDet ? am : am + (gateDet - am) * detRel;
            if (gateThr < 0) gateOpen = true;
            else if (gateOpen && gateDet < gateThr * 0.7f) gateOpen = false;
            else if (! gateOpen && gateDet > gateThr) gateOpen = true;
            gateGain = (gateOpen ? 1.f : 0.f) + (gateGain - (gateOpen ? 1.f : 0.f)) * (gateOpen ? gAtk : gRel);
            vL *= gateGain; vR *= gateGain;
        }

        // vocoder bypass: the raw voice goes through the dirt instead
        float wL = vL * vocOn, wR = vR * vocOn;
        if (vocOn < 0.9999f)
        {
            float rl = xL, rr = xR;
            const float amt = std::max (drive, crunch);
            if (amt > 0.001f)
            {
                const float k = dbToGain (amt * 34.f);
                const float m2 = std::min (1.f, amt * 5.f);
                rl += (shape (dirtMode, rl * k) / std::sqrt (k) * 1.5f - rl) * m2;
                rr += (shape (dirtMode, rr * k) / std::sqrt (k) * 1.5f - rr) * m2;
            }
            wL += rl * (1.f - vocOn); wR += rr * (1.f - vocOn);
        }

        // ---- destroy (bitcrush + downsample)
        if (destroy > 0.001f)
        {
            const float step = 1.f / (1.f + destroy * destroy * 30.f);
            holdPhase += step;
            if (holdPhase >= 1.f)
            {
                holdPhase -= 1.f;
                const float bits = 16.f - destroy * 12.5f;
                const float q = std::exp2 (bits - 1.f);
                holdL = std::round (wL * q) / q; holdR = std::round (wR * q) / q;
            }
            const float m3 = std::min (1.f, destroy * 4.f);
            wL += (holdL - wL) * m3; wR += (holdR - wR) * m3;
        }

        // ---- width
        dlL.push (wL); dlR.push (wR);
        const float haas = sHaas.getNextValue();
        if (wOn > 0.0001f)
        {
            if (widthMode == 1 || haas > 0.0001f)
            {
                const float d = 1.f + haas * width * 0.025f * fs;
                const float dr = dlR.read (d);
                wR = wR + (dr - wR) * haas * wOn;
            }
            if (widthMode == 2)
            {
                chorusPhase += 0.55f / fs; if (chorusPhase >= 1.f) chorusPhase -= 1.f;
                const float a = twoPi * chorusPhase;
                const float d1 = (0.007f + 0.0025f * std::sin (a)) * fs;
                const float d2 = (0.009f + 0.0025f * std::sin (a + 1.7f)) * fs;
                const float chL = dlL.read (d1), chR = dlR.read (d2);
                wL = wL * (1.f - 0.35f * width) + chR * 0.8f * width;
                wR = wR * (1.f - 0.35f * width) + chL * 0.8f * width;
            }
            float mid = 0.5f * (wL + wR), side = 0.5f * (wL - wR);
            if (widthMode == 0) side *= 1.f + 0.6f * width;
            const float hp = sideHp2.process (sideHp1.process (side));
            side += (hp - side) * wOn;
            wL = mid + side; wR = mid - side;
        }

        // ---- mix + auto gain
        rmsDry = xL * xL + xR * xR + (rmsDry - (xL * xL + xR * xR)) * rmsCoef;
        rmsWet = wL * wL + wR * wR + (rmsWet - (wL * wL + wR * wR)) * rmsCoef;
        if (autoGainOn)
        {
            if (rmsDry > 1.0e-7f && rmsWet > 1.0e-9f)
            {
                const float target = juce::jlimit (0.05f, 16.f, std::sqrt (rmsDry / rmsWet));
                autoGain = target + (autoGain - target) * agCoef;
            }
        }
        else autoGain = 1.f + (autoGain - 1.f) * agCoef;
        const float mix = sMix.getNextValue();
        float yL = xL * (1.f - mix) + wL * autoGain * mix;
        float yR = xR * (1.f - mix) + wR * autoGain * mix;

        // ---- clip
        const float clipOn = sClipOn.getNextValue();
        const float c = sCeiling.getNextValue();
        if (clipOn > 0.0001f)
        {
            auto clip = [c, clipHard] (float x)
            {
                if (clipHard) return juce::jlimit (-c, c, x);
                const float a = std::abs (x), knee = c * 0.6f;
                if (a <= knee) return x;
                const float r = c - knee;
                const float y = knee + r * std::tanh ((a - knee) / r);
                return x < 0 ? -y : y;
            };
            yL += (clip (yL) - yL) * clipOn;
            yR += (clip (yR) - yR) * clipOn;
        }

        const float gOut = sOutGain.getNextValue();
        yL *= gOut; yR *= gOut;
        if (! std::isfinite (yL)) yL = 0;
        if (! std::isfinite (yR)) yR = 0;

        oL[s] = yL;
        if (outCh > 1) oR[s] = yR; else oL[s] = 0.5f * (yL + yR);
        peakOut[0] = std::max (peakOut[0], std::abs (yL));
        peakOut[1] = std::max (peakOut[1], std::abs (yR));

        wave[(size_t) wpos] = 0.5f * (yL + yR);
        wpos = (wpos + 1) & (waveSize - 1);
    }
    waveWrite = wpos;

    // ---- data for the UI
    for (int c = 0; c < 2; ++c)
    {
        inPeak[c] = std::max (inPeak[c].load(), peakIn[c]);
        outPeak[c] = std::max (outPeak[c].load(), peakOut[c]);
    }
    for (int i = 0; i < curBands; ++i)
        bandLevel[(size_t) i] = std::max (bandLevel[(size_t) i].load(), bandPk[(size_t) i]);
    for (int i = 0; i < maxVoices; ++i)
        uiNotes[(size_t) i] = (voices[(size_t) i].active && pitchMode != 1) ? voices[(size_t) i].note : -1;
    if (pitchMode != 1) uiAutoHz = 0;

    for (int c = 2; c < buffer.getNumChannels(); ++c)
        if (c >= outCh) buffer.clear (c, 0, n);
}

//==============================================================================
void VoiceFuckerProcessor::resetToDefaults()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
}

juce::StringArray VoiceFuckerProcessor::getFactoryPresetNames() const
{
    juce::StringArray s;
    for (auto& p : factoryPresets()) s.add (p.name);
    return s;
}

void VoiceFuckerProcessor::loadFactoryPreset (int index)
{
    const auto& list = factoryPresets();
    if (index < 0 || index >= (int) list.size()) return;
    loadingPreset = true;
    resetToDefaults();
    for (auto& [id, value] : list[(size_t) index].values)
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
    loadingPreset = false;
    presetName = list[(size_t) index].name;
    presetDirty = false;
}

juce::File VoiceFuckerProcessor::getUserPresetFolder() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
        .getChildFile ("Primal Rage Lab").getChildFile ("Voice Fucker").getChildFile ("Presets");
}

juce::Array<juce::File> VoiceFuckerProcessor::getUserPresets() const
{
    auto files = getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.vfpreset");
    files.sort();
    return files;
}

bool VoiceFuckerProcessor::saveUserPreset (const juce::String& name)
{
    auto folder = getUserPresetFolder();
    if (! folder.createDirectory()) return false;
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr) return false;
    const auto safe = juce::File::createLegalFileName (name.trim());
    if (safe.isEmpty()) return false;
    if (! xml->writeTo (folder.getChildFile (safe + ".vfpreset"))) return false;
    presetName = safe;
    presetDirty = false;
    return true;
}

bool VoiceFuckerProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    loadingPreset = true;
    resetToDefaults();
    auto tree = juce::ValueTree::fromXml (*xml);
    for (auto child : tree)
    {
        const auto id = child.getProperty ("id").toString();
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 ((float) child.getProperty ("value")));
    }
    loadingPreset = false;
    presetName = file.getFileNameWithoutExtension();
    presetDirty = false;
    return true;
}

//==============================================================================
void VoiceFuckerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("uiScale", uiScale, nullptr);
    state.setProperty ("screenMode", screenMode, nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetDirty", presetDirty.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void VoiceFuckerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            loadingPreset = true;
            auto tree = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (tree);
            uiScale = juce::jlimit (0.6f, 2.0f, (float) tree.getProperty ("uiScale", 1.0f));
            screenMode = (int) tree.getProperty ("screenMode", 0);
            presetName = tree.getProperty ("presetName", "Init").toString();
            loadingPreset = false;
            presetDirty = (bool) tree.getProperty ("presetDirty", false);
        }
}

juce::AudioProcessorEditor* VoiceFuckerProcessor::createEditor() { return new VoiceFuckerEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoiceFuckerProcessor(); }
