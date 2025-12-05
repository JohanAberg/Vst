#pragma once

#include "Utilities.h"

namespace gptaudio::dsp
{
    class AdaptiveBiasFilter
    {
    public:
        void prepare(double sampleRate);
        void setAmount(double amount);
        void setResponse(double milliseconds);
        void setHeadroom(double headroomDb);
        double process(double input);
        void reset(double value = 0.0);

    private:
        double sr {48000.0};
        double amount {0.25};
        double headroom {0.0};
        OnePole envelope;
        SlewLimiter sag;
        double lastBias {0.0};
    };
} // namespace gptaudio::dsp
