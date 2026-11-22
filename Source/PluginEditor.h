#pragma once
#include "PluginProcessor.h"

namespace PetroColours
{
    // główny odcień: RGB(105, 61, 8), czyli ciemny brąz (obramowania paneli, akcenty tła)
    const juce::Colour bgTop   { 0xff2a1805 };
    const juce::Colour bgBot   { 0xff150c02 };
    const juce::Colour panel   { 0xff3a2207 };
    const juce::Colour panelLn { 0xff693d08 };
    const juce::Colour inset   { 0xff1c1004 };
    const juce::Colour edge    { 0xff8a5a1c };
    const juce::Colour edgeHi  { 0xffc98a2e };
    const juce::Colour cream   { 0xfff3e6c8 };
    const juce::Colour muted   { 0xffb8996a };

    const juce::Colour gold      { 0xffe9b85a };   // struny
    const juce::Colour copper    { 0xffdb8444 };   // pudło
    const juce::Colour honey     { 0xfff0c874 };   // obejmująca przestrzeń
    const juce::Colour verdigris { 0xff6cc2a4 };   // ruch
    const juce::Colour rose      { 0xffd9786a };   // bloom
    const juce::Colour amber     { 0xfff2a23c };   // saturacja, taśma, inflator
}

class PetroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PetroLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// Pokrętło: scroll myszy zmienia wartość, Shift/Ctrl + scroll = precyzyjnie
class KnobSlider : public juce::Slider
{
public:
    KnobSlider()
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
        setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                             juce::MathConstants<float>::pi * 2.75f, true);
        setScrollWheelEnabled (true);
        setMouseDragSensitivity (220);
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        auto d = w;
        if (e.mods.isShiftDown() || e.mods.isCtrlDown())
        {
            d.deltaX *= 0.2f;
            d.deltaY *= 0.2f;
        }
        juce::Slider::mouseWheelMove (e, d);
    }
};

// Segmentowy przełącznik dwustanowy (np. SAFE | EXTREME)
class ModeSwitch : public juce::ToggleButton
{
public:
    juce::String leftLabel { "SAFE" }, rightLabel { "EXTREME" };
protected:
    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

// Mały przełącznik (dioda + nazwa)
class ChipToggle : public juce::ToggleButton
{
protected:
    void paintButton (juce::Graphics&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

// Miernik korelacji L/R (-1 ... +1)
class CorrMeter : public juce::Component, private juce::Timer
{
public:
    explicit CorrMeter (std::atomic<float>& source) : src (source) { startTimerHz (20); }
    ~CorrMeter() override { stopTimer(); }
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override
    {
        v += 0.35f * (src.load() - v);
        repaint();
    }

    std::atomic<float>& src;
    float v = 0.0f;
};

class PetrofiaEditor : public juce::AudioProcessorEditor,
                       private juce::Timer
{
public:
    explicit PetrofiaEditor (PetrofiaProcessor&);
    ~PetrofiaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob  { KnobSlider slider; juce::Label label; std::unique_ptr<SliderAtt> att; };
    struct Tgl   { juce::ToggleButton btn; std::unique_ptr<ButtonAtt> att; };
    struct Chip  { ChipToggle btn; std::unique_ptr<ButtonAtt> att; };
    struct ModeSw { ModeSwitch btn; std::unique_ptr<ButtonAtt> att; };
    struct Combo { juce::ComboBox box; juce::Label label; std::unique_ptr<ComboAtt> att; };

    struct Panel
    {
        juce::String title;
        juce::Colour colour;
        juce::Rectangle<int> bounds;
        juce::String onParam;
        juce::ToggleButton* onBtn = nullptr;
        std::vector<juce::Component*> kids;
    };

    auto addKnob  (Panel&, const juce::String& id, const juce::String& name) -> Knob*;
    auto addOn    (Panel&, const juce::String& id) -> Tgl*;
    auto addChip  (Panel&, const juce::String& id, const juce::String& text) -> Chip*;
    auto addCombo (Panel&, const juce::String& id, const juce::String& name) -> Combo*;

    void placeKnobs (juce::Rectangle<int> area, std::vector<Knob*> ks);
    void timerCallback() override;

    PetrofiaProcessor& proc;
    PetroLookAndFeel laf;

    Panel masterP, strP, bodyP, embP, motP, bloomP, satP, tapeP, infP;

    std::vector<std::unique_ptr<Knob>>   knobs;
    std::vector<std::unique_ptr<Tgl>>    toggles;
    std::vector<std::unique_ptr<Chip>>   chips;
    std::vector<std::unique_ptr<Combo>>  combos;
    std::unique_ptr<ModeSw> modeSw;
    CorrMeter meter;

    // wskaźniki do elementów (do układu)
    Knob *kIntensity = nullptr, *kMix = nullptr, *kOut = nullptr;
    std::vector<Knob*> strKnobs, bodyKnobs, embKnobs, motKnobs, bloomKnobs, satKnobs, tapeKnobs, infKnobs;
    Chip *cDelta = nullptr, *cAuto = nullptr, *cBassMono = nullptr,
         *cBassLeft = nullptr, *cKeyLock = nullptr, *cEco = nullptr, *cProtect = nullptr;
    Combo *cbRoot = nullptr, *cbScale = nullptr, *cbSpeed = nullptr, *cbType = nullptr;

    struct Grain { int y; float a; float t; };
    std::vector<Grain> grain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PetrofiaEditor)
};
