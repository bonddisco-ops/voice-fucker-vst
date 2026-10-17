#pragma once
#include "PluginProcessor.h"
#include "UI.h"

class VoiceFuckerEditor;

//==============================================================================
// Bulging CRT screen: live waveform or vocoder band spectrum
class CrtScreen : public juce::Component, private juce::Timer
{
public:
    explicit CrtScreen (VoiceFuckerProcessor&);
    ~CrtScreen() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    juce::Point<float> bulge (float x, float y) const;
    juce::Path glassPath (juce::Rectangle<float>) const;
    juce::String noteText() const;
    VoiceFuckerProcessor& proc;
    std::array<float, VoiceFuckerProcessor::maxBands> spec {}, hold {};
    std::vector<float> snap;
    juce::Rectangle<float> glass, toggleArea;
};

//==============================================================================
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter (VoiceFuckerProcessor&, bool isInput);
    ~LevelMeter() override { stopTimer(); }
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    VoiceFuckerProcessor& proc;
    bool input;
    float level[2] {}, peakHold = -100.f;
    int holdFrames = 0;
};

//==============================================================================
class PresetBar : public juce::Component, private juce::Timer
{
public:
    explicit PresetBar (VoiceFuckerProcessor&);
    ~PresetBar() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void step (int dir);
    void showMenu();
    void savePreset();
    struct Entry { juce::String name; int factory = -1; juce::File file; };
    juce::Array<Entry> entries() const;
    VoiceFuckerProcessor& proc;
    juce::String shown;
    bool dirtyShown = false;
    std::unique_ptr<juce::AlertWindow> saveWindow;
};

//==============================================================================
class HarmPanel : public juce::Component, private juce::Timer
{
public:
    explicit HarmPanel (VoiceFuckerProcessor&);
    ~HarmPanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int stripX0 = 40, stripStep = 54;
private:
    void timerCallback() override;
    float panFor (int i) const;
    VoiceFuckerProcessor& proc;
    juce::OwnedArray<ui::Chip> chips;
    juce::OwnedArray<ui::Fader> faders;
    std::unique_ptr<ui::PowerButton> power;
    std::unique_ptr<ui::Led> scaleLed;
    std::unique_ptr<ui::Combo> key;
    std::unique_ptr<ui::Segmented> pan;
    std::unique_ptr<ui::Knob> level, detune, spread;
    juce::int64 lastHash = 0;
};

//==============================================================================
class MainPanel : public juce::Component, private juce::Timer
{
public:
    explicit MainPanel (VoiceFuckerProcessor&, VoiceFuckerEditor&);
    ~MainPanel() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    static constexpr int W = 870, H = 850;
private:
    void timerCallback() override;
    struct Module { juce::String title; juce::Rectangle<int> r; juce::String onParam; juce::Array<juce::Component*> items; };
    ui::Knob* knob (const juce::String& id, const juce::String& label, float cx, float cy, Module* m = nullptr);
    void showSettings();

    VoiceFuckerProcessor& proc;
    VoiceFuckerEditor& editor;
    juce::Image asphalt;
    juce::OwnedArray<juce::Component> owned;
    std::vector<Module> modules;
    CrtScreen screen;
    LevelMeter inMeter, outMeter;
    PresetBar presets;
    HarmPanel harm;
    juce::Rectangle<int> gearArea;
};

//==============================================================================
class VoiceFuckerEditor : public juce::AudioProcessorEditor
{
public:
    explicit VoiceFuckerEditor (VoiceFuckerProcessor&);
    ~VoiceFuckerEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setScale (float s);
private:
    VoiceFuckerProcessor& proc;
    ui::Look look;
    MainPanel content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceFuckerEditor)
};
