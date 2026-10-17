// Offline test: renders a synthetic voice through Voice Fucker with several settings,
// checks levels / stability / CPU, and saves a screenshot of the editor.
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <set>

namespace
{
    // crude singing voice: glottal pulses through two formant resonators, with "s" bursts and pauses
    juce::AudioBuffer<float> makeVoice (double sr, float seconds, std::vector<float>& f0Track)
    {
        const int n = (int) (sr * seconds);
        juce::AudioBuffer<float> b (2, n);
        vf::Biquad f1, f2, f3, sHp;
        vf::Noise nz;
        float ph = 0;
        sHp.setHighPass ((float) sr, 5000.f);
        f0Track.assign ((size_t) n, 0.f);
        const float vowels[3][3] = { { 730, 1090, 2440 }, { 270, 2290, 3010 }, { 300, 870, 2240 } };
        int lastV = -1;
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) i / (float) sr;
            const float seg = std::fmod (t, 1.0f);
            const int v = ((int) t) % 3;
            if (v != lastV)
            {
                f1.setBandPass ((float) sr, vowels[v][0], 6.f); f2.setBandPass ((float) sr, vowels[v][1], 8.f); f3.setBandPass ((float) sr, vowels[v][2], 10.f);
                lastV = v;
            }
            float x = 0;
            if (seg < 0.7f)
            {
                const float f0 = 130.f * std::pow (2.f, (float) ((int) (t * 2.f) % 4) * 3.f / 12.f) * (1.f + 0.01f * std::sin (t * 30.f));
                f0Track[(size_t) i] = f0;
                ph += f0 / (float) sr; if (ph >= 1) ph -= 1;
                const float pulse = ph < 0.1f ? std::sin (ph / 0.1f * vf::pi) : 0.f;
                x = f1.process (pulse) * 2.f + f2.process (pulse) * 1.2f + f3.process (pulse) * 0.6f;
                x *= std::min (1.f, seg * 20.f) * std::min (1.f, (0.7f - seg) * 20.f);
            }
            else if (seg < 0.82f)
                x = sHp.process (nz.next()) * 0.25f;   // "sss"
            b.setSample (0, i, x * 0.5f);
            b.setSample (1, i, x * 0.5f);
        }
        return b;
    }

    float rms (const juce::AudioBuffer<float>& b, int ch, int start, int len)
    {
        double s = 0;
        for (int i = start; i < start + len; ++i) s += (double) b.getSample (ch, i) * b.getSample (ch, i);
        return (float) std::sqrt (s / std::max (1, len));
    }

    void setP (VoiceFuckerProcessor& p, const juce::String& id, float v)
    {
        auto* prm = p.apvts.getParameter (id);
        prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    }

    void writeWav (const juce::AudioBuffer<float>& b, double sr, const juce::File& f)
    {
        f.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? argv[1] : "/tmp/vftest");
    outDir.createDirectory();
    const double sr = 48000.0;
    const int block = 256;
    std::vector<float> f0;
    auto voice = makeVoice (sr, 6.f, f0);
    writeWav (voice, sr, outDir.getChildFile ("input.wav"));

    struct Case { juce::String name; std::function<void (VoiceFuckerProcessor&)> setup; bool midi; };
    std::vector<Case> cases {
        { "default_auto", [] (VoiceFuckerProcessor&) {}, false },
        { "clean_robot", [] (VoiceFuckerProcessor& p) { setP (p, "dirt_on", 0); setP (p, "width_on", 0); setP (p, "autogain", 0); }, false },
        { "fixed_note", [] (VoiceFuckerProcessor& p) { setP (p, "car_pitch", 2); }, false },
        { "midi_chord", [] (VoiceFuckerProcessor& p) { setP (p, "car_pitch", 0); }, true },
        { "harm_spectral_100", [] (VoiceFuckerProcessor& p) { setP (p, "voc_bands", 100); setP (p, "harm_pan", 1);
                                   for (int i = 0; i < 9; ++i) setP (p, ParamIDs::ivOn (i), 1); }, false },
        { "harm_fan", [] (VoiceFuckerProcessor& p) { setP (p, ParamIDs::ivOn (1), 1); setP (p, ParamIDs::ivOn (7), 1); setP (p, ParamIDs::ivOn (0), 1); }, false },
        { "crunch_max", [] (VoiceFuckerProcessor& p) { setP (p, "dirt_crunch", 100); setP (p, "dirt_drive", 100); setP (p, "dirt_destroy", 70); setP (p, "dirt_mode", 5); }, false },
        { "fuzz", [] (VoiceFuckerProcessor& p) { setP (p, "dirt_crunch", 80); setP (p, "dirt_mode", 3); }, false },
        { "ott", [] (VoiceFuckerProcessor& p) { setP (p, "comp_on", 1); setP (p, "ott_depth", 100); }, false },
        { "glue", [] (VoiceFuckerProcessor& p) { setP (p, "comp_on", 1); setP (p, "comp_mode", 1); setP (p, "glue_thresh", -30); }, false },
        { "pre_post_eq", [] (VoiceFuckerProcessor& p) { setP (p, "pre_on", 1); setP (p, "pre_lc", 300); setP (p, "post_on", 1); setP (p, "post_hc", 2000); setP (p, "dry_gain", -6); setP (p, "wet_gain", -6); }, false },
        { "dry_only", [] (VoiceFuckerProcessor& p) { setP (p, "dry_gain", 0); setP (p, "wet_gain", -60); setP (p, "clip_on", 0); setP (p, "autogain", 0); }, false },
        { "pre_all_dry", [] (VoiceFuckerProcessor& p) { setP (p, "dry_gain", 0); setP (p, "wet_gain", -60); setP (p, "pre_on", 1); setP (p, "pre_target", 1); setP (p, "pre_lc", 2000); }, false },
        { "pre_fx_dry", [] (VoiceFuckerProcessor& p) { setP (p, "dry_gain", 0); setP (p, "wet_gain", -60); setP (p, "pre_on", 1); setP (p, "pre_target", 0); setP (p, "pre_lc", 2000); }, false },
        { "parallel_postfx", [] (VoiceFuckerProcessor& p) { setP (p, "dry_gain", 0); setP (p, "wet_gain", -5); setP (p, "post_on", 1); setP (p, "post_target", 0); setP (p, "post_hc", 1500); }, false },
        { "parallel_postall", [] (VoiceFuckerProcessor& p) { setP (p, "dry_gain", 0); setP (p, "wet_gain", -5); setP (p, "post_on", 1); setP (p, "post_target", 1); setP (p, "post_hc", 1500); }, false },
        { "harm_only_voc", [] (VoiceFuckerProcessor& p) { setP (p, "harm_dry", -60); setP (p, ParamIDs::ivOn (1), 1); setP (p, ParamIDs::ivOn (7), 1); }, false },
        { "harm_only_raw", [] (VoiceFuckerProcessor& p) { setP (p, "voc_on", 0); setP (p, "harm_dry", -60); setP (p, ParamIDs::ivOn (1), 1); setP (p, ParamIDs::ivOn (7), 1); }, false },
        { "harm_dry_only", [] (VoiceFuckerProcessor& p) { setP (p, "harm_wet", -60); setP (p, ParamIDs::ivOn (1), 1); setP (p, ParamIDs::ivOn (7), 1); }, false },
        { "voc_off_harm", [] (VoiceFuckerProcessor& p) { setP (p, "voc_on", 0); setP (p, ParamIDs::ivOn (1), 1); setP (p, ParamIDs::ivOn (7), 1); setP (p, ParamIDs::ivOn (0), 1); setP (p, "dirt_crunch", 0); setP (p, "dirt_drive", 0); }, false },
        { "gate_high", [] (VoiceFuckerProcessor& p) { setP (p, "voc_gate", -20); }, false },
        { "vocoder_off", [] (VoiceFuckerProcessor& p) { setP (p, "voc_on", 0); setP (p, "dirt_crunch", 60); }, false },
        { "noise_carrier", [] (VoiceFuckerProcessor& p) { setP (p, "car_src", 3); }, false },
        { "haas_chorus", [] (VoiceFuckerProcessor& p) { setP (p, "width_mode", 1); setP (p, "width_amt", 100); }, false },
        { "chorus", [] (VoiceFuckerProcessor& p) { setP (p, "width_mode", 2); setP (p, "width_amt", 100); }, false },
        { "formant_down", [] (VoiceFuckerProcessor& p) { setP (p, "voc_formant", -12); }, false },
        { "formant_up", [] (VoiceFuckerProcessor& p) { setP (p, "voc_formant", 12); }, false },
    };
    for (int fp = 0; fp < PresetManager::getFactoryNames().size(); ++fp)
        cases.push_back ({ "preset_" + juce::String (fp), [fp] (VoiceFuckerProcessor& p) { p.presets.loadFactory (fp); }, false });

    bool ok = true;
    for (auto& c : cases)
    {
        VoiceFuckerProcessor p;
        p.setPlayConfigDetails (2, 2, sr, block);
        p.prepareToPlay (sr, block);
        c.setup (p);
        juce::AudioBuffer<float> out (2, voice.getNumSamples());
        juce::AudioBuffer<float> buf (4, block);   // main stereo + sidechain stereo
        double autoErr = 0; int autoN = 0;
        std::set<int> seen[Tables::numIntervals];
        const auto t0 = juce::Time::getHighResolutionTicks();
        for (int pos = 0; pos < voice.getNumSamples(); pos += block)
        {
            const int len = std::min (block, voice.getNumSamples() - pos);
            buf.setSize (4, len, false, false, true);
            buf.clear();
            for (int ch = 0; ch < 2; ++ch) buf.copyFrom (ch, 0, voice, ch, pos, len);
            juce::MidiBuffer midi;
            if (c.midi)
            {
                const int beat = (int) (sr * 1.5);
                if (pos % beat < block && pos + len > 0)
                {
                    const int off = (beat - pos % beat) % beat;
                    if (off < len) {
                        const int root = 45 + ((pos / beat) % 3) * 2;
                        for (int k : { 0, 3, 7 }) midi.addEvent (juce::MidiMessage::noteOff (1, root - 2 + k), off);
                        for (int k : { 0, 3, 7 }) midi.addEvent (juce::MidiMessage::noteOn (1, root + k, (juce::uint8) 100), off);
                    }
                }
            }
            p.processBlock (buf, midi);
            for (int j = 0; j < Tables::numIntervals; ++j) seen[j].insert (p.uiInterval[(size_t) j].load());
            for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, buf, ch, 0, len);
            const float hz = p.uiAutoHz.load();
            const float truth = f0[(size_t) std::min (pos + len, (int) f0.size() - 1)];
            if (hz > 0 && truth > 0 && pos > (int) sr / 2)
            { autoErr += std::abs (vf::hzToMidi (hz) - vf::hzToMidi (truth)); ++autoN; }
        }
        const double secs = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);
        bool finite = true;
        float peak = 0;
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < out.getNumSamples(); ++i)
            {
                const float v = out.getSample (ch, i);
                if (! std::isfinite (v)) finite = false;
                peak = std::max (peak, std::abs (v));
            }
        const int a = (int) (sr * 1.0), len = (int) (sr * 4.0);
        const float rIn = rms (voice, 0, a, len), rL = rms (out, 0, a, len), rR = rms (out, 1, a, len);
        // stereo correlation as a width indicator
        double sLR = 0, sLL = 0, sRR = 0;
        for (int i = a; i < a + len; ++i) { const double l = out.getSample (0, i), r = out.getSample (1, i); sLR += l * r; sLL += l * l; sRR += r * r; }
        const double corr = sLR / std::sqrt (sLL * sRR + 1e-20);
        // silence between phrases (gate): level in the gap after the "s"
        const float gap = rms (out, 0, (int) (sr * 2.88), (int) (sr * 0.1));
        std::printf ("%-20s in %6.1f dB  out L %6.1f R %6.1f dB  peak %6.1f dB  corr %5.2f  gap %6.1f dB  cpu %5.1f%%  pitchErr %s  %s\n",
                     c.name.toRawUTF8(), vf::gainToDb (rIn), vf::gainToDb (rL), vf::gainToDb (rR), vf::gainToDb (peak), corr,
                     vf::gainToDb (gap), secs / 6.0 * 100.0,
                     autoN > 0 ? juce::String (autoErr / autoN, 2).toRawUTF8() : "-", finite ? "" : "NON-FINITE!");
        if (! finite || peak > 1.5f || rL < 1e-4f) ok = false;
        for (int j = 0; j < Tables::numIntervals; ++j)
            if (seen[j].size() > 1) { std::printf ("   INTERVAL %s JUMPS (%d values)\n", Tables::intervalNames[j], (int) seen[j].size()); ok = false; }
        if (c.name == "dry_only")
        {
            double e = 0, r = 0;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = a; i < a + len; ++i) { const double d = out.getSample (ch, i) - voice.getSample (ch, i); e += d * d; r += voice.getSample (ch, i) * voice.getSample (ch, i); }
            const double errDb = 10.0 * std::log10 (e / (r + 1e-20) + 1e-20);
            std::printf ("   dry path vs input: %.1f dB error\n", errDb);
            if (errDb > -60.0) ok = false;
        }
        writeWav (out, sr, outDir.getChildFile (c.name + ".wav"));
    }

    // state round trip
    {
        VoiceFuckerProcessor a, b;
        a.presets.loadFactory (2);
        juce::MemoryBlock mb; a.getStateInformation (mb);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        const bool same = b.presets.getCurrentName() == a.presets.getCurrentName()
                          && std::abs (b.apvts.getRawParameterValue ("dirt_crunch")->load() - a.apvts.getRawParameterValue ("dirt_crunch")->load()) < 0.01f;
        std::printf ("state round trip: %s\n", same ? "ok" : "FAILED");
        ok = ok && same;
    }

    // 2.0 sessions: Mix and Harm Level become Dry / Wet gains
    {
        VoiceFuckerProcessor a;
        auto st = a.apvts.copyState();
        for (const char* id : { "dry_gain", "wet_gain", "harm_dry", "harm_wet" })
            st.removeChild (st.getChildWithProperty ("id", id), nullptr);
        auto add = [&st] (const char* id, float v) { juce::ValueTree c ("PARAM"); c.setProperty ("id", id, nullptr); c.setProperty ("value", v, nullptr); st.appendChild (c, nullptr); };
        add ("mix", 50.0f); add ("harm_level", 50.0f);
        juce::MemoryBlock mb;
        if (auto xml = st.createXml()) juce::AudioProcessor::copyXmlToBinary (*xml, mb);
        VoiceFuckerProcessor b;
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        const float d = b.apvts.getRawParameterValue ("dry_gain")->load(), w = b.apvts.getRawParameterValue ("wet_gain")->load();
        const float hw = b.apvts.getRawParameterValue ("harm_wet")->load();
        const bool good = std::abs (d + 6.02f) < 0.2f && std::abs (w + 6.02f) < 0.2f && std::abs (hw + 6.02f) < 0.2f;
        std::printf ("old Mix 50 -> dry %.1f wet %.1f, harm level 50 -> wet %.1f: %s\n", d, w, hw, good ? "ok" : "FAILED");
        ok = ok && good;
    }

    // editor screenshots
    {
        VoiceFuckerProcessor p;
        p.setPlayConfigDetails (2, 2, sr, block);
        p.prepareToPlay (sr, block);
        setP (p, ParamIDs::ivOn (1), 1); setP (p, ParamIDs::ivOn (7), 1); setP (p, ParamIDs::ivOn (0), 1); setP (p, "comp_on", 1);
        setP (p, "pre_on", 1); setP (p, "pre_lc", 150);
        juce::AudioBuffer<float> buf (4, block);
        auto feed = [&] (int blocks, int start)
        {
            for (int k = 0; k < blocks; ++k)
            {
                buf.clear();
                for (int ch = 0; ch < 2; ++ch) buf.copyFrom (ch, 0, voice, ch, start + (k * block) % (int) (sr * 4), block);
                juce::MidiBuffer m; p.processBlock (buf, m);
            }
        };
        feed ((int) (sr * 1.5) / block, (int) sr);
        struct Shot { const char* name; const char* theme; int view; int tour; bool ceiling; int comp = -1; int w = 0, h = 0; };
        const Shot shots[] = { { "ui_wave", "Asphalt", 0, -1, false }, { "ui_spectrum", "Asphalt", 1, -1, false }, { "ui_eq", "Asphalt", 2, -1, false },
                               { "ui_ceiling", "Asphalt", 0, -1, true }, { "ui_mango", "Mango", 1, -1, false }, { "ui_luxe", "Luxe", 0, -1, false },
                               { "ui_rage", "Rage", 0, -1, false }, { "ui_ultra", "Ultraviolet", 1, -1, false }, { "ui_tour", "Asphalt", 0, 10, false },
                               { "ui_welcome", "Asphalt", 0, -2, false },
                               { "ui_glue", "Asphalt", 0, -1, false, 1 }, { "ui_ott", "Asphalt", 2, -1, false, 0 },
                               { "ui_ott_mango", "Mango", 2, -1, false, 0 }, { "ui_wide", "Asphalt", 0, -1, false, -1, 1500, 760 },
                               { "ui_tall", "Rage", 1, -1, false, -1, 900, 1000 } };
        for (auto& sh : shots)
        {
            std::unique_ptr<juce::AudioProcessorEditor> base (p.createEditor());
            auto* ed = dynamic_cast<VoiceFuckerEditor*> (base.get());
            setP (p, "comp_on", 1); setP (p, "comp_mode", sh.comp == 1 ? 1 : 0);
            if (sh.comp == 1) setP (p, "glue_thresh", -30);
            ed->setSize (sh.w > 0 ? sh.w : VoiceFuckerEditor::baseW, sh.h > 0 ? sh.h : VoiceFuckerEditor::baseH);
            ed->hideOverlays();
            ed->setThemeByName (sh.theme);
            ed->getScope().setView ((ScopeView::View) sh.view);
            if (sh.tour >= 0) ed->startTour (sh.tour);
            if (sh.tour == -2) ed->showWelcome();
            if (sh.ceiling) ed->touchCeiling();
            if (sh.comp >= 0) ed->touchComp (sh.comp == 1 ? 1 : 2);
            for (int i = 0; i < 8; ++i) { feed (6, (int) sr + i * block * 6); ed->getScope().advanceFrames (1); juce::MessageManager::getInstance()->runDispatchLoopUntil (20); }
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            juce::PNGImageFormat png;
            auto f = outDir.getChildFile (juce::String (sh.name) + ".png");
            f.deleteFile();
            juce::FileOutputStream os (f);
            png.writeImageToStream (img, os);
            std::printf ("screenshot: %s (%d x %d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
        }
    }

    std::printf ("%s\n", ok ? "ALL OK" : "PROBLEMS FOUND");
    return ok ? 0 : 1;
}
