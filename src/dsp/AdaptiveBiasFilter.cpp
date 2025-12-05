#include "AdaptiveBiasFilter.h"

#include <cmath>

namespace gptaudio::dsp
{
void AdaptiveBiasFilter::prepare(double sampleRate)
{
    sr = sampleRate;
    envelope.reset(sr, 12.0, 0.0);
    sag.reset(sr, 5.0, 80.0, 0.0);
}

void AdaptiveBiasFilter::setAmount(double newAmount)
{
    amount = std::clamp(newAmount, 0.0, 1.0);
}

void AdaptiveBiasFilter::setResponse(double milliseconds)
{
    const double clamped = std::clamp(milliseconds, 5.0, 200.0);
    envelope.setCutoff(1.0 / (clamped * 0.001));
    sag.reset(sr, clamped, clamped * 3.0, lastBias);
}

void AdaptiveBiasFilter::setHeadroom(double headroomDbIn)
{
    headroom = headroomDbIn;
}

void AdaptiveBiasFilter::reset(double value)
{
    lastBias = value;
    envelope.reset(sr, 12.0, value);
    sag.reset(sr, 5.0, 80.0, value);
}

double AdaptiveBiasFilter::process(double input)
{
    const double level = envelope.process(std::abs(input));
    const double sagged = sag.process(level);

    const double dynamic = -amount * sagged * std::pow(10.0, headroom / 20.0);
    lastBias = dynamic;
    return dynamic;
}
} // namespace gptaudio::dsp
