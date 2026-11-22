#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "Dsp.h"

class PetrofiaProcessor : public juce::AudioProcessor
{
public:
    PetrofiaProcessor();
    ~PetrofiaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Petrofia"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> corr { 0.0f };   // korelacja L/R na wyjściu (-1..1), do miernika w interfejsie

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // sekcje
    petro::PianoStrings strings;
    petro::SoundBody    body;
    petro::Embrace      embrace;
    petro::Motion       motion;
    petro::ColorChain   color;

    juce::dsp::Oversampling<float> oversampling { 2, 1,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };

    juce::AudioBuffer<float> work, dryBuf;
    petro::DelayLine dryDelay[2];
    int dryDelaySamples = 0, blockMax = 512;
    double sr = 44100.0;

    petro::Biquad strInHp, railHp[2], sideHp;
    float env = 0.0f, envAtk = 0.01f, envRel = 0.001f;
    bool strWas = false, bodyWas = false, embWas = false;

    // wygładzanie
    juce::SmoothedValue<float> scaleSm, sStr, sBody, sEmb, sMotD, sMotS, mixSm, outSm;

    // poziomy / korelacja
    float eDry = 0.0f, eWet = 0.0f, autoG = 1.0f, cLR = 0.0f, cLL = 0.0f, cRR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PetrofiaProcessor)
};
