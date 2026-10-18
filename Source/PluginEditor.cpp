#include "PluginEditor.h"
#include "Logo.h"

using namespace juce;

namespace
{
    constexpr int X0 = 96, X1 = 1064;
    constexpr int scopeY = 80, scopeH = 224;
    constexpr int row1Y = 312, row1H = 170, carrierW = 428, vocW = 508;
    constexpr int harmY = 490, harmH = 190;
    constexpr int row3Y = 688, row3H = 152;
    constexpr int dirtW = 186, compW = 302, widthW = 170, mixW = 136, clipW = 102;
    constexpr int row3Gap = (X1 - X0 - (dirtW + compW + widthW + mixW + clipW)) / 4;
    const Rectangle<int> logoArea (96, 10, 58, 60);
    const Rectangle<int> titleArea (96, 10, 360, 60);
    bool askedThisRun = false;

    PropertiesFile::Options settingsOptions()
    {
        PropertiesFile::Options o;
        o.applicationName = "Voice Fucker";
        o.folderName = "Primal Rage Lab";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        o.millisecondsBeforeSaving = 0;
        return o;
    }
}

//==============================================================================
void ChipButton::paintButton (Graphics& g, bool highlighted, bool)
{
    const auto& t = themeOf (*this);
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();
    if (on)
    {
        g.setColour (t.accent.withAlpha (0.22f));
        g.fillRoundedRectangle (r.expanded (1.0f), 7.0f);
    }
    g.setColour (on ? t.accent : Colour (0xff1b1b1f).brighter (highlighted ? 0.12f : 0.0f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    g.setColour (on ? Colour (0xff111114) : Colour (0xffececf0));
    g.setFont (uiFont (13.0f, true));
    g.drawText (shown, r, Justification::centred);
    if (moved)
    {
        g.setColour (on ? Colour (0xff111114) : t.accent);
        g.fillEllipse (r.getRight() - 9.0f, r.getY() + 4.0f, 5.0f, 5.0f);
    }
}

//==============================================================================
HarmStrips::HarmStrips (VoiceFuckerProcessor& p) : proc (p)
{
    for (int i = 0; i < Tables::numIntervals; ++i)
    {
        auto* f = faders.add (new Slider (Slider::LinearVertical, Slider::NoTextBox));
        f->setWantsKeyboardFocus (false);
        f->setSliderSnapsToMousePosition (false);
        addAndMakeVisible (f);
        faderAtt.add (new AudioProcessorValueTreeState::SliderAttachment (proc.apvts, ParamIDs::ivDb (i), *f));
        auto* prm = proc.apvts.getParameter (ParamIDs::ivDb (i));
        f->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        f->onValueChange = [this] { repaint(); };

        auto* c = chips.add (new ChipButton());
        c->shown = Tables::intervalNames[i];
        addAndMakeVisible (c);
        chipAtt.add (new AudioProcessorValueTreeState::ButtonAttachment (proc.apvts, ParamIDs::ivOn (i), *c));
        c->onStateChange = [this] { repaint(); };
        // touching the fader of a muted voice switches it on
        f->onDragStart = [this, i] { if (! chips[i]->getToggleState()) chips[i]->setToggleState (true, sendNotificationSync); };
    }
    startTimerHz (30);
}

void HarmStrips::resized()
{
    for (int i = 0; i < Tables::numIntervals; ++i)
    {
        const int x = stripX0 + i * stripStep;
        faders[i]->setBounds (x - 14, 20, 28, 84);
        chips[i]->setBounds (x - 25, 108, 50, 26);
    }
}

void HarmStrips::setHelp (bool on, Help::Lang l)
{
    helpOn = on; lang = l;
    for (auto* f : faders) f->setTooltip (on ? Help::tip ("fader", l) : String());
    for (auto* c : chips) c->setTooltip (on ? Help::tip ("chip", l) : String());
    setTooltip (on ? Help::tip ("panDot", l) : String());
}

void HarmStrips::timerCallback()
{
    flicker += 0.11f;
    const bool on = proc.apvts.getRawParameterValue (ParamIDs::harmOn)->load() > 0.5f;
    for (int i = 0; i < Tables::numIntervals; ++i)
    {
        const int real = proc.uiInterval[(size_t) i].load();
        const String txt = (real > 0 ? "+" : "") + String (real);
        const bool moved = real != Tables::intervals[i];
        if (chips[i]->shown != txt || chips[i]->moved != moved)
        {
            chips[i]->shown = txt;
            chips[i]->moved = moved;
            chips[i]->repaint();
        }
        const bool vOn = chips[i]->getToggleState();
        if ((bool) faders[i]->getProperties().getWithDefault ("voiceOn", true) != vOn)
        {
            faders[i]->getProperties().set ("voiceOn", vOn);
            faders[i]->repaint();
        }
    }
    setAlpha (on ? 1.0f : 0.4f);
    if (roundToInt (proc.apvts.getRawParameterValue (ParamIDs::harmPan)->load()) == 1)
        repaint (0, panY - 12, getWidth(), 24);
}

float HarmStrips::effectivePan (int i) const
{
    const float p = proc.apvts.getRawParameterValue (ParamIDs::ivPan (i))->load();
    const float s = proc.apvts.getRawParameterValue (ParamIDs::harmSpread)->load() * 0.01f;
    if (std::abs (p) < 0.001f) return 0.0f;
    return p + ((p > 0 ? 1.0f : -1.0f) - p) * s;
}

void HarmStrips::paint (Graphics& g)
{
    const auto& t = themeOf (*this);
    g.setColour (Colours::black.withAlpha (t.lightBg ? 0.12f : 0.25f));
    g.fillRoundedRectangle (0.0f, 0.0f, (float) (stripX0 + 8 * stripStep + 64), (float) getHeight(), 9.0f);

    // dB scale
    auto* f0 = faders[0];
    for (const float db : { 0.0f, -12.0f, -24.0f, -36.0f })
    {
        const float y = (float) f0->getY() + (float) f0->getPositionOfValue (db);
        g.setColour (Colours::white.withAlpha (0.05f));
        g.drawHorizontalLine (roundToInt (y), 8.0f, (float) (stripX0 + 8 * stripStep + 30));
        g.setColour (Colour ((uint8) 200, 200, 210, 0.45f));
        g.setFont (uiFont (10.0f));
        g.drawText (String ((int) db), stripX0 + 8 * stripStep + 32, (int) y - 6, 28, 12, Justification::centredLeft);
    }

    const int mode = roundToInt (proc.apvts.getRawParameterValue (ParamIDs::harmPan)->load());
    const float sp = proc.apvts.getRawParameterValue (ParamIDs::harmSpread)->load() * 0.01f;
    for (int i = 0; i < Tables::numIntervals; ++i)
    {
        const float x = (float) (stripX0 + i * stripStep);
        const bool vOn = chips[i]->getToggleState();
        const float db = (float) faders[i]->getValue();
        g.setColour (vOn ? Colour (0xffececf0) : Colour (0xff7a7a82));
        g.setFont (uiFont (12.0f, true));
        g.drawText (vOn ? (db > 0.05f ? "+" : "") + String (db, 1) : String ("off"), (int) x - 28, 3, 56, 14, Justification::centred);

        // stereo position
        const float hw = 26.0f, py = (float) panY;
        g.setColour (Colours::white.withAlpha (0.16f));
        g.drawLine (x - hw, py, x + hw, py, 1.0f);
        g.setColour (Colours::white.withAlpha (0.30f));
        g.fillRect (x - 0.5f, py - 4.0f, 1.0f, 8.0f);
        g.fillRect (x - hw - 0.5f, py - 3.0f, 1.0f, 6.0f);
        g.fillRect (x + hw - 0.5f, py - 3.0f, 1.0f, 6.0f);
        if (! vOn) continue;
        if (mode == 1)
        {
            const float w = hw * (0.15f + 0.85f * sp);
            for (int k = -6; k <= 6; ++k)
            {
                const float fl = 0.35f + 0.65f * std::abs (std::sin (flicker * 2.7f + (float) i * 1.7f + (float) k * 2.3f));
                g.setColour (t.accent.withAlpha (fl * (1.0f - std::abs ((float) k) / 8.0f)));
                g.fillRect (x + (float) k / 6.0f * w - 2.0f, py - 5.0f, 4.0f, 10.0f);
            }
        }
        else
        {
            const float p = proc.apvts.getRawParameterValue (ParamIDs::ivPan (i))->load();
            const float e = effectivePan (i);
            if (std::abs (e - p) > 0.01f)
            {
                g.setColour (t.accent.withAlpha (0.35f));
                g.fillEllipse (x + p * hw - 3.5f, py - 3.5f, 7.0f, 7.0f);
            }
            g.setColour (t.accent.withAlpha (0.25f));
            g.fillEllipse (x + e * hw - 9.0f, py - 9.0f, 18.0f, 18.0f);
            g.setColour (t.accent);
            g.fillEllipse (x + e * hw - 5.5f, py - 5.5f, 11.0f, 11.0f);
        }
    }
}

int HarmStrips::stripAt (Point<int> pos) const
{
    if (std::abs (pos.y - panY) > 13) return -1;
    for (int i = 0; i < Tables::numIntervals; ++i)
        if (std::abs (pos.x - (stripX0 + i * stripStep)) <= 30) return i;
    return -1;
}

void HarmStrips::mouseMove (const MouseEvent& e)
{
    const int i = stripAt (e.getPosition());
    const bool manual = roundToInt (proc.apvts.getRawParameterValue (ParamIDs::harmPan)->load()) == 0;
    setMouseCursor (i >= 0 && manual && chips[i]->getToggleState() ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor);
}

void HarmStrips::mouseDown (const MouseEvent& e)
{
    dragging = -1;
    const int i = stripAt (e.getPosition());
    if (i < 0 || roundToInt (proc.apvts.getRawParameterValue (ParamIDs::harmPan)->load()) != 0 || ! chips[i]->getToggleState()) return;
    dragging = i;
    proc.apvts.getParameter (ParamIDs::ivPan (i))->beginChangeGesture();
    mouseDrag (e);
}

void HarmStrips::mouseDrag (const MouseEvent& e)
{
    if (dragging < 0) return;
    auto* prm = proc.apvts.getParameter (ParamIDs::ivPan (dragging));
    const float v = jlimit (-1.0f, 1.0f, (float) (e.x - (stripX0 + dragging * stripStep)) / 26.0f);
    prm->setValueNotifyingHost (prm->convertTo0to1 (std::abs (v) < 0.06f ? 0.0f : v));
    repaint();
}

void HarmStrips::mouseUp (const MouseEvent&)
{
    if (dragging >= 0) proc.apvts.getParameter (ParamIDs::ivPan (dragging))->endChangeGesture();
    dragging = -1;
}

void HarmStrips::mouseDoubleClick (const MouseEvent& e)
{
    const int i = stripAt (e.getPosition());
    if (i < 0) return;
    auto* prm = proc.apvts.getParameter (ParamIDs::ivPan (i));
    prm->setValueNotifyingHost (prm->convertTo0to1 (0.0f));
    repaint();
}

//==============================================================================
String VoiceFuckerEditor::loadSetting (const String& key, const String& fallback)
{
    PropertiesFile f (settingsOptions());
    return f.getValue (key, fallback);
}

void VoiceFuckerEditor::saveSetting (const String& key, const String& value)
{
    PropertiesFile f (settingsOptions());
    f.setValue (key, value);
    f.saveIfNeeded();
}

void VoiceFuckerEditor::addCombo (Component& parent, ComboBox& box, const char* paramId, const StringArray& items,
                                  std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment>& att)
{
    box.addItemList (items, 1);
    box.setWantsKeyboardFocus (false);
    box.setMouseClickGrabsKeyboardFocus (false);
    parent.addAndMakeVisible (box);
    att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, paramId, box);
}

VoiceFuckerEditor::VoiceFuckerEditor (VoiceFuckerProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    using namespace ParamIDs;
    setLookAndFeel (&laf);
    addAndMakeVisible (content);

    content.addAndMakeVisible (presetBar);
    content.addAndMakeVisible (gear);
    gear.onClick = [this] { showSettingsMenu(); };
    content.addAndMakeVisible (scope);
    content.addAndMakeVisible (inCol);
    content.addAndMakeVisible (outCol);
    for (auto* b : { &carrierBlock, &vocBlock, &harmBlock, &dirtBlock, &compBlock, &widthBlock, &mixBlock, &clipBlock })
        content.addAndMakeVisible (*b);

    auto& apvts = proc.apvts;

    // Carrier
    addCombo (carrierBlock, sourceBox, carSrc, Tables::sources, sourceAtt);
    pitchSeg = std::make_unique<Segmented> (*apvts.getParameter (carPitch), Tables::pitchModes);
    carrierBlock.addAndMakeVisible (*pitchSeg);
    note.init (carrierBlock, apvts, "Note", carNote, false);
    glide.init (carrierBlock, apvts, "Glide", carGlide, false);
    detune.init (carrierBlock, apvts, "Detune", carDetune, false);
    spread.init (carrierBlock, apvts, "Spread", carSpread, false);
    noise.init (carrierBlock, apvts, "Noise", carNoise, false);

    // Vocoder
    bands.init (vocBlock, apvts, "Bands", vocBands, false);
    formant.init (vocBlock, apvts, "Formant", vocFormant, false);
    attack.init (vocBlock, apvts, "Attack", vocAttack, false);
    release.init (vocBlock, apvts, "Release", vocRelease, false);
    sib.init (vocBlock, apvts, "Sibilance", vocSib, false);
    gate.init (vocBlock, apvts, "Gate", vocGate, false);

    // Harmonize
    harmBlock.addAndMakeVisible (strips);
    scaleButton.setWantsKeyboardFocus (false);
    scaleButton.getProperties().set ("fontSize", 12.0);
    harmBlock.addAndMakeVisible (scaleButton);
    scaleAtt2 = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, harmScale, scaleButton);
    addCombo (harmBlock, rootBox, harmRoot, Tables::roots, rootAtt);
    addCombo (harmBlock, scaleBox, harmMode, Tables::scaleNames(), scaleAtt);
    panSeg = std::make_unique<Segmented> (*apvts.getParameter (harmPan), Tables::panModes, true);
    harmBlock.addAndMakeVisible (*panSeg);
    harmDry.init (harmBlock, apvts, "Dry", ParamIDs::harmDry, true);
    harmWet.init (harmBlock, apvts, "Wet", ParamIDs::harmWet, true);
    harmDetune.init (harmBlock, apvts, "Detune", ParamIDs::harmDetune, true);
    harmSpread.init (harmBlock, apvts, "Spread", ParamIDs::harmSpread, true);
    harmSpread.slider.onValueChange = [this] { strips.repaint(); };

    // Dirt
    addCombo (dirtBlock, dirtModeBox, ParamIDs::dirtMode, Tables::dirtModes, dirtModeAtt);
    drive.init (dirtBlock, apvts, "Drive", dirtDrive, true);
    crunch.init (dirtBlock, apvts, "Crunch", dirtCrunch, true);
    destroy.init (dirtBlock, apvts, "Destroy", dirtDestroy, true);

    // Comp
    addCombo (compBlock, compModeBox, ParamIDs::compMode, Tables::compModes, compModeAtt);
    ottDepth.init (compBlock, apvts, "Depth", ParamIDs::ottDepth, true);
    ottTime.init (compBlock, apvts, "Time", ParamIDs::ottTime, true);
    ottUp.init (compBlock, apvts, "Up", ParamIDs::ottUp, true);
    ottDown.init (compBlock, apvts, "Down", ParamIDs::ottDown, true);
    glueThresh.init (compBlock, apvts, "Thresh", ParamIDs::glueThresh, true);
    glueRatio.init (compBlock, apvts, "Ratio", ParamIDs::glueRatio, true);
    glueAttack.init (compBlock, apvts, "Attack", ParamIDs::glueAttack, true);
    glueRelease.init (compBlock, apvts, "Release", ParamIDs::glueRelease, true);
    compMix.init (compBlock, apvts, "Mix", ParamIDs::compMix, true);

    // Width
    widthSeg = std::make_unique<Segmented> (*apvts.getParameter (widthMode), Tables::widthModes, true);
    widthBlock.addAndMakeVisible (*widthSeg);
    width.init (widthBlock, apvts, "Width", widthAmt, true);
    mono.init (widthBlock, apvts, "Mono bass", widthMono, true);

    // Mix
    autoGainButton.setWantsKeyboardFocus (false);
    autoGainButton.getProperties().set ("fontSize", 12.0);
    mixBlock.addAndMakeVisible (autoGainButton);
    autoGainAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (apvts, ParamIDs::autoGain, autoGainButton);
    dryGain.init (mixBlock, apvts, "Dry", ParamIDs::dryGain, true);
    wetGain.init (mixBlock, apvts, "Wet", ParamIDs::wetGain, true);

    // Clip
    clipMode = std::make_unique<Segmented> (*apvts.getParameter (ParamIDs::clipMode), Tables::clipModes, true);
    clipBlock.addAndMakeVisible (*clipMode);
    ceiling.init (clipBlock, apvts, "Ceiling", ParamIDs::ceiling, true);
    for (auto* k : { &ottDepth, &ottTime, &ottUp, &ottDown, &glueThresh, &glueRatio, &glueAttack, &glueRelease, &compMix })
        k->slider.onDragStart = [this] { if (compBlock.isOn()) showComp(); };
    ceiling.slider.onDragStart = [this] { scope.touchCeiling (true); };
    ceiling.slider.onDragEnd = [this] { scope.touchCeiling (false); };
    ceiling.slider.onValueChange = [this] { if (ceiling.slider.isMouseOverOrDragging()) scope.touchCeiling (ceiling.slider.isMouseButtonDown()); };

    // Overlays
    content.addChildComponent (about);
    content.addChildComponent (tour);
    content.addChildComponent (welcome);
    tour.resolveTarget = [this] (const String& name) { return tourTarget (name); };
    tour.onStep = [this] (const String& name)
    {
        if (name == "pills") scope.setView (ScopeView::View::eq);
        else if (name == "scope") scope.setView (ScopeView::View::wave);
        else if (name == "vocoder") scope.setView (ScopeView::View::spectrum);
    };
    tour.onFinish = [] { saveSetting ("tour", "never"); };
    welcome.onLanguage = [this] (Help::Lang l)
    {
        lang = l;
        saveSetting ("helpLang", l == Help::Lang::ru ? "ru" : "en");
        applyHelp();
    };
    welcome.onChoice = [this] (int choice)
    {
        welcome.setVisible (false);
        if (choice == 0) tour.start (lang);
        else if (choice == 2) saveSetting ("tour", "never");
    };

    scope.onBarsChanged = [] (int i) { saveSetting ("bars", String (i)); };
    scope.setBarsIndex (loadSetting ("bars", "1").getIntValue());
    scope.onViewChanged = [] (ScopeView::View v) { saveSetting ("view", String ((int) v)); };
    scope.setView ((ScopeView::View) jlimit (0, 2, loadSetting ("view", "0").getIntValue()));

    helpOn = loadSetting ("help", "on") != "off";
    lang = loadSetting ("helpLang", "en") == "ru" ? Help::Lang::ru : Help::Lang::en;
    applyTheme (Theme::byName (loadSetting ("theme", "Asphalt")), false);
    welcome.setLang (lang);
    applyHelp();
    // size: this plugin instance first, then the last size used anywhere, then 100 %
    int w = proc.editorW.load(), h = proc.editorH.load();
    if (w <= 0 || h <= 0)
    {
        w = loadSetting ("width", "0").getIntValue();
        h = loadSetting ("height", "0").getIntValue();
    }
    if (w <= 0 || h <= 0)
    {
        const float sc = jlimit (0.6f, 2.0f, loadSetting ("scale", "1.0").getFloatValue());
        w = roundToInt (baseW * sc); h = roundToInt (baseH * sc);
    }
    setResizable (true, true);
    setResizeLimits ((int) (baseW * 0.55f), (int) (baseH * 0.55f), baseW * 3, baseH * 3);   // any shape
    setSize (jlimit ((int) (baseW * 0.55f), baseW * 3, w), jlimit ((int) (baseH * 0.55f), baseH * 3, h));
    sizeReady = true;

    if (loadSetting ("tour", "") != "never" && ! askedThisRun)
    {
        askedThisRun = true;
        showWelcome();
    }
    timerCallback();
    startTimerHz (30);
}

VoiceFuckerEditor::~VoiceFuckerEditor()
{
    stopTimer();
    tooltipWindow.reset();
    setLookAndFeel (nullptr);
}

//==============================================================================
void VoiceFuckerEditor::resized()
{
    if (getWidth() < 10 || getHeight() < 10) return;
    // uniform scale (knobs never stretch); the extra room of a wider or taller window goes to the screen and the gaps
    scale = jmin ((float) getWidth() / (float) baseW, (float) getHeight() / (float) baseH);
    logicalW = jmax (baseW, roundToInt ((float) getWidth() / scale));
    logicalH = jmax (baseH, roundToInt ((float) getHeight() / scale));
    content.setTransform (AffineTransform::scale (scale));
    content.setBounds (0, 0, logicalW, logicalH);
    if (sizeReady)
    {
        proc.editorW = getWidth(); proc.editorH = getHeight();
        saveSetting ("width", String (getWidth()));
        saveSetting ("height", String (getHeight()));
    }

    const int dx = logicalW - baseW, dy = logicalH - baseH, x1 = X1 + dx;
    presetBar.setBounds (466 + dx / 2, 24, 330, 30);
    gear.setBounds (x1 - 40, 24, 40, 30);
    scope.setBounds (X0, scopeY, x1 - X0, scopeH + dy);
    inCol.setBounds (10, 10, 72, logicalH - 20);
    outCol.setBounds (logicalW - 82, 10, 72, logicalH - 20);
    about.setBounds (0, 0, logicalW, logicalH);
    tour.setBounds (0, 0, logicalW, logicalH);
    welcome.setBounds (0, 0, logicalW, logicalH);

    carrierBlock.setBounds (X0, row1Y + dy, carrierW + dx / 2, row1H);
    vocBlock.setBounds (x1 - vocW - dx / 2, row1Y + dy, vocW + dx / 2, row1H);
    harmBlock.setBounds (X0, harmY + dy, x1 - X0, harmH);
    int x = X0;
    const int gap = row3Gap + dx / 4;
    for (auto [blk, w] : { std::pair<Block*, int> { &dirtBlock, dirtW }, { &compBlock, compW }, { &widthBlock, widthW }, { &mixBlock, mixW }, { &clipBlock, clipW } })
    {
        blk->setBounds (x, row3Y + dy, w, row3H);
        x += w + gap;
    }
    if (dx > 0) clipBlock.setBounds (x1 - clipW, row3Y + dy, clipW, row3H);

    // Carrier / Vocoder
    // knobs spread evenly over the width a block has (wider window -> more air between them)
    auto spreadKnobs = [] (std::initializer_list<Knob*> knobs, int x0, int width, int y, int kw)
    {
        const int n = (int) knobs.size();
        const float step = (float) (width - kw) / (float) jmax (1, n - 1);
        int i = 0;
        for (auto* k : knobs) k->setBounds (x0 + roundToInt (step * (float) i++), y, kw);
    };
    const int carW = carrierBlock.getWidth(), vW = vocBlock.getWidth();
    sourceBox.setBounds (14, 32, 140 + (carW - carrierW) / 3, 24);
    pitchSeg->setBounds (sourceBox.getRight() + 10, 32, carW - 14 - sourceBox.getRight() - 10, 24);
    spreadKnobs ({ &note, &glide, &detune, &spread, &noise }, 14, carW - 28 - (carrierW - 28 - 400), 62, 80);
    spreadKnobs ({ &bands, &formant, &attack, &release, &sib, &gate }, 14, 480 + (vW - vocW), 62, 80);

    // Harmonize: the faders keep their spacing, the extra room is shared out
    const int hx = (harmBlock.getWidth() - (X1 - X0)) / 2;
    strips.setBounds (10 + hx / 2, 30, 600, harmH - 36);
    const int rx = 628 + hx, rw = 328 + hx;
    scaleButton.setBounds (rx, 32, 76, 24);
    rootBox.setBounds (rx + 80, 32, 66, 24);
    scaleBox.setBounds (rx + 152, 32, rw - 152, 24);
    panSeg->setBounds (rx, 66, rw, 24);
    spreadKnobs ({ &harmDry, &harmWet, &harmDetune, &harmSpread }, rx + 6, rw - 12, 98, 58);

    // Row 3
    const int cy = 32, ky = 60;
    dirtModeBox.setBounds (10, cy, 110, 22);
    drive.setBounds (11, ky, 58); crunch.setBounds (69, ky, 58); destroy.setBounds (127, ky, 58);
    compModeBox.setBounds (10, cy, 92, 22);
    Knob* ott[4] = { &ottDepth, &ottTime, &ottUp, &ottDown };
    Knob* glue[4] = { &glueThresh, &glueRatio, &glueAttack, &glueRelease };
    for (int i = 0; i < 4; ++i) { ott[i]->setBounds (11 + i * 58, ky, 58); glue[i]->setBounds (11 + i * 58, ky, 58); }
    compMix.setBounds (11 + 4 * 58, ky, 58);
    widthSeg->setBounds (10, cy, widthW - 20, 22);
    width.setBounds (18, ky, 58); mono.setBounds (94, ky, 58);
    autoGainButton.setBounds (10, cy, mixW - 20, 22);
    dryGain.setBounds (8, ky, 58); wetGain.setBounds (70, ky, 58);
    clipMode->setBounds (10, cy, clipW - 20, 22);
    ceiling.setBounds ((clipW - 58) / 2, ky, 58);
}

void VoiceFuckerEditor::showComp()
{
    if (roundToInt (proc.apvts.getRawParameterValue (ParamIDs::compMode)->load()) == 1)
        scope.setView (ScopeView::View::wave);
    else
        scope.showOtt();
}

void VoiceFuckerEditor::timerCallback()
{
    using namespace ParamIDs;
    auto raw = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };

    // Comp switched on or to another mode: Glue is set up on the waveform, OTT on its band view
    {
        const int cOn = raw (compOn) > 0.5f ? 1 : 0, cMode = roundToInt (raw (compMode));
        if (lastCompOn >= 0 && cOn == 1 && (cOn != lastCompOn || cMode != lastCompMode))
            showComp();
        lastCompOn = cOn;
        lastCompMode = cMode;
    }
    inCol.push (proc.inPeak[0].exchange (0.0f), proc.inPeak[1].exchange (0.0f));
    outCol.push (proc.outPeak[0].exchange (0.0f), proc.outPeak[1].exchange (0.0f));

    const bool glueMode = roundToInt (raw (ParamIDs::compMode)) == 1;
    for (auto* k : { &ottDepth, &ottTime, &ottUp, &ottDown }) k->setVisible (! glueMode);
    for (auto* k : { &glueThresh, &glueRatio, &glueAttack, &glueRelease }) k->setVisible (glueMode);

    auto dim = [] (std::initializer_list<Knob*> knobs, std::initializer_list<Component*> others, bool off)
    {
        for (auto* k : knobs) k->setDimmed (off);
        for (auto* c : others) c->setAlpha (off ? 0.4f : 1.0f);
    };
    const bool vocOff = ! vocBlock.isOn();
    dim ({ &bands, &formant, &attack, &release, &sib, &gate }, {}, vocOff);
    const int src = roundToInt (raw (carSrc));
    const bool synthless = src == 4 || vocOff;
    const int pm = roundToInt (raw (carPitch));
    note.setDimmed (synthless || pm != 2 || src == 3);
    glide.setDimmed (synthless || src == 3 || pm == 2);
    detune.setDimmed (synthless || src == 3);
    spread.setDimmed (vocOff);
    noise.setDimmed (vocOff);
    sourceBox.setAlpha (vocOff ? 0.4f : 1.0f);
    pitchSeg->setAlpha (vocOff || src == 3 || src == 4 ? 0.4f : 1.0f);
    const bool harmOff = ! harmBlock.isOn();
    dim ({ &harmDry, &harmWet, &harmDetune, &harmSpread }, { &scaleButton, &rootBox, &scaleBox, panSeg.get() }, harmOff);
    rootBox.setAlpha (harmOff || raw (harmScale) < 0.5f ? 0.4f : 1.0f);
    scaleBox.setAlpha (rootBox.getAlpha());
    dim ({ &drive, &crunch, &destroy }, { &dirtModeBox }, ! dirtBlock.isOn());
    dim ({ &ottDepth, &ottTime, &ottUp, &ottDown, &glueThresh, &glueRatio, &glueAttack, &glueRelease, &compMix }, { &compModeBox }, ! compBlock.isOn());
    dim ({ &width, &mono }, { widthSeg.get() }, ! widthBlock.isOn());
    dim ({ &ceiling }, { clipMode.get() }, ! clipBlock.isOn());
}

//==============================================================================
void VoiceFuckerEditor::applyTheme (const Theme& t, bool remember)
{
    theme = &t;
    laf.setTheme (t);
    if (remember) saveSetting ("theme", t.name);
    buildBackground();
    content.sendLookAndFeelChange();
    for (auto* k : { &note, &glide, &detune, &spread, &noise, &bands, &formant, &attack, &release, &sib, &gate,
                     &harmDry, &harmWet, &harmDetune, &harmSpread, &drive, &crunch, &destroy, &ottDepth, &ottTime, &ottUp, &ottDown,
                     &glueThresh, &glueRatio, &glueAttack, &glueRelease, &compMix, &width, &mono, &dryGain, &wetGain, &ceiling,
                     &inCol.knob, &outCol.knob })
        k->label.setColour (Label::textColourId, t.mute);
    welcome.setLang (lang);
    content.repaint();
}

void VoiceFuckerEditor::applyHelp()
{
    if (helpOn && tooltipWindow == nullptr)
    {
        tooltipWindow = std::make_unique<TooltipWindow> (this, 600);
        tooltipWindow->setLookAndFeel (&laf);
    }
    else if (! helpOn)
        tooltipWindow.reset();

    auto tip = [this] (const char* key) { return helpOn ? Help::tip (key, lang) : String(); };
    inCol.knob.setTooltip (tip ("input"));
    outCol.knob.setTooltip (tip ("output"));
    inCol.setTooltip (tip ("inMeter"));
    outCol.setTooltip (tip ("outMeter"));
    presetBar.setTooltip (tip ("presetName"));
    presetBar.prev.setTooltip (tip ("prev"));
    presetBar.next.setTooltip (tip ("next"));
    presetBar.save.setTooltip (tip ("save"));
    gear.setTooltip (tip ("gear"));
    for (auto* b : { &dirtBlock, &compBlock, &widthBlock, &clipBlock })
        if (auto* pw = b->getPowerButton()) pw->setTooltip (tip ("power"));
    if (auto* pw = vocBlock.getPowerButton()) pw->setTooltip (tip ("vocPower"));
    if (auto* pw = harmBlock.getPowerButton()) pw->setTooltip (tip ("harmPower"));

    sourceBox.setTooltip (tip ("source"));
    pitchSeg->setTooltip (tip ("pitch"));
    note.setTooltip (tip ("note")); glide.setTooltip (tip ("glide")); detune.setTooltip (tip ("detune"));
    spread.setTooltip (tip ("spread")); noise.setTooltip (tip ("noise"));
    bands.setTooltip (tip ("bands")); formant.setTooltip (tip ("formant")); attack.setTooltip (tip ("attack"));
    release.setTooltip (tip ("release")); sib.setTooltip (tip ("sibilance")); gate.setTooltip (tip ("gate"));
    scaleButton.setTooltip (tip ("scaleOn")); rootBox.setTooltip (tip ("root")); scaleBox.setTooltip (tip ("scale"));
    panSeg->setTooltip (tip ("panMode"));
    harmDry.setTooltip (tip ("harmDry")); harmWet.setTooltip (tip ("harmWet")); harmDetune.setTooltip (tip ("harmDetune")); harmSpread.setTooltip (tip ("harmSpread"));
    strips.setHelp (helpOn, lang);
    dirtModeBox.setTooltip (tip ("dirtMode"));
    drive.setTooltip (tip ("drive")); crunch.setTooltip (tip ("crunch")); destroy.setTooltip (tip ("destroy"));
    compModeBox.setTooltip (tip ("compMode"));
    ottDepth.setTooltip (tip ("ottDepth")); ottTime.setTooltip (tip ("ottTime")); ottUp.setTooltip (tip ("ottUp")); ottDown.setTooltip (tip ("ottDown"));
    glueThresh.setTooltip (tip ("glueThresh")); glueRatio.setTooltip (tip ("glueRatio"));
    glueAttack.setTooltip (tip ("glueAttack")); glueRelease.setTooltip (tip ("glueRelease")); compMix.setTooltip (tip ("compMix"));
    widthSeg->setTooltip (tip ("widthMode")); width.setTooltip (tip ("width")); mono.setTooltip (tip ("monoBass"));
    autoGainButton.setTooltip (tip ("autoGain")); dryGain.setTooltip (tip ("dryGain")); wetGain.setTooltip (tip ("wetGain"));
    clipMode->setTooltip (tip ("clipMode")); ceiling.setTooltip (tip ("ceiling"));
    scope.setHelp (helpOn, lang);
    timerCallback();
}

void VoiceFuckerEditor::setScale (float s, bool remember)
{
    setSize (roundToInt (baseW * s), roundToInt (baseH * s));
    if (remember) saveSetting ("scale", String (s, 3));
}

void VoiceFuckerEditor::showSettingsMenu()
{
    PopupMenu themes, sizes, help, langs, menu;
    int id = 1;
    for (const auto& t : Theme::all())
        themes.addItem (id++, t.name, true, &t == theme);
    const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f, 2.0f };
    for (int i = 0; i < 5; ++i)
        sizes.addItem (100 + i, String (roundToInt (scales[i] * 100)) + "%", true, std::abs (scale - scales[i]) < 0.01f);
    help.addItem (200, "On", true, helpOn);
    help.addItem (201, "Off", true, ! helpOn);
    langs.addItem (300, "English", true, lang == Help::Lang::en);
    langs.addItem (301, String::fromUTF8 ("Русский"), true, lang == Help::Lang::ru);
    menu.addSubMenu ("Theme", themes);
    menu.addSubMenu ("Size", sizes);
    menu.addSubMenu ("Help", help);
    menu.addSubMenu ("Help language", langs);
    menu.addSeparator();
    menu.addItem (400, "Start tour");
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&gear), [this, scales] (int r)
    {
        if (r >= 1 && r <= (int) Theme::all().size()) applyTheme (Theme::all()[(size_t) (r - 1)], true);
        else if (r >= 100 && r < 105) setScale (scales[r - 100], true);
        else if (r == 200 || r == 201) { helpOn = r == 200; saveSetting ("help", helpOn ? "on" : "off"); applyHelp(); }
        else if (r == 300 || r == 301)
        {
            lang = r == 301 ? Help::Lang::ru : Help::Lang::en;
            saveSetting ("helpLang", r == 301 ? "ru" : "en");
            welcome.setLang (lang);
            applyHelp();
        }
        else if (r == 400) tour.start (lang);
    });
}

void VoiceFuckerEditor::Content::mouseDown (const MouseEvent& e)
{
    if (titleArea.contains (e.getPosition()))
    {
        editor.about.setVisible (true);
        editor.about.toFront (false);
    }
}

Rectangle<float> VoiceFuckerEditor::tourTarget (const String& name)
{
    auto area = [this] (Component& c) { return content.getLocalArea (&c, c.getLocalBounds()).toFloat(); };
    if (name == "inCol")     return area (inCol);
    if (name == "outCol")    return area (outCol);
    if (name == "presets")   return area (presetBar);
    if (name == "gear")      return area (gear);
    if (name == "scope")     return area (scope);
    if (name == "pills")     return content.getLocalArea (&scope, scope.pillsArea()).toFloat();
    if (name == "noteTag")   return content.getLocalArea (&scope, scope.noteArea().expanded (6, 3)).toFloat();
    if (name == "carrier")   return area (carrierBlock);
    if (name == "midi")      return area (sourceBox).getUnion (area (*pitchSeg)).expanded (6.0f);
    if (name == "vocoder")   return area (vocBlock);
    if (name == "harmonize") return area (harmBlock);
    if (name == "harmScale") return area (scaleButton).getUnion (area (scaleBox)).expanded (6.0f);
    if (name == "dirt")      return area (dirtBlock);
    if (name == "comp")      return area (compBlock);
    if (name == "width")     return area (widthBlock);
    if (name == "mix")       return area (mixBlock);
    if (name == "clip")      return area (clipBlock);
    return {};
}

//==============================================================================
void VoiceFuckerEditor::buildBackground()
{
    const auto& t = *theme;
    const int s = 2, W = baseW * s, H = baseH * s;
    background = Image (Image::RGB, W, H, false);
    if (t.gloss)
    {
        Graphics g (background);
        ColourGradient bg (Colour (0xff1c1c21), 0.0f, 0.0f, Colour (0xff141418), 0.0f, (float) H, false);
        bg.addColour (0.45, Colour (0xff0b0b0d));
        g.setGradientFill (bg);
        g.fillAll();
        struct Sheen { float x, w, a; };
        for (const auto& sh : { Sheen { 0.18f, 0.22f, 0.07f }, Sheen { 0.62f, 0.08f, 0.05f }, Sheen { 0.80f, 0.30f, 0.04f } })
        {
            const float x0 = (float) W * sh.x, w0 = (float) W * sh.w;
            ColourGradient sg (Colours::white.withAlpha (0.0f), x0 - w0, 0.0f, Colours::white.withAlpha (0.0f), x0 + w0, (float) H * 0.3f, false);
            sg.addColour (0.5, Colour ((uint8) 255, 248, 230, sh.a));
            g.setGradientFill (sg);
            Path p;
            p.startNewSubPath (x0 - w0, 0.0f); p.lineTo (x0 + w0, 0.0f);
            p.lineTo (x0 + w0 - (float) W * 0.25f, (float) H); p.lineTo (x0 - w0 - (float) W * 0.25f, (float) H);
            p.closeSubPath();
            g.fillPath (p);
        }
        return;
    }
    Random rnd (1234);
    constexpr int gw = 30, gh = 22;
    float coarse[gw * gh];
    for (auto& v : coarse) v = rnd.nextFloat() * 14.0f - 7.0f;
    {
        Image::BitmapData bd (background, Image::BitmapData::writeOnly);
        const float br = t.bg.getFloatRed() * 255.0f, bgc = t.bg.getFloatGreen() * 255.0f, bb = t.bg.getFloatBlue() * 255.0f;
        auto ch = [] (float f) { return (uint8) jlimit (0, 255, roundToInt (f)); };
        for (int y = 0; y < H; ++y)
        {
            const float fy = (float) y / (float) (H - 1) * (gh - 1);
            const int y0 = jmin (gh - 2, (int) fy);
            const float ty = fy - (float) y0;
            for (int x = 0; x < W; ++x)
            {
                const float fx = (float) x / (float) (W - 1) * (gw - 1);
                const int x0 = jmin (gw - 2, (int) fx);
                const float tx = fx - (float) x0;
                const float c = coarse[y0 * gw + x0] * (1 - tx) * (1 - ty) + coarse[y0 * gw + x0 + 1] * tx * (1 - ty)
                              + coarse[(y0 + 1) * gw + x0] * (1 - tx) * ty + coarse[(y0 + 1) * gw + x0 + 1] * tx * ty;
                const float nz = c + (rnd.nextFloat() - 0.5f) * 30.0f;
                if (rnd.nextFloat() < 0.004f) bd.setPixelColour (x, y, t.bg.interpolatedWith (t.speck, 0.4f + rnd.nextFloat() * 0.6f));
                else bd.setPixelColour (x, y, Colour (ch (br + nz), ch (bgc + nz), ch (bb + nz * 1.1f)));
            }
        }
    }
    Graphics g (background);
    ColourGradient sheen (Colour (0xffb4c4dc).withAlpha (t.lightBg ? 0.10f : 0.06f), (float) W * 0.7f, (float) H * 0.12f,
                          Colours::transparentWhite, (float) W * 1.05f, (float) H * 0.12f, true);
    g.setGradientFill (sheen);
    g.fillEllipse ((float) W * 0.35f, 0.0f, (float) W * 0.7f, (float) H * 0.3f);
    ColourGradient vig (Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.45f, t.vignette, 0.0f, 0.0f, true);
    g.setGradientFill (vig);
    g.fillRect (0, 0, W, H);
}

void VoiceFuckerEditor::paintContent (Graphics& g)
{
    const auto& t = *theme;
    g.drawImage (background, Rectangle<float> (0.0f, 0.0f, (float) logicalW, (float) logicalH));
    const auto la = logoArea.toFloat();
    if (t.gloss)
    {
        ColourGradient gold (Colour (0xfff6dc9a), la.getX(), la.getY(), Colour (0xffa47a2c), la.getX(), la.getBottom(), false);
        gold.addColour (0.55, Colour (0xffe8c46a));
        Logo::draw (g, la, FillType (gold));
    }
    else
        Logo::draw (g, la, FillType (t.logo));
    g.setColour (t.ink);
    g.setFont (uiFont (30.0f, true, 0.08f));
    g.drawText ("VOICE FUCKER", logoArea.getRight() + 12, 12, 360, 34, Justification::centredLeft);
    g.setColour (t.mute);
    g.setFont (uiFont (11.0f, true, 0.3f));
    g.drawText ("PRIMAL RAGE LAB", logoArea.getRight() + 14, 46, 320, 16, Justification::centredLeft);

    // signal path arrows in the bottom row
    auto arrow = [&] (const Component& a, const Component& b)
    {
        const float x0 = (float) a.getRight(), x1 = (float) b.getX();
        if (x1 - x0 < 8.0f) return;
        const float cx = 0.5f * (x0 + x1), cy = (float) a.getBounds().getCentreY();
        const float half = jmin (7.0f, (x1 - x0) * 0.35f);
        Path p;
        p.startNewSubPath (cx - half, cy); p.lineTo (cx + half, cy);
        p.startNewSubPath (cx + half - 4.0f, cy - 4.0f); p.lineTo (cx + half, cy); p.lineTo (cx + half - 4.0f, cy + 4.0f);
        g.setColour (t.accent.withAlpha (0.65f));
        g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    };
    arrow (carrierBlock, vocBlock);
    const Component* row3[5] = { &dirtBlock, &compBlock, &widthBlock, &mixBlock, &clipBlock };
    for (int i = 0; i < 4; ++i) arrow (*row3[i], *row3[i + 1]);
}
