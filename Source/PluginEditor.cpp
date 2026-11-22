#include "PluginEditor.h"

using namespace PetroColours;

//==============================================================================
// LookAndFeel
//==============================================================================
PetroLookAndFeel::PetroLookAndFeel()
{
    setColour (juce::PopupMenu::backgroundColourId, inset);
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, panelLn);
    setColour (juce::PopupMenu::highlightedTextColourId, cream);
    setColour (juce::ComboBox::backgroundColourId, inset);
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::arrowColourId, muted);
    setColour (juce::ComboBox::focusedOutlineColourId, edgeHi);
}

void PetroLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, juce::Slider& s)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  c      = bounds.getCentre();
    const float knobR  = radius * 0.70f;
    const float angle  = startAngle + pos * (endAngle - startAngle);
    const auto  accent = s.findColour (juce::Slider::rotarySliderFillColourId);

    // tor
    const float arcR = radius - 3.0f;
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff4a2b06));
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // wartość
    if (pos > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (accent);
        g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // cień + korpus (drewno / mosiądz)
    g.setColour (juce::Colour (0xff0a0500).withAlpha (0.55f));
    g.fillEllipse (c.x - knobR, c.y - knobR + 3.0f, knobR * 2.0f, knobR * 2.0f);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb08a52), c.x - knobR * 0.5f, c.y - knobR * 0.6f,
                                             juce::Colour (0xff2a1704), c.x + knobR * 0.6f, c.y + knobR * 0.8f, true));
    g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);

    g.setColour (juce::Colour (0xff120a02).withAlpha (0.85f));
    g.drawEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.5f);

    // wskaźnik
    juce::Path ptr;
    ptr.addRoundedRectangle (-1.8f, -knobR + 4.0f, 3.6f, knobR * 0.45f, 1.5f);
    g.setColour (cream);
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void PetroLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const auto accent = b.findColour (juce::ToggleButton::tickColourId);

    g.setColour (on ? accent.withAlpha (0.18f) : inset);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);

    g.setColour (on ? accent : (highlighted ? edgeHi : edge));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.4f);

    auto led = juce::Rectangle<float> (r.getX() + 9.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
    if (on)
    {
        g.setColour (accent.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
    }
    g.setColour (on ? accent : juce::Colour (0xff5a3a12));
    g.fillEllipse (led);

    g.setColour (on ? cream : muted);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText (on ? "ON" : "OFF", r.withTrimmedLeft (22.0f).toNearestInt(), juce::Justification::centred);
}

juce::Label* PetroLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::Font (juce::FontOptions (12.5f)));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, cream);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    return l;
}

//==============================================================================
// Własne przyciski i miernik
//==============================================================================
void ModeSwitch::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const auto accent = findColour (juce::ToggleButton::tickColourId);
    const bool right = getToggleState();
    const float h = r.getHeight();

    g.setColour (inset);
    g.fillRoundedRectangle (r, h * 0.5f);

    auto left  = r.withWidth (r.getWidth() * 0.5f);
    auto rightR = r.withTrimmedLeft (r.getWidth() * 0.5f);

    g.setColour (accent.withAlpha (0.30f));
    g.fillRoundedRectangle ((right ? rightR : left).reduced (2.0f), (h - 4.0f) * 0.5f);

    g.setColour (highlighted ? edgeHi : edge);
    g.drawRoundedRectangle (r, h * 0.5f, 1.4f);

    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.setColour (right ? muted : cream);
    g.drawText (leftLabel, left.toNearestInt(), juce::Justification::centred);
    g.setColour (right ? cream : muted);
    g.drawText (rightLabel, rightR.toNearestInt(), juce::Justification::centred);
}

void ChipToggle::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const auto accent = findColour (juce::ToggleButton::tickColourId);
    const bool on = getToggleState();

    g.setColour (on ? accent.withAlpha (0.16f) : inset);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (on ? accent : (highlighted ? edgeHi : juce::Colour (0xff5a3a12)));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);

    auto led = juce::Rectangle<float> (r.getX() + 7.0f, r.getCentreY() - 2.5f, 5.0f, 5.0f);
    if (on)
    {
        g.setColour (accent.withAlpha (0.35f));
        g.fillEllipse (led.expanded (2.0f));
    }
    g.setColour (on ? accent : juce::Colour (0xff5a3a12));
    g.fillEllipse (led);

    g.setColour (on ? cream : muted);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText (getButtonText(), r.withTrimmedLeft (14.0f).toNearestInt(), juce::Justification::centred);
}

void CorrMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (inset);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (edge);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    auto bar = r.reduced (6.0f, 6.0f);
    bar.removeFromLeft (46.0f);   // miejsce na opis
    const float cx = bar.getCentreX();
    const float half = bar.getWidth() * 0.5f;
    const float val = juce::jlimit (-1.0f, 1.0f, v);

    g.setColour (panelLn);
    g.fillRoundedRectangle (bar, 3.0f);

    const juce::Colour c = val >= 0.0f ? (val > 0.3f ? verdigris : gold) : rose;
    auto fill = juce::Rectangle<float> (val >= 0.0f ? cx : cx + val * half, bar.getY(),
                                        std::abs (val) * half, bar.getHeight());
    g.setColour (c);
    g.fillRoundedRectangle (fill, 3.0f);

    g.setColour (cream.withAlpha (0.6f));
    g.drawLine (cx, bar.getY() - 2.0f, cx, bar.getBottom() + 2.0f, 1.0f);

    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("PHASE", getLocalBounds().withWidth (50).translated (6, 0), juce::Justification::centredLeft);
}

//==============================================================================
// Editor
//==============================================================================
PetrofiaEditor::PetrofiaEditor (PetrofiaProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), meter (p.corr)
{
    setLookAndFeel (&laf);

    masterP.title = "MASTER";     masterP.colour = gold;
    strP.title    = "STRINGS";    strP.colour    = gold;
    bodyP.title   = "BODY";       bodyP.colour   = copper;
    embP.title    = "EMBRACE";    embP.colour    = honey;
    motP.title    = "MOTION";     motP.colour    = verdigris;
    bloomP.title  = "BLOOM";      bloomP.colour  = rose;
    satP.title    = "SATURATION"; satP.colour    = amber;
    tapeP.title   = "TAPE";       tapeP.colour   = honey;
    infP.title    = "INFLATOR";   infP.colour    = copper;

    // ----- MASTER -----
    kIntensity = addKnob (masterP, "intensity", "Intensity");
    kMix       = addKnob (masterP, "mix",       "Mix");
    kOut       = addKnob (masterP, "outGain",   "Output");

    modeSw = std::make_unique<ModeSw>();
    modeSw->btn.leftLabel = "SAFE";
    modeSw->btn.rightLabel = "EXTREME";
    modeSw->btn.setColour (juce::ToggleButton::tickColourId, gold);
    modeSw->btn.setClickingTogglesState (true);
    addAndMakeVisible (modeSw->btn);
    modeSw->att = std::make_unique<ButtonAtt> (proc.apvts, "extreme", modeSw->btn);

    cDelta    = addChip (masterP, "delta",     "DELTA");
    cAuto     = addChip (masterP, "autoLevel", "AUTO LEVEL");
    cBassMono = addChip (masterP, "bassMono",  "BASS MONO");
    addAndMakeVisible (meter);

    // ----- STRINGS -----
    addOn (strP, "strOn");
    strKnobs.push_back (addKnob (strP, "strAmt",     "Amount"));
    strKnobs.push_back (addKnob (strP, "strSustain", "Sustain"));
    strKnobs.push_back (addKnob (strP, "strDamp",    "Damping"));
    strKnobs.push_back (addKnob (strP, "strDetune",  "Detune"));
    strKnobs.push_back (addKnob (strP, "strSpread",  "Spread"));
    strKnobs.push_back (addKnob (strP, "strClarity", "Clarity"));
    strKnobs.push_back (addKnob (strP, "strLow",     "Low"));
    strKnobs.push_back (addKnob (strP, "strHigh",    "High"));
    strKnobs.push_back (addKnob (strP, "strTune",    "Tuning"));
    cBassLeft = addChip (strP, "strBassLeft", "BASS LEFT");
    cKeyLock  = addChip (strP, "strKeyLock",  "KEY LOCK");
    cEco      = addChip (strP, "strEco",      "ECO");
    cbRoot    = addCombo (strP, "strRoot",  "Root");
    cbScale   = addCombo (strP, "strScale", "Scale");

    // ----- BODY -----
    addOn (bodyP, "bodyOn");
    bodyKnobs.push_back (addKnob (bodyP, "bodyAmt",  "Amount"));
    bodyKnobs.push_back (addKnob (bodyP, "bodySize", "Size"));
    bodyKnobs.push_back (addKnob (bodyP, "bodyWood", "Wood"));
    bodyKnobs.push_back (addKnob (bodyP, "bodyRoom", "Room"));

    // ----- EMBRACE -----
    addOn (embP, "embOn");
    embKnobs.push_back (addKnob (embP, "embAmt",   "Amount"));
    embKnobs.push_back (addKnob (embP, "embPos",   "Position"));
    embKnobs.push_back (addKnob (embP, "embWidth", "Width"));
    embKnobs.push_back (addKnob (embP, "embDecay", "Decay"));

    // ----- MOTION -----
    addOn (motP, "motOn");
    motKnobs.push_back (addKnob (motP, "motDepth",   "Depth"));
    motKnobs.push_back (addKnob (motP, "motRate",    "Rate"));
    motKnobs.push_back (addKnob (motP, "motShimmer", "Shimmer"));

    // ----- BLOOM -----
    addOn (bloomP, "bloomOn");
    bloomKnobs.push_back (addKnob (bloomP, "bloomDrive", "Drive"));
    bloomKnobs.push_back (addKnob (bloomP, "bloomSing",  "Sing"));

    // ----- SATURATION -----
    addOn (satP, "satOn");
    satKnobs.push_back (addKnob (satP, "satDrive", "Drive"));
    satKnobs.push_back (addKnob (satP, "satChar",  "Character"));
    satKnobs.push_back (addKnob (satP, "satMix",   "Mix"));

    // ----- TAPE -----
    addOn (tapeP, "tapeOn");
    tapeKnobs.push_back (addKnob (tapeP, "tapeDrive",   "Drive"));
    tapeKnobs.push_back (addKnob (tapeP, "tapeFlutter", "Flutter"));
    tapeKnobs.push_back (addKnob (tapeP, "tapeHiss",    "Hiss"));
    cbSpeed = addCombo (tapeP, "tapeSpeed", "Speed");
    cbType  = addCombo (tapeP, "tapeType",  "Type");

    // ----- INFLATOR -----
    addOn (infP, "infOn");
    infKnobs.push_back (addKnob (infP, "infInput",  "Input"));
    infKnobs.push_back (addKnob (infP, "infEffect", "Effect"));
    infKnobs.push_back (addKnob (infP, "infCurve",  "Curve"));
    cProtect = addChip (infP, "infProtect", "BASS PROTECT");

    // delikatne słoje drewna w tle
    juce::Random rnd (77);
    for (int y = 0; y < 700; y += 3)
        grain.push_back ({ y, 0.012f + 0.03f * rnd.nextFloat(), 0.8f + 1.4f * rnd.nextFloat() });

    setSize (1200, 700);
    startTimerHz (10);
    timerCallback();
}

PetrofiaEditor::~PetrofiaEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

auto PetrofiaEditor::addKnob (Panel& p, const juce::String& id, const juce::String& name) -> Knob*
{
    knobs.push_back (std::make_unique<Knob>());
    auto* k = knobs.back().get();

    addAndMakeVisible (k->slider);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, p.colour);
    k->slider.setColour (juce::Slider::textBoxTextColourId, cream);
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k->slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);   // ponownie, już z naszym LookAndFeel

    k->label.setText (name.toUpperCase(), juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    k->label.setColour (juce::Label::textColourId, muted);
    k->label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k->label);

    k->att = std::make_unique<SliderAtt> (proc.apvts, id, k->slider);

    p.kids.push_back (&k->slider);
    p.kids.push_back (&k->label);
    return k;
}

auto PetrofiaEditor::addOn (Panel& p, const juce::String& id) -> Tgl*
{
    toggles.push_back (std::make_unique<Tgl>());
    auto* t = toggles.back().get();
    t->btn.setColour (juce::ToggleButton::tickColourId, p.colour);
    t->btn.setClickingTogglesState (true);
    addAndMakeVisible (t->btn);
    t->att = std::make_unique<ButtonAtt> (proc.apvts, id, t->btn);
    p.onParam = id;
    p.onBtn = &t->btn;
    return t;
}

auto PetrofiaEditor::addChip (Panel& p, const juce::String& id, const juce::String& text) -> Chip*
{
    chips.push_back (std::make_unique<Chip>());
    auto* c = chips.back().get();
    c->btn.setButtonText (text);
    c->btn.setColour (juce::ToggleButton::tickColourId, p.colour);
    c->btn.setClickingTogglesState (true);
    addAndMakeVisible (c->btn);
    c->att = std::make_unique<ButtonAtt> (proc.apvts, id, c->btn);
    if (&p != &masterP)
        p.kids.push_back (&c->btn);
    return c;
}

auto PetrofiaEditor::addCombo (Panel& p, const juce::String& id, const juce::String& name) -> Combo*
{
    combos.push_back (std::make_unique<Combo>());
    auto* c = combos.back().get();

    c->label.setText (name.toUpperCase(), juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centredLeft);
    c->label.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    c->label.setColour (juce::Label::textColourId, muted);
    c->label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (c->label);

    c->box.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (c->box);
    c->att = std::make_unique<ComboAtt> (proc.apvts, id, c->box);

    p.kids.push_back (&c->label);
    p.kids.push_back (&c->box);
    return c;
}

void PetrofiaEditor::placeKnobs (juce::Rectangle<int> area, std::vector<Knob*> ks)
{
    if (ks.empty())
        return;
    const int w = area.getWidth() / (int) ks.size();
    for (auto* k : ks)
    {
        auto cell = area.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (16));
        k->slider.setBounds (cell);
    }
}

void PetrofiaEditor::resized()
{
    const int m = 16, gap = 8, headerH = 140;

    auto area = getLocalBounds().reduced (m, 0);
    area.removeFromBottom (14);
    auto header = area.removeFromTop (headerH);
    header.removeFromLeft (290);

    // ---------- MASTER ----------
    masterP.bounds = header.withTrimmedTop (8).withTrimmedBottom (6);
    {
        auto mi = masterP.bounds.reduced (12, 0);
        mi.removeFromTop (30);
        mi.removeFromBottom (6);
        placeKnobs (mi.removeFromLeft (3 * 96), { kIntensity, kMix, kOut });
        mi.removeFromLeft (20);

        const int x0 = mi.getX();
        const int y0 = masterP.bounds.getY() + 12;
        modeSw->btn.setBounds (x0, y0, 150, 26);
        cDelta->btn.setBounds    (x0 + 160, y0 + 1, 96, 24);
        cAuto->btn.setBounds     (x0 + 264, y0 + 1, 112, 24);
        cBassMono->btn.setBounds (x0 + 384, y0 + 1, 104, 24);
        meter.setBounds (x0, y0 + 48, juce::jmin (mi.getWidth(), 488), 26);
    }

    // ---------- rząd 1: STRINGS ----------
    const int rowH = (area.getHeight() - 2 * gap) / 3;
    auto row1 = area.removeFromTop (rowH);
    area.removeFromTop (gap);
    auto row2 = area.removeFromTop (rowH);
    area.removeFromTop (gap);
    auto row3 = area;

    strP.bounds = row1;
    {
        auto si = strP.bounds.reduced (12, 0);
        si.removeFromTop (34);
        si.removeFromBottom (8);
        placeKnobs (si.removeFromLeft (9 * 80), strKnobs);
        si.removeFromLeft (16);

        const int cx = si.getX();
        const int cy = strP.bounds.getY() + 46;
        cBassLeft->btn.setBounds (cx,       cy, 120, 24);
        cKeyLock->btn.setBounds  (cx + 128, cy, 120, 24);
        cEco->btn.setBounds      (cx + 256, cy, 80,  24);

        const int y2 = cy + 24 + 24;
        cbRoot->label.setBounds  (cx,       y2,      110, 14);
        cbRoot->box.setBounds    (cx,       y2 + 16, 110, 26);
        cbScale->label.setBounds (cx + 122, y2,      150, 14);
        cbScale->box.setBounds   (cx + 122, y2 + 16, 150, 26);
    }

    // ---------- rząd 2: BODY | EMBRACE | MOTION | BLOOM ----------
    auto layoutKnobPanel = [this] (Panel& p, juce::Rectangle<int> r, std::vector<Knob*>& ks)
    {
        p.bounds = r;
        auto in = r.reduced (12, 0);
        in.removeFromTop (34);
        in.removeFromBottom (8);
        placeKnobs (in, ks);
    };

    const int totalW = area.getWidth() + 0;   // szerokość zawartości
    {
        const float unit = (float) (row2.getWidth() - 3 * gap - 4 * 24) / 13.0f;
        auto mk = [unit] (int cnt) { return juce::roundToInt (unit * (float) cnt) + 24; };

        auto bodyR = row2.removeFromLeft (mk (4));  row2.removeFromLeft (gap);
        auto embR  = row2.removeFromLeft (mk (4));  row2.removeFromLeft (gap);
        auto motR  = row2.removeFromLeft (mk (3));  row2.removeFromLeft (gap);
        auto bloomR = row2;

        layoutKnobPanel (bodyP,  bodyR,  bodyKnobs);
        layoutKnobPanel (embP,   embR,   embKnobs);
        layoutKnobPanel (motP,   motR,   motKnobs);
        layoutKnobPanel (bloomP, bloomR, bloomKnobs);
    }
    (void) totalW;

    // ---------- rząd 3: SATURATION | TAPE | INFLATOR ----------
    {
        const float unit = (float) (row3.getWidth() - 2 * gap - 3 * 24) / 12.5f;

        auto satR  = row3.removeFromLeft (juce::roundToInt (unit * 3.0f) + 24);  row3.removeFromLeft (gap);
        auto tapeR = row3.removeFromLeft (juce::roundToInt (unit * 5.0f) + 24);  row3.removeFromLeft (gap);
        auto infR  = row3;

        layoutKnobPanel (satP, satR, satKnobs);

        // TAPE: 3 pokrętła + dwie listy (Speed, Type)
        tapeP.bounds = tapeR;
        {
            auto in = tapeR.reduced (12, 0);
            in.removeFromTop (34);
            in.removeFromBottom (8);
            placeKnobs (in.removeFromLeft (juce::roundToInt (unit * 3.0f)), tapeKnobs);
            in.removeFromLeft (10);
            const int cx = in.getX();
            const int cy = tapeP.bounds.getY() + 42;
            cbSpeed->label.setBounds (cx, cy,      in.getWidth(), 14);
            cbSpeed->box.setBounds   (cx, cy + 16, in.getWidth(), 26);
            cbType->label.setBounds  (cx, cy + 56, in.getWidth(), 14);
            cbType->box.setBounds    (cx, cy + 72, in.getWidth(), 26);
        }

        // INFLATOR: 3 pokrętła + przełącznik ochrony basu
        infP.bounds = infR;
        {
            auto in = infR.reduced (12, 0);
            in.removeFromTop (34);
            in.removeFromBottom (8);
            placeKnobs (in.removeFromLeft (juce::roundToInt (unit * 3.0f)), infKnobs);
            in.removeFromLeft (6);
            cProtect->btn.setBounds (in.getX(), infP.bounds.getY() + 78, juce::jmin (in.getWidth(), 130), 24);
        }
    }

    // przełączniki ON/OFF w wierszach tytułowych
    for (auto* p : { &strP, &bodyP, &embP, &motP, &bloomP, &satP, &tapeP, &infP })
        if (p->onBtn != nullptr)
            p->onBtn->setBounds (p->bounds.getRight() - 12 - 66, p->bounds.getY() + 9, 66, 24);
}

void PetrofiaEditor::paint (juce::Graphics& g)
{
    // ciemny brąz z delikatnymi słojami
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBot, 0.0f, (float) getHeight(), false));
    g.fillAll();

    for (const auto& gr : grain)
    {
        g.setColour (panelLn.withAlpha (gr.a));
        g.fillRect (0.0f, (float) gr.y, (float) getWidth(), gr.t);
    }

    // poświata w rogu
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb8741c).withAlpha (0.16f),
                                             (float) getWidth() * 0.15f, 0.0f,
                                             juce::Colours::transparentBlack,
                                             (float) getWidth() * 0.15f, 320.0f, true));
    g.fillAll();

    // nagłówek
    g.setColour (cream);
    g.setFont (juce::Font (juce::FontOptions (40.0f, juce::Font::bold)));
    g.drawText ("PETROFIA", 24, 14, 270, 50, juce::Justification::centredLeft);

    g.setColour (muted);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText ("inside the instrument", 26, 62, 260, 20, juce::Justification::centredLeft);

    // motyw strun pod tytułem
    for (int i = 0; i < 9; ++i)
    {
        const float y = 96.0f + (float) i * 4.5f;
        g.setColour (gold.withAlpha (0.50f - 0.04f * (float) i));
        g.drawLine (26.0f, y, 270.0f - (float) i * 12.0f, y, 1.0f);
    }

    // panele
    for (auto* p : { &masterP, &strP, &bodyP, &embP, &motP, &bloomP, &satP, &tapeP, &infP })
    {
        auto r = p->bounds.toFloat();

        g.setColour (panel.withAlpha (0.94f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (panelLn);
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.4f);

        g.setColour (p->colour);
        g.fillRoundedRectangle (r.getX() + 14.0f, r.getY() + 15.0f, 4.0f, 14.0f, 2.0f);

        g.setColour (cream.withAlpha (0.94f));
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText (p->title, (int) r.getX() + 25, (int) r.getY() + 9, 150, 26, juce::Justification::centredLeft);
    }
}

void PetrofiaEditor::timerCallback()
{
    // wyłączona sekcja jest przyciemniona (nadal można nią kręcić)
    for (auto* p : { &strP, &bodyP, &embP, &motP, &bloomP, &satP, &tapeP, &infP })
    {
        if (p->onParam.isEmpty())
            continue;

        const bool on = proc.apvts.getRawParameterValue (p->onParam)->load() > 0.5f;
        for (auto* kid : p->kids)
            kid->setAlpha (on ? 1.0f : 0.4f);
    }
}
