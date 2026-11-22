#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace petro
{
constexpr float kPi    = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float coefHz (float fc, double sr)
{
    return 1.0f - std::exp (-kTwoPi * fc / (float) sr);
}

inline float coefMs (float ms, double sr)
{
    return 1.0f - std::exp (-1.0f / (juce::jmax (0.01f, ms) * 0.001f * (float) sr));
}

//==============================================================================
// Podstawowe klocki
//==============================================================================
struct DelayLine
{
    std::vector<float> buf;
    int w = 0, n = 0;

    void prepare (int size) { n = juce::jmax (4, size); buf.assign ((size_t) n, 0.0f); w = 0; }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }

    void push (float x)
    {
        buf[(size_t) w] = x;
        if (++w >= n) w = 0;
    }

    // opóźnienie w próbkach (>= 1), interpolacja liniowa
    float read (float d) const
    {
        float rp = (float) w - d;
        while (rp < 0.0f) rp += (float) n;
        int i1 = (int) rp;
        const float f = rp - (float) i1;
        if (i1 >= n) i1 -= n;
        int i2 = i1 + 1;
        if (i2 >= n) i2 = 0;
        return buf[(size_t) i1] + f * (buf[(size_t) i2] - buf[(size_t) i1]);
    }

    // opóźnienie całkowite: d = 1 oznacza ostatnio wstawioną próbkę (zero opóźnienia)
    float readInt (int d) const
    {
        int i = w - d;
        while (i < 0) i += n;
        return buf[(size_t) i];
    }
};

struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f, z1 = 0.0f, z2 = 0.0f;

    void setCoeffs (float nb0, float nb1, float nb2, float a0, float na1, float na2)
    {
        const float r = 1.0f / a0;
        b0 = nb0 * r; b1 = nb1 * r; b2 = nb2 * r; a1 = na1 * r; a2 = na2 * r;
    }

    void highpass (double sr, float fc, float q = 0.70710678f)
    {
        const float w0 = kTwoPi * fc / (float) sr, cs = std::cos (w0), al = std::sin (w0) / (2.0f * q);
        setCoeffs ((1.0f + cs) * 0.5f, -(1.0f + cs), (1.0f + cs) * 0.5f, 1.0f + al, -2.0f * cs, 1.0f - al);
    }

    void lowpass (double sr, float fc, float q = 0.70710678f)
    {
        const float w0 = kTwoPi * fc / (float) sr, cs = std::cos (w0), al = std::sin (w0) / (2.0f * q);
        setCoeffs ((1.0f - cs) * 0.5f, 1.0f - cs, (1.0f - cs) * 0.5f, 1.0f + al, -2.0f * cs, 1.0f - al);
    }

    void peaking (double sr, float fc, float q, float gainDb)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float w0 = kTwoPi * fc / (float) sr, cs = std::cos (w0), al = std::sin (w0) / (2.0f * q);
        setCoeffs (1.0f + al * A, -2.0f * cs, 1.0f - al * A, 1.0f + al / A, -2.0f * cs, 1.0f - al / A);
    }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() { z1 = z2 = 0.0f; }
};

// Filtr stanu (TPT): zwraca pasmo o wzmocnieniu 1 w środku, stabilny przy modulacji
struct SVFBand
{
    float ic1 = 0.0f, ic2 = 0.0f, k = 0.5f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;

    void set (float fc, float q, double sr)
    {
        const float f = juce::jmin (fc, 0.45f * (float) sr);
        const float g = std::tan (kPi * f / (float) sr);
        k  = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    float process (float x)
    {
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return k * v1;
    }

    void reset() { ic1 = ic2 = 0.0f; }
};

//==============================================================================
// STRINGS: 88 strun (po 2 na nutę), pobudzanych wejściem (rezonans sympatyczny)
//==============================================================================
class PianoStrings
{
public:
    static constexpr int numNotes = 88;   // MIDI 21..108

    struct Params
    {
        float sustain = 2.0f, damping = 0.5f, detune = 4.0f, spread = 0.6f, tuning = 440.0f;
        int low = 36, high = 96, root = 0, scale = 0;
        bool keyLock = false, bassLeft = false, eco = false;
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        strs.assign ((size_t) (numNotes * 2), Str());
        for (int note = 0; note < numNotes; ++note)
        {
            const float fMin = 27.5f * std::pow (2.0f, (float) note / 12.0f) * 0.92f;
            const int size = (int) ((float) sr / fMin) + 16;
            for (int k = 0; k < 2; ++k)
            {
                auto& s = strs[(size_t) (note * 2 + k)];
                s.buf.assign ((size_t) size, 0.0f);
                s.n = size;
            }
        }
        gateCoef = coefMs (30.0f, sr);
        haveParams = false;
    }

    void reset()
    {
        for (auto& s : strs)
        {
            std::fill (s.buf.begin(), s.buf.end(), 0.0f);
            s.lp = 0.0f;
            s.w = 0;
            s.gate = 0.0f;
            s.cleared = true;
        }
    }

    void setParams (const Params& p)
    {
        if (haveParams && same (p, cur))
            return;
        cur = p;
        haveParams = true;

        static const int majorS[7]   = { 0, 2, 4, 5, 7, 9, 11 };
        static const int minorS[7]   = { 0, 2, 3, 5, 7, 8, 10 };
        static const int pentMaj[5]  = { 0, 2, 4, 7, 9 };
        static const int pentMin[5]  = { 0, 3, 5, 7, 10 };

        const int* sc = majorS;
        int scLen = 7;
        if (p.scale == 1) { sc = minorS; }
        else if (p.scale == 2) { sc = pentMaj; scLen = 5; }
        else if (p.scale == 3) { sc = pentMin; scLen = 5; }

        const float fc = 12000.0f * std::pow (0.08f, p.damping);
        const float dampC = coefHz (fc, sr);

        for (int note = 0; note < numNotes; ++note)
        {
            const int midi = 21 + note;
            bool active = (midi >= p.low && midi <= p.high);
            if (active && p.keyLock)
            {
                const int pc = (((midi - p.root) % 12) + 12) % 12;
                bool inScale = false;
                for (int i = 0; i < scLen; ++i)
                    if (sc[i] == pc) inScale = true;
                active = inScale;
            }

            const float f0 = p.tuning * std::pow (2.0f, (float) (midi - 69) / 12.0f);
            const float notePan = p.bassLeft ? ((float) note / 87.0f * 2.0f - 1.0f) * p.spread : 0.0f;

            for (int k = 0; k < 2; ++k)
            {
                auto& s = strs[(size_t) (note * 2 + k)];
                const float sign = (k == 0) ? -1.0f : 1.0f;
                const float cents = -sign * 0.5f * p.detune;
                const float f = f0 * std::pow (2.0f, cents / 1200.0f);
                const float loopLen = (float) sr / f;

                s.damp  = dampC;
                s.delay = juce::jlimit (2.0f, (float) (s.n - 2), loopLen - (1.0f - s.damp) / s.damp);

                const float t60 = juce::jlimit (0.1f, 12.0f, p.sustain * std::pow (f / 261.6f, -0.25f));
                s.g   = std::pow (10.0f, -3.0f * loopLen / ((float) sr * t60));
                s.exc = 1.0f - s.g;

                float pan = p.bassLeft ? notePan + sign * 0.25f * p.spread
                                       : sign * 0.6f * p.spread;
                pan = juce::jlimit (-1.0f, 1.0f, pan);
                const float ang = (pan + 1.0f) * kPi * 0.25f;
                s.panL = std::cos (ang);
                s.panR = std::sin (ang);

                s.gateTarget = (active && ! (p.eco && k == 1)) ? 1.0f : 0.0f;
            }
        }
    }

    void process (float in, float& outL, float& outR)
    {
        float aL = 0.0f, aR = 0.0f;

        for (auto& s : strs)
        {
            s.gate += (s.gateTarget - s.gate) * gateCoef;

            if (s.gate < 1.0e-4f && s.gateTarget == 0.0f)
            {
                if (! s.cleared)
                {
                    std::fill (s.buf.begin(), s.buf.end(), 0.0f);
                    s.lp = 0.0f;
                    s.cleared = true;
                }
                s.gate = 0.0f;
                continue;
            }
            s.cleared = false;

            float rp = (float) s.w - s.delay;
            if (rp < 0.0f) rp += (float) s.n;
            int i1 = (int) rp;
            const float f = rp - (float) i1;
            if (i1 >= s.n) i1 -= s.n;
            int i2 = i1 + 1;
            if (i2 >= s.n) i2 = 0;

            const float out = s.buf[(size_t) i1] + f * (s.buf[(size_t) i2] - s.buf[(size_t) i1]);
            s.lp += s.damp * (out - s.lp);
            s.buf[(size_t) s.w] = in * s.exc * s.gate + s.g * s.lp;
            if (++s.w >= s.n) s.w = 0;

            const float o = out * s.gate;
            aL += o * s.panL;
            aR += o * s.panR;
        }

        outL = aL;
        outR = aR;
    }

private:
    struct Str
    {
        std::vector<float> buf;
        int w = 0, n = 4;
        float delay = 100.0f, g = 0.99f, lp = 0.0f, damp = 0.5f, exc = 0.01f;
        float gate = 0.0f, gateTarget = 0.0f, panL = 0.7f, panR = 0.7f;
        bool cleared = false;
    };

    static bool same (const Params& a, const Params& b)
    {
        return a.sustain == b.sustain && a.damping == b.damping && a.detune == b.detune
            && a.spread == b.spread && a.tuning == b.tuning && a.low == b.low && a.high == b.high
            && a.root == b.root && a.scale == b.scale && a.keyLock == b.keyLock
            && a.bassLeft == b.bassLeft && a.eco == b.eco;
    }

    std::vector<Str> strs;
    double sr = 44100.0;
    float gateCoef = 0.001f;
    Params cur;
    bool haveParams = false;
};

//==============================================================================
// BODY: drewniane pudło (rezonanse modalne) + pierwsze odbicia od wieka
//==============================================================================
class SoundBody
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        juce::Random rnd (1234);
        for (int k = 0; k < numModes; ++k)
        {
            auto& m = modes[k];
            m.baseF = 80.0f * std::pow (2500.0f / 80.0f, (float) k / (float) (numModes - 1))
                      * (0.92f + 0.16f * rnd.nextFloat());
            m.q   = 8.0f + 22.0f * rnd.nextFloat();
            m.amp = 0.4f + 0.6f * rnd.nextFloat();
            m.jL  = 1.0f + 0.02f * (rnd.nextFloat() * 2.0f - 1.0f);
            m.jR  = 1.0f + 0.02f * (rnd.nextFloat() * 2.0f - 1.0f);
            m.aL  = 0.7f + 0.6f * rnd.nextFloat();
            m.aR  = 0.7f + 0.6f * rnd.nextFloat();
        }
        lineL.prepare ((int) (sr * 0.06) + 8);
        lineR.prepare ((int) (sr * 0.06) + 8);
        lastSize = lastWood = -1.0f;
        reset();
    }

    void reset()
    {
        for (auto& m : modes) { m.sL.reset(); m.sR.reset(); }
        lineL.clear();
        lineR.clear();
        for (int t = 0; t < numTaps; ++t) lpL[t] = lpR[t] = 0.0f;
    }

    void setParams (float size, float wood, float roomAmt)
    {
        room = roomAmt;
        if (size == lastSize && wood == lastWood)
            return;
        lastSize = size;
        lastWood = wood;

        const float shift = std::pow (2.0f, (size - 0.5f) * 0.9f);
        for (auto& m : modes)
        {
            const float f = m.baseF * shift;
            const float tilt = juce::jlimit (0.2f, 4.0f, std::pow (f / 400.0f, (wood - 0.5f) * 1.4f));
            m.gL = m.amp * m.aL * tilt;
            m.gR = m.amp * m.aR * tilt;
            m.sL.set (f * m.jL, m.q, sr);
            m.sR.set (f * m.jR, m.q, sr);
        }

        static const float tL[numTaps] = { 2.3f, 4.1f, 6.7f, 9.9f, 14.3f, 19.0f };
        static const float tR[numTaps] = { 2.9f, 4.8f, 7.4f, 10.8f, 13.1f, 17.6f };
        static const float tG[numTaps] = { 0.60f, -0.50f, 0.42f, -0.34f, 0.26f, -0.20f };
        const float sc = 0.7f + 0.6f * size;
        for (int t = 0; t < numTaps; ++t)
        {
            tapSL[t] = tL[t] * 0.001f * (float) sr * sc;
            tapSR[t] = tR[t] * 0.001f * (float) sr * sc;
            tapG[t]  = tG[t];
        }
        tapC = coefHz (3000.0f + 6000.0f * wood, sr);
    }

    void process (float inL, float inR, float& oL, float& oR)
    {
        float aL = 0.0f, aR = 0.0f;
        for (auto& m : modes)
        {
            aL += m.gL * m.sL.process (inL);
            aR += m.gR * m.sR.process (inR);
        }
        aL *= 0.30f;
        aR *= 0.30f;

        lineL.push (inL);
        lineR.push (inR);
        float rL = 0.0f, rR = 0.0f;
        for (int t = 0; t < numTaps; ++t)
        {
            // odbicia biorą sygnał z przeciwnego kanału, więc "owijają" pole stereo
            const float xl = lineR.read (tapSL[t]);
            const float xr = lineL.read (tapSR[t]);
            lpL[t] += tapC * (xl - lpL[t]);
            lpR[t] += tapC * (xr - lpR[t]);
            rL += tapG[t] * lpL[t];
            rR += tapG[t] * lpR[t];
        }
        oL = aL + room * 0.5f * rL;
        oR = aR + room * 0.5f * rR;
    }

private:
    static constexpr int numModes = 20;
    static constexpr int numTaps  = 6;

    struct Mode
    {
        float baseF = 100.0f, q = 10.0f, amp = 1.0f, jL = 1.0f, jR = 1.0f, aL = 1.0f, aR = 1.0f;
        float gL = 1.0f, gR = 1.0f;
        SVFBand sL, sR;
    };

    Mode modes[numModes];
    DelayLine lineL, lineR;
    float lpL[numTaps] {}, lpR[numTaps] {}, tapSL[numTaps] {}, tapSR[numTaps] {}, tapG[numTaps] {};
    float tapC = 0.3f, room = 0.4f, lastSize = -1.0f, lastWood = -1.0f;
    double sr = 44100.0;
};

//==============================================================================
// EMBRACE: sieć opóźnień (FDN) "wnętrza pianina" + szerokość; bas poniżej ~150 Hz nietknięty
//==============================================================================
class Embrace
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        static const float ms[N] = { 7.3f, 8.8f, 10.7f, 12.9f, 15.1f, 17.9f, 21.1f, 24.7f };
        static const float sl[N] = { 1, 1, -1, 1, -1, -1, 1, -1 };
        static const float sr2[N] = { 1, -1, 1, 1, -1, 1, -1, -1 };

        for (int i = 0; i < N; ++i)
        {
            baseLen[i] = ms[i] * 0.001f * (float) sr;
            dl[i].prepare ((int) (baseLen[i] * 1.8f) + 64);
            modRate[i] = 0.07f + 0.031f * (float) i;
            modPh[i]   = 0.123f * (float) i;
            inSign[i]  = (i % 3 == 0) ? -1.0f : 1.0f;
            sgnL[i] = sl[i];
            sgnR[i] = sr2[i];
        }
        modDepth = 0.0004f * (float) sr;

        hpIn.highpass (sr, 150.0f);
        hpOutL.highpass (sr, 100.0f);
        hpOutR.highpass (sr, 100.0f);

        scaleSm.reset (sr, 0.4);
        scaleSm.setCurrentAndTargetValue (1.0f);
        reset();
        setParams (0.5f, 0.6f, 0.4f);
    }

    void reset()
    {
        for (int i = 0; i < N; ++i) { dl[i].clear(); lp[i] = 0.0f; }
        hpIn.reset(); hpOutL.reset(); hpOutR.reset();
    }

    // pos: 0 = "przy klawiaturze", 1 = "pod wiekiem"
    void setParams (float pos, float widthAmt, float decay)
    {
        width = widthAmt;
        const float scaleT = 0.7f + 0.9f * pos;
        scaleSm.setTargetValue (scaleT);

        const float rt60 = (0.12f + 2.2f * std::pow (decay, 1.5f)) * (0.8f + 0.4f * pos);
        for (int i = 0; i < N; ++i)
        {
            const float len = baseLen[i] * scaleT;
            g[i] = std::pow (10.0f, -3.0f * len / ((float) sr * rt60));
        }
        dampC = coefHz (9000.0f * std::pow (0.35f, pos), sr);
    }

    void process (float inL, float inR, float& oL, float& oR)
    {
        const float scale = scaleSm.getNextValue();
        const float mono = hpIn.process (0.5f * (inL + inR));

        float v[N];
        float accL = 0.0f, accR = 0.0f;
        for (int i = 0; i < N; ++i)
        {
            modPh[i] += modRate[i] / (float) sr;
            if (modPh[i] >= 1.0f) modPh[i] -= 1.0f;

            const float d = juce::jmax (4.0f, baseLen[i] * scale + modDepth * std::sin (kTwoPi * modPh[i]));
            const float x = dl[i].read (d);
            lp[i] += dampC * (x - lp[i]);
            accL += sgnL[i] * lp[i];
            accR += sgnR[i] * lp[i];
            v[i] = lp[i] * g[i];
        }

        // macierz Hadamarda 8x8 (szybka transformata)
        for (int i = 0; i < 8; i += 2)
        {
            const float a = v[i], b = v[i + 1];
            v[i] = a + b; v[i + 1] = a - b;
        }
        for (int i = 0; i < 8; i += 4)
            for (int j = 0; j < 2; ++j)
            {
                const float a = v[i + j], b = v[i + j + 2];
                v[i + j] = a + b; v[i + j + 2] = a - b;
            }
        for (int j = 0; j < 4; ++j)
        {
            const float a = v[j], b = v[j + 4];
            v[j] = a + b; v[j + 4] = a - b;
        }

        for (int i = 0; i < N; ++i)
            dl[i].push (mono * inSign[i] * 0.25f + v[i] * 0.353553f);

        const float l = accL * 0.35f, r = accR * 0.35f;
        const float M = 0.5f * (l + r);
        const float S = 0.5f * (l - r) * (0.3f + 1.7f * width);
        oL = hpOutL.process (M + S);
        oR = hpOutR.process (M - S);
    }

private:
    static constexpr int N = 8;
    DelayLine dl[N];
    float baseLen[N] {}, lp[N] {}, g[N] {}, modRate[N] {}, modPh[N] {}, inSign[N] {}, sgnL[N] {}, sgnR[N] {};
    float dampC = 0.5f, modDepth = 10.0f, width = 0.6f;
    Biquad hpIn, hpOutL, hpOutR;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> scaleSm;
    double sr = 44100.0;
};

//==============================================================================
// MOTION: mikromodulacje M/S w 6 pasmach (obroty obrazu stereo, "oddychanie") + shimmer na side
//==============================================================================
class Motion
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        static const float freqs[NB] = { 180.0f, 400.0f, 900.0f, 2000.0f, 4500.0f, 9000.0f };
        for (int k = 0; k < NB; ++k)
        {
            bpM[k].set (freqs[k], 1.2f, sr);
            bpS[k].set (freqs[k], 1.2f, sr);
            r1[k] = 0.050f + 0.043f * (float) k;
            r2[k] = 0.113f + 0.061f * (float) k;
            r3[k] = 0.070f + 0.052f * (float) k;
            ph1[k] = 0.17f * (float) k;
            ph2[k] = 0.31f * (float) k;
            ph3[k] = 0.23f * (float) k;
        }
        sideDelay.prepare ((int) (sr * 0.03) + 8);
        sideHp.highpass (sr, 300.0f);
        reset();
    }

    void reset()
    {
        for (int k = 0; k < NB; ++k)
        {
            bpM[k].reset(); bpS[k].reset();
            th[k] = thT[k] = thInc[k] = gm[k] = gmT[k] = gmInc[k] = 0.0f;
        }
        sideDelay.clear();
        sideHp.reset();
        counter = 0;
    }

    void setRate (float r) { rate = r; }
    void setLive (float d, float s) { depth = d; shimmer = s; }

    void process (float& l, float& r)
    {
        if ((counter++ & 15) == 0)
        {
            for (int k = 0; k < NB; ++k)
            {
                ph1[k] += rate * r1[k] * 16.0f / (float) sr;  if (ph1[k] >= 1.0f) ph1[k] -= 1.0f;
                ph2[k] += rate * r2[k] * 16.0f / (float) sr;  if (ph2[k] >= 1.0f) ph2[k] -= 1.0f;
                ph3[k] += rate * r3[k] * 16.0f / (float) sr;  if (ph3[k] >= 1.0f) ph3[k] -= 1.0f;

                thT[k] = depth * 0.30f * (0.6f * std::sin (kTwoPi * ph1[k]) + 0.4f * std::sin (kTwoPi * ph2[k]));
                gmT[k] = depth * 0.35f * std::sin (kTwoPi * ph3[k]);
                thInc[k] = (thT[k] - th[k]) / 16.0f;
                gmInc[k] = (gmT[k] - gm[k]) / 16.0f;
            }
        }

        const float M = 0.5f * (l + r);
        const float S = 0.5f * (l - r);
        float dM = 0.0f, dS = 0.0f;

        for (int k = 0; k < NB; ++k)
        {
            const float bM = bpM[k].process (M);
            const float bS = bpS[k].process (S);
            const float t = th[k];
            const float c1 = -0.5f * t * t;           // cos(t) - 1 (mały kąt)
            dM += bM * c1 - bS * t;                   // obrót (M,S) o kąt t w paśmie k
            dS += bS * c1 + bM * t + bS * gm[k];      // + mikro-zmiana poziomu side
            th[k] += thInc[k];
            gm[k] += gmInc[k];
        }

        // shimmer: bardzo delikatny mikro-chorus tylko na side, powyżej 300 Hz
        shPh += 0.27f / (float) sr;
        if (shPh >= 1.0f) shPh -= 1.0f;
        sideDelay.push (S);
        const float d = (0.0065f + 0.0025f * std::sin (kTwoPi * shPh)) * (float) sr;
        dS += shimmer * 0.5f * sideHp.process (sideDelay.read (d));

        const float M2 = M + dM, S2 = S + dS;
        l = M2 + S2;
        r = M2 - S2;
    }

private:
    static constexpr int NB = 6;
    SVFBand bpM[NB], bpS[NB];
    float th[NB] {}, thT[NB] {}, thInc[NB] {}, gm[NB] {}, gmT[NB] {}, gmInc[NB] {};
    float r1[NB] {}, r2[NB] {}, r3[NB] {}, ph1[NB] {}, ph2[NB] {}, ph3[NB] {};
    float depth = 0.0f, shimmer = 0.0f, rate = 1.0f, shPh = 0.0f;
    int counter = 0;
    DelayLine sideDelay;
    Biquad sideHp;
    double sr = 44100.0;
};

//==============================================================================
// COLOR: BLOOM -> SATURATION -> TAPE -> INFLATOR (pracuje na 2x oversamplingu)
//==============================================================================
class ColorChain
{
public:
    struct Params
    {
        float bloomDrive = 0.0f, bloomSing = 0.0f;
        float satDrive = 0.0f, satChar = 0.5f, satMix = 1.0f;
        int tapeSpeed = 1, tapeType = 1;
        float tapeDrive = 0.0f, tapeFlutter = 0.0f, tapeHiss = 0.0f;
        float infInputDb = 0.0f, infEffect = 0.0f, infCurve = 0.0f;
        bool infProtect = true;
    };

    // opóźnienie (w próbkach bazowych) wnoszone przez bazę linii flutteru - kompensowane w torze suchym
    int flutterBaseSamples() const { return flutterBase; }

    void prepare (double baseSr)
    {
        sr2 = baseSr * 2.0;
        flutterBase = (int) std::round (0.0008 * baseSr);
        smC     = coefMs (15.0f, sr2);
        dcC     = coefHz (20.0f, sr2);
        satLowC = coefHz (120.0f, sr2);
        emphC   = coefHz (3500.0f, sr2);
        noiseC  = coefHz (6000.0f, sr2);

        for (int c = 0; c < 2; ++c)
        {
            hpBloom[c].highpass (sr2, 220.0f);
            hpInf[c].highpass (sr2, 140.0f);
            flutDl[c].prepare ((int) (sr2 * 0.004) + 16);
        }
        reset();
        setParams (Params());
        for (auto* s : { &sBd, &sBs, &sSd, &sSc, &sSm, &sTd, &sTf, &sTh, &sIe, &sIc, &sIg })
            s->cur = s->tgt;
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            hpBloom[c].reset(); hpInf[c].reset(); bump[c].reset(); roll[c].reset();
            fm1[c].reset(); fm2[c].reset(); flutDl[c].clear();
            dcBloom[c] = dcSat[c] = dcTape[c] = dcInf[c] = 0.0f;
            satLow[c] = emphE[c] = emphD[c] = noiseS[c] = 0.0f;
        }
        counter = 0;
    }

    void setParams (const Params& p)
    {
        sBd.tgt = p.bloomDrive;  sBs.tgt = p.bloomSing;
        sSd.tgt = p.satDrive;    sSc.tgt = p.satChar;     sSm.tgt = p.satMix;
        sTd.tgt = p.tapeDrive;   sTf.tgt = p.tapeFlutter; sTh.tgt = p.tapeHiss;
        sIe.tgt = p.infEffect;   sIc.tgt = p.infCurve;
        sIg.tgt = juce::Decibels::decibelsToGain (p.infInputDb);
        infProtect = p.infProtect;

        // TAPE: prędkość i typ taśmy
        static const float bumpHz[3]   = { 150.0f, 95.0f, 55.0f };
        static const float cornerHz[3] = { 14000.0f, 18000.0f, 22000.0f };
        static const float hissMul[3]  = { 2.0f, 1.0f, 0.5f };
        static const float typeKm[3]   = { 1.0f, 1.3f, 1.8f };
        static const float typeBs[3]   = { 0.02f, 0.10f, 0.06f };
        static const float typeEm[3]   = { 0.6f, 0.8f, 1.0f };

        const int sp = juce::jlimit (0, 2, p.tapeSpeed);
        const int ty = juce::jlimit (0, 2, p.tapeType);
        typeK = typeKm[ty]; typeBias = typeBs[ty]; emphA = typeEm[ty]; hissGain = 0.003f * hissMul[sp];

        const float td = p.tapeDrive;
        const float bumpDb = 3.0f * juce::jmin (1.0f, td * 1.5f);
        const float corner = juce::jmin (0.45f * (float) sr2,
                                         24000.0f + (cornerHz[sp] - 24000.0f) * juce::jmin (1.0f, td * 1.2f));
        for (int c = 0; c < 2; ++c)
        {
            bump[c].peaking (sr2, bumpHz[sp], 0.9f, bumpDb);
            roll[c].lowpass (sr2, corner, 0.70710678f);
        }
    }

    // przetwarzanie w miejscu, n = liczba próbek w 2x
    void process2x (float* l, float* r, int n)
    {
        float* ch[2] = { l, r };

        for (int i = 0; i < n; ++i)
        {
            const float bd = sBd.next (smC), bs = sBs.next (smC);
            const float sd = sSd.next (smC), sc = sSc.next (smC), sm = sSm.next (smC);
            const float td = sTd.next (smC), tf = sTf.next (smC), th = sTh.next (smC);
            const float ie = sIe.next (smC), ic = sIc.next (smC), ig = sIg.next (smC);

            singPh1 += 0.13f / (float) sr2;  if (singPh1 >= 1.0f) singPh1 -= 1.0f;
            singPh2 += 0.21f / (float) sr2;  if (singPh2 >= 1.0f) singPh2 -= 1.0f;
            wowPh   += 0.6f  / (float) sr2;  if (wowPh   >= 1.0f) wowPh   -= 1.0f;
            flutPh  += 7.0f  / (float) sr2;  if (flutPh  >= 1.0f) flutPh  -= 1.0f;

            if ((counter++ & 31) == 0)
            {
                for (int c = 0; c < 2; ++c)
                {
                    const float co = (c == 0) ? 0.0f : 1.0f;
                    const float f1 = 650.0f  * std::exp2 (0.45f * std::sin (kTwoPi * (singPh1 + 0.11f * co)));
                    const float f2 = 2100.0f * std::exp2 (0.40f * std::sin (kTwoPi * (singPh2 + 0.23f * co)));
                    fm1[c].set (f1, 3.5f, sr2);
                    fm2[c].set (f2, 4.0f, sr2);
                }
            }

            const float bBlend = juce::jmin (1.0f, bd * 4.0f);
            const float sBlend = juce::jmin (1.0f, sd * 3.0f) * sm;
            const float tBlend = juce::jmin (1.0f, td * 3.0f);
            const float pkM1   = juce::jmin (2.5f, 1.0f + 0.9f * bs) - 1.0f;
            const float flDepth = tf * 0.00035f * (float) sr2;
            const float flBase  = 2.0f * (float) flutterBase;

            for (int c = 0; c < 2; ++c)
            {
                float x = ch[c][i];
                const float co = (c == 0) ? 0.0f : 1.0f;

                // ---------- BLOOM: saturacja "ekscyterowa" + śpiewające formanty ----------
                {
                    const float k = 1.0f + 6.0f * bd, bias = 0.15f * bd;
                    const float h = hpBloom[c].process (x);
                    const float s = std::tanh (k * (h + bias)) / k - std::tanh (k * bias) / k;
                    const float res = (s - h) * bBlend;
                    dcBloom[c] += dcC * (res - dcBloom[c]);
                    x += res - dcBloom[c];

                    if (bs > 1.0e-3f)
                    {
                        x += pkM1 * fm1[c].process (x);
                        x += pkM1 * fm2[c].process (x);
                    }
                    else { fm1[c].reset(); fm2[c].reset(); }
                }

                // ---------- SATURATION: lampa <-> transformator ----------
                {
                    const float k = 1.0f + 5.0f * sd, b = 0.12f * sd;
                    const float yTube = (std::tanh (k * (x + b)) - std::tanh (k * b)) / k;
                    satLow[c] += satLowC * (x - satLow[c]);
                    const float hi = x - satLow[c];
                    const float k2 = 1.0f + 7.0f * sd;
                    const float yTr = hi + std::tanh (k2 * satLow[c]) / k2;
                    const float y = yTube + sc * (yTr - yTube);
                    const float res = (y - x) * sBlend;
                    dcSat[c] += dcC * (res - dcSat[c]);
                    x += res - dcSat[c];
                }

                // ---------- TAPE: pre-emfaza -> saturacja -> de-emfaza, head bump, rolloff, flutter, szum ----------
                {
                    const float a = emphA;
                    const float lpx = emphE[c] + emphC * (x - emphE[c]);
                    emphE[c] = lpx;
                    float p = x + a * (x - lpx);

                    const float k = (1.0f + 4.0f * td) * typeK, b = typeBias * td;
                    const float q = (std::tanh (k * (p + b)) - std::tanh (k * b)) / k;
                    const float res = (q - p) * tBlend;
                    dcTape[c] += dcC * (res - dcTape[c]);
                    p += res - dcTape[c];

                    // dokładna odwrotność pre-emfazy (de-emfaza)
                    const float xo = (p + a * (1.0f - emphC) * emphD[c]) / (1.0f + a - a * emphC);
                    emphD[c] += emphC * (xo - emphD[c]);
                    x = xo;

                    x = bump[c].process (x);
                    x = roll[c].process (x);

                    flutDl[c].push (x);
                    const float mod = 0.6f * std::sin (kTwoPi * (wowPh + 0.07f * co))
                                    + 0.4f * std::sin (kTwoPi * (flutPh + 0.11f * co));
                    x = flutDl[c].read (juce::jmax (2.0f, flBase + flDepth * mod));

                    if (th > 1.0e-4f)
                    {
                        noiseS[c] += noiseC * ((rng.nextFloat() * 2.0f - 1.0f) - noiseS[c]);
                        x += noiseS[c] * hissGain * th;
                    }
                }

                // ---------- INFLATOR: wielomianowy waveshaper (odstęp od basu opcjonalnie) ----------
                {
                    const float hh = hpInf[c].process (x);
                    const float base = infProtect ? hh : x;
                    const float u = juce::jlimit (-1.0f, 1.0f, base * ig);
                    const float ev = u * u - std::abs (u) * u * u;                  // człon parzysty (asymetria)
                    const float v = (1.5f * u - 0.5f * u * u * u + 0.6f * ic * ev) / ig;
                    const float res = ie * (v - base);
                    dcInf[c] += dcC * (res - dcInf[c]);
                    x += res - dcInf[c];
                }

                ch[c][i] = x;
            }
        }
    }

private:
    struct Sm
    {
        float cur = 0.0f, tgt = 0.0f;
        float next (float c) { cur += c * (tgt - cur); return cur; }
    };

    Sm sBd, sBs, sSd, sSc, sSm, sTd, sTf, sTh, sIe, sIc, sIg;

    Biquad hpBloom[2], hpInf[2], bump[2], roll[2];
    SVFBand fm1[2], fm2[2];
    DelayLine flutDl[2];
    float dcBloom[2] {}, dcSat[2] {}, dcTape[2] {}, dcInf[2] {};
    float satLow[2] {}, emphE[2] {}, emphD[2] {}, noiseS[2] {};

    float singPh1 = 0.0f, singPh2 = 0.0f, wowPh = 0.0f, flutPh = 0.0f;
    float smC = 0.01f, dcC = 0.001f, satLowC = 0.01f, emphC = 0.1f, noiseC = 0.3f;
    float typeK = 1.0f, typeBias = 0.0f, emphA = 0.8f, hissGain = 0.003f;
    bool infProtect = true;
    int flutterBase = 38, counter = 0;
    double sr2 = 88200.0;
    juce::Random rng;
};

} // namespace petro
