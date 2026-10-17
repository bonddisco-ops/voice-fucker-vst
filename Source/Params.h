#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs, choice lists and the parameter layout of Voice Fucker
namespace ParamIDs
{
    inline constexpr const char* input    = "in_gain";
    inline constexpr const char* output   = "out_gain";

    // Carrier
    inline constexpr const char* carSrc    = "car_src";
    inline constexpr const char* carPitch  = "car_pitch";
    inline constexpr const char* carNote   = "car_note";
    inline constexpr const char* carGlide  = "car_glide";
    inline constexpr const char* carDetune = "car_detune";
    inline constexpr const char* carSpread = "car_spread";
    inline constexpr const char* carNoise  = "car_noise";

    // Vocoder
    inline constexpr const char* vocOn      = "voc_on";
    inline constexpr const char* vocBands   = "voc_bands";
    inline constexpr const char* vocFormant = "voc_formant";
    inline constexpr const char* vocAttack  = "voc_attack";
    inline constexpr const char* vocRelease = "voc_release";
    inline constexpr const char* vocSib     = "voc_sib";
    inline constexpr const char* vocGate    = "voc_gate";

    // Harmonize
    inline constexpr const char* harmOn     = "harm_on";
    inline constexpr const char* harmScale  = "harm_scale";
    inline constexpr const char* harmRoot   = "harm_root";
    inline constexpr const char* harmMode   = "harm_mode";      // scale type
    inline constexpr const char* harmPan    = "harm_pan";
    inline constexpr const char* harmLevel  = "harm_level";
    inline constexpr const char* harmDetune = "harm_detune";
    inline constexpr const char* harmSpread = "harm_spread";
    inline juce::String ivOn  (int i) { return "harm_iv" + juce::String (i) + "_on"; }
    inline juce::String ivDb  (int i) { return "harm_iv" + juce::String (i) + "_db"; }
    inline juce::String ivPan (int i) { return "harm_iv" + juce::String (i) + "_pan"; }

    // Dirt
    inline constexpr const char* dirtOn      = "dirt_on";
    inline constexpr const char* dirtMode    = "dirt_mode";
    inline constexpr const char* dirtDrive   = "dirt_drive";
    inline constexpr const char* dirtCrunch  = "dirt_crunch";
    inline constexpr const char* dirtDestroy = "dirt_destroy";

    // Comp
    inline constexpr const char* compOn      = "comp_on";
    inline constexpr const char* compMode    = "comp_mode";
    inline constexpr const char* ottDepth    = "ott_depth";
    inline constexpr const char* ottTime     = "ott_time";
    inline constexpr const char* ottUp       = "ott_up";
    inline constexpr const char* ottDown     = "ott_down";
    inline constexpr const char* glueThresh  = "glue_thresh";
    inline constexpr const char* glueRatio   = "glue_ratio";
    inline constexpr const char* glueAttack  = "glue_attack";
    inline constexpr const char* glueRelease = "glue_release";
    inline constexpr const char* compMix     = "comp_mix";

    // Width
    inline constexpr const char* widthOn   = "width_on";
    inline constexpr const char* widthMode = "width_mode";
    inline constexpr const char* widthAmt  = "width_amt";
    inline constexpr const char* widthMono = "width_mono";

    // Mix / clip
    inline constexpr const char* mix      = "mix";
    inline constexpr const char* autoGain = "autogain";
    inline constexpr const char* clipOn   = "clip_on";
    inline constexpr const char* clipMode = "clip_mode";
    inline constexpr const char* ceiling  = "clip_ceiling";

    // Filters: "pre_" or "post_" + one of these
    inline constexpr const char* fOn      = "on";
    inline constexpr const char* fLc      = "lc";
    inline constexpr const char* fHc      = "hc";
    inline constexpr const char* fLcSlope = "lcslope";
    inline constexpr const char* fHcSlope = "hcslope";
    inline constexpr const char* fPkFreq  = "pkfreq";
    inline constexpr const char* fPkGain  = "pkgain";
    inline constexpr const char* fPkQ     = "pkq";

    inline juce::String filterId (bool post, const char* name) { return juce::String (post ? "post_" : "pre_") + name; }
}

namespace Tables
{
    inline constexpr int numIntervals = 9;
    inline constexpr int intervals[numIntervals] = { -24, -12, -7, -5, -3, 3, 5, 7, 12 };
    inline const char* intervalNames[numIntervals] = { "-24", "-12", "-7", "-5", "-3", "+3", "+5", "+7", "+12" };
    inline constexpr float intervalDefaultDb[numIntervals] = { -9.f, -6.f, -4.f, -4.f, -4.f, -4.f, -4.f, -4.f, -6.f };
    inline constexpr float intervalDefaultPan[numIntervals] = { -0.3f, 0.3f, -0.5f, 0.5f, -0.7f, 0.7f, -0.6f, 0.6f, 0.0f };

    inline const juce::StringArray sources    { "Saw", "Square", "Supersaw", "Noise", "Sidechain", "Synth + SC" };
    inline const juce::StringArray pitchModes { "MIDI", "Auto", "Fixed" };
    inline const juce::StringArray dirtModes  { "Soft", "Tape", "Tube", "Fuzz", "Hard", "Fold" };
    inline const juce::StringArray compModes  { "OTT", "Glue" };
    inline const juce::StringArray glueRatios { "2:1", "4:1", "10:1" };
    inline constexpr float glueRatioValues[] = { 2.0f, 4.0f, 10.0f };
    inline const juce::StringArray widthModes { "Spectral", "Haas", "Chorus" };
    inline const juce::StringArray panModes   { "Manual", "Spectral" };
    inline const juce::StringArray clipModes  { "Soft", "Hard" };
    inline const juce::StringArray slopes     { "12 dB/oct", "24 dB/oct", "48 dB/oct" };
    inline const juce::StringArray roots      { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    struct Scale { const char* name; std::vector<int> notes; };
    inline const std::vector<Scale>& scales()
    {
        static const std::vector<Scale> s {
            { "Major", { 0, 2, 4, 5, 7, 9, 11 } },            { "Minor", { 0, 2, 3, 5, 7, 8, 10 } },
            { "Dorian", { 0, 2, 3, 5, 7, 9, 10 } },           { "Phrygian", { 0, 1, 3, 5, 7, 8, 10 } },
            { "Lydian", { 0, 2, 4, 6, 7, 9, 11 } },           { "Mixolydian", { 0, 2, 4, 5, 7, 9, 10 } },
            { "Locrian", { 0, 1, 3, 5, 6, 8, 10 } },          { "Harmonic Major", { 0, 2, 4, 5, 7, 8, 11 } },
            { "Harmonic Minor", { 0, 2, 3, 5, 7, 8, 11 } },   { "Overtone", { 0, 2, 4, 6, 7, 9, 10 } },
            { "Jazz Minor", { 0, 2, 3, 5, 7, 9, 11 } },       { "Blues Major", { 0, 2, 3, 4, 7, 9 } },
            { "Blues Minor", { 0, 3, 5, 6, 7, 10 } },         { "Double Harmonic Major", { 0, 1, 4, 5, 7, 8, 11 } },
            { "Double Harmonic Minor", { 0, 2, 3, 6, 7, 8, 11 } }, { "Whole Tone", { 0, 2, 4, 6, 8, 10 } },
            { "Half Diminished", { 0, 2, 3, 5, 6, 8, 10 } },  { "Diminished WH", { 0, 2, 3, 5, 6, 8, 9, 11 } },
            { "Diminished HW", { 0, 1, 3, 4, 6, 7, 9, 10 } }, { "Major Pentatonic", { 0, 2, 4, 7, 9 } },
            { "Minor Pentatonic", { 0, 3, 5, 7, 10 } },       { "Major Triad", { 0, 4, 7 } },
            { "Minor Triad", { 0, 3, 7 } },
        };
        return s;
    }
    inline juce::StringArray scaleNames() { juce::StringArray a; for (auto& s : scales()) a.add (s.name); return a; }

    inline bool inScale (int note, int root, int scaleIndex)
    {
        const auto& sc = scales()[(size_t) juce::jlimit (0, (int) scales().size() - 1, scaleIndex)];
        const int pc = ((note - root) % 12 + 12) % 12;
        for (int n : sc.notes) if (n == pc) return true;
        return false;
    }

    // The interval that really sounds, fixed by the root and the scale: thirds, fourths and fifths
    // move by at most a semitone to stay in the scale (counted from the root)
    inline int snapInterval (int iv, bool useScale, int root, int scaleIndex)
    {
        const int a = std::abs (iv), sign = iv < 0 ? -1 : 1;
        if (! useScale || a == 12 || a == 24) return iv;
        for (int c : { a, a + 1, a - 1 })
            if (inScale (root + sign * c, root, scaleIndex)) return sign * c;
        return iv;
    }

    inline juce::String noteName (int midi)
    {
        return roots[((midi % 12) + 12) % 12] + juce::String (midi / 12 - 1);
    }
}

namespace Params
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
