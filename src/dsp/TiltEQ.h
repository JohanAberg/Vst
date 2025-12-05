#pragma once

#include "Utilities.h"

namespace gptaudio::dsp
{
    class TiltEQ
    {
    public:
        void prepare(double sampleRate);
        void setTilt(double tilt); // -1 .. +1
        void setPivot(double pivotHz);
        double process(double input, int channel);
        void reset(double value = 0.0);

    private:
        double sr {48000.0};
        double pivot {1600.0};
        double tilt {0.0};
        Biquad lowShelf[2];
        Biquad highShelf[2];
    };
} // namespace gptaudio::dsp
