#include "PluginEditor.h"

using namespace ui;

namespace
{
    float toDb (float g) { return 20.f * std::log10 (std::max (g, 1.0e-6f)); }

    void drawChevron (juce::Graphics& g, float cx, float cy, bool left, juce::Colour c)
    {
        juce::Path p;
        const float d = left ? 1.f : -1.f;
        p.startNewSubPath (cx + 2.5f * d, cy - 5.f); p.lineTo (cx - 2.5f * d, cy); p.lineTo (cx + 2.5f * d, cy + 5.f);
        g.setColour (c);
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
// CrtScreen
//==============================================================================
CrtScreen::CrtScreen (VoiceFuckerProcessor& p) : proc (p)
{
    snap.resize (2048);
    startTimerHz (40);
}

void CrtScreen::timerCallback()
{
    const int n = proc.numBandsUi.load();
    for (int i = 0; i < n; ++i)
    {
        const float v = proc.bandLevel[(size_t) i].exchange (0.f);
        const float db = toDb (v * 2.5f);
        const float norm = juce::jlimit (0.f, 1.f, (db + 60.f) / 58.f);
        spec[(size_t) i] = std::max (norm, spec[(size_t) i] * 0.84f);
        hold[(size_t) i] = std::max (spec[(size_t) i], hold[(size_t) i] - 0.008f);
    }
    repaint();
}

juce::Point<float> CrtScreen::bulge (float x, float y) const
{
    const auto c = glass.getCentre();
    const float u = (x - c.x) / (glass.getWidth() * 0.52f), v = (y - c.y) / (glass.getHeight() * 0.58f);
    const float k = 1.f + 0.06f * (u * u + v * v);
    return { c.x + (x - c.x) / k, c.y + (y - c.y) / k };
}

juce::Path CrtScreen::glassPath (juce::Rectangle<float> r) const
{
    const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight(), bx = 10.f, by = 6.f;
    juce::Path p;
    p.startNewSubPath (x + 22, y - by);
    p.quadraticTo (x + w / 2, y - by * 2.2f, x + w - 22, y - by);
    p.quadraticTo (x + w + bx, y - by + 4, x + w + bx, y + 20);
    p.quadraticTo (x + w + bx * 1.8f, y + h / 2, x + w + bx, y + h - 20);
    p.quadraticTo (x + w + bx, y + h + by - 4, x + w - 22, y + h + by);
    p.quadraticTo (x + w / 2, y + h + by * 2.2f, x + 22, y + h + by);
    p.quadraticTo (x - bx, y + h + by - 4, x - bx, y + h - 20);
    p.quadraticTo (x - bx * 1.8f, y + h / 2, x - bx, y + 20);
    p.quadraticTo (x - bx, y - by + 4, x + 22, y - by);
    p.closeSubPath();
    return p;
}

juce::String CrtScreen::noteText() const
{
    const int mode = (int) proc.apvts.getRawParameterValue ("car_pitch")->load();
    const int src = (int) proc.apvts.getRawParameterValue ("car_src")->load();
    if (src == 3) return "NOISE";
    if (src == 4) return "SIDECHAIN";
    if (mode == 2) return "FIXED  " + Params::noteName ((int) proc.apvts.getRawParameterValue ("car_note")->load());
    if (mode == 1)
    {
        const float hz = proc.uiAutoHz.load();
        return hz > 0 ? "AUTO  " + Params::noteName (juce::roundToInt (vf::hzToMidi (hz))) : juce::String ("AUTO  --");
    }
    juce::Array<int> notes;
    for (auto& n : proc.uiNotes) if (n.load() >= 0) notes.addIfNotAlreadyThere (n.load());
    notes.sort();
    juce::String s ("MIDI ");
    if (notes.isEmpty()) return s + " --";
    for (int i = 0; i < std::min (4, notes.size()); ++i) s << " " << Params::noteName (notes[i]);
    if (notes.size() > 4) s << " +";
    return s;
}

void CrtScreen::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    // TV body
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a272b), 0, 0, juce::Colour (0xff0c0b0d), 0, b.getHeight(), false));
    g.fillRoundedRectangle (b, 22.f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (b.reduced (0.5f), 22.f, 1.f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (b.reduced (8.f, 6.f), 18.f, 3.f);

    glass = { 32.f, 16.f, b.getWidth() - 64.f, b.getHeight() - 30.f };
    const auto gp = glassPath (glass);
    const auto c = glass.getCentre();

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (gp);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1418), c.x, c.y, juce::Colour (0xff030203), c.x + 380.f, c.y, true));
        g.fillPath (gp);

        // grid
        g.setColour (col::pink.withAlpha (0.07f));
        for (int i = 1; i < 8; ++i)
        {
            juce::Path p; const float x = glass.getX() + glass.getWidth() * (float) i / 8.f;
            for (int j = 0; j <= 30; ++j) { auto q = bulge (x, glass.getY() + glass.getHeight() * (float) j / 30.f); j ? p.lineTo (q) : p.startNewSubPath (q); }
            g.strokePath (p, juce::PathStrokeType (1.f));
        }
        for (int i = 1; i < 4; ++i)
        {
            juce::Path p; const float y = glass.getY() + glass.getHeight() * (float) i / 4.f;
            for (int j = 0; j <= 50; ++j) { auto q = bulge (glass.getX() + glass.getWidth() * (float) j / 50.f, y); j ? p.lineTo (q) : p.startNewSubPath (q); }
            g.strokePath (p, juce::PathStrokeType (1.f));
        }

        const float x0 = glass.getX() + 20.f, x1 = glass.getRight() - 20.f;
        if (proc.screenMode == 0)
        {
            // waveform, triggered on a rising zero crossing
            const int w = proc.waveWrite.load();
            const int N = (int) snap.size();
            for (int i = 0; i < N; ++i) snap[(size_t) i] = proc.wave[(size_t) ((w - N + i) & (VoiceFuckerProcessor::waveSize - 1))];
            int trig = 0;
            for (int i = N / 2 - 1; i > 1; --i) if (snap[(size_t) i - 1] < 0.f && snap[(size_t) i] >= 0.f) { trig = i; break; }
            const int len = N / 2;
            float pk = 0.05f;
            for (int i = 0; i < len; ++i) pk = std::max (pk, std::abs (snap[(size_t) (trig + i)]));
            const float gain = std::min (1.f / pk, 6.f) * 0.85f;
            juce::Path p;
            const int pts = 340;
            for (int i = 0; i <= pts; ++i)
            {
                const int si = trig + (int) ((float) i / (float) pts * (float) (len - 1));
                const float v = juce::jlimit (-1.f, 1.f, snap[(size_t) si] * gain);
                auto q = bulge (x0 + (x1 - x0) * (float) i / (float) pts, c.y - v * glass.getHeight() * 0.4f);
                i ? p.lineTo (q) : p.startNewSubPath (q);
            }
            g.setColour (col::pink.withAlpha (0.12f));
            g.strokePath (p, juce::PathStrokeType (9.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (col::pink.withAlpha (0.3f));
            g.strokePath (p, juce::PathStrokeType (4.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (juce::Colour (0xffffe0ee));
            g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        else
        {
            const int n = proc.numBandsUi.load();
            const float bw = (x1 - x0) / (float) n;
            const float base = glass.getBottom() - 14.f, hMax = glass.getHeight() - 34.f;
            for (int i = 0; i < n; ++i)
            {
                const float h = spec[(size_t) i] * hMax;
                const float bx = x0 + bw * (float) i;
                auto p1 = bulge (bx + 1.f, base - h), p2 = bulge (bx + bw - 1.f, base);
                juce::ColourGradient grad (juce::Colour (0xffffd0e4), 0, p1.y, juce::Colour (0x99781e46), 0, p2.y, false);
                grad.addColour (0.25, col::pink);
                g.setGradientFill (grad);
                g.fillRect (juce::Rectangle<float> (p1, p2));
                auto ph = bulge (bx + 1.f, base - hold[(size_t) i] * hMax);
                g.setColour (juce::Colour (0xffffd0e4));
                g.fillRect (ph.x, ph.y, p2.x - p1.x, 1.5f);
            }
            g.setColour (juce::Colours::white.withAlpha (0.45f));
            g.setFont (font (10.f));
            const float fLo = 90.f, fHi = 11000.f;
            for (float f : { 100.f, 1000.f, 10000.f })
            {
                const float fr = std::log (f / fLo) / std::log (fHi / fLo);
                auto q = bulge (x0 + (x1 - x0) * fr, glass.getBottom() - 2.f);
                g.drawText (f >= 1000.f ? juce::String ((int) (f / 1000.f)) + "k" : juce::String ((int) f), (int) q.x - 20, (int) q.y - 10, 40, 12, juce::Justification::centred);
            }
        }

        // scanlines
        g.setColour (juce::Colours::black.withAlpha (0.22f));
        for (float y = glass.getY() - 10.f; y < glass.getBottom() + 12.f; y += 3.f) g.fillRect (glass.getX() - 12.f, y, glass.getWidth() + 24.f, 1.2f);

        // vignette (elliptic)
        {
            juce::Graphics::ScopedSaveState s2 (g);
            g.addTransform (juce::AffineTransform::scale (1.f, glass.getHeight() / glass.getWidth() * 1.5f, c.x, c.y));
            juce::ColourGradient vg (juce::Colours::transparentBlack, c.x, c.y, juce::Colours::black.withAlpha (0.85f), c.x + glass.getWidth() * 0.62f, c.y, true);
            vg.addColour (0.72, juce::Colours::black.withAlpha (0.2f));
            g.setGradientFill (vg);
            g.fillRect (glass.expanded (20.f, 400.f));
        }
        // glass highlight
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.13f), 0, glass.getY() - 8.f, juce::Colours::white.withAlpha (0.f), 0, glass.getY() + 56.f, false));
        g.fillEllipse (c.x - 340.f, glass.getY() - 38.f, 600.f, 92.f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.strokePath (gp, juce::PathStrokeType (1.2f));

    // Wave / Spectrum switch
    toggleArea = { glass.getX() + 12.f, glass.getY() + 8.f, 128.f, 24.f };
    g.setColour (juce::Colour (0xe6141215));
    g.fillRoundedRectangle (toggleArea, 6.f);
    const char* names[2] = { "Wave", "Spectrum" };
    const float ws[2] = { 52.f, 76.f };
    float tx = toggleArea.getX();
    for (int i = 0; i < 2; ++i)
    {
        auto r = juce::Rectangle<float> (tx, toggleArea.getY(), ws[i], 24.f);
        if (i == proc.screenMode) { g.setColour (col::pink); g.fillRoundedRectangle (r, 6.f); }
        g.setColour (i == proc.screenMode ? juce::Colours::white : juce::Colour (0xffbdb5ba));
        g.setFont (font (11.5f));
        g.drawText (names[i], r, juce::Justification::centred);
        tx += ws[i];
    }

    // pitch source readout
    const auto txt = noteText();
    g.setFont (font (11.5f));
    const float tw = juce::GlyphArrangement::getStringWidth (font (11.5f), txt) + 30.f;
    auto nr = juce::Rectangle<float> (glass.getRight() - 12.f - tw, glass.getY() + 8.f, tw, 24.f);
    g.setColour (juce::Colour (0xe6141215));
    g.fillRoundedRectangle (nr, 6.f);
    g.setColour (col::pink.withAlpha (0.3f));
    g.fillEllipse (nr.getX() + 6.f, nr.getCentreY() - 7.f, 14.f, 14.f);
    g.setColour (col::pink);
    g.fillEllipse (nr.getX() + 9.f, nr.getCentreY() - 4.f, 8.f, 8.f);
    g.setColour (col::value);
    g.drawText (txt, nr.withTrimmedLeft (24.f), juce::Justification::centredLeft);
}

void CrtScreen::mouseDown (const juce::MouseEvent& e)
{
    if (toggleArea.contains (e.position))
    {
        proc.screenMode = e.position.x < toggleArea.getX() + 52.f ? 0 : 1;
        repaint();
    }
}

//==============================================================================
// LevelMeter
//==============================================================================
LevelMeter::LevelMeter (VoiceFuckerProcessor& p, bool isInput) : proc (p), input (isInput)
{
    level[0] = level[1] = -100.f;
    startTimerHz (30);
}

void LevelMeter::timerCallback()
{
    float pk = -100.f;
    for (int c = 0; c < 2; ++c)
    {
        const float v = (input ? proc.inPeak[c] : proc.outPeak[c]).exchange (0.f);
        const float db = toDb (v);
        level[c] = std::max (db, level[c] - 1.5f);
        pk = std::max (pk, db);
    }
    if (pk >= peakHold) { peakHold = pk; holdFrames = 45; }
    else if (--holdFrames < 0) peakHold = std::max (pk, peakHold - 0.6f);
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    const float cx = (float) getWidth() * 0.5f;
    g.setColour (col::text);
    const juce::String label (input ? "IN" : "OUT");
    const auto f = font (11.f, true);
    const float lw = juce::GlyphArrangement::getStringWidth (f, label) + 2.5f * (float) (label.length() - 1);
    drawSpaced (g, label, f, cx - lw * 0.5f, 16.f, 2.5f);

    const bool over = peakHold > 0.f;
    auto box = juce::Rectangle<float> (cx - 20.f, 26.f, 40.f, 20.f);
    g.setColour (juce::Colour (0xff161417));
    g.fillRoundedRectangle (box, 5.f);
    g.setColour (over ? juce::Colour (0xffff3b5c) : juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (box, 5.f, 1.2f);
    g.setColour (over ? juce::Colour (0xffff6f9a) : juce::Colour (0xffc9c2c6));
    g.setFont (font (11.f));
    g.drawText (peakHold < -60.f ? juce::String ("-inf") : (peakHold > 0.05f ? "+" : "") + juce::String (peakHold, 1), box, juce::Justification::centred);

    const float top = 62.f, bottom = (float) getHeight() - 8.f;
    auto yFor = [&] (float db) { return bottom - (bottom - top) * juce::jlimit (0.f, 1.f, (db + 60.f) / 66.f); };
    for (int c = 0; c < 2; ++c)
    {
        const float bx = cx - 9.f + (float) c * 11.f;
        g.setColour (juce::Colour (0xff232024));
        g.fillRect (bx, top, 7.f, bottom - top);
        const float y = yFor (level[c]);
        g.setGradientFill (juce::ColourGradient (col::pinkHot, 0, top, juce::Colour (0xff6d5867), 0, bottom, false));
        g.fillRect (bx, y, 7.f, bottom - y);
        if (level[c] > 0.f) { g.setColour (juce::Colour (0xffff3b5c)); g.fillRect (bx, top, 7.f, yFor (0.f) - top); }
    }
    const float hy = yFor (peakHold);
    g.setColour (juce::Colour (0xffffc2dc));
    g.fillRect (cx - 9.f, hy, 18.f, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    for (float db : { 0.f, -6.f, -12.f, -24.f, -36.f, -48.f })
        g.fillRect (cx + (input ? 12.f : -18.f), yFor (db), 5.f, 1.f);
}

//==============================================================================
// Preset bar
//==============================================================================
PresetBar::PresetBar (VoiceFuckerProcessor& p) : proc (p) { startTimerHz (6); }

void PresetBar::timerCallback()
{
    if (proc.getPresetName() != shown || proc.isPresetDirty() != dirtyShown) repaint();
}

juce::Array<PresetBar::Entry> PresetBar::entries() const
{
    juce::Array<Entry> e;
    auto f = proc.getFactoryPresetNames();
    for (int i = 0; i < f.size(); ++i) e.add ({ f[i], i, {} });
    for (auto& file : proc.getUserPresets()) e.add ({ file.getFileNameWithoutExtension(), -1, file });
    return e;
}

void PresetBar::paint (juce::Graphics& g)
{
    shown = proc.getPresetName(); dirtyShown = proc.isPresetDirty();
    drawChevron (g, 10.f, 16.f, true, juce::Colour (0xffbdb5ba));
    auto box = juce::Rectangle<float> (22.f, 2.f, 182.f, 28.f);
    g.setColour (juce::Colour (0xff1c1a1d));
    g.fillRoundedRectangle (box, 6.f);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.drawRoundedRectangle (box, 6.f, 1.f);
    g.setColour (juce::Colour (0xffece6e9));
    g.setFont (font (12.5f));
    g.drawText (shown + (dirtyShown ? " *" : ""), box.withTrimmedLeft (12.f).withTrimmedRight (26.f), juce::Justification::centredLeft, true);
    juce::Path a; a.startNewSubPath (box.getRight() - 18.f, 14.f); a.lineTo (box.getRight() - 13.f, 19.f); a.lineTo (box.getRight() - 8.f, 14.f);
    g.setColour (col::pink);
    g.strokePath (a, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    drawChevron (g, 216.f, 16.f, false, juce::Colour (0xffbdb5ba));
    // save (floppy)
    g.setColour (col::text);
    auto fl = juce::Rectangle<float> (232.f, 6.f, 20.f, 20.f);
    g.drawRoundedRectangle (fl, 3.f, 1.6f);
    g.drawRect (fl.getX() + 5.f, fl.getY(), 10.f, 6.f, 1.4f);
    g.drawRect (fl.getX() + 4.f, fl.getY() + 11.f, 12.f, 9.f, 1.4f);
}

void PresetBar::mouseDown (const juce::MouseEvent& e)
{
    if (e.x < 20) step (-1);
    else if (e.x < 205) showMenu();
    else if (e.x < 228) step (1);
    else savePreset();
}

void PresetBar::step (int dir)
{
    auto list = entries();
    if (list.isEmpty()) return;
    int cur = -1;
    for (int i = 0; i < list.size(); ++i) if (list[i].name == proc.getPresetName()) cur = i;
    const int next = (cur + dir + list.size()) % list.size();
    auto& en = list.getReference (next);
    if (en.factory >= 0) proc.loadFactoryPreset (en.factory); else proc.loadUserPreset (en.file);
    repaint();
}

void PresetBar::showMenu()
{
    auto list = entries();
    juce::PopupMenu m;
    m.addSectionHeader ("Factory");
    for (int i = 0; i < list.size(); ++i)
    {
        if (i > 0 && list[i].factory < 0 && list[i - 1].factory >= 0) m.addSectionHeader ("User");
        m.addItem (i + 1, list[i].name, true, list[i].name == proc.getPresetName());
    }
    m.addSeparator();
    m.addItem (1000, "Save preset...");
    m.addItem (1001, "Open presets folder");
    juce::Component::SafePointer<PresetBar> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (182),
                     [safe, list] (int r)
                     {
                         if (safe == nullptr || r == 0) return;
                         if (r == 1000) { safe->savePreset(); return; }
                         if (r == 1001)
                         {
                             auto f = safe->proc.getUserPresetFolder();
                             f.createDirectory();
                             f.startAsProcess();
                             return;
                         }
                         auto& en = list.getReference (r - 1);
                         if (en.factory >= 0) safe->proc.loadFactoryPreset (en.factory); else safe->proc.loadUserPreset (en.file);
                         safe->repaint();
                     });
}

void PresetBar::savePreset()
{
    saveWindow = std::make_unique<juce::AlertWindow> ("Save preset", "Preset name:", juce::MessageBoxIconType::NoIcon, this);
    saveWindow->setLookAndFeel (&getLookAndFeel());
    saveWindow->addTextEditor ("name", proc.getPresetName() == "Init" ? juce::String ("My preset") : proc.getPresetName());
    saveWindow->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    saveWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PresetBar> safe (this);
    saveWindow->enterModalState (true, juce::ModalCallbackFunction::create ([safe] (int r)
    {
        if (safe == nullptr) return;
        if (r == 1 && safe->saveWindow != nullptr)
            safe->proc.saveUserPreset (safe->saveWindow->getTextEditorContents ("name"));
        if (safe->saveWindow != nullptr) safe->saveWindow->setLookAndFeel (nullptr);
        safe->saveWindow.reset();
        safe->repaint();
    }), false);
}

//==============================================================================
// Harmonize
//==============================================================================
HarmPanel::HarmPanel (VoiceFuckerProcessor& p) : proc (p)
{
    auto& s = proc.apvts;
    for (int i = 0; i < Params::numIntervals; ++i)
    {
        auto* f = faders.add (new ui::Fader (s, Params::ivDb (i)));
        addAndMakeVisible (f);
        auto* c = chips.add (new ui::Chip (s, Params::ivOn (i), Params::intervalNames[i]));
        addAndMakeVisible (c);
        // touching a fader of a muted voice switches it on
        f->onDragStart = [this, i] { if (! chips[i]->getToggleState()) chips[i]->setToggleState (true, juce::sendNotificationSync); };
    }
    power = std::make_unique<ui::PowerButton> (s, "harm_on");   addAndMakeVisible (*power);
    scaleLed = std::make_unique<ui::Led> (s, "harm_scale", ""); addAndMakeVisible (*scaleLed);
    key = std::make_unique<ui::Combo> (s, "harm_key");           addAndMakeVisible (*key);
    pan = std::make_unique<ui::Segmented> (s, "harm_pan", Params::panModes); addAndMakeVisible (*pan);
    level = std::make_unique<ui::Knob> (s, "harm_level", "Level");    addAndMakeVisible (*level);
    detune = std::make_unique<ui::Knob> (s, "harm_detune", "Detune"); addAndMakeVisible (*detune);
    spread = std::make_unique<ui::Knob> (s, "harm_spread", "Spread"); addAndMakeVisible (*spread);
    startTimerHz (20);
}

void HarmPanel::resized()
{
    for (int i = 0; i < Params::numIntervals; ++i)
    {
        const int x = stripX0 + i * stripStep;
        faders[i]->setBounds (x - 14, 52, 28, 108);
        chips[i]->setBounds (x - 21, 166, 42, 24);
    }
    power->setBounds (6, 11, 20, 20);
    scaleLed->setBounds (526, 52, 22, 22);
    key->setBounds (552, 52, 162, 22);
    pan->setBounds (530, 92, 184, 22);
    level->setCentre (561.f, 163.f);
    detune->setCentre (622.f, 163.f);
    spread->setCentre (683.f, 163.f);
}

float HarmPanel::panFor (int i) const
{
    auto& s = proc.apvts;
    const int mode = (int) s.getRawParameterValue ("harm_pan")->load();
    const float sp = s.getRawParameterValue ("harm_spread")->load() * 0.01f;
    juce::Array<int> act;
    for (int j = 0; j < Params::numIntervals; ++j) if (chips[j]->getToggleState()) act.add (j);
    const int k = act.indexOf (i), n = act.size();
    if (k < 0) return 0.f;
    if (mode == 0) return (k % 2 ? 1.f : -1.f) * sp * (0.5f + 0.5f * (float) (k + 1) / (float) n);
    if (mode == 1) return n > 1 ? (-1.f + 2.f * (float) k / (float) (n - 1)) * sp : 0.f;
    return 0.f;
}

void HarmPanel::timerCallback()
{
    auto& s = proc.apvts;
    juce::int64 h = 0;
    for (int i = 0; i < Params::numIntervals; ++i)
        h = h * 31 + (chips[i]->getToggleState() ? 1 : 0) * 7 + juce::roundToInt (faders[i]->getValue() * 10.0);
    h = h * 31 + juce::roundToInt (s.getRawParameterValue ("harm_pan")->load());
    h = h * 31 + juce::roundToInt (s.getRawParameterValue ("harm_spread")->load());
    const bool on = s.getRawParameterValue ("harm_on")->load() > 0.5f;
    h = h * 31 + (on ? 1 : 0);
    if (h != lastHash)
    {
        lastHash = h;
        for (int i = 0; i < Params::numIntervals; ++i)
        {
            faders[i]->voiceOn = chips[i]->getToggleState();
            faders[i]->blockOn = on;
            chips[i]->blockOn = on;
            faders[i]->repaint(); chips[i]->repaint();
        }
        const float a = on ? 1.f : 0.4f;
        for (juce::Component* c : { (juce::Component*) scaleLed.get(), (juce::Component*) key.get(), (juce::Component*) pan.get(),
                                    (juce::Component*) level.get(), (juce::Component*) detune.get(), (juce::Component*) spread.get() })
            c->setAlpha (a);
        for (int i = 0; i < Params::numIntervals; ++i) { faders[i]->setAlpha (a); chips[i]->setAlpha (a); }
        pan->active = on;
        repaint();
    }
}

void HarmPanel::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (col::panel);
    g.fillRoundedRectangle (b, 8.f);
    g.setColour (col::hair);
    g.drawRoundedRectangle (b.reduced (0.5f), 8.f, 1.f);
    const bool on = proc.apvts.getRawParameterValue ("harm_on")->load() > 0.5f;
    g.setColour (on ? col::text : juce::Colour (0xff6a6267));
    drawSpaced (g, "HARMONIZE", font (10.5f, true), 30.f, 25.f, 2.5f);

    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (14.f, 34.f, (float) (stripStep * Params::numIntervals) + 6.f, 172.f, 8.f);

    const float alpha = on ? 1.f : 0.4f;
    // dB scale
    auto* f0 = faders[0];
    for (float db : { 0.f, -12.f, -24.f, -36.f })
    {
        const float y = (float) f0->getY() + (float) f0->getPositionOfValue (db);
        g.setColour (juce::Colours::white.withAlpha (0.05f * alpha));
        g.drawHorizontalLine (juce::roundToInt (y), 22.f, (float) (stripX0 + 8 * stripStep + 18));
        g.setColour (juce::Colour (0xff5f585c).withMultipliedAlpha (alpha));
        g.setFont (font (9.5f));
        g.drawText (juce::String ((int) db), stripX0 + 8 * stripStep + 18, (int) y - 6, 26, 12, juce::Justification::centredLeft);
    }

    const int mode = (int) proc.apvts.getRawParameterValue ("harm_pan")->load();
    for (int i = 0; i < Params::numIntervals; ++i)
    {
        const float x = (float) (stripX0 + i * stripStep);
        const bool vOn = chips[i]->getToggleState();
        const float db = (float) faders[i]->getValue();
        g.setColour ((vOn ? juce::Colour (0xfff2ecef) : juce::Colour (0xff7d7579)).withMultipliedAlpha (alpha));
        g.setFont (font (11.f, true));
        g.drawText (vOn ? (db > 0.05f ? "+" : "") + juce::String (db, 1) : juce::String ("off"), (int) x - 24, 36, 48, 13, juce::Justification::centred);

        // pan indicator
        const float py = 201.f;
        g.setColour (juce::Colours::white.withAlpha (0.12f * alpha));
        g.drawLine (x - 16.f, py, x + 16.f, py, 1.f);
        g.setColour (juce::Colours::white.withAlpha (0.25f * alpha));
        g.fillRect (x - 0.5f, py - 3.f, 1.f, 6.f);
        if (vOn)
        {
            if (mode == 2) { g.setColour (juce::Colour (0xffffb3d4).withAlpha (0.8f * alpha)); g.fillRect (x - 16.f, py - 1.5f, 32.f, 3.f); }
            else { g.setColour (col::pink.withMultipliedAlpha (alpha)); g.fillEllipse (x + panFor (i) * 16.f - 3.2f, py - 3.2f, 6.4f, 6.4f); }
        }
    }

    g.setColour (col::hair);
    g.drawVerticalLine (518, 40.f, b.getHeight() - 14.f);
    drawCaption (g, "Scale", 530.f, 46.f, alpha);
    drawCaption (g, "Pan", 530.f, 86.f, alpha);
}

//==============================================================================
// MainPanel
//==============================================================================
ui::Knob* MainPanel::knob (const juce::String& id, const juce::String& label, float cx, float cy, Module* m)
{
    auto* k = new ui::Knob (proc.apvts, id, label);
    owned.add (k);
    addAndMakeVisible (k);
    k->setCentre (cx, cy);
    if (m != nullptr) m->items.add (k);
    return k;
}

MainPanel::MainPanel (VoiceFuckerProcessor& p, VoiceFuckerEditor& e)
    : proc (p), editor (e), screen (p), inMeter (p, true), outMeter (p, false), presets (p), harm (p)
{
    // asphalt texture
    asphalt = juce::Image (juce::Image::RGB, W * 2, H * 2, false);
    {
        juce::Image::BitmapData bd (asphalt, juce::Image::BitmapData::writeOnly);
        juce::Random rnd (0x5eed);
        for (int y = 0; y < bd.height; ++y)
            for (int x = 0; x < bd.width; ++x)
            {
                float v = 26.f + (rnd.nextFloat() - 0.5f) * 22.f;
                if (rnd.nextFloat() < 0.003f) v += 40.f + rnd.nextFloat() * 50.f;
                const auto u = (juce::uint8) juce::jlimit (0, 255, (int) v);
                bd.setPixelColour (x, y, juce::Colour (u, (juce::uint8) juce::jmax (0, u - 1), (juce::uint8) juce::jmin (255, u + 3)));
            }
    }

    addAndMakeVisible (screen);   screen.setBounds (72, 58, 726, 184);
    addAndMakeVisible (inMeter);  inMeter.setBounds (4, 8, 54, 730);
    addAndMakeVisible (outMeter); outMeter.setBounds (812, 8, 54, 730);
    addAndMakeVisible (presets);  presets.setBounds (378, 12, 260, 32);
    addAndMakeVisible (harm);     harm.setBounds (72, 438, 726, 214);
    gearArea = { 762, 12, 34, 30 };

    knob ("in_gain", "Input", 31.f, 792.f);
    knob ("out_gain", "Output", 839.f, 792.f);

    auto& s = proc.apvts;
    modules.reserve (8);
    auto addModule = [&] (const juce::String& t, juce::Rectangle<int> r, const juce::String& onId) -> Module&
    {
        modules.push_back ({ t, r, onId, {} });
        auto& m = modules.back();
        if (onId.isNotEmpty())
        {
            auto* pb = new ui::PowerButton (s, onId);
            owned.add (pb); addAndMakeVisible (pb);
            pb->setBounds (r.getX() + 6, r.getY() + 11, 20, 20);
        }
        return m;
    };
    auto place = [&] (Module& m, juce::Component* c, juce::Rectangle<int> r)
    {
        owned.add (c); addAndMakeVisible (c); c->setBounds (r); m.items.add (c);
    };
    auto row = [&] (Module& m, std::initializer_list<std::pair<const char*, const char*>> ks)
    {
        const float step = (float) m.r.getWidth() / (float) ks.size();
        int i = 0;
        for (auto& [id, label] : ks) knob (id, label, (float) m.r.getX() + step * ((float) i++ + 0.5f), (float) m.r.getY() + 126.f, &m);
    };

    {
        auto& m = addModule ("CARRIER", { 72, 250, 326, 180 }, {});
        place (m, new ui::Combo (s, "car_src"), { m.r.getX() + 14, m.r.getY() + 52, 118, 22 });
        place (m, new ui::Segmented (s, "car_pitch", Params::pitchModes), { m.r.getX() + 142, m.r.getY() + 52, 170, 22 });
        row (m, { { "car_note", "Note" }, { "car_glide", "Glide" }, { "car_detune", "Detune" }, { "car_spread", "Spread" }, { "car_noise", "Noise" } });
    }
    {
        auto& m = addModule ("VOCODER", { 406, 250, 392, 180 }, "voc_on");
        row (m, { { "voc_bands", "Bands" }, { "voc_formant", "Formant" }, { "voc_attack", "Attack" },
                  { "voc_release", "Release" }, { "voc_sib", "Sibilance" }, { "voc_gate", "Gate" } });
    }
    {
        auto& m = addModule ("DIRT", { 72, 660, 258, 180 }, "dirt_on");
        place (m, new ui::Segmented (s, "dirt_mode", Params::dirtModes), { m.r.getX() + 14, m.r.getY() + 52, 150, 22 });
        row (m, { { "dirt_drive", "Drive" }, { "dirt_crunch", "Crunch" }, { "dirt_destroy", "Destroy" } });
    }
    {
        auto& m = addModule ("WIDTH", { 338, 660, 180, 180 }, "width_on");
        place (m, new ui::Segmented (s, "width_mode", Params::widthModes), { m.r.getX() + 12, m.r.getY() + 52, 156, 22 });
        row (m, { { "width_amt", "Width" }, { "width_mono", "Mono bass" } });
    }
    {
        auto& m = addModule ("MIX", { 526, 660, 124, 180 }, {});
        place (m, new ui::Led (s, "autogain", "Auto gain"), { m.r.getX() + 10, m.r.getY() + 52, 108, 22 });
        row (m, { { "mix", "Mix" } });
    }
    {
        auto& m = addModule ("CLIP", { 658, 660, 140, 180 }, "clip_on");
        place (m, new ui::Segmented (s, "clip_mode", Params::clipModes), { m.r.getX() + 14, m.r.getY() + 52, 100, 22 });
        row (m, { { "clip_ceiling", "Ceiling" } });
    }

    startTimerHz (10);
    timerCallback();
}

void MainPanel::timerCallback()
{
    bool changed = false;
    for (auto& m : modules)
    {
        if (m.onParam.isEmpty()) continue;
        const bool on = proc.apvts.getRawParameterValue (m.onParam)->load() > 0.5f;
        for (auto* c : m.items)
        {
            const float a = on ? 1.f : 0.4f;
            if (std::abs (c->getAlpha() - a) > 0.01f) { c->setAlpha (a); changed = true; }
            if (auto* sg = dynamic_cast<ui::Segmented*> (c)) if (sg->active != on) { sg->active = on; sg->repaint(); }
        }
    }
    // Note knob only matters in Fixed pitch mode
    const bool fixed = (int) proc.apvts.getRawParameterValue ("car_pitch")->load() == 2;
    if (auto* noteKnob = dynamic_cast<ui::Knob*> (modules[0].items[2]))
        noteKnob->setAlpha (fixed ? 1.f : 0.4f);
    if (changed) repaint();
}

void MainPanel::paint (juce::Graphics& g)
{
    g.drawImage (asphalt, getLocalBounds().toFloat());

    // header
    drawLogo (g, { 80.f, 7.f, 46.f, 48.f }, juce::Colour (0xff1b191c));
    g.setColour (juce::Colours::white);
    drawSpaced (g, "VOICE FUCKER", font (27.f, true), 136.f, 34.f, 2.f);
    g.setColour (juce::Colour (0xff9a9297));
    drawSpaced (g, "PRIMAL RAGE LAB", font (9.5f, true), 138.f, 50.f, 4.f);

    // gear
    auto gr = gearArea.toFloat();
    g.setColour (juce::Colour (0xff1c1a1d));
    g.fillRoundedRectangle (gr, 6.f);
    const auto gc = gr.getCentre();
    {
        juce::Path gear;
        const int teeth = 8;
        for (int i = 0; i < teeth * 4; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * ((float) i / (float) (teeth * 4)) - juce::MathConstants<float>::halfPi;
            const float r = (i % 4 == 0 || i % 4 == 1) ? 9.5f : 7.2f;
            const auto pt = juce::Point<float> (gc.x + std::cos (a) * r, gc.y + std::sin (a) * r);
            if (i == 0) gear.startNewSubPath (pt); else gear.lineTo (pt);
        }
        gear.closeSubPath();
        gear.addEllipse (gc.x - 3.2f, gc.y - 3.2f, 6.4f, 6.4f);
        gear.setUsingNonZeroWinding (false);
        g.setColour (col::value);
        g.fillPath (gear);
    }

    // modules
    for (auto& m : modules)
    {
        auto r = m.r.toFloat();
        g.setColour (col::panel);
        g.fillRoundedRectangle (r, 8.f);
        g.setColour (col::hair);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.f, 1.f);
        const bool on = m.onParam.isEmpty() || proc.apvts.getRawParameterValue (m.onParam)->load() > 0.5f;
        g.setColour (on ? col::text : juce::Colour (0xff6a6267));
        drawSpaced (g, m.title, font (10.5f, true), r.getX() + (m.onParam.isEmpty() ? 14.f : 30.f), r.getY() + 25.f, 2.5f);
    }
    auto cap = [&] (int mi, const char* t, int dx)
    {
        const auto& m = modules[(size_t) mi];
        const bool on = m.onParam.isEmpty() || proc.apvts.getRawParameterValue (m.onParam)->load() > 0.5f;
        drawCaption (g, t, (float) (m.r.getX() + dx), (float) m.r.getY() + 46.f, on ? 1.f : 0.4f);
    };
    cap (0, "Source", 14); cap (0, "Pitch", 142);
    cap (2, "Mode", 14); cap (3, "Mode", 12); cap (4, "Gain", 12); cap (5, "Shape", 14);
}

void MainPanel::mouseDown (const juce::MouseEvent& e)
{
    if (gearArea.contains (e.getPosition())) showSettings();
}

void MainPanel::showSettings()
{
    juce::PopupMenu m;
    m.addSectionHeader ("Window size");
    const float sizes[] = { 0.7f, 0.8f, 0.9f, 1.0f, 1.15f, 1.3f, 1.5f };
    for (int i = 0; i < 7; ++i)
        m.addItem (i + 1, juce::String (juce::roundToInt (sizes[i] * 100.f)) + " %", true, std::abs (proc.uiScale - sizes[i]) < 0.01f);
    m.addSeparator();
    m.addItem (100, "Voice Fucker 1.0  -  Primal Rage Lab", false, false);
    juce::Component::SafePointer<MainPanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (gearArea)),
                     [safe, sizes] (int r)
                     {
                         if (safe != nullptr && r >= 1 && r <= 7) safe->editor.setScale (sizes[r - 1]);
                     });
}

//==============================================================================
// Editor
//==============================================================================
VoiceFuckerEditor::VoiceFuckerEditor (VoiceFuckerProcessor& p)
    : AudioProcessorEditor (&p), proc (p), content (p, *this)
{
    setLookAndFeel (&look);
    addAndMakeVisible (content);
    content.setBounds (0, 0, MainPanel::W, MainPanel::H);
    setResizable (true, true);
    setResizeLimits (juce::roundToInt (MainPanel::W * 0.6), juce::roundToInt (MainPanel::H * 0.6), MainPanel::W * 2, MainPanel::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) MainPanel::W / (double) MainPanel::H);
    const float s = proc.uiScale;
    setSize (juce::roundToInt ((float) MainPanel::W * s), juce::roundToInt ((float) MainPanel::H * s));
}

VoiceFuckerEditor::~VoiceFuckerEditor() { setLookAndFeel (nullptr); }

void VoiceFuckerEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (0xff1a181b)); }

void VoiceFuckerEditor::resized()
{
    const float s = (float) getWidth() / (float) MainPanel::W;
    content.setTransform (juce::AffineTransform::scale (s));
    proc.uiScale = s;
}

void VoiceFuckerEditor::setScale (float s)
{
    setSize (juce::roundToInt ((float) MainPanel::W * s), juce::roundToInt ((float) MainPanel::H * s));
}
