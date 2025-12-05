#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace gptaudio::dsp
{
    template <typename T>
    constexpr T clamp(T value, T low, T high)
    {
        return std::min(std::max(value, low), high);
    }

    template <typename T>
    constexpr T dbToLinear(T db)
    {
        return std::pow(static_cast<T>(10), db / static_cast<T>(20));
    }

    template <typename T>
    constexpr T linearToDb(T value)
    {
        return static_cast<T>(20) * std::log10(std::max(value, static_cast<T>(1e-12)));
    }

    template <typename T>
    inline T softSign(T value)
    {
        return value / (static_cast<T>(1) + std::abs(value));
    }

    struct OnePole
    {
        void reset(double sampleRate, double cutoffHz, double initial = 0.0);
        double process(double input);
        void setCutoff(double cutoffHz);

        double sampleRate {48000.0};
        double cutoff {10.0};
        double alpha {0.0};
        double state {0.0};
    };

    struct SlewLimiter
    {
        void reset(double sampleRate, double riseMs, double fallMs, double initial = 0.0);
        double process(double input);

        double sampleRate {48000.0};
        double riseCoef {0.0};
        double fallCoef {0.0};
        double state {0.0};
    };

    struct Biquad
    {
        enum class Type
        {
            LowPass,
            HighPass,
            Peaking,
            LowShelf,
            HighShelf
        };

        void reset(double sampleRate);
        void setLowPass(double cutoff, double q);
        void setHighPass(double cutoff, double q);
        void setPeaking(double cutoff, double q, double gainDb);
        void setLowShelf(double cutoff, double gainDb);
        void setHighShelf(double cutoff, double gainDb);
        double process(double input);

    private:
        void update(double b0In, double b1In, double b2In, double a1In, double a2In);

        double sr {48000.0};
        double b0 {1.0}, b1 {0.0}, b2 {0.0}, a1 {0.0}, a2 {0.0};
        double z1 {0.0}, z2 {0.0};
    };

    class ParameterSmoother
    {
    public:
        void reset(double sampleRate, double timeMs, double initialValue = 0.0);
        double process(double targetValue);

    private:
        double sr {48000.0};
        double coef {0.0};
        double state {0.0};
    };
} // namespace gptaudio::dsp
