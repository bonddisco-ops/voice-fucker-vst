#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// All parameter IDs and the parameter layout of Voice Fucker.
namespace Params
{
    // Harmonize intervals, ordered from lowest to highest
    inline constexpr int numIntervals = 9;
    inline constexpr int intervals[numIntervals] = { -24, -12, -7, -5, -3, 3, 5, 7, 12 };
    inline const char* intervalNames[numIntervals] = { "-24", "-12", "-7", "-5", "-3", "+3", "+5", "+7", "+12" };
    inline constexpr float intervalDefaultDb[numIntervals] = { -9.f, -6.f, -4.f, -4.f, -4.f, -4.f, -4.f, -4.f, -6.f };

    inline juce::String ivOn (int i) { return "harm_iv" + juce::String (i) + "_on"; }
    inline juce::String ivDb (int i) { return "harm_iv" + juce::String (i) + "_db"; }

    inline const juce::StringArray sources { "Saw", "Square", "Supersaw", "Noise", "Sidechain", "Synth + SC" };
    inline const juce::StringArray pitchModes { "MIDI", "Auto", "Fixed" };
    inline const juce::StringArray dirtModes { "Tube", "Hard", "Fold" };
    inline const juce::StringArray widthModes { "Spectral", "Haas", "Chorus" };
    inline const juce::StringArray panModes { "Alternate", "Fan", "Spectral" };
    inline const juce::StringArray clipModes { "Soft", "Hard" };

    inline juce::StringArray keyNames()
    {
        const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        juce::StringArray s;
        for (auto* x : n) s.add (juce::String (x) + " minor");
        for (auto* x : n) s.add (juce::String (x) + " major");
        return s;
    }

    inline juce::String noteName (int midi)
    {
        const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String (n[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 1);
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        std::vector<std::unique_ptr<RangedAudioParameter>> p;
        auto pid = [] (const char* id) { return ParameterID { id, 1 }; };
        auto pidS = [] (const String& id) { return ParameterID { id, 1 }; };

        auto pct = AudioParameterFloatAttributes().withLabel ("%")
                       .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " %"; });
        auto db = AudioParameterFloatAttributes().withLabel ("dB")
                      .withStringFromValueFunction ([] (float v, int) { return (v > 0.04f ? "+" : "") + String (v, 1) + " dB"; });
        auto ms = AudioParameterFloatAttributes().withLabel ("ms")
                      .withStringFromValueFunction ([] (float v, int) { return v < 10.f ? String (v, 1) + " ms" : String (roundToInt (v)) + " ms"; });
        auto hz = AudioParameterFloatAttributes().withLabel ("Hz")
                      .withStringFromValueFunction ([] (float v, int) { return v >= 1000.f ? String (v / 1000.f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; });

        auto range = [] (float lo, float hi, float centre) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (centre); return r; };

        // Input / output
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("in_gain"), "Input", NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f, db));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("out_gain"), "Output", NormalisableRange<float> (-24.f, 24.f, 0.1f), 0.f, db));

        // Carrier
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("car_src"), "Source", sources, 2));
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("car_pitch"), "Pitch", pitchModes, 1));
        p.push_back (std::make_unique<AudioParameterInt> (pid ("car_note"), "Note", 24, 72, 36,
                         AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return noteName (v); })));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("car_glide"), "Glide", range (0.f, 500.f, 60.f), 25.f, ms));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("car_detune"), "Detune", NormalisableRange<float> (0.f, 100.f), 35.f, pct));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("car_spread"), "Spread", NormalisableRange<float> (0.f, 100.f), 60.f, pct));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("car_noise"), "Noise", NormalisableRange<float> (0.f, 100.f), 10.f, pct));

        // Vocoder
        p.push_back (std::make_unique<AudioParameterBool> (pid ("voc_on"), "Vocoder On", true));
        p.push_back (std::make_unique<AudioParameterInt> (pid ("voc_bands"), "Bands", 4, 100, 28));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("voc_formant"), "Formant", NormalisableRange<float> (-12.f, 12.f, 0.1f), 0.f,
                         AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return (v > 0.04f ? "+" : "") + String (v, 1) + " st"; })));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("voc_attack"), "Attack", range (0.1f, 50.f, 5.f), 2.f, ms));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("voc_release"), "Release", range (5.f, 500.f, 60.f), 50.f, ms));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("voc_sib"), "Sibilance", NormalisableRange<float> (0.f, 100.f), 40.f, pct));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("voc_gate"), "Gate", NormalisableRange<float> (-80.f, -20.f, 0.1f), -60.f,
                         AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v <= -79.9f ? String ("Off") : String (roundToInt (v)) + " dB"; })));

        // Harmonize
        p.push_back (std::make_unique<AudioParameterBool> (pid ("harm_on"), "Harmonize On", true));
        for (int i = 0; i < numIntervals; ++i)
        {
            p.push_back (std::make_unique<AudioParameterBool> (pidS (ivOn (i)), String ("Harm ") + intervalNames[i] + " On", false));
            p.push_back (std::make_unique<AudioParameterFloat> (pidS (ivDb (i)), String ("Harm ") + intervalNames[i] + " Level",
                                                                NormalisableRange<float> (-36.f, 6.f, 0.1f), intervalDefaultDb[i], db));
        }
        p.push_back (std::make_unique<AudioParameterBool> (pid ("harm_scale"), "Scale", true));
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("harm_key"), "Key", keyNames(), 0));
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("harm_pan"), "Pan", panModes, 1));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("harm_level"), "Harm Level", NormalisableRange<float> (0.f, 100.f), 80.f, pct));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("harm_detune"), "Harm Detune", NormalisableRange<float> (0.f, 100.f), 20.f, pct));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("harm_spread"), "Harm Spread", NormalisableRange<float> (0.f, 100.f), 70.f, pct));

        // Dirt
        p.push_back (std::make_unique<AudioParameterBool> (pid ("dirt_on"), "Dirt On", true));
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("dirt_mode"), "Dirt Mode", dirtModes, 0));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("dirt_drive"), "Drive", NormalisableRange<float> (0.f, 100.f), 30.f, pct));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("dirt_crunch"), "Crunch", NormalisableRange<float> (0.f, 100.f), 35.f, pct));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("dirt_destroy"), "Destroy", NormalisableRange<float> (0.f, 100.f), 0.f, pct));

        // Width
        p.push_back (std::make_unique<AudioParameterBool> (pid ("width_on"), "Width On", true));
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("width_mode"), "Width Mode", widthModes, 0));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("width_amt"), "Width", NormalisableRange<float> (0.f, 100.f), 50.f, pct));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("width_mono"), "Mono Bass", range (20.f, 500.f, 120.f), 120.f, hz));

        // Mix
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("mix"), "Mix", NormalisableRange<float> (0.f, 100.f), 100.f, pct));
        p.push_back (std::make_unique<AudioParameterBool> (pid ("autogain"), "Auto Gain", true));

        // Clip
        p.push_back (std::make_unique<AudioParameterBool> (pid ("clip_on"), "Clip On", true));
        p.push_back (std::make_unique<AudioParameterChoice> (pid ("clip_mode"), "Clip Mode", clipModes, 0));
        p.push_back (std::make_unique<AudioParameterFloat> (pid ("clip_ceiling"), "Ceiling", NormalisableRange<float> (-12.f, 0.f, 0.1f), -0.3f, db));

        return { p.begin(), p.end() };
    }
}
