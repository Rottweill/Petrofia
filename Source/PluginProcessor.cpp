#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    using StrFn = std::function<juce::String (float, int)>;
    using ValFn = std::function<float (const juce::String&)>;

    const juce::StringArray noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    juce::String midiName (int m)
    {
        return noteNames[((m % 12) + 12) % 12] + juce::String (m / 12 - 1);
    }

    float noteFromString (const juce::String& t)
    {
        auto s = t.trim().toUpperCase();
        if (s.isEmpty() || juce::CharacterFunctions::isDigit (s[0]) || s[0] == '-')
            return s.getFloatValue();

        juce::String name = s.substring (0, 1);
        int pos = 1;
        if (s.length() > 1 && s[1] == '#') { name += "#"; pos = 2; }
        const int pc = noteNames.indexOf (name);
        const int oct = s.substring (pos).getIntValue();
        return (float) ((oct + 1) * 12 + juce::jmax (0, pc));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout PetrofiaProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto add = [&] (const char* id, const char* name, R range, float def, StrFn toStr, ValFn fromStr)
    {
        p.push_back (std::make_unique<P> (juce::ParameterID { id, 1 }, name, range, def,
                                          juce::AudioParameterFloatAttributes()
                                              .withStringFromValueFunction (toStr)
                                              .withValueFromStringFunction (fromStr)));
    };
    auto addBool = [&] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };
    auto addChoice = [&] (const char* id, const char* name, juce::StringArray items, int def)
    {
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def));
    };

    const StrFn pctS = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    const ValFn pctV = [] (const juce::String& t) { return t.getFloatValue() * 0.01f; };
    const ValFn numV = [] (const juce::String& t) { return t.getFloatValue(); };
    const StrFn dbS  = [] (float v, int) { return juce::String (v, 1) + " dB"; };
    const StrFn secS = [] (float v, int) { return juce::String (v, 2) + " s"; };
    const StrFn ctS  = [] (float v, int) { return juce::String (v, 1) + " ct"; };
    const StrFn hzS  = [] (float v, int) { return juce::String (v, 1) + " Hz"; };
    const StrFn xS   = [] (float v, int) { return juce::String (v, 2) + "x"; };
    const StrFn noteS = [] (float v, int) { return midiName (juce::roundToInt (v)); };
    const ValFn noteV = [] (const juce::String& t) { return noteFromString (t); };

    // --- MASTER ---
    add ("intensity", "Intensity",    R (0.0f, 1.0f, 0.001f),    0.8f, pctS, pctV);
    add ("mix",       "Mix",          R (0.0f, 1.0f, 0.001f),    1.0f, pctS, pctV);
    add ("outGain",   "Output",       R (-12.0f, 12.0f, 0.1f),   0.0f, dbS,  numV);
    addBool ("extreme",   "Extreme Mode",   false);
    addBool ("delta",     "Delta",          false);
    addBool ("autoLevel", "Auto Level",     true);
    addBool ("bassMono",  "Bass Mono",      true);

    // --- STRINGS ---
    addBool ("strOn", "Strings On", true);
    add ("strAmt",     "Strings Amount",  R (0.0f, 1.0f, 0.001f),           0.5f,   pctS, pctV);
    add ("strSustain", "Strings Sustain", R (0.2f, 8.0f, 0.01f, 0.5f),      2.0f,   secS, numV);
    add ("strDamp",    "Strings Damping", R (0.0f, 1.0f, 0.001f),           0.5f,   pctS, pctV);
    add ("strDetune",  "Strings Detune",  R (0.0f, 15.0f, 0.1f),            4.0f,   ctS,  numV);
    add ("strSpread",  "Strings Spread",  R (0.0f, 1.0f, 0.001f),           0.6f,   pctS, pctV);
    add ("strClarity", "Strings Clarity", R (0.0f, 1.0f, 0.001f),           0.4f,   pctS, pctV);
    add ("strLow",     "Strings Low",     R (21.0f, 72.0f, 1.0f),           36.0f,  noteS, noteV);
    add ("strHigh",    "Strings High",    R (60.0f, 108.0f, 1.0f),          96.0f,  noteS, noteV);
    add ("strTune",    "Strings Tuning",  R (415.0f, 466.0f, 0.1f),         440.0f, hzS,  numV);
    addBool ("strKeyLock",  "Strings Key Lock",  false);
    addChoice ("strRoot",   "Strings Root",  noteNames, 0);
    addChoice ("strScale",  "Strings Scale", juce::StringArray { "Major", "Minor", "Pent Major", "Pent Minor" }, 0);
    addBool ("strBassLeft", "Strings Bass Left", false);
    addBool ("strEco",      "Strings Eco",       false);

    // --- BODY ---
    addBool ("bodyOn", "Body On", true);
    add ("bodyAmt",  "Body Amount", R (0.0f, 1.0f, 0.001f), 0.5f, pctS, pctV);
    add ("bodySize", "Body Size",   R (0.0f, 1.0f, 0.001f), 0.5f, pctS, pctV);
    add ("bodyWood", "Body Wood",   R (0.0f, 1.0f, 0.001f), 0.5f, pctS, pctV);
    add ("bodyRoom", "Body Room",   R (0.0f, 1.0f, 0.001f), 0.4f, pctS, pctV);

    // --- EMBRACE ---
    addBool ("embOn", "Embrace On", true);
    add ("embAmt",   "Embrace Amount",   R (0.0f, 1.0f, 0.001f), 0.5f, pctS, pctV);
    add ("embPos",   "Embrace Position", R (0.0f, 1.0f, 0.001f), 0.5f, pctS, pctV);
    add ("embWidth", "Embrace Width",    R (0.0f, 1.0f, 0.001f), 0.6f, pctS, pctV);
    add ("embDecay", "Embrace Decay",    R (0.0f, 1.0f, 0.001f), 0.4f, pctS, pctV);

    // --- MOTION ---
    addBool ("motOn", "Motion On", true);
    add ("motDepth",   "Motion Depth",   R (0.0f, 1.0f, 0.001f),       0.5f, pctS, pctV);
    add ("motRate",    "Motion Rate",    R (0.25f, 4.0f, 0.01f, 0.5f), 1.0f, xS,   numV);
    add ("motShimmer", "Motion Shimmer", R (0.0f, 1.0f, 0.001f),       0.3f, pctS, pctV);

    // --- BLOOM ---
    addBool ("bloomOn", "Bloom On", true);
    add ("bloomDrive", "Bloom Drive", R (0.0f, 1.0f, 0.001f), 0.4f, pctS, pctV);
    add ("bloomSing",  "Bloom Sing",  R (0.0f, 1.0f, 0.001f), 0.4f, pctS, pctV);

    // --- SATURATION ---
    addBool ("satOn", "Saturation On", true);
    add ("satDrive", "Saturation Drive",     R (0.0f, 1.0f, 0.001f), 0.4f, pctS, pctV);
    add ("satChar",  "Saturation Character", R (0.0f, 1.0f, 0.001f), 0.5f, pctS, pctV);
    add ("satMix",   "Saturation Mix",       R (0.0f, 1.0f, 0.001f), 1.0f, pctS, pctV);

    // --- TAPE ---
    addBool ("tapeOn", "Tape On", true);
    addChoice ("tapeSpeed", "Tape Speed", juce::StringArray { "7.5 ips", "15 ips", "30 ips" }, 1);
    addChoice ("tapeType",  "Tape Type",  juce::StringArray { "Studio", "Warm", "Crush" }, 1);
    add ("tapeDrive",   "Tape Drive",   R (0.0f, 1.0f, 0.001f), 0.4f, pctS, pctV);
    add ("tapeFlutter", "Tape Flutter", R (0.0f, 1.0f, 0.001f), 0.2f, pctS, pctV);
    add ("tapeHiss",    "Tape Hiss",    R (0.0f, 1.0f, 0.001f), 0.0f, pctS, pctV);

    // --- INFLATOR ---
    addBool ("infOn", "Inflator On", false);
    add ("infInput",  "Inflator Input",  R (-6.0f, 12.0f, 0.1f),  0.0f, dbS,  numV);
    add ("infEffect", "Inflator Effect", R (0.0f, 1.0f, 0.001f),  0.4f, pctS, pctV);
    add ("infCurve",  "Inflator Curve",  R (-1.0f, 1.0f, 0.001f), 0.0f, pctS, pctV);
    addBool ("infProtect", "Inflator Bass Protect", true);

    return { p.begin(), p.end() };
}

PetrofiaProcessor::PetrofiaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool PetrofiaProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void PetrofiaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    blockMax = juce::jmax (64, samplesPerBlock);

    strings.prepare (sr);
    body.prepare (sr);
    embrace.prepare (sr);
    motion.prepare (sr);
    color.prepare (sr);

    oversampling.initProcessing ((size_t) blockMax);
    oversampling.reset();

    const int latency = (int) std::round (oversampling.getLatencyInSamples()) + color.flutterBaseSamples();
    setLatencySamples (latency);
    dryDelaySamples = latency;
    for (auto& d : dryDelay)
        d.prepare (latency + 16);

    work.setSize (2, blockMax, false, true, false);
    dryBuf.setSize (2, blockMax, false, true, false);

    strInHp.highpass (sr, 25.0f);
    for (auto& h : railHp) h.highpass (sr, 80.0f);
    sideHp.highpass (sr, 120.0f);
    strInHp.reset(); sideHp.reset();
    for (auto& h : railHp) h.reset();

    env = 0.0f;
    envAtk = petro::coefMs (5.0f, sr);
    envRel = petro::coefMs (120.0f, sr);
    strWas = bodyWas = embWas = false;

    for (auto* s : { &scaleSm, &sStr, &sBody, &sEmb, &sMotD, &sMotS, &mixSm, &outSm })
    {
        s->reset (sr, 0.03);
        s->setCurrentAndTargetValue (0.0f);
    }
    mixSm.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load());
    outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("outGain")->load()));

    eDry = eWet = cLR = cLL = cRR = 0.0f;
    autoG = 1.0f;
}

void PetrofiaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (buffer.getNumChannels() < 2)
        return;

    const int n = buffer.getNumSamples();

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    auto on  = [&] (const char* id) { return get (id) > 0.5f; };

    // ---------- MASTER ----------
    const bool extreme  = on ("extreme");
    const bool delta    = on ("delta");
    const bool autoOn   = on ("autoLevel");
    const bool bassMono = on ("bassMono");

    // Intensity: nieliniowe skalowanie (dół skali bardzo delikatny); Master-safe vs Extreme
    const float scaleT = std::pow (get ("intensity"), 1.6f) * (extreme ? 1.6f : 0.6f);
    scaleSm.setTargetValue (scaleT);
    mixSm.setTargetValue (get ("mix"));
    outSm.setTargetValue (juce::Decibels::decibelsToGain (get ("outGain")));

    // ---------- sekcje działające w paśmie bazowym ----------
    const float fStr  = on ("strOn")   ? 1.0f : 0.0f;
    const float fBody = on ("bodyOn")  ? 1.0f : 0.0f;
    const float fEmb  = on ("embOn")   ? 1.0f : 0.0f;
    const float fMot  = on ("motOn")   ? 1.0f : 0.0f;

    sStr .setTargetValue (get ("strAmt")     * fStr);
    sBody.setTargetValue (get ("bodyAmt")    * fBody);
    sEmb .setTargetValue (get ("embAmt")     * fEmb);
    sMotD.setTargetValue (get ("motDepth")   * fMot);
    sMotS.setTargetValue (get ("motShimmer") * fMot);

    petro::PianoStrings::Params sp;
    sp.sustain  = get ("strSustain");
    sp.damping  = get ("strDamp");
    sp.detune   = get ("strDetune");
    sp.spread   = get ("strSpread");
    sp.tuning   = get ("strTune");
    sp.low      = juce::roundToInt (get ("strLow"));
    sp.high     = juce::roundToInt (get ("strHigh"));
    sp.root     = juce::roundToInt (get ("strRoot"));
    sp.scale    = juce::roundToInt (get ("strScale"));
    sp.keyLock  = on ("strKeyLock");
    sp.bassLeft = on ("strBassLeft");
    sp.eco      = on ("strEco");
    strings.setParams (sp);

    const float clarity = get ("strClarity");
    body.setParams (get ("bodySize"), get ("bodyWood"), get ("bodyRoom"));
    embrace.setParams (get ("embPos"), get ("embWidth"), get ("embDecay"));
    motion.setRate (get ("motRate"));

    // ---------- sekcje kolorystyczne (2x oversampling) ----------
    const float fBloom = on ("bloomOn") ? scaleT : 0.0f;
    const float fSat   = on ("satOn")   ? scaleT : 0.0f;
    const float fTape  = on ("tapeOn")  ? scaleT : 0.0f;
    const float fInf   = on ("infOn")   ? scaleT : 0.0f;

    petro::ColorChain::Params cp;
    cp.bloomDrive  = get ("bloomDrive")  * fBloom;
    cp.bloomSing   = get ("bloomSing")   * fBloom;
    cp.satDrive    = get ("satDrive")    * fSat;
    cp.satChar     = get ("satChar");
    cp.satMix      = get ("satMix");
    cp.tapeSpeed   = juce::roundToInt (get ("tapeSpeed"));
    cp.tapeType    = juce::roundToInt (get ("tapeType"));
    cp.tapeDrive   = get ("tapeDrive")   * fTape;
    cp.tapeFlutter = get ("tapeFlutter") * fTape;
    cp.tapeHiss    = get ("tapeHiss")    * fTape;
    cp.infInputDb  = get ("infInput");
    cp.infEffect   = get ("infEffect")   * fInf;
    cp.infCurve    = get ("infCurve");
    cp.infProtect  = on ("infProtect");
    color.setParams (cp);

    const float agC   = petro::coefMs (500.0f, sr);
    const float lvC   = petro::coefMs (300.0f, sr);
    const float corrC = petro::coefMs (300.0f, sr);

    for (int start = 0; start < n; start += blockMax)
    {
        const int len = juce::jmin (blockMax, n - start);

        float* bl = buffer.getWritePointer (0) + start;
        float* br = buffer.getWritePointer (1) + start;
        float* wl = work.getWritePointer (0);
        float* wr = work.getWritePointer (1);
        float* dl = dryBuf.getWritePointer (0);
        float* dr = dryBuf.getWritePointer (1);

        // ===== część 1: tor równoległy (STRINGS, BODY, EMBRACE) i MOTION - pasmo bazowe =====
        for (int i = 0; i < len; ++i)
        {
            const float inL = bl[i], inR = br[i];

            dryDelay[0].push (inL);
            dryDelay[1].push (inR);
            dl[i] = dryDelay[0].readInt (dryDelaySamples + 1);
            dr[i] = dryDelay[1].readInt (dryDelaySamples + 1);

            const bool strAct  = sStr.isSmoothing()  || sStr.getTargetValue()  > 1.0e-6f;
            const bool bodyAct = sBody.isSmoothing() || sBody.getTargetValue() > 1.0e-6f;
            const bool embAct  = sEmb.isSmoothing()  || sEmb.getTargetValue()  > 1.0e-6f;
            const bool motAct  = sMotD.isSmoothing() || sMotD.getTargetValue() > 1.0e-6f
                              || sMotS.isSmoothing() || sMotS.getTargetValue() > 1.0e-6f;

            const float scale = scaleSm.getNextValue();
            const float aStr  = sStr.getNextValue()  * scale;
            const float aBody = sBody.getNextValue() * scale;
            const float aEmb  = sEmb.getNextValue()  * scale;
            const float mDep  = sMotD.getNextValue() * scale;
            const float mShm  = sMotS.getNextValue() * scale;

            const float mono = 0.5f * (inL + inR);
            const float am = std::abs (mono);
            env += (am > env ? envAtk : envRel) * (am - env);

            // STRINGS: rezonujące struny, pan wg wysokości (opcjonalnie), "clarity" = ducking szyny przez wejście
            float railSL = 0.0f, railSR = 0.0f;
            if (strAct)
            {
                if (! strWas) { strings.reset(); strWas = true; }
                float sl = 0.0f, sr_ = 0.0f;
                strings.process (strInHp.process (mono), sl, sr_);
                const float duck = 1.0f / (1.0f + clarity * 6.0f * env);
                railSL = railHp[0].process (sl)  * 0.35f * aStr * duck;
                railSR = railHp[1].process (sr_) * 0.35f * aStr * duck;
            }
            else strWas = false;

            // BODY: pudło zasilane suchym sygnałem i strunami
            float bL = 0.0f, bR = 0.0f;
            if (bodyAct)
            {
                if (! bodyWas) { body.reset(); bodyWas = true; }
                body.process (inL + railSL, inR + railSR, bL, bR);
                bL *= 0.8f * aBody;
                bR *= 0.8f * aBody;
            }
            else bodyWas = false;

            // EMBRACE: wnętrze instrumentu (zasilane całością)
            float eL = 0.0f, eR = 0.0f;
            if (embAct)
            {
                if (! embWas) { embrace.reset(); embWas = true; }
                embrace.process (inL + railSL + bL, inR + railSR + bR, eL, eR);
                eL *= aEmb;
                eR *= aEmb;
            }
            else embWas = false;

            float yL = inL + railSL + bL + eL;
            float yR = inR + railSR + bR + eR;

            // MOTION: mikromodulacje M/S
            if (motAct)
            {
                motion.setLive (mDep, mShm);
                motion.process (yL, yR);
            }

            wl[i] = yL;
            wr[i] = yR;
        }

        // ===== część 2: BLOOM -> SATURATION -> TAPE -> INFLATOR na 2x oversamplingu =====
        {
            juce::dsp::AudioBlock<const float> inBlk (work.getArrayOfReadPointers(), 2, (size_t) len);
            auto& up = oversampling.processSamplesUp (inBlk);
            color.process2x (up.getChannelPointer (0), up.getChannelPointer (1), (int) up.getNumSamples());

            juce::dsp::AudioBlock<float> outBlk (work.getArrayOfWritePointers(), 2, (size_t) len);
            oversampling.processSamplesDown (outBlk);
        }

        // ===== część 3: wyjście (bas mono, auto-level, mix, delta, output, miernik) =====
        for (int i = 0; i < len; ++i)
        {
            float wetL = wl[i], wetR = wr[i];
            const float dryL = dl[i], dryR = dr[i];

            // dół pasma zawsze mono (side powyżej ok. 120 Hz)
            const float M = 0.5f * (wetL + wetR);
            float S = 0.5f * (wetL - wetR);
            const float Shp = sideHp.process (S);
            if (bassMono) S = Shp;
            wetL = M + S;
            wetR = M - S;

            // uczciwe A/B: wyrównanie głośności mokrego sygnału do suchego
            eDry += lvC * (0.5f * (dryL * dryL + dryR * dryR) - eDry);
            eWet += lvC * (0.5f * (wetL * wetL + wetR * wetR) - eWet);
            float gT = 1.0f;
            if (autoOn && eDry > 1.0e-7f && eWet > 1.0e-9f)
                gT = juce::jlimit (0.5f, 2.0f, std::sqrt (eDry / eWet));
            autoG += agC * (gT - autoG);
            wetL *= autoG;
            wetR *= autoG;

            const float mixV = mixSm.getNextValue();
            float oL = dryL + mixV * (wetL - dryL);
            float oR = dryR + mixV * (wetR - dryR);
            if (delta)
            {
                oL = mixV * (wetL - dryL);
                oR = mixV * (wetR - dryR);
            }

            const float og = outSm.getNextValue();
            oL *= og;
            oR *= og;

            bl[i] = oL;
            br[i] = oR;

            cLR += corrC * (oL * oR - cLR);
            cLL += corrC * (oL * oL - cLL);
            cRR += corrC * (oR * oR - cRR);
        }
    }

    corr.store (cLR / std::sqrt (cLL * cRR + 1.0e-12f));
}

juce::AudioProcessorEditor* PetrofiaProcessor::createEditor()
{
    return new PetrofiaEditor (*this);
}

void PetrofiaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PetrofiaProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PetrofiaProcessor();
}
