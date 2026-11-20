// =============================================================================
//  WANTED LEVEL - motore DSP (C++ puro, nessuna dipendenza da JUCE)
//
//  Catena del segnale (parte "wet"):
//    input -> drive -> saturazione (soft -> asimmetrica -> hard clip -> fold)
//          -> + sirena filtrata in sottofondo
//          -> bitcrusher + sample-rate reducer
//          -> compressore brutale
//          -> filtro tono + DC blocker -> auto-gain
//    -> mix dry/wet -> output gain
//
//  Tutto e' guidato da un solo valore "level" che va da 0 a 5 (le stelle).
// =============================================================================
#pragma once

#include <algorithm>
#include <cmath>

namespace wanted
{
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 2.0f * kPi;

inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g)  noexcept { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }

// Rampa 0..1 di "level" tra a e b
inline float ramp (float x, float a, float b) noexcept
{
    return std::clamp ((x - a) / (b - a), 0.0f, 1.0f);
}

// -----------------------------------------------------------------------------
// Biquad passa-banda (RBJ) per rendere la sirena "lontana / filtrata"
// -----------------------------------------------------------------------------
struct BandPass
{
    float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    void set (float freq, float q, float sr) noexcept
    {
        const float w0    = kTwoPi * freq / sr;
        const float alpha = std::sin (w0) / (2.0f * q);
        const float a0    = 1.0f + alpha;
        b0 =  alpha / a0;
        b1 =  0.0f;
        b2 = -alpha / a0;
        a1 = -2.0f * std::cos (w0) / a0;
        a2 = (1.0f - alpha) / a0;
    }

    float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() noexcept { z1 = z2 = 0.0f; }
};

// -----------------------------------------------------------------------------
// Sirena della polizia: "wail" lento che a 5 stelle diventa "yelp" veloce
// -----------------------------------------------------------------------------
struct Siren
{
    float phase = 0, wailLfo = 0, yelpLfo = 0, panLfo = 0;
    BandPass bp, bp2;

    void prepare (float sr) noexcept
    {
        bp.set  (1100.0f, 1.4f, sr);
        bp2.set (1100.0f, 0.9f, sr);
        bp.reset(); bp2.reset();
        phase = wailLfo = yelpLfo = panLfo = 0.0f;
    }

    // chaos 0..1 : morph da wail a yelp. Ritorna campione mono, pan 0..1 in outPan
    float next (float sr, float chaos, float& outPan) noexcept
    {
        wailLfo += 1.0f / (3.2f * sr);  if (wailLfo >= 1.0f) wailLfo -= 1.0f;
        yelpLfo += 1.0f / (0.28f * sr); if (yelpLfo >= 1.0f) yelpLfo -= 1.0f;
        panLfo  += 1.0f / (6.0f * sr);  if (panLfo  >= 1.0f) panLfo  -= 1.0f;

        const float wail  = 0.5f - 0.5f * std::cos (kTwoPi * wailLfo);
        const float yelp  = 0.5f - 0.5f * std::cos (kTwoPi * yelpLfo);
        const float sweep = wail + chaos * (yelp - wail);
        const float freq  = 620.0f + 820.0f * sweep;

        phase += freq / sr;
        if (phase >= 1.0f) phase -= 1.0f;

        // onda "quadrata morbida" = timbro da sirena elettronica
        const float raw = std::tanh (3.0f * std::sin (kTwoPi * phase));
        outPan = 0.5f + 0.32f * std::sin (kTwoPi * panLfo);   // passa da sx a dx
        return bp2.process (bp.process (raw)) * 1.6f;
    }
};

// -----------------------------------------------------------------------------
// Engine principale
// -----------------------------------------------------------------------------
class Engine
{
public:
    static constexpr int kMaxChannels = 2;

    void prepare (double sampleRate) noexcept
    {
        sr = (float) std::max (8000.0, sampleRate);
        siren.prepare (sr);

        for (auto& c : ch) c = ChannelState{};

        smoothCoef = std::exp (-1.0f / (0.030f * sr));            // 30 ms
        envAtt     = std::exp (-1.0f / (0.010f * sr));
        envRel     = std::exp (-1.0f / (0.300f * sr));
        dcR        = 1.0f - kTwoPi * 20.0f / sr;

        compEnv = 0.0f; inEnv = 0.0f;
        dsPhase = 1.0e9f;                                          // forza il primo "hold"
        first   = true;
        counter = 0;
    }

    // data: puntatori ai canali (processati in place)
    void process (float* const* data, int numCh, int numSamples,
                  float levelTarget, float mixTarget, float outGainTarget) noexcept
    {
        numCh = std::min (numCh, kMaxChannels);
        if (numCh <= 0) return;

        levelTarget = std::clamp (levelTarget, 0.0f, 5.0f);
        mixTarget   = std::clamp (mixTarget,   0.0f, 1.0f);

        if (first)
        {
            level = levelTarget; mix = mixTarget; outGain = outGainTarget;
            updateDerived();
            first = false;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            // --- smoothing parametri ------------------------------------------
            level   = levelTarget   + smoothCoef * (level   - levelTarget);
            mix     = mixTarget     + smoothCoef * (mix     - mixTarget);
            outGain = outGainTarget + smoothCoef * (outGain - outGainTarget);

            if ((counter++ & 15) == 0)
                updateDerived();

            // --- inviluppo dell'ingresso (per far "seguire" la sirena) ---------
            float inPeak = 0.0f;
            for (int c = 0; c < numCh; ++c)
                inPeak = std::max (inPeak, std::abs (data[c][i]));
            const float ec = inPeak > inEnv ? envAtt : envRel;
            inEnv = inPeak + ec * (inEnv - inPeak);

            // --- sirena -------------------------------------------------------
            float sirenL = 0.0f, sirenR = 0.0f;
            if (d.sirenGain > 0.0f)
            {
                float pan = 0.5f;
                const float s = siren.next (sr, d.sirenChaos, pan)
                              * d.sirenGain * std::min (1.0f, inEnv * 4.0f);
                if (numCh == 1) { sirenL = s; }
                else
                {
                    sirenL = s * std::cos (pan * kPi * 0.5f) * 1.414f;
                    sirenR = s * std::sin (pan * kPi * 0.5f) * 1.414f;
                }
            }

            // --- distorsione + sirena + crusher (per canale) -----------------
            float wet[kMaxChannels] = { 0.0f, 0.0f };
            float dry[kMaxChannels] = { 0.0f, 0.0f };

            const bool holdNow = (d.dsFactor > 1.0f) && ((dsPhase += 1.0f) >= d.dsFactor);
            if (holdNow) dsPhase -= d.dsFactor;
            if (dsPhase > d.dsFactor) dsPhase = 0.0f;

            for (int c = 0; c < numCh; ++c)
            {
                const float x = data[c][i];
                dry[c] = x;

                float y = saturate (x);
                y += (c == 0 ? sirenL : sirenR);

                // bitcrusher
                if (d.crushOn)
                    y = std::round (y * d.steps) / d.steps;

                // sample-rate reducer
                auto& st = ch[c];
                if (d.dsFactor > 1.0f)
                {
                    if (holdNow || ! st.holdValid) { st.hold = y; st.holdValid = true; }
                    y = st.hold;
                }
                else
                {
                    st.holdValid = false;
                }

                wet[c] = y;
            }

            // --- compressore (stereo-linked) ---------------------------------
            float compGain = 1.0f;
            if (d.compOn)
            {
                float peak = 0.0f;
                for (int c = 0; c < numCh; ++c) peak = std::max (peak, std::abs (wet[c]));
                const float cc = peak > compEnv ? d.compAtt : d.compRel;
                compEnv = peak + cc * (compEnv - peak);

                const float over = gainToDb (compEnv) - d.thrDb;
                const float gr   = over > 0.0f ? over * (1.0f - 1.0f / d.ratio) : 0.0f;
                compGain = dbToGain (d.makeupDb - gr);
            }

            // --- tono, DC, auto-gain, mix, output -----------------------------
            for (int c = 0; c < numCh; ++c)
            {
                auto& st = ch[c];
                float y = wet[c] * compGain;

                st.lp = y + d.lpA * (st.lp - y);          // passa-basso one-pole
                y = st.lp;

                const float dc = y - st.dcX + dcR * st.dcY; // DC blocker
                st.dcX = y; st.dcY = dc;
                y = dc * d.autoGain;

                float out = dry[c] + mix * (y - dry[c]);
                out *= outGain;
                data[c][i] = std::clamp (out, -2.0f, 2.0f); // protezione orecchie
            }
        }
    }

private:
    struct ChannelState
    {
        float hold = 0, lp = 0, dcX = 0, dcY = 0;
        bool  holdValid = false;
    };

    struct Derived
    {
        float satAmt = 0, drive = 1, asym = 0, tanhAsym = 0, hardAmt = 0, foldAmt = 0;
        bool  crushOn = false; float steps = 32768.0f;
        float dsFactor = 1.0f;
        bool  compOn = false; float thrDb = 0, ratio = 1, compAtt = 0, compRel = 0, makeupDb = 0;
        float sirenGain = 0, sirenChaos = 0;
        float lpA = 0, autoGain = 1;
    };

    // Mappa il livello (0..5 stelle) su tutti i parametri interni
    void updateDerived() noexcept
    {
        const float L = level;

        // 1 stella: saturazione morbida -> 2: piu' drive e asimmetria (tubo)
        d.satAmt   = ramp (L, 0.0f, 1.0f);
        d.drive    = dbToGain (L * 6.0f);                  // fino a +30 dB
        d.asym     = 0.25f * ramp (L, 1.0f, 3.0f);
        d.tanhAsym = std::tanh (d.asym);

        // 3 stelle: hard clip -> 5 stelle: wavefolding
        d.hardAmt  = ramp (L, 2.0f, 4.0f);
        d.foldAmt  = ramp (L, 4.0f, 5.0f);

        // bitcrusher da 2.5 stelle: 16 bit -> 4 bit
        const float crush = ramp (L, 2.5f, 5.0f);
        d.crushOn = crush > 0.0f;
        d.steps   = std::pow (2.0f, (16.0f - 12.0f * crush) - 1.0f);

        // riduzione sample-rate da 3.5 stelle: fino a 1/20
        const float ds = ramp (L, 3.5f, 5.0f);
        d.dsFactor = 1.0f + 19.0f * ds * ds;

        // compressione brutale da 2 stelle
        const float comp = ramp (L, 2.0f, 5.0f);
        d.compOn   = comp > 0.0f;
        d.thrDb    = -3.0f - 27.0f * comp;
        d.ratio    = 1.0f + 19.0f * comp;
        d.compAtt  = std::exp (-1.0f / ((0.005f - 0.0045f * comp) * sr));
        d.compRel  = std::exp (-1.0f / ((0.150f - 0.090f  * comp) * sr));
        d.makeupDb = 0.8f * (-d.thrDb) * (1.0f - 1.0f / d.ratio);

        // sirene da 3.5 stelle, "yelp" impazzito a 5
        const float sir = ramp (L, 3.5f, 5.0f);
        d.sirenGain  = 0.35f * std::pow (sir, 1.5f);
        d.sirenChaos = ramp (L, 4.5f, 5.0f);

        // tono (un po' piu' scuro quando la situazione degenera)
        const float cutoff = 18000.0f - 8000.0f * ramp (L, 3.0f, 5.0f);
        d.lpA = std::exp (-kTwoPi * std::min (cutoff, 0.45f * sr) / sr);

        // compensazione automatica del volume
        d.autoGain = dbToGain (-L * 1.8f + 8.0f * ramp (L, 4.0f, 5.0f));
    }

    float saturate (float x) const noexcept
    {
        if (d.satAmt <= 0.0f) return x;

        const float x1   = x * d.drive;
        const float soft = std::tanh (x1 + d.asym) - d.tanhAsym;
        const float hard = std::clamp (x1, -1.0f, 1.0f);
        float y = soft + d.hardAmt * (hard - soft);

        if (d.foldAmt > 0.0f)
        {
            // wavefolder sul segnale "drivato": piu' alzi, piu' si ripiega su se stesso
            const float f = std::sin (x1 * 0.35f);
            y += d.foldAmt * (f - y);
        }
        return x + d.satAmt * (y - x);
    }

    float sr = 44100.0f;
    float level = 1.0f, mix = 1.0f, outGain = 1.0f;
    float smoothCoef = 0, envAtt = 0, envRel = 0, dcR = 0.995f;
    float compEnv = 0, inEnv = 0, dsPhase = 0;
    bool  first = true;
    unsigned counter = 0;

    Derived d;
    Siren siren;
    ChannelState ch[kMaxChannels];
};

} // namespace wanted
