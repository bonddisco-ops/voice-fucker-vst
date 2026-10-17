#pragma once
#include "PluginProcessor.h"
#include "Components.h"

//==============================================================================
// On/off button of one harmony interval
class ChipButton : public juce::ToggleButton
{
public:
    ChipButton() { setWantsKeyboardFocus (false); setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    juce::String shown;      // interval that really sounds
    bool moved = false;      // the scale moved it
};

// The mixer part of Harmonize: dB readouts, faders, interval buttons and stereo dots
class HarmStrips : public juce::Component, public juce::SettableTooltipClient, private juce::Timer
{
public:
    explicit HarmStrips (VoiceFuckerProcessor&);
    ~HarmStrips() override { stopTimer(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void setHelp (bool on, Help::Lang);

    static constexpr int stripX0 = 30, stripStep = 62, panY = 146;

private:
    void timerCallback() override;
    int stripAt (juce::Point<int>) const;
    float effectivePan (int i) const;

    VoiceFuckerProcessor& proc;
    juce::OwnedArray<juce::Slider> faders;
    juce::OwnedArray<ChipButton> chips;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::SliderAttachment> faderAtt;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> chipAtt;
    int dragging = -1;
    float flicker = 0.0f;
    bool helpOn = true;
    Help::Lang lang = Help::Lang::en;
};

//==============================================================================
class VoiceFuckerEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit VoiceFuckerEditor (VoiceFuckerProcessor&);
    ~VoiceFuckerEditor() override;

    void resized() override;
    void paint (juce::Graphics&) override {}

    static constexpr int baseW = 1160, baseH = 850;

    static juce::String loadSetting (const juce::String& key, const juce::String& fallback);
    static void saveSetting (const juce::String& key, const juce::String& value);

    // For tests and screenshots
    void setThemeByName (const juce::String& name) { applyTheme (Theme::byName (name), false); }
    ScopeView& getScope() { return scope; }
    void startTour (int step) { tour.start (lang); tour.showStep (step); }
    void showWelcome() { welcome.setVisible (true); welcome.toFront (false); }
    void hideOverlays() { welcome.setVisible (false); tour.setVisible (false); }
    void touchCeiling() { scope.touchCeiling (true); }

private:
    class Content : public juce::Component
    {
    public:
        explicit Content (VoiceFuckerEditor& e) : editor (e) {}
        void paint (juce::Graphics& g) override { editor.paintContent (g); }
        void mouseDown (const juce::MouseEvent& e) override;
    private:
        VoiceFuckerEditor& editor;
    };

    void timerCallback() override;
    void paintContent (juce::Graphics&);
    void applyTheme (const Theme&, bool remember);
    void applyHelp();
    void setScale (float scale, bool remember);
    void showSettingsMenu();
    void buildBackground();
    void addCombo (juce::Component& parent, juce::ComboBox&, const char* paramId, const juce::StringArray& items,
                   std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>&);
    juce::Rectangle<float> tourTarget (const juce::String& name);

    VoiceFuckerProcessor& proc;
    VFLookAndFeel laf;                // first, so it outlives every component
    const Theme* theme = nullptr;
    Content content { *this };
    juce::Image background;
    float scale = 1.0f;
    bool helpOn = true;
    Help::Lang lang = Help::Lang::en;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    PresetBar presetBar { proc };
    IconButton gear { IconButton::Icon::gear };
    ScopeView scope { proc };
    MeterColumn inCol { "IN", proc.apvts, ParamIDs::input, "Input" };
    MeterColumn outCol { "OUT", proc.apvts, ParamIDs::output, "Output" };

    Block carrierBlock { "CARRIER", proc.apvts, nullptr, false };
    Block vocBlock     { "VOCODER", proc.apvts, ParamIDs::vocOn, false };
    Block harmBlock    { "HARMONIZE", proc.apvts, ParamIDs::harmOn, false };
    Block dirtBlock    { "DIRT", proc.apvts, ParamIDs::dirtOn, false };
    Block compBlock    { "COMP", proc.apvts, ParamIDs::compOn, false };
    Block widthBlock   { "WIDTH", proc.apvts, ParamIDs::widthOn, false };
    Block mixBlock     { "MIX", proc.apvts, nullptr, false };
    Block clipBlock    { "CLIP", proc.apvts, ParamIDs::clipOn, false };

    juce::ComboBox sourceBox, rootBox, scaleBox, dirtModeBox, compModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sourceAtt, rootAtt, scaleAtt, dirtModeAtt, compModeAtt;
    std::unique_ptr<Segmented> pitchSeg, panSeg, widthSeg, clipMode;
    juce::ToggleButton scaleButton { "Scale" }, autoGainButton { "Auto gain" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> scaleAtt2, autoGainAtt;
    HarmStrips strips { proc };

    Knob note, glide, detune, spread, noise;
    Knob bands, formant, attack, release, sib, gate;
    Knob harmLevel, harmDetune, harmSpread;
    Knob drive, crunch, destroy;
    Knob ottDepth, ottTime, ottUp, ottDown, glueThresh, glueRatio, glueAttack, glueRelease, compMix;
    Knob width, mono, mix, ceiling;

    AboutOverlay about;
    TourOverlay tour;
    WelcomePrompt welcome;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoiceFuckerEditor)
};
