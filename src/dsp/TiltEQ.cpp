#include "TiltEQ.h"

#include <algorithm>

namespace gptaudio::dsp
{
void TiltEQ::prepare(double sampleRate)
{
    sr = sampleRate;
    for (auto& filter : lowShelf)
    {
        filter.reset(sr);
        filter.setLowShelf(pivot, 0.0);
    }
    for (auto& filter : highShelf)
    {
        filter.reset(sr);
        filter.setHighShelf(pivot, 0.0);
    }
}

void TiltEQ::setTilt(double newTilt)
{
    tilt = std::clamp(newTilt, -1.0, 1.0);
    const double gainDb = tilt * 6.0;
    for (auto& filter : lowShelf)
    {
        filter.setLowShelf(pivot, -gainDb);
    }
    for (auto& filter : highShelf)
    {
        filter.setHighShelf(pivot, gainDb);
    }
}

void TiltEQ::setPivot(double pivotHz)
{
    pivot = std::clamp(pivotHz, 200.0, 8000.0);
    setTilt(tilt);
}

double TiltEQ::process(double input, int channel)
{
    const int idx = std::clamp(channel, 0, 1);
    const double lows = lowShelf[idx].process(input);
    const double highs = highShelf[idx].process(lows);
    return highs;
}

void TiltEQ::reset(double value)
{
    for (auto& filter : lowShelf)
    {
        filter.reset(sr);
        filter.process(value);
    }
    for (auto& filter : highShelf)
    {
        filter.reset(sr);
        filter.process(value);
    }
}
} // namespace gptaudio::dsp
