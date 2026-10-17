#include "UI.h"

namespace ui
{
    juce::Font font (float height, bool bold)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }

    void drawSpaced (juce::Graphics& g, const juce::String& t, juce::Font f, float x, float baseline, float spacing)
    {
        g.setFont (f);
        for (auto ch : t)
        {
            const juce::String s = juce::String::charToString (ch);
            g.drawSingleLineText (s, juce::roundToInt (x), juce::roundToInt (baseline));
            x += juce::GlyphArrangement::getStringWidth (f, s) + spacing;
        }
    }

    void drawCaption (juce::Graphics& g, const juce::String& t, float x, float baseline, float alpha)
    {
        g.setColour (col::muted.withMultipliedAlpha (alpha));
        drawSpaced (g, t.toUpperCase(), font (9.5f, true), x, baseline, 1.5f);
    }

    juce::Path logoPath()
    {
        // Primal Rage Lab gorilla, traced from the original logo (viewBox origin 243,165)
        static const char* polys[] = {
            "498,613 468,674 468,679 532,679 532,674 502,613", "602,581 596,584 546,666 544,677 548,683 583,693",
            "398,581 417,692 426,692 452,683 456,678 455,668 402,581", "402,572 404,580 460,672 466,673 492,619 491,609 482,602 408,572",
            "611,571 607,571 587,687 591,687 659,622 657,612", "599,571 523,600 509,609 509,622 534,673 540,672 599,575",
            "388,571 343,612 341,623 409,687 413,687 393,571", "502,604 589,571 591,566 502,566",
            "409,566 411,571 445,582 450,587 492,604 498,604 498,567", "670,534 615,546 615,551 653,578 657,578",
            "329,533 342,578 348,578 385,551 385,546", "670,486 666,486 618,537 618,541 672,529",
            "329,486 327,528 382,541 382,537 335,486", "603,540 581,484 571,480 562,484 507,540 507,544 603,545",
            "397,540 397,545 493,544 493,540 437,483 430,480 420,482", "569,466 536,452 508,531 512,531 570,471",
            "430,466 429,471 488,531 492,531 464,452", "634,423 629,423 611,536 615,536 666,480 665,470",
            "625,423 621,423 579,467 603,532 607,532", "374,422 392,533 397,533 421,473 421,466 378,422",
            "366,422 334,471 333,478 385,536 389,536 371,422", "528,406 537,447 572,464 577,464 580,457 585,456 622,418 622,413 541,394 533,395",
            "472,408 469,397 458,394 377,413 377,417 404,445 408,446 419,461 429,464 464,446", "384,349 380,349 299,412 297,418 361,411 375,389",
            "615,348 627,394 638,410 652,414 703,418 703,414 619,348", "502,337 502,524 506,524 533,445 506,337",
            "494,336 467,451 494,524 498,524 498,336", "616,339 617,344 690,401 701,406 701,402 654,326",
            "384,339 346,326 300,400 299,406 306,404 384,343", "340,326 249,336 249,341 284,413 288,413 335,339",
            "659,325 659,329 707,408 716,413 751,336", "505,318 521,386 615,407 615,403 517,324",
            "495,318 488,320 383,404 383,408 479,386", "516,316 519,323 618,402 625,404 613,353 605,343 592,336",
            "484,316 408,336 395,343 386,357 375,404 385,400 481,323", "661,318 661,322 746,332 746,328 703,289 694,290",
            "339,318 306,290 296,289 254,328 254,332 337,323", "512,293 564,288 565,283 542,273 535,273 513,288",
            "436,283 436,288 487,292 487,288 465,273 458,273", "472,270 474,276 498,293 502,293 529,274 529,270 506,267",
            "261,204 251,244 256,250 270,251 274,250 265,204", "739,203 735,203 726,246 726,250 729,251 745,250 749,244",
            "622,203 618,203 546,263 543,271 567,281 580,279 621,209", "378,203 379,209 420,279 434,281 457,271 454,263 382,203",
            "637,193 631,196 588,270 586,278 590,278 661,233 661,225", "363,193 339,225 339,233 407,276 413,277 413,272 367,193",
            "688,176 673,218 674,226 687,237 719,247 694,179", "312,176 306,179 281,247 310,238 326,226 326,215",
            "696,173 720,239 725,240 733,192 725,185", "304,173 273,186 267,192 275,241 280,239",
            "315,172 332,225 336,225 356,196 355,188 348,182", "686,171 650,183 644,191 646,201 664,225 668,225" };
        juce::Path p;
        for (auto* s : polys)
        {
            auto pts = juce::StringArray::fromTokens (s, " ", "");
            bool first = true;
            for (auto& pt : pts)
            {
                const float x = pt.upToFirstOccurrenceOf (",", false, false).getFloatValue() - 243.f;
                const float y = pt.fromFirstOccurrenceOf (",", false, false).getFloatValue() - 165.f;
                if (first) p.startNewSubPath (x, y); else p.lineTo (x, y);
                first = false;
            }
            p.closeSubPath();
        }
        return p;
    }

    void drawLogo (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour seam)
    {
        static const juce::Path p = logoPath();
        const auto t = juce::RectanglePlacement (juce::RectanglePlacement::centred)
                           .getTransformToFit ({ 0, 0, 514, 534 }, area);
        juce::Path tp (p);
        tp.applyTransform (t);
        g.setColour (juce::Colour (0xffff4f98));
        g.fillPath (tp);
        // thick seams between the polygons (house style): 9 units of the 514-wide logo
        g.setColour (seam);
        g.strokePath (tp, juce::PathStrokeType (9.f * area.getWidth() / 514.f, juce::PathStrokeType::curved));
    }

    //==========================================================================
    Look::Look()
    {
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1a1d));
        setColour (juce::PopupMenu::textColourId, col::value);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::pink);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::PopupMenu::headerTextColourId, col::muted);
        setColour (juce::ComboBox::textColourId, col::value);
        setColour (juce::ComboBox::arrowColourId, col::pink);
        setColour (juce::TextEditor::backgroundColourId, col::field);
        setColour (juce::TextEditor::textColourId, col::value);
        setColour (juce::TextEditor::highlightColourId, col::pinkDim);
        setColour (juce::TextEditor::outlineColourId, col::hair);
        setColour (juce::TextEditor::focusedOutlineColourId, col::pink);
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1c1a1d));
        setColour (juce::AlertWindow::textColourId, col::value);
        setColour (juce::AlertWindow::outlineColourId, col::pink);
        setColour (juce::TextButton::buttonColourId, col::field);
        setColour (juce::TextButton::textColourOffId, col::value);
        setColour (juce::Slider::textBoxTextColourId, col::value);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff1c1a1d));
        setColour (juce::Slider::textBoxOutlineColourId, col::pink);
    }

    void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                 float, float, juce::Slider& s)
    {
        const float a0 = juce::degreesToRadians (-135.f), sweep = juce::degreesToRadians (270.f);
        const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
        const float R = 18.f;
        const bool on = s.isEnabled();

        // dotted scale
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        for (int i = 0; i <= 40; ++i)
        {
            const float a = a0 + sweep * (float) i / 40.f;
            g.fillEllipse (c.x + std::sin (a) * (R + 3.f) - 0.7f, c.y - std::cos (a) * (R + 3.f) - 0.7f, 1.4f, 1.4f);
        }
        // track
        juce::Path track;
        track.addCentredArc (c.x, c.y, R + 7.f, R + 7.f, 0.f, a0, a0 + sweep, true);
        g.setColour (juce::Colour (0xff2e2a2e));
        g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // value arc with glow
        if (pos > 0.002f)
        {
            juce::Path arc;
            arc.addCentredArc (c.x, c.y, R + 7.f, R + 7.f, 0.f, a0, a0 + sweep * pos, true);
            if (on)
            {
                g.setColour (col::pink.withAlpha (0.12f));
                g.strokePath (arc, juce::PathStrokeType (9.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (col::pink.withAlpha (0.2f));
                g.strokePath (arc, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            g.setGradientFill (juce::ColourGradient (col::pinkDeep, c.x - R, c.y + R, col::pinkHot, c.x + R, c.y - R, false));
            g.strokePath (arc, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // shadow
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.7f), c.x + 2.f, c.y + 6.f,
                                                 juce::Colours::transparentBlack, c.x + 2.f + R * 1.35f, c.y + 6.f, true));
        g.fillEllipse (c.x + 2.f - R * 1.35f, c.y + 6.f - R * 1.35f, R * 2.7f, R * 2.7f);

        // knurled edge
        for (int i = 0; i < 48; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / 48.f;
            g.setColour ((i % 2) ? juce::Colour (0xff17161a) : juce::Colour (0xff3a383e));
            g.drawLine (c.x + std::cos (a) * (R - 1.f), c.y + std::sin (a) * (R - 1.f),
                        c.x + std::cos (a) * (R + 1.2f), c.y + std::sin (a) * (R + 1.2f), 1.6f);
        }
        // metal body
        juce::ColourGradient body (juce::Colour (0xff5a575f), c.x - 6.f, c.y - 7.f, juce::Colour (0xff141317), c.x + R, c.y + R, true);
        body.addColour (0.5, juce::Colour (0xff2a282d));
        g.setGradientFill (body);
        g.fillEllipse (c.x - R + 1.f, c.y - R + 1.f, (R - 1.f) * 2.f, (R - 1.f) * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        juce::Path rim; rim.addCentredArc (c.x, c.y, R - 1.5f, R - 1.5f, 0.f, -2.2f, -0.6f, true);
        g.strokePath (rim, juce::PathStrokeType (1.f));
        g.setColour (juce::Colours::white.withAlpha (0.13f));
        g.fillEllipse (c.x - 11.f, c.y - 12.f, 12.f, 6.f);

        // pointer
        const float a = a0 + sweep * pos;
        g.setColour (on ? juce::Colour (0xfff4f0f2) : juce::Colour (0xff8a8287));
        g.drawLine (c.x + std::sin (a) * 5.f, c.y - std::cos (a) * 5.f,
                    c.x + std::sin (a) * (R - 5.f), c.y - std::cos (a) * (R - 5.f), 2.4f);
    }

    void Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                                 juce::Slider::SliderStyle, juce::Slider& s)
    {
        auto* f = dynamic_cast<Fader*> (&s);
        const bool voiceOn = f == nullptr || f->voiceOn;
        const bool lit = voiceOn && (f == nullptr || f->blockOn);
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y, bottom = (float) (y + h);

        g.setColour (juce::Colour (0xff141215));
        g.fillRoundedRectangle (cx - 3.f, top, 6.f, bottom - top, 3.f);
        if (voiceOn && sliderPos < bottom)
        {
            g.setGradientFill (juce::ColourGradient (lit ? col::pinkHot : col::pinkDim, cx, sliderPos,
                                                     lit ? juce::Colour (0xff7a2a52) : juce::Colour (0xff3a2a32), cx, bottom, false));
            g.fillRoundedRectangle (cx - 3.f, sliderPos, 6.f, bottom - sliderPos, 3.f);
        }
        // cap
        auto capR = juce::Rectangle<float> (cx - 11.f, sliderPos - 7.f, 22.f, 14.f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (capR.translated (0, 2), 3.f);
        juce::ColourGradient cg (juce::Colour (0xff2a282d), capR.getX(), 0, juce::Colour (0xff1c1b1f), capR.getRight(), 0, false);
        cg.addColour (0.5, juce::Colour (0xff5a5760));
        g.setGradientFill (cg);
        g.fillRoundedRectangle (capR, 3.f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (capR, 3.f, 1.f);
        g.setColour (voiceOn ? col::pink : juce::Colour (0xff8a8287));
        g.fillRect (cx - 7.f, sliderPos - 0.75f, 14.f, 1.5f);
    }

    void Look::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
        g.setColour (juce::Colour (0xff1d1b1e));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (box.isMouseOver (true) ? col::pink.withAlpha (0.35f) : col::hair);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.f, 1.f);
        juce::Path arrow;
        const float ax = (float) w - 14.f, ay = (float) h * 0.5f;
        arrow.startNewSubPath (ax - 4.f, ay - 2.f); arrow.lineTo (ax, ay + 2.f); arrow.lineTo (ax + 4.f, ay - 2.f);
        g.setColour (col::pink);
        g.strokePath (arrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void Look::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds (4, 0, box.getWidth() - 24, box.getHeight());
        label.setFont (getComboBoxFont (box));
    }

    void Look::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
    {
        g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
        g.setColour (col::pink.withAlpha (0.35f));
        g.drawRect (0, 0, w, h, 1);
    }

    //==========================================================================
    Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& l)
        : param (s.getParameter (id)), label (l)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setMouseDragSensitivity (220);
        slider.setVelocityBasedMode (false);
        slider.setPopupDisplayEnabled (false, false, nullptr);
        addAndMakeVisible (slider);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
        slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
        slider.addListener (this);
        slider.addMouseListener (this, false);
        setInterceptsMouseClicks (false, true);
    }

    Knob::~Knob() { slider.removeListener (this); }

    void Knob::resized() { slider.setBounds (getLocalBounds().withSizeKeepingCentre (58, 58)); }

    void Knob::paint (juce::Graphics& g)
    {
        const bool hot = slider.isMouseOverOrDragging();
        g.setFont (font (11.f, true));
        g.setColour (hot ? col::pink : juce::Colour (0xffcfc8cc));
        g.drawText (label, 0, 2, getWidth(), 14, juce::Justification::centred);
        g.setFont (font (11.f));
        g.setColour (col::value);
        g.drawText (param->getCurrentValueAsText(), 0, getHeight() - 16, getWidth(), 14, juce::Justification::centred);
    }

    //==========================================================================
    PowerButton::PowerButton (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
        setClickingTogglesState (true);
    }

    void PowerButton::paintButton (juce::Graphics& g, bool over, bool)
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const bool on = getToggleState();
        g.setColour (on ? col::pink : (over ? juce::Colour (0xff8a7d84) : juce::Colour (0xff5b5358)));
        juce::Path p;
        p.addCentredArc (c.x, c.y, 5.5f, 5.5f, 0.f, 0.7f, juce::MathConstants<float>::twoPi - 0.7f, true);
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.x, c.y - 7.5f, c.x, c.y - 1.f, 1.8f);
    }

    //==========================================================================
    Led::Led (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& t)
    {
        setButtonText (t);
        att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
        setClickingTogglesState (true);
    }

    void Led::paintButton (juce::Graphics& g, bool over, bool)
    {
        const bool on = getToggleState();
        const float cy = (float) getHeight() * 0.5f;
        if (on)
        {
            g.setColour (col::pink.withAlpha (0.25f));
            g.fillEllipse (2.f, cy - 8.f, 16.f, 16.f);
        }
        g.setColour (on ? col::pink : (over ? juce::Colour (0xff5a4a52) : juce::Colour (0xff3a3236)));
        g.fillEllipse (5.f, cy - 5.f, 10.f, 10.f);
        if (getButtonText().isNotEmpty())
        {
            g.setColour (col::value);
            g.setFont (font (12.f));
            g.drawText (getButtonText(), 22, 0, getWidth() - 22, getHeight(), juce::Justification::centredLeft);
        }
    }

    //==========================================================================
    Segmented::Segmented (juce::AudioProcessorValueTreeState& s, const juce::String& id, juce::StringArray l)
        : labels (std::move (l))
    {
        auto* p = s.getParameter (id);
        att = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { index = juce::roundToInt (v); repaint(); });
        att->sendInitialUpdate();
    }

    void Segmented::paint (juce::Graphics& g)
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (col::field);
        g.fillRoundedRectangle (r, 5.f);
        const float w = r.getWidth() / (float) labels.size();
        for (int i = 0; i < labels.size(); ++i)
        {
            auto cell = juce::Rectangle<float> (r.getX() + w * (float) i, r.getY(), w, r.getHeight());
            if (i == index)
            {
                g.setColour (active ? col::pink : col::pinkDim);
                g.fillRoundedRectangle (cell, 5.f);
            }
            g.setColour (i == index ? juce::Colours::white : juce::Colour (0xffbdb5ba));
            g.setFont (font (labels[i].length() > 7 ? 10.5f : 11.5f));
            g.drawText (labels[i], cell, juce::Justification::centred);
        }
    }

    void Segmented::mouseDown (const juce::MouseEvent& e)
    {
        const int i = juce::jlimit (0, labels.size() - 1, (int) ((float) e.x / ((float) getWidth() / (float) labels.size())));
        att->setValueAsCompleteGesture ((float) i);
    }

    //==========================================================================
    Combo::Combo (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
            addItemList (p->choices, 1);
        att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (s, id, *this);
    }

    //==========================================================================
    Chip::Chip (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& t)
    {
        setButtonText (t);
        att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (s, id, *this);
        setClickingTogglesState (true);
    }

    void Chip::paintButton (juce::Graphics& g, bool over, bool)
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        if (on && blockOn)
        {
            g.setColour (col::pink.withAlpha (0.22f));
            g.fillRoundedRectangle (r.expanded (2.f), 7.f);
        }
        g.setColour (on ? (blockOn ? col::pink : col::pinkDim) : (over ? juce::Colour (0xff242125) : col::field));
        g.fillRoundedRectangle (r, 5.f);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawRoundedRectangle (r, 5.f, 1.f);
        g.setColour (on ? juce::Colours::white : juce::Colour (0xffbdb5ba));
        g.setFont (font (12.f, true));
        g.drawText (getButtonText(), r, juce::Justification::centred);
    }

    Fader::Fader (juce::AudioProcessorValueTreeState& s, const juce::String& id)
    {
        setSliderStyle (juce::Slider::LinearVertical);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setSliderSnapsToMousePosition (false);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, *this);
        auto* p = s.getParameter (id);
        setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    }
}
