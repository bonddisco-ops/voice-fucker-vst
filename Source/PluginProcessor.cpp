#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace vf;

//==============================================================================
// Parameter layout
//==============================================================================
namespace
{
    String hzText (float v) { return v < 1000.0f ? String (roundToInt (v)) + " Hz" : String (v / 1000.0f, 1) + " kHz"; }
    float hzFromText (const String& t)
    {
        const auto s = t.trim().toLowerCase();
        const float v = s.getFloatValue();
        return s.containsChar ('k') ? v * 1000.0f : v;
    }
    String dbText (float v) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; }
    String msText (float v) { return v < 10.0f ? String (v, 1) + " ms" : (v < 1000.0f ? String (roundToInt (v)) + " ms" : String (v / 1000.0f, 2) + " s"); }
}

AudioProcessorValueTreeState::ParameterLayout Params::createLayout()
{
    using namespace ParamIDs;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (const String& id, const String& name, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 100.0f), def,
                   AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; }));
    };
    auto hzParam = [] (const String& id, const String& name, float lo, float hi, float def, float offLow, float offHigh)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.0f, 0.25f), def,
                   AudioParameterFloatAttributes().withLabel ("Hz")
                       .withStringFromValueFunction ([offLow, offHigh] (float v, int)
                       {
                           if ((offLow > 0.0f && v <= offLow + 0.05f) || (offHigh > 0.0f && v >= offHigh - 10.0f)) return String ("Off");
                           return hzText (v);
                       })
                       .withValueFromStringFunction ([offLow, offHigh] (const String& t)
                       {
                           if (t.trim().equalsIgnoreCase ("off")) return offLow > 0.0f ? offLow : offHigh;
                           return hzFromText (t);
                       }));
    };
    auto dbParam = [] (const String& id, const String& name, float lo, float hi, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.1f), def,
                   AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return dbText (v); }));
    };
    auto msParam = [] (const String& id, const String& name, float lo, float hi, float def, float skew)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.0f, skew), def,
                   AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction ([] (float v, int) { return msText (v); }));
    };
    auto offDbParam = [] (const String& id, const String& name, float hi, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (Params::offDb, hi, 0.1f, 2.5f), def,
                   AudioParameterFloatAttributes().withLabel ("dB")
                       .withStringFromValueFunction ([] (float v, int) { return v <= Params::offDb + 0.05f ? String ("Off") : dbText (v); })
                       .withValueFromStringFunction ([] (const String& t) { return t.trim().equalsIgnoreCase ("off") ? Params::offDb : t.getFloatValue(); }));
    };
    auto choice = [] (const String& id, const String& name, const StringArray& items, int def)
    { return std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def); };
    auto toggle = [] (const String& id, const String& name, bool def)
    { return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def); };

    layout.add (dbParam (input, "Input", -24.0f, 24.0f, 0.0f));
    layout.add (dbParam (output, "Output", -24.0f, 12.0f, 0.0f));

    // Carrier
    layout.add (choice (carSrc, "Source", Tables::sources, 2));
    layout.add (choice (carPitch, "Pitch", Tables::pitchModes, 1));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { carNote, 1 }, "Note", 24, 72, 36,
                    AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return Tables::noteName (v); })));
    layout.add (msParam (carGlide, "Glide", 0.0f, 500.0f, 25.0f, 0.4f));
    layout.add (pct (carDetune, "Detune", 35.0f));
    layout.add (pct (carSpread, "Spread", 60.0f));
    layout.add (pct (carNoise, "Noise", 10.0f));

    // Vocoder
    layout.add (toggle (vocOn, "Vocoder On", true));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { vocBands, 1 }, "Bands", 4, 100, 28));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { vocFormant, 1 }, "Formant", NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return (v > 0.04f ? "+" : "") + String (v, 1) + " st"; })));
    layout.add (msParam (vocAttack, "Attack", 0.1f, 50.0f, 2.0f, 0.35f));
    layout.add (msParam (vocRelease, "Release", 5.0f, 500.0f, 50.0f, 0.4f));
    layout.add (pct (vocSib, "Sibilance", 40.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { vocGate, 1 }, "Gate", NormalisableRange<float> (-80.0f, 0.0f, 0.1f), -60.0f,
                    AudioParameterFloatAttributes().withLabel ("dB")
                        .withStringFromValueFunction ([] (float v, int) { return v <= -79.9f ? String ("Off") : String (roundToInt (v)) + " dB"; })));

    // Harmonize
    layout.add (toggle (harmOn, "Harmonize On", true));
    for (int i = 0; i < Tables::numIntervals; ++i)
    {
        const String n = String ("Harm ") + Tables::intervalNames[i];
        layout.add (toggle (ivOn (i), n + " On", false));
        layout.add (dbParam (ivDb (i), n + " Level", -36.0f, 6.0f, Tables::intervalDefaultDb[i]));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ivPan (i), 1 }, n + " Pan", NormalisableRange<float> (-1.0f, 1.0f, 0.01f),
                        Tables::intervalDefaultPan[i],
                        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                        {
                            const int p = roundToInt (std::abs (v) * 100.0f);
                            return p == 0 ? String ("C") : String (p) + (v < 0 ? " L" : " R");
                        })));
    }
    layout.add (toggle (harmScale, "Scale On", true));
    layout.add (choice (harmRoot, "Root", Tables::roots, 0));
    layout.add (choice (harmMode, "Scale", Tables::scaleNames(), 1));
    layout.add (choice (harmPan, "Pan Mode", Tables::panModes, 0));
    layout.add (offDbParam (harmDry, "Harm Dry", 6.0f, 0.0f));
    layout.add (offDbParam (harmWet, "Harm Wet", 6.0f, -2.0f));
    layout.add (pct (harmDetune, "Harm Detune", 20.0f));
    layout.add (pct (harmSpread, "Harm Spread", 30.0f));

    // Dirt
    layout.add (toggle (dirtOn, "Dirt On", true));
    layout.add (choice (dirtMode, "Dirt Mode", Tables::dirtModes, 2));
    layout.add (pct (dirtDrive, "Drive", 30.0f));
    layout.add (pct (dirtCrunch, "Crunch", 35.0f));
    layout.add (pct (dirtDestroy, "Destroy", 0.0f));

    // Comp
    layout.add (toggle (compOn, "Comp On", false));
    layout.add (choice (compMode, "Comp Mode", Tables::compModes, 0));
    layout.add (pct (ottDepth, "OTT Depth", 50.0f));
    layout.add (pct (ottTime, "OTT Time", 50.0f));
    layout.add (pct (ottUp, "OTT Up", 100.0f));
    layout.add (pct (ottDown, "OTT Down", 100.0f));
    layout.add (dbParam (glueThresh, "Glue Threshold", -40.0f, 0.0f, -12.0f));
    layout.add (choice (glueRatio, "Glue Ratio", Tables::glueRatios, 1));
    layout.add (msParam (glueAttack, "Glue Attack", 0.1f, 30.0f, 3.0f, 0.35f));
    layout.add (msParam (glueRelease, "Glue Release", 50.0f, 1200.0f, 300.0f, 0.5f));
    layout.add (pct (compMix, "Comp Mix", 100.0f));

    // Width
    layout.add (toggle (widthOn, "Width On", true));
    layout.add (choice (widthMode, "Width Mode", Tables::widthModes, 0));
    layout.add (pct (widthAmt, "Width", 50.0f));
    layout.add (hzParam (widthMono, "Mono Bass", 20.0f, 500.0f, 120.0f, 0.0f, 0.0f));

    // Mix / clip
    layout.add (offDbParam (dryGain, "Dry Gain", 6.0f, Params::offDb));
    layout.add (offDbParam (wetGain, "Wet Gain", 6.0f, 0.0f));
    layout.add (toggle (autoGain, "Auto Gain", true));
    layout.add (toggle (clipOn, "Clip On", true));
    layout.add (choice (clipMode, "Clip Mode", Tables::clipModes, 0));
    layout.add (dbParam (ceiling, "Ceiling", -12.0f, 0.0f, -0.3f));

    // Pre / post filters
    for (const bool post : { false, true })
    {
        const String n = post ? "Post " : "Pre ";
        layout.add (toggle (filterId (post, fOn), n + "EQ On", false));
        layout.add (choice (filterId (post, fTarget), n + "EQ Target", Tables::fxTargets, 0));
        layout.add (hzParam (filterId (post, fLc), n + "Low Cut", 20.0f, 20000.0f, 20.0f, 20.0f, 0.0f));
        layout.add (hzParam (filterId (post, fHc), n + "High Cut", 20.0f, 20000.0f, 20000.0f, 0.0f, 20000.0f));
        layout.add (choice (filterId (post, fLcSlope), n + "Low Cut Slope", Tables::slopes, 1));
        layout.add (choice (filterId (post, fHcSlope), n + "High Cut Slope", Tables::slopes, 1));
        layout.add (hzParam (filterId (post, fPkFreq), n + "Peak Frequency", 20.0f, 20000.0f, 1000.0f, 0.0f, 0.0f));
        layout.add (dbParam (filterId (post, fPkGain), n + "Peak Gain", -18.0f, 18.0f, 0.0f));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { filterId (post, fPkQ), 1 }, n + "Peak Width",
                        NormalisableRange<float> (0.3f, 10.0f, 0.0f, 0.4f), 1.0f,
                        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return "Q " + String (v, 2); })));
    }
    return layout;
}

void Params::upgradeState (ValueTree& tree)
{
    auto find = [&tree] (const char* id) { return tree.getChildWithProperty ("id", id); };
    auto add = [&tree] (const char* id, float v)
    {
        ValueTree c ("PARAM");
        c.setProperty ("id", id, nullptr);
        c.setProperty ("value", v, nullptr);
        tree.appendChild (c, nullptr);
    };
    auto toDb = [] (float g) { return g < 0.001f ? Params::offDb : jmax (Params::offDb, Decibels::gainToDecibels (g)); };

    auto mix = find (ParamIDs::oldMix);
    if (mix.isValid() && ! find (ParamIDs::dryGain).isValid())
    {
        const float m = jlimit (0.0f, 1.0f, (float) mix.getProperty ("value") * 0.01f);
        add (ParamIDs::dryGain, toDb (1.0f - m));
        add (ParamIDs::wetGain, toDb (m));
    }
    if (mix.isValid()) tree.removeChild (mix, nullptr);

    auto lvl = find ("harm_level");
    if (lvl.isValid() && ! find (ParamIDs::harmWet).isValid())
        add (ParamIDs::harmWet, toDb ((float) lvl.getProperty ("value") * 0.01f));
    if (lvl.isValid()) tree.removeChild (lvl, nullptr);
}

//==============================================================================
namespace
{
    constexpr float uni7[7] = { -1.f, -0.62f, -0.28f, 0.f, 0.28f, 0.62f, 1.f };
    constexpr float uni3[3] = { -1.f, 0.f, 1.f };

    inline void panGains (float pos, float& l, float& r)   // -1..1, centre = unity in both
    {
        const float a = (jlimit (-1.f, 1.f, pos) + 1.f) * (vf::pi * 0.25f);
        l = std::min (1.f, std::cos (a) * 1.41421356f);
        r = std::min (1.f, std::sin (a) * 1.41421356f);
    }
}

VoiceFuckerProcessor::VoiceFuckerProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", AudioChannelSet::stereo(), true)
                          .withOutput ("Output", AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", Params::createLayout())
{
    using namespace ParamIDs;
    pInGain = raw (input); pOutGain = raw (output);
    pSrc = raw (carSrc); pPitch = raw (carPitch); pNote = raw (carNote); pGlide = raw (carGlide);
    pDetune = raw (carDetune); pSpread = raw (carSpread); pNoise = raw (carNoise);
    pVocOn = raw (vocOn); pBands = raw (vocBands); pFormant = raw (vocFormant); pAttack = raw (vocAttack);
    pRelease = raw (vocRelease); pSib = raw (vocSib); pGate = raw (vocGate);
    pHarmOn = raw (harmOn); pScale = raw (harmScale); pRoot = raw (harmRoot); pMode = raw (harmMode); pPan = raw (harmPan);
    pHarmDry = raw (harmDry); pHarmWet = raw (harmWet); pHarmDetune = raw (harmDetune); pHarmSpread = raw (harmSpread);
    pDirtOn = raw (dirtOn); pDirtMode = raw (dirtMode); pDrive = raw (dirtDrive); pCrunch = raw (dirtCrunch); pDestroy = raw (dirtDestroy);
    pCompOn = raw (compOn); pCompMode = raw (compMode); pOttDepth = raw (ottDepth); pOttTime = raw (ottTime);
    pOttUp = raw (ottUp); pOttDown = raw (ottDown); pGlueThresh = raw (glueThresh); pGlueRatio = raw (glueRatio);
    pGlueAttack = raw (glueAttack); pGlueRelease = raw (glueRelease); pCompMix = raw (compMix);
    pWidthOn = raw (widthOn); pWidthMode = raw (widthMode); pWidth = raw (widthAmt); pMono = raw (widthMono);
    pDryGain = raw (dryGain); pWetGain = raw (wetGain); pAutoGain = raw (ParamIDs::autoGain); pClipOn = raw (clipOn); pClipMode = raw (clipMode); pCeiling = raw (ceiling);
    for (int i = 0; i < Tables::numIntervals; ++i)
    {
        pIvOn[(size_t) i] = raw (ivOn (i)); pIvDb[(size_t) i] = raw (ivDb (i)); pIvPan[(size_t) i] = raw (ivPan (i));
        uiInterval[(size_t) i].store (Tables::intervals[i]);
    }
    bindFilter (preP, false);
    bindFilter (postP, true);
    for (auto& b : bandLevel) b.store (0.f);
    for (auto& n : uiNotes) n.store (-1);
    for (auto& v : uiOttGain) v.store (0.f);
    for (auto& v : uiOttLevel) v.store (-100.f);
}

void VoiceFuckerProcessor::bindFilter (FilterParams& f, bool post)
{
    using namespace ParamIDs;
    f.on = raw (filterId (post, fOn)); f.lc = raw (filterId (post, fLc)); f.hc = raw (filterId (post, fHc));
    f.lcSlope = raw (filterId (post, fLcSlope)); f.hcSlope = raw (filterId (post, fHcSlope));
    f.pkF = raw (filterId (post, fPkFreq)); f.pkG = raw (filterId (post, fPkGain)); f.pkQ = raw (filterId (post, fPkQ));
    f.target = raw (filterId (post, fTarget));
}

bool VoiceFuckerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != AudioChannelSet::stereo() && out != AudioChannelSet::mono()) return false;
    const auto in = layouts.getMainInputChannelSet();
    if (in != AudioChannelSet::stereo() && in != AudioChannelSet::mono()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != AudioChannelSet::stereo() && sc != AudioChannelSet::mono()) return false;
    }
    return true;
}

//==============================================================================
void VoiceFuckerProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    currentSampleRate = sampleRate;
    pitch.prepare (sampleRate);
    shifter.prepare (sampleRate);
    curBands = -1;
    for (auto& b : bands) b = Band();
    for (auto& v : voices) v = Voice();
    lastLogFreq = -1;
    sib1.setHighPass ((float) sr, 4500.f); sib2 = sib1;
    dlL.prepare ((int) (sr * 0.05)); dlR.prepare ((int) (sr * 0.05));
    sideHp1.reset(); sideHp2.reset(); curMonoHz = -1;
    gateDet = 0; gateGain = 1; gateOpen = true;
    rmsDry = rmsWet = 0; autoGain = 1;
    holdL = holdR = holdPhase = 0;
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0;
    for (int i = 0; i < numWideBands; ++i)
    {
        const float fc = 150.f * std::pow (2.f, (float) i * std::log2 (9000.f / 150.f) / (float) (numWideBands - 1));
        wide[i].b1.setBandPass ((float) sr, std::min (fc, (float) sr * 0.45f), 1.6f);
        wide[i].b2 = wide[i].b1;
        wide[i].b1.reset(); wide[i].b2.reset();
    }
    for (int i = 0; i < numCrunchBands; ++i)   // Crunch: each band rips on its own
    {
        const float fc = 160.f * std::pow (2.f, (float) i * std::log2 (7000.f / 160.f) / (float) (numCrunchBands - 1));
        auto& b = crunchBands[i];
        b.l1.setBandPass ((float) sr, std::min (fc, (float) sr * 0.45f), 1.1f);
        b.l1.reset(); b.l2 = b.r1 = b.r2 = b.l1;
    }

    const dsp::ProcessSpec spec { sampleRate, (uint32) jmax (1, samplesPerBlock), 2 };
    auto prepFilter = [&] (cdsp::FilterBank& f, const FilterParams& fp)
    { f.prepare (spec, fp.lc->load(), fp.hc->load(), fp.pkF->load(), fp.pkG->load(), fp.pkQ->load()); };
    prepFilter (preFilter, preP);
    prepFilter (postFilter, postP);
    ott.prepare (spec);
    glue.reset();
    agComp.prepare (sr, 0.8, 0.3, 0.3, 10.0f);

    auto prep = [&] (SmoothedValue<float>& s, float v, double t = 0.03) { s.reset (sampleRate, t); s.setCurrentAndTargetValue (v); };
    prep (sInGain, dbToGain (*pInGain)); prep (sOutGain, dbToGain (*pOutGain));
    prep (sDirtOn, *pDirtOn > 0.5f); prep (sWidthOn, *pWidthOn > 0.5f); prep (sClipOn, *pClipOn > 0.5f);
    prep (sVocOn, *pVocOn > 0.5f); prep (sCompOn, *pCompOn > 0.5f);
    prep (sPreOn, *preP.on > 0.5f); prep (sPostOn, *postP.on > 0.5f);
    prep (sDryGain, Params::offDbToGain (*pDryGain)); prep (sWetGain, Params::offDbToGain (*pWetGain));
    prep (sPreAll, *preP.target > 0.5f); prep (sPostAll, *postP.target > 0.5f); prep (sDrive, *pDrive * 0.01f); prep (sCrunch, *pCrunch * 0.01f);
    prep (sDestroy, *pDestroy * 0.01f); prep (sWidth, *pWidth * 0.01f); prep (sHaas, 0.f, 0.08);
    prep (sHarmDry, Params::offDbToGain (*pHarmDry)); prep (sHarmWet, Params::offDbToGain (*pHarmWet)); prep (sCeiling, dbToGain (*pCeiling));
    prep (sCompMix, *pCompMix * 0.01f); prep (sOttDepth, *pOttDepth * 0.01f);

    scBuffer.setSize (2, jmax (1, samplesPerBlock));
    bucketLen = jmax (1, roundToInt (sr * 0.001));
    bucketPos = 0;
    bucketHi = bucketLo = bucketIn = bucketPre = 0;
}

void VoiceFuckerProcessor::updateBands (int n, float formant)
{
    n = jlimit (4, maxBands, n);
    const float fs = (float) sr;
    const float fLo = 90.f, fHi = std::min (11000.f, fs * 0.42f);
    const float bwOct = std::log2 (fHi / fLo) / (float) n;
    const float r = std::pow (2.f, bwOct);
    const float q = jlimit (0.5f, 40.f, std::sqrt (r) / (r - 1.f) * 0.8f);
    const float shift = std::pow (2.f, formant / 12.f);
    auto keep = [] (Biquad& dst, const Biquad& src) { const float z1 = dst.z1, z2 = dst.z2; dst = src; dst.z1 = z1; dst.z2 = z2; };

    for (int i = 0; i < n; ++i)
    {
        auto& b = bands[(size_t) i];
        const float fc = fLo * std::pow (2.f, bwOct * ((float) i + 0.5f));
        Biquad m; m.setBandPass (fs, fc, q);
        keep (b.m1, m); keep (b.m2, m);
        const float fcC = fc * shift;
        b.enabled = fcC < fs * 0.45f && fcC > 25.f;
        Biquad c; c.setBandPass (fs, jlimit (25.f, fs * 0.45f, fcC), q);
        keep (b.cl1, c); keep (b.cl2, c); keep (b.cr1, c); keep (b.cr2, c);
        keep (b.hl1, c); keep (b.hl2, c); keep (b.hr1, c); keep (b.hr2, c);
    }
    if (n != curBands)
        for (int i = curBands < 0 ? 0 : std::min (curBands, n); i < maxBands; ++i)
        {
            auto& b = bands[(size_t) i];
            for (auto* f : { &b.m1, &b.m2, &b.cl1, &b.cl2, &b.cr1, &b.cr2, &b.hl1, &b.hl2, &b.hr1, &b.hr2 }) f->reset();
            b.envM = b.envCL = b.envCR = b.envHL = b.envHR = 0;
        }
    curBands = n;
    curFormant = formant;
    numBandsUi = n;
}

void VoiceFuckerProcessor::computeHarmonyLayout (int vi, float baseNote)
{
    auto& v = voices[(size_t) vi];
    const bool useScale = *pScale > 0.5f;
    const int root = (int) *pRoot, mode = (int) *pMode;
    ignoreUnused (baseNote);
    for (int j = 0; j < Tables::numIntervals; ++j)
        v.harmRatio[j] = std::pow (2.f, (float) Tables::snapInterval (Tables::intervals[j], useScale, root, mode) / 12.f);
}

float VoiceFuckerProcessor::harmPanFor (int j, int panMode, float spread) const
{
    if (panMode != 0) return 0.f;
    const float p = jlimit (-1.f, 1.f, pIvPan[(size_t) j]->load());
    if (std::abs (p) < 0.001f) return 0.f;
    return p + ((p > 0 ? 1.f : -1.f) - p) * spread;   // Spread pushes every voice towards its own side
}

void VoiceFuckerProcessor::handleMidi (const MidiMessage& m)
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
void VoiceFuckerProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const float fs = (float) sr;
    auto mainIn = getBusBuffer (buffer, true, 0);
    auto out = getBusBuffer (buffer, false, 0);
    const int inCh = mainIn.getNumChannels(), outCh = out.getNumChannels();
    if (n == 0 || outCh == 0 || inCh == 0) return;

    // sidechain copy
    bool haveSc = false;
    if (getBusCount (true) > 1 && getBus (true, 1) != nullptr && getBus (true, 1)->isEnabled())
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0)
        {
            if (scBuffer.getNumSamples() < n) scBuffer.setSize (2, n, false, false, true);
            scBuffer.copyFrom (0, 0, sc, 0, 0, n);
            scBuffer.copyFrom (1, 0, sc, sc.getNumChannels() > 1 ? 1 : 0, 0, n);
            haveSc = true;
        }
    }

    // tempo for the scope grid
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying()) if (auto ppq = pos->getPpqPosition()) beatClock = *ppq;
        }
    scope.beatCount.store (scope.writeCount.load (std::memory_order_relaxed), std::memory_order_relaxed);
    scope.beatAt.store (beatClock - (double) bucketPos / sr * bpm / 60.0, std::memory_order_relaxed);
    scope.bpm.store (bpm, std::memory_order_relaxed);
    beatClock += (double) n / sr * bpm / 60.0;

    // ---------------- block parameters
    const int src = (int) *pSrc;
    const int pitchMode = (int) *pPitch;
    const bool synthOn = src <= 3 || src == 5;
    const bool scOn = (src == 4 || src == 5) && haveSc;
    const int osc = src == 5 ? 2 : src;
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
    int hList[Tables::numIntervals]; int nH = 0;
    float hGain[Tables::numIntervals] {}, hL[Tables::numIntervals] {}, hR[Tables::numIntervals] {};
    for (int j = 0; j < Tables::numIntervals; ++j)
        if (harmOn && *pIvOn[(size_t) j] > 0.5f) hList[nH++] = j;
    for (int k = 0; k < nH; ++k)
    {
        const int j = hList[k];
        hGain[j] = dbToGain (*pIvDb[(size_t) j]);
        const float pos = panMode == 0 ? harmPanFor (j, 0, hSpread) : (k % 2 ? 1.f : -1.f) * hSpread;
        panGains (pos, hL[j], hR[j]);
    }
    const bool hSpectral = panMode == 1 && nH > 0;
    const float hDet[3] = { std::pow (2.f, -hDetCents / 1200.f), 1.f, std::pow (2.f, hDetCents / 1200.f) };
    const int hWave = osc == 3 ? 0 : osc;

    if (pitchMode != lastPitchMode)
    {
        for (auto& v : voices) v.active = false;
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

    // harmonies of the raw voice (vocoder off) follow the detected note
    float rawRatio[Tables::numIntervals];
    {
        const bool useScale = *pScale > 0.5f;
        for (int j = 0; j < Tables::numIntervals; ++j)
            rawRatio[j] = std::pow (2.f, (float) Tables::snapInterval (Tables::intervals[j], useScale, (int) *pRoot, (int) *pMode) / 12.f);
        for (auto& v : voices)
            for (int j = 0; j < Tables::numIntervals; ++j) v.harmRatio[j] = rawRatio[j];
    }
    shifter.setWindowFromPeriod (detectedPeriod);

    // vocoder settings
    const int nb = (int) *pBands;
    const float formant = *pFormant;
    if (nb != curBands || std::abs (formant - curFormant) > 0.005f) updateBands (nb, formant);
    const float atk = onePoleCoef (*pAttack, fs), rel = onePoleCoef (*pRelease, fs);
    const float cAtk = onePoleCoef (0.6f, fs), cRel = onePoleCoef (25.f, fs);
    const float sibAmt = *pSib * 0.01f;
    const float gateThr = *pGate <= -79.9f ? -1.f : dbToGain (*pGate);
    const float gAtk = onePoleCoef (1.f, fs), gRel = onePoleCoef (60.f, fs), detRel = onePoleCoef (40.f, fs);

    const int dirtMode = (int) *pDirtMode;
    const int widthMode = (int) *pWidthMode;
    const bool clipHard = *pClipMode > 0.5f;
    const bool autoGainOn = *pAutoGain > 0.5f;
    const int compModeV = (int) *pCompMode;
    const float glueThr = *pGlueThresh;
    const float glueRat = Tables::glueRatioValues[jlimit (0, 2, (int) *pGlueRatio)];
    const float ottUpV = *pOttUp * 0.01f, ottDownV = *pOttDown * 0.01f;
    ott.setTime (sr, *pOttTime * 0.01f);
    glue.setup (sr, *pGlueAttack, *pGlueRelease);

    const float monoHz = *pMono;
    if (std::abs (monoHz - curMonoHz) > 0.5f)
    {
        sideHp1.setHighPass (fs, monoHz, 0.7071f);
        { const float z1 = sideHp2.z1, z2 = sideHp2.z2; sideHp2 = sideHp1; sideHp2.z1 = z1; sideHp2.z2 = z2; }
        curMonoHz = monoHz;
    }
    preFilter.setTargets (*preP.lc, *preP.hc, (int) *preP.lcSlope, (int) *preP.hcSlope, *preP.pkF, *preP.pkG, *preP.pkQ);
    postFilter.setTargets (*postP.lc, *postP.hc, (int) *postP.lcSlope, (int) *postP.hcSlope, *postP.pkF, *postP.pkG, *postP.pkQ);

    sInGain.setTargetValue (dbToGain (*pInGain)); sOutGain.setTargetValue (dbToGain (*pOutGain));
    sDirtOn.setTargetValue (*pDirtOn > 0.5f ? 1.f : 0.f); sWidthOn.setTargetValue (*pWidthOn > 0.5f ? 1.f : 0.f);
    sClipOn.setTargetValue (*pClipOn > 0.5f ? 1.f : 0.f); sVocOn.setTargetValue (*pVocOn > 0.5f ? 1.f : 0.f);
    sCompOn.setTargetValue (*pCompOn > 0.5f ? 1.f : 0.f);
    sPreOn.setTargetValue (*preP.on > 0.5f ? 1.f : 0.f); sPostOn.setTargetValue (*postP.on > 0.5f ? 1.f : 0.f);
    sDryGain.setTargetValue (Params::offDbToGain (*pDryGain)); sWetGain.setTargetValue (Params::offDbToGain (*pWetGain));
    sPreAll.setTargetValue (*preP.target > 0.5f ? 1.f : 0.f); sPostAll.setTargetValue (*postP.target > 0.5f ? 1.f : 0.f); sDrive.setTargetValue (*pDrive * 0.01f); sCrunch.setTargetValue (*pCrunch * 0.01f);
    sDestroy.setTargetValue (*pDestroy * 0.01f); sWidth.setTargetValue (*pWidth * 0.01f);
    sHaas.setTargetValue (widthMode == 1 ? 1.f : 0.f);
    sHarmDry.setTargetValue (harmOn ? Params::offDbToGain (*pHarmDry) : 1.f);
    sHarmWet.setTargetValue (Params::offDbToGain (*pHarmWet)); sCeiling.setTargetValue (dbToGain (*pCeiling));
    sCompMix.setTargetValue (*pCompMix * 0.01f); sOttDepth.setTargetValue (*pOttDepth * 0.01f);

    const float vAtk = onePoleCoef (3.f, fs), vRel = onePoleCoef (70.f, fs);
    const float rmsCoef = onePoleCoef (300.f, fs), agCoef = onePoleCoef (80.f, fs);
    const float dcR = 1.f - 2.f * vf::pi * 12.f / fs;

    float peakIn[2] = { 0, 0 }, peakOut[2] = { 0, 0 };
    float glueGrMin = 0, glueInMax = -100, ottG[3] {}, ottLvl[3] = { -100, -100, -100 };
    std::array<float, maxBands> bandPk {};

    auto midiIt = midi.cbegin();
    const auto midiEnd = midi.cend();
    const float* inL = mainIn.getReadPointer (0);
    const float* inR = mainIn.getReadPointer (inCh > 1 ? 1 : 0);
    float* oL = out.getWritePointer (0);
    float* oR = out.getWritePointer (outCh > 1 ? 1 : 0);
    int specPos = spectrum.writePos.load (std::memory_order_relaxed);

    for (int s = 0; s < n; ++s)
    {
        while (midiIt != midiEnd && (*midiIt).samplePosition <= s)
        {
            if (pitchMode == 0) handleMidi ((*midiIt).getMessage());
            ++midiIt;
        }

        const float gIn = sInGain.getNextValue();
        const float xL = inL[s] * gIn, xR = inR[s] * gIn;
        peakIn[0] = std::max (peakIn[0], std::abs (xL));
        peakIn[1] = std::max (peakIn[1], std::abs (xR));

        // ---- pitch detection always runs on the clean voice
        if (pitch.push (0.5f * (xL + xR)))
        {
            if (pitch.freq > 50.f && pitch.freq < 1100.f && pitch.confidence > 0.6f)
            {
                detectedHz = pitch.freq;
                detectedPeriod = 1.f / pitch.freq;
                if (pitchMode == 1)
                {
                    autoLogTarget = std::log2 (pitch.freq);
                    voices[0].logTarget = autoLogTarget;
                    computeHarmonyLayout (0, hzToMidi (pitch.freq));
                }
            }
        }

        // ---- Pre EQ. FX: only what goes into the processing. All: the clean voice too
        float eL = xL, eR = xR;
        {
            const float pre = sPreOn.getNextValue();
            preFilter.tick();
            if (pre > 0.0001f)
            {
                const float fl = preFilter.process (0, xL), fr = preFilter.process (1, xR);
                eL += (fl - eL) * pre; eR += (fr - eR) * pre;
            }
        }
        const float preAll = sPreAll.getNextValue();
        const float dryL = xL + (eL - xL) * preAll, dryR = xR + (eR - xR) * preAll;   // the clean voice for Dry Gain
        const float mod = 0.5f * (eL + eR);
        shifter.push (mod);

        const float dOn = sDirtOn.getNextValue();
        const float drive = sDrive.getNextValue() * dOn;
        const float crunch = sCrunch.getNextValue() * dOn;
        const float destroy = sDestroy.getNextValue() * dOn;
        const float wOn = sWidthOn.getNextValue();
        const float width = sWidth.getNextValue() * wOn;
        const float vocOn = sVocOn.getNextValue();
        const float hDry = sHarmDry.getNextValue();   // what came into Harmonize
        const float hWet = sHarmWet.getNextValue();   // the harmony voices

        // ---- carrier (main) and the harmony carrier (its own channels, so Harmonize can balance them)
        float cL = 0, cR = 0, hcL = 0, hcR = 0;
        if (synthOn && vocOn > 0.0001f)
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
                        const float g = hGain[j] * amp * 0.577f;
                        float sum = 0;
                        for (int u = 0; u < 3; ++u)
                        {
                            const float dt = std::min (0.45f, fh * hDet[u] / fs);
                            float& ph = v.hPhase[j][u];
                            sum += hWave == 1 ? squareBlep (ph, dt) : sawBlep (ph, dt);
                            ph += dt; if (ph >= 1.f) ph -= 1.f;
                        }
                        if (hSpectral) hcL += sum * g;
                        else { hcL += sum * g * hL[j]; hcR += sum * g * hR[j]; }
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

        // ---- vocoder: main voice (v) and harmonies (h) come out separately
        float vL = 0, vR = 0, hvL = 0, hvR = 0;
        const bool haveHarm = nH > 0 && hWet > 0.00001f;
        if (vocOn > 0.0001f)
        {
            const float w = widthMode == 0 ? width : 0.f;
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
                const float gl = (i % 2 == 0) ? 1.f : 1.f - w, gr = (i % 2 == 0) ? 1.f - w : 1.f;
                vL += bl * b.envM / (b.envCL + 1.0e-4f) * gl;
                vR += br * b.envM / (b.envCR + 1.0e-4f) * gr;

                if (haveHarm)
                {
                    // harmonies keep their level next to the main carrier, as if they were mixed into it
                    const float hl = b.hl2.process (b.hl1.process (hcL));
                    const float ahl = std::abs (hl);
                    b.envHL = ahl + (b.envHL - ahl) * (ahl > b.envHL ? cAtk : cRel);
                    const float refL = std::max (b.envCL, b.envHL) + 1.0e-4f;
                    if (hSpectral)
                    {
                        const float yh = hl * b.envM / refL;
                        const float side = (i % 2 ? 1.f : -1.f) * hSpread;
                        hvL += yh * (1.f - std::max (0.f, side));
                        hvR += yh * (1.f + std::min (0.f, side));
                    }
                    else
                    {
                        const float hr = b.hr2.process (b.hr1.process (hcR));
                        const float ahr = std::abs (hr);
                        b.envHR = ahr + (b.envHR - ahr) * (ahr > b.envHR ? cAtk : cRel);
                        const float refR = std::max (b.envCR, b.envHR) + 1.0e-4f;
                        hvL += hl * b.envM / refL * gl;
                        hvR += hr * b.envM / refR * gr;
                    }
                }
                bandPk[(size_t) i] = std::max (bandPk[(size_t) i], b.envM);
            }
            const float sib = sib2.process (sib1.process (mod)) * sibAmt * 1.6f;
            vL += sib; vR += sib;
        }

        // gate
        {
            const float am = std::abs (mod);
            gateDet = am > gateDet ? am : am + (gateDet - am) * detRel;
            if (gateThr < 0) gateOpen = true;
            else if (gateOpen && gateDet < gateThr * 0.7f) gateOpen = false;
            else if (! gateOpen && gateDet > gateThr) gateOpen = true;
            gateGain = (gateOpen ? 1.f : 0.f) + (gateGain - (gateOpen ? 1.f : 0.f)) * (gateOpen ? gAtk : gRel);
        }

        // ---- Harmonize: Dry = what came in (robot or voice), Wet = the harmony voices
        const float vg = vocOn * gateGain;
        float wL = (vL * hDry + hvL * hWet) * vg, wR = (vR * hDry + hvR * hWet) * vg;
        if (vocOn < 0.9999f)
        {
            float rl = eL * hDry, rr = eR * hDry;
            for (int k = 0; k < nH; ++k)
            {
                const int j = hList[k];
                const float h = shifter.read (j, rawRatio[j]) * hGain[j] * hWet;
                rl += h * hL[j]; rr += h * hR[j];
            }
            wL += rl * (1.f - vocOn); wR += rr * (1.f - vocOn);
        }
        else
        {
            for (int k = 0; k < nH; ++k) shifter.read (hList[k], rawRatio[hList[k]]);   // keep grains running
        }

        // DC blocker
        {
            const float yl = wL - dcX[0] + dcR * dcY[0]; dcX[0] = wL; dcY[0] = yl; wL = yl;
            const float yr = wR - dcX[1] + dcR * dcY[1]; dcX[1] = wR; dcY[1] = yr; wR = yr;
        }

        // ---- Dirt: everything that came out of Harmonize. Drive warms, Crunch rips each band, Destroy crushes
        if (drive > 0.001f)
        {
            const float k = dbToGain (drive * 30.f), m2 = std::min (1.f, drive * 5.f), norm = 1.4f / std::sqrt (k);
            wL += (shape (dirtMode, wL * k) * norm - wL) * m2;
            wR += (shape (dirtMode, wR * k) * norm - wR) * m2;
        }
        if (crunch > 0.001f)
        {
            const float k = dbToGain (crunch * 34.f), m2 = std::min (1.f, crunch * 5.f), norm = 1.2f / std::sqrt (k);
            float addL = 0, addR = 0;
            for (auto& b : crunchBands)
            {
                const float bl = b.l2.process (b.l1.process (wL)), br = b.r2.process (b.r1.process (wR));
                addL += shape (dirtMode, bl * k) * norm - bl;
                addR += shape (dirtMode, br * k) * norm - br;
            }
            wL += addL * m2; wR += addR * m2;
        }
        else
        {
            for (auto& b : crunchBands) { b.l2.process (b.l1.process (wL)); b.r2.process (b.r1.process (wR)); }
        }
        if (destroy > 0.001f)
        {
            holdPhase += 1.f / (1.f + destroy * destroy * 30.f);
            if (holdPhase >= 1.f)
            {
                holdPhase -= 1.f;
                const float q = std::exp2 (16.f - destroy * 12.5f - 1.f);
                holdL = std::round (wL * q) / q; holdR = std::round (wR * q) / q;
            }
            const float m3 = std::min (1.f, destroy * 4.f);
            wL += (holdL - wL) * m3; wR += (holdR - wR) * m3;
        }

        // ---- Comp
        {
            const float cOn = sCompOn.getNextValue();
            const float cMix = sCompMix.getNextValue();
            const float depth = sOttDepth.getNextValue();
            if (cOn > 0.0001f)
            {
                float y[2] = { wL, wR };
                if (compModeV == 0) { y[0] = ott.process (0, wL, depth, ottUpV, ottDownV); y[1] = ott.process (1, wR, depth, ottUpV, ottDownV); }
                else glue.process (y, 2, glueThr, glueRat);
                if (compModeV == 0)
                    for (int b = 0; b < 3; ++b)
                    {
                        const float gdb = 0.5f * (ott.gdb[0][b] + ott.gdb[1][b]) * depth;
                        if (std::abs (gdb) > std::abs (ottG[b])) ottG[b] = gdb;
                        ottLvl[b] = std::max (ottLvl[b], cdsp::gainToDb (std::max (ott.env[0][b], ott.env[1][b])));
                    }
                else
                {
                    glueGrMin = std::min (glueGrMin, glue.grDb);
                    glueInMax = std::max (glueInMax, cdsp::gainToDb (glue.env));
                }
                agComp.measure (0, wL, y[0]); agComp.measure (1, wR, y[1]);
                agComp.update (true, 2);
                wL += (y[0] * agComp.gain - wL) * cMix * cOn;
                wR += (y[1] * agComp.gain - wR) * cMix * cOn;
            }
        }

        // ---- Width
        dlL.push (wL); dlR.push (wR);
        const float haas = sHaas.getNextValue();
        if (wOn > 0.0001f)
        {
            if (widthMode == 1 || haas > 0.0001f)
            {
                const float d = 1.f + haas * width * 0.025f * fs;
                const float dr = dlR.read (d);
                wR += (dr - wR) * haas * wOn;
            }
            if (widthMode == 2)
            {
                chorusPhase += 0.55f / fs; if (chorusPhase >= 1.f) chorusPhase -= 1.f;
                const float a = twoPi * chorusPhase;
                const float chL = dlL.read ((0.007f + 0.0025f * std::sin (a)) * fs), chR = dlR.read ((0.009f + 0.0025f * std::sin (a + 1.7f)) * fs);
                wL = wL * (1.f - 0.35f * width) + chR * 0.8f * width;
                wR = wR * (1.f - 0.35f * width) + chL * 0.8f * width;
            }
            const float mid = 0.5f * (wL + wR);
            float side = 0.5f * (wL - wR);
            if (widthMode == 0)
            {
                side *= 1.f + 0.6f * width;
                // without the vocoder there are no bands to spread, so split the voice here
                const float rawPart = 1.f - vocOn;
                if (rawPart > 0.0001f && width > 0.0001f)
                {
                    float extra = 0;
                    for (int i = 0; i < numWideBands; ++i)
                        extra += wide[i].b2.process (wide[i].b1.process (mid)) * (i % 2 ? 1.f : -1.f);
                    side += extra * width * 0.8f * rawPart;
                }
            }
            const float hp = sideHp2.process (sideHp1.process (side));
            side += (hp - side) * wOn;
            wL = mid + side; wR = mid - side;
        }

        // ---- Post EQ (FX: the processing only) -> Wet Gain, + Dry Gain (clean voice), Post EQ (All: everything)
        rmsDry += (1.f - rmsCoef) * (xL * xL + xR * xR - rmsDry);
        rmsWet += (1.f - rmsCoef) * (wL * wL + wR * wR - rmsWet);
        if (autoGainOn)
        {
            if (rmsDry > 1.0e-7f && rmsWet > 1.0e-9f)
            {
                const float target = jlimit (0.05f, 16.f, std::sqrt (rmsDry / rmsWet));
                autoGain = target + (autoGain - target) * agCoef;
            }
        }
        else autoGain = 1.f + (autoGain - 1.f) * agCoef;
        const float wetG = sWetGain.getNextValue() * autoGain, dryG = sDryGain.getNextValue();
        const float postAll = sPostAll.getNextValue();
        const float post = sPostOn.getNextValue();
        postFilter.tick();
        // one filter serves both targets: in All the clean voice goes through it too (crossfaded, so switching never clicks)
        const float dL = dryL * dryG, dR = dryR * dryG;
        float yL, yR;
        {
            const float inL2 = wL * wetG + dL * postAll, inR2 = wR * wetG + dR * postAll;
            float fl = inL2, fr = inR2;
            if (post > 0.0001f)
            {
                fl = inL2 + (postFilter.process (0, inL2) - inL2) * post;
                fr = inR2 + (postFilter.process (1, inR2) - inR2) * post;
            }
            yL = fl + dL * (1.f - postAll);
            yR = fr + dR * (1.f - postAll);
            if (postAll < 0.5f) { wL = fl; wR = fr; }   // what the spectrum shows as "effect"
        }
        const float preClip = std::max (std::abs (yL), std::abs (yR));

        // ---- Clip
        const float clipOnV = sClipOn.getNextValue();
        const float c = sCeiling.getNextValue();
        if (clipOnV > 0.0001f)
        {
            auto clip = [c, clipHard] (float x)
            {
                if (clipHard) return jlimit (-c, c, x);
                const float a = std::abs (x), knee = c * 0.6f;
                if (a <= knee) return x;
                const float r = c - knee;
                const float y = knee + r * std::tanh ((a - knee) / r);
                return x < 0 ? -y : y;
            };
            yL += (clip (yL) - yL) * clipOnV;
            yR += (clip (yR) - yR) * clipOnV;
        }
        const float gOut = sOutGain.getNextValue();
        yL *= gOut; yR *= gOut;
        if (! std::isfinite (yL)) yL = 0;
        if (! std::isfinite (yR)) yR = 0;

        oL[s] = yL;
        if (outCh > 1) oR[s] = yR; else oL[s] = 0.5f * (yL + yR);
        peakOut[0] = std::max (peakOut[0], std::abs (yL));
        peakOut[1] = std::max (peakOut[1], std::abs (yR));

        // ---- display data
        const float monoIn = 0.5f * (xL + xR), monoOut = 0.5f * (yL + yR);
        spectrum.in[specPos] = 0.5f * (eL + eR);
        spectrum.out[specPos] = 0.5f * (wL + wR);
        specPos = (specPos + 1) & (Spectrum::size - 1);
        if (bucketPos == 0) { bucketHi = bucketLo = monoOut; bucketIn = std::abs (monoIn); bucketPre = preClip * gOut; }
        else
        {
            bucketHi = std::max (bucketHi, monoOut); bucketLo = std::min (bucketLo, monoOut);
            bucketIn = std::max (bucketIn, std::abs (monoIn)); bucketPre = std::max (bucketPre, preClip * gOut);
        }
        if (++bucketPos >= bucketLen)
        {
            const int count = scope.writeCount.load (std::memory_order_relaxed);
            const int idx = count & (Scope::size - 1);
            scope.hi[idx] = bucketHi; scope.lo[idx] = bucketLo; scope.in[idx] = bucketIn; scope.preClip[idx] = bucketPre;
            scope.writeCount.store (count + 1, std::memory_order_release);
            bucketPos = 0;
        }
    }
    spectrum.writePos.store (specPos, std::memory_order_release);

    for (int c = 0; c < 2; ++c)
    {
        inPeak[c] = std::max (inPeak[c].load(), peakIn[c]);
        outPeak[c] = std::max (outPeak[c].load(), peakOut[c]);
    }
    for (int i = 0; i < curBands; ++i)
        bandLevel[(size_t) i] = std::max (bandLevel[(size_t) i].load(), bandPk[(size_t) i]);
    int lowest = 1000;
    for (int i = 0; i < maxVoices; ++i)
    {
        const int note = (voices[(size_t) i].active && pitchMode != 1) ? voices[(size_t) i].note : -1;
        uiNotes[(size_t) i] = note;
        if (note >= 0) lowest = std::min (lowest, note);
    }
    uiAutoHz = detectedHz;
    uiGlueGr = std::min (uiGlueGr.load(), glueGrMin);
    uiGlueIn = std::max (uiGlueIn.load(), glueInMax);
    for (int b = 0; b < 3; ++b)
    {
        if (std::abs (ottG[b]) > std::abs (uiOttGain[(size_t) b].load())) uiOttGain[(size_t) b] = ottG[b];
        uiOttLevel[(size_t) b] = std::max (uiOttLevel[(size_t) b].load(), ottLvl[b]);
    }
    ignoreUnused (lowest);
    for (int j = 0; j < Tables::numIntervals; ++j)
        uiInterval[(size_t) j] = Tables::snapInterval (Tables::intervals[j], *pScale > 0.5f, (int) *pRoot, (int) *pMode);
    for (int c = 2; c < buffer.getNumChannels(); ++c)
        if (c >= outCh) buffer.clear (c, 0, n);
}

//==============================================================================
void VoiceFuckerProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presets.getCurrentName(), nullptr);
    if (editorW > 0) { state.setProperty ("uiW", editorW.load(), nullptr); state.setProperty ("uiH", editorH.load(), nullptr); }
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void VoiceFuckerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = ValueTree::fromXml (*xml);
            Params::upgradeState (tree);
            apvts.replaceState (tree);
            presets.setCurrentNameFromState (tree.getProperty ("presetName", "Init").toString());
            if ((int) tree.getProperty ("uiW", 0) > 0) { editorW = (int) tree.getProperty ("uiW"); editorH = (int) tree.getProperty ("uiH"); }
        }
}

AudioProcessorEditor* VoiceFuckerProcessor::createEditor() { return new VoiceFuckerEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VoiceFuckerProcessor(); }
