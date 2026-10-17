#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

// Shared look & widgets for the Primal Rage Lab house style (asphalt black + pink).
namespace ui
{
    namespace col
    {
        const juce::Colour pink     { 0xffff5fa2 };
        const juce::Colour pinkHot  { 0xffff7ab5 };
        const juce::Colour pinkDeep { 0xffb8306c };
        const juce::Colour pinkDim  { 0xff6a3a50 };
        const juce::Colour bg       { 0xff1d1b1e };
        const juce::Colour text     { 0xffd9d2d6 };
        const juce::Colour value    { 0xffe6e0e3 };
        const juce::Colour muted    { 0xff8f878c };
        const juce::Colour field    { 0xff1a181b };
        const juce::Colour panel    { 0x590a0a0c };
        const juce::Colour hair     { 0x0dffffff };
    }

    juce::Font font (float height, bool bold = false);
    void drawCaption (juce::Graphics& g, const juce::String& t, float x, float baseline, float alpha = 1.f);
    void drawSpaced (juce::Graphics& g, const juce::String& t, juce::Font f, float x, float baseline, float spacing);
    juce::Path logoPath();   // gorilla, in a 514 x 534 box
    void drawLogo (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour seamColour);

    //==========================================================================
    class Look : public juce::LookAndFeel_V4
    {
    public:
        Look();
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos, float minPos,
                               float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
        void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override { return font (12.f); }
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
        juce::Font getPopupMenuFont() override { return font (13.f); }
        void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    };

    //==========================================================================
    // Rotary knob with name above and value below
    class Knob : public juce::Component, private juce::Slider::Listener
    {
    public:
        Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label);
        ~Knob() override;
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override { repaint(); }
        void setCentre (float cx, float cy) { setBounds (juce::roundToInt (cx - 34), juce::roundToInt (cy - 47), 68, 98); }
        juce::Slider slider;
    private:
        void sliderValueChanged (juce::Slider*) override { repaint(); }
        juce::RangedAudioParameter* param;
        juce::String label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    //==========================================================================
    class PowerButton : public juce::ToggleButton
    {
    public:
        PowerButton (juce::AudioProcessorValueTreeState&, const juce::String& paramId);
        void paintButton (juce::Graphics&, bool over, bool down) override;
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    //==========================================================================
    class Led : public juce::ToggleButton
    {
    public:
        Led (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& text);
        void paintButton (juce::Graphics&, bool over, bool down) override;
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    //==========================================================================
    // Segmented switch bound to a choice parameter
    class Segmented : public juce::Component
    {
    public:
        Segmented (juce::AudioProcessorValueTreeState&, const juce::String& paramId, juce::StringArray labels);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        bool active = true;
    private:
        juce::StringArray labels;
        int index = 0;
        std::unique_ptr<juce::ParameterAttachment> att;
    };

    //==========================================================================
    class Combo : public juce::ComboBox
    {
    public:
        Combo (juce::AudioProcessorValueTreeState&, const juce::String& paramId);
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
    };

    //==========================================================================
    // On/off chip for one harmony interval
    class Chip : public juce::ToggleButton
    {
    public:
        Chip (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& text);
        void paintButton (juce::Graphics&, bool over, bool down) override;
        bool blockOn = true;
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    // Vertical level fader for one harmony interval
    class Fader : public juce::Slider
    {
    public:
        Fader (juce::AudioProcessorValueTreeState&, const juce::String& paramId);
        bool voiceOn = false, blockOn = true;
    private:
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
}
