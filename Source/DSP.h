#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

// Small, allocation-free DSP building blocks used by the processor.
namespace vf
{
    constexpr float pi = 3.14159265358979f;
    constexpr float twoPi = 6.28318530717959f;

    inline float dbToGain (float db) { return std::pow (10.f, db * 0.05f); }
    inline float gainToDb (float g)  { return 20.f * std::log10 (std::max (g, 1.0e-9f)); }
    inline float midiToHz (float n)  { return 440.f * std::pow (2.f, (n - 69.f) / 12.f); }
    inline float hzToMidi (float f)  { return 69.f + 12.f * std::log2 (std::max (f, 1.f) / 440.f); }
    inline float onePoleCoef (float ms, float fs) { return ms <= 0.f ? 0.f : std::exp (-1.f / (0.001f * ms * fs)); }

    // ---------------------------------------------------------------- biquad (TDF II)
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

        inline float process (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
        void reset() { z1 = z2 = 0; }

        void setBandPass (float fs, float f, float q)   // 0 dB peak gain
        {
            const float w = twoPi * f / fs, alpha = std::sin (w) / (2.f * q), a0 = 1.f + alpha;
            b0 = alpha / a0; b1 = 0; b2 = -alpha / a0;
            a1 = -2.f * std::cos (w) / a0; a2 = (1.f - alpha) / a0;
        }
        void setHighPass (float fs, float f, float q = 0.7071f)
        {
            const float w = twoPi * f / fs, alpha = std::sin (w) / (2.f * q), c = std::cos (w), a0 = 1.f + alpha;
            b0 = (1.f + c) * 0.5f / a0; b1 = -(1.f + c) / a0; b2 = b0;
            a1 = -2.f * c / a0; a2 = (1.f - alpha) / a0;
        }
        void setLowPass (float fs, float f, float q = 0.7071f)
        {
            const float w = twoPi * f / fs, alpha = std::sin (w) / (2.f * q), c = std::cos (w), a0 = 1.f + alpha;
            b0 = (1.f - c) * 0.5f / a0; b1 = (1.f - c) / a0; b2 = b0;
            a1 = -2.f * c / a0; a2 = (1.f - alpha) / a0;
        }
    };

    // ---------------------------------------------------------------- noise
    struct Noise
    {
        uint32_t s = 0x12345678u;
        explicit Noise (uint32_t seed = 0x12345678u) : s (seed) {}
        inline float next() noexcept   // white, -1..1
        {
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            return (float) (int32_t) s * (1.0f / 2147483648.0f);
        }
    };

    // ---------------------------------------------------------------- band-limited oscillator (PolyBLEP)
    inline float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)       { t /= dt; return t + t - t * t - 1.f; }
        if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
        return 0.f;
    }
    inline float sawBlep (float phase, float dt) noexcept    { return 2.f * phase - 1.f - polyBlep (phase, dt); }
    inline float squareBlep (float phase, float dt) noexcept
    {
        float v = phase < 0.5f ? 1.f : -1.f;
        v += polyBlep (phase, dt);
        float p2 = phase + 0.5f; if (p2 >= 1.f) p2 -= 1.f;
        v -= polyBlep (p2, dt);
        return v;
    }

    // ---------------------------------------------------------------- saturation shapes
    // mode: 0 tube (warm, asymmetric), 1 hard (aggressive), 2 fold (wavefolder)
    inline float shape (int mode, float x) noexcept
    {
        switch (mode)
        {
            case 0:  return std::tanh (x + 0.25f) - 0.24491866f;                 // tanh(0.25)
            case 1:  { const float a = std::abs (x); return a < 1.f ? x * (1.5f - 0.5f * a * a) : (x > 0 ? 1.f : -1.f); }
            default: return std::sin (x * 1.3f) * (1.f / 1.3f) * (1.f + 0.15f * std::min (std::abs (x), 6.f)) / 1.9f;
        }
    }

    // ---------------------------------------------------------------- fractional delay line
    struct DelayLine
    {
        std::vector<float> buf; int w = 0, mask = 0;
        void prepare (int maxSamples)
        {
            int n = 1; while (n < maxSamples + 4) n <<= 1;
            buf.assign ((size_t) n, 0.f); mask = n - 1; w = 0;
        }
        void reset() { std::fill (buf.begin(), buf.end(), 0.f); }
        inline void push (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) & mask; }
        inline float read (float delay) const noexcept   // delay in samples, >= 1
        {
            const float rp = (float) w - delay;
            const int i0 = (int) std::floor (rp);
            const float fr = rp - (float) i0;
            const float a = buf[(size_t) (i0 & mask)], b = buf[(size_t) ((i0 + 1) & mask)];
            return a + (b - a) * fr;
        }
    };

    // ---------------------------------------------------------------- YIN pitch detector on a decimated signal
    class PitchDetector
    {
    public:
        void prepare (double sampleRate)
        {
            factor = std::max (1, (int) std::round (sampleRate / 11025.0));
            dsRate = (float) (sampleRate / factor);
            window = 512;
            maxLag = std::min (window / 2, (int) (dsRate / 55.f));
            minLag = std::max (2, (int) (dsRate / 1100.f));
            hist.assign ((size_t) window, 0.f);
            lin.assign ((size_t) window, 0.f);
            diff.assign ((size_t) maxLag + 2, 0.f);
            lp.setLowPass ((float) sampleRate, std::min (2500.f, (float) sampleRate * 0.2f), 0.7071f);
            lp2 = lp;
            pos = 0; acc = 0; accN = 0; hopCount = 0; freq = 0; confidence = 0;
        }

        // feed one input sample; returns true when a new estimate was computed
        inline bool push (float x) noexcept
        {
            acc += lp2.process (lp.process (x)); ++accN;
            if (accN < factor) return false;
            hist[(size_t) pos] = acc / (float) accN;
            pos = (pos + 1) % window; acc = 0; accN = 0;
            if (++hopCount < 96) return false;
            hopCount = 0;
            analyse();
            return true;
        }

        float freq = 0;          // last detected frequency in Hz (0 = unvoiced)
        float confidence = 0;

    private:
        void analyse() noexcept
        {
            const int W = window - maxLag;
            for (int j = 0; j < window; ++j) lin[(size_t) j] = hist[(size_t) ((pos + j) % window)];
            float energy = 0;
            for (int j = 0; j < window; ++j) { const float v = at (j); energy += v * v; }
            if (energy / (float) window < 1.0e-6f) { freq = 0; confidence = 0; return; }

            diff[0] = 0;
            for (int tau = 1; tau <= maxLag; ++tau)
            {
                float s = 0;
                for (int j = 0; j < W; ++j) { const float d = at (j) - at (j + tau); s += d * d; }
                diff[(size_t) tau] = s;
            }
            // cumulative mean normalised difference
            float running = 0;
            diff[0] = 1;
            for (int tau = 1; tau <= maxLag; ++tau)
            {
                running += diff[(size_t) tau];
                diff[(size_t) tau] = running > 0 ? diff[(size_t) tau] * (float) tau / running : 1.f;
            }
            int best = -1;
            for (int tau = minLag; tau < maxLag; ++tau)
            {
                if (diff[(size_t) tau] < 0.18f)
                {
                    while (tau + 1 < maxLag && diff[(size_t) tau + 1] < diff[(size_t) tau]) ++tau;
                    best = tau; break;
                }
            }
            if (best < 0) { freq = 0; confidence = 0; return; }
            // octave-up guard: if a multiple of the period is a much deeper dip, the real period is longer
            for (int m = 2; m <= 3 && diff[(size_t) best] > 0.06f; ++m)
            {
                const int c = best * m;
                if (c + 3 >= maxLag) break;
                int bi = c; for (int k = c - 3; k <= c + 3; ++k) if (diff[(size_t) k] < diff[(size_t) bi]) bi = k;
                if (diff[(size_t) bi] < diff[(size_t) best] * 0.45f) { best = bi; break; }
            }
            while (best + 1 < maxLag && diff[(size_t) best + 1] < diff[(size_t) best]) ++best;
            while (best - 1 > minLag && diff[(size_t) best - 1] < diff[(size_t) best]) --best;
            float t = (float) best;
            const float y0 = diff[(size_t) best - 1], y1 = diff[(size_t) best], y2 = diff[(size_t) best + 1];
            const float den = y0 - 2 * y1 + y2;
            if (std::abs (den) > 1.0e-9f) t += 0.5f * (y0 - y2) / den;
            freq = dsRate / t;
            confidence = 1.f - y1;
        }
        inline float at (int j) const noexcept { return lin[(size_t) j]; }

        int factor = 4, window = 512, maxLag = 200, minLag = 10, pos = 0, accN = 0, hopCount = 0;
        float dsRate = 11025, acc = 0;
        std::vector<float> hist, lin, diff;
        Biquad lp, lp2;
    };
}
