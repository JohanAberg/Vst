#include "Utilities.h"

#include <cmath>
#include <numbers>

namespace gptaudio::dsp
{
void OnePole::reset(double newSampleRate, double cutoffHz, double initial)
{
    sampleRate = newSampleRate;
    cutoff = cutoffHz;
    state = initial;
    setCutoff(cutoffHz);
}

void OnePole::setCutoff(double cutoffHz)
{
    cutoff = cutoffHz;
    const double omega = 2.0 * std::numbers::pi * cutoff / sampleRate;
    alpha = omega / (omega + 1.0);
}

double OnePole::process(double input)
{
    state += alpha * (input - state);
    return state;
}

void SlewLimiter::reset(double newSampleRate, double riseMs, double fallMs, double initial)
{
    sampleRate = newSampleRate;
    const double riseTimeSamples = std::max(riseMs, 0.01) * 0.001 * sampleRate;
    const double fallTimeSamples = std::max(fallMs, 0.01) * 0.001 * sampleRate;

    riseCoef = std::exp(-1.0 / riseTimeSamples);
    fallCoef = std::exp(-1.0 / fallTimeSamples);
    state = initial;
}

double SlewLimiter::process(double input)
{
    const double coef = (input > state) ? riseCoef : fallCoef;
    state = input + coef * (state - input);
    return state;
}

void Biquad::reset(double sampleRateIn)
{
    sr = sampleRateIn;
    z1 = z2 = 0.0;
}

void Biquad::setLowPass(double cutoff, double q)
{
    const double omega = 2.0 * std::numbers::pi * cutoff / sr;
    const double sn = std::sin(omega);
    const double cs = std::cos(omega);
    const double alpha = sn / (2.0 * q);

    const double b0In = (1.0 - cs) / 2.0;
    const double b1In = 1.0 - cs;
    const double b2In = (1.0 - cs) / 2.0;
    const double a0 = 1.0 + alpha;
    const double a1In = -2.0 * cs;
    const double a2In = 1.0 - alpha;

    update(b0In / a0, b1In / a0, b2In / a0, a1In / a0, a2In / a0);
}

void Biquad::setHighPass(double cutoff, double q)
{
    const double omega = 2.0 * std::numbers::pi * cutoff / sr;
    const double sn = std::sin(omega);
    const double cs = std::cos(omega);
    const double alpha = sn / (2.0 * q);

    const double b0In = (1.0 + cs) / 2.0;
    const double b1In = -(1.0 + cs);
    const double b2In = (1.0 + cs) / 2.0;
    const double a0 = 1.0 + alpha;
    const double a1In = -2.0 * cs;
    const double a2In = 1.0 - alpha;

    update(b0In / a0, b1In / a0, b2In / a0, a1In / a0, a2In / a0);
}

void Biquad::setPeaking(double cutoff, double q, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double omega = 2.0 * std::numbers::pi * cutoff / sr;
    const double sn = std::sin(omega);
    const double cs = std::cos(omega);
    const double alpha = sn / (2.0 * q);

    const double b0In = 1.0 + alpha * A;
    const double b1In = -2.0 * cs;
    const double b2In = 1.0 - alpha * A;
    const double a0 = 1.0 + alpha / A;
    const double a1In = -2.0 * cs;
    const double a2In = 1.0 - alpha / A;

    update(b0In / a0, b1In / a0, b2In / a0, a1In / a0, a2In / a0);
}

void Biquad::setLowShelf(double cutoff, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double omega = 2.0 * std::numbers::pi * cutoff / sr;
    const double sn = std::sin(omega);
    const double cs = std::cos(omega);
    const double beta = std::sqrt(A) / 0.70710678;

    const double b0In = A * ((A + 1.0) - (A - 1.0) * cs + beta * sn);
    const double b1In = 2.0 * A * ((A - 1.0) - (A + 1.0) * cs);
    const double b2In = A * ((A + 1.0) - (A - 1.0) * cs - beta * sn);
    const double a0 = (A + 1.0) + (A - 1.0) * cs + beta * sn;
    const double a1In = -2.0 * ((A - 1.0) + (A + 1.0) * cs);
    const double a2In = (A + 1.0) + (A - 1.0) * cs - beta * sn;

    update(b0In / a0, b1In / a0, b2In / a0, a1In / a0, a2In / a0);
}

void Biquad::setHighShelf(double cutoff, double gainDb)
{
    const double A = std::pow(10.0, gainDb / 40.0);
    const double omega = 2.0 * std::numbers::pi * cutoff / sr;
    const double sn = std::sin(omega);
    const double cs = std::cos(omega);
    const double beta = std::sqrt(A) / 0.70710678;

    const double b0In = A * ((A + 1.0) + (A - 1.0) * cs + beta * sn);
    const double b1In = -2.0 * A * ((A - 1.0) + (A + 1.0) * cs);
    const double b2In = A * ((A + 1.0) + (A - 1.0) * cs - beta * sn);
    const double a0 = (A + 1.0) - (A - 1.0) * cs + beta * sn;
    const double a1In = 2.0 * ((A - 1.0) - (A + 1.0) * cs);
    const double a2In = (A + 1.0) - (A - 1.0) * cs - beta * sn;

    update(b0In / a0, b1In / a0, b2In / a0, a1In / a0, a2In / a0);
}

double Biquad::process(double input)
{
    const double output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;
    return output;
}

void Biquad::update(double b0In, double b1In, double b2In, double a1In, double a2In)
{
    b0 = b0In;
    b1 = b1In;
    b2 = b2In;
    a1 = a1In;
    a2 = a2In;
    z1 = z2 = 0.0;
}

void ParameterSmoother::reset(double sampleRateIn, double timeMs, double initialValue)
{
    sr = sampleRateIn;
    const double timeSamples = std::max(timeMs, 0.01) * 0.001 * sr;
    coef = std::exp(-1.0 / timeSamples);
    state = initialValue;
}

double ParameterSmoother::process(double targetValue)
{
    state = targetValue + coef * (state - targetValue);
    return state;
}
} // namespace gptaudio::dsp
