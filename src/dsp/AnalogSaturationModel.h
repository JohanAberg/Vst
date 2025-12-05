#pragma once

#include <cstdint>
#include <vector>

#include "AdaptiveBiasFilter.h"
#include "NonLinearStages.h"
#include "TiltEQ.h"
#include "Utilities.h"

namespace gptaudio::dsp
{
    class AnalogSaturationModel
    {
    public:
        void prepare(double sampleRate, int maxBlockSize);
        void reset();

        void setDriveDb(double driveDb);
        void setBias(double bias);
        void setEvenOdd(double evenOdd);
        void setMode(int modeIndex);
        void setTone(double tilt);
        void setMix(double mix);
        void setOutputDb(double outputDb);
        void setDynamics(double amount);
        void setTransient(double emphasis);

        void process(float** inputs, float** outputs, int32_t numChannels, int32_t numSamples);

    private:
        void applyModeConfiguration();

        double sr {48000.0};
        int blockSize {512};
        double driveLinear {1.0};
        double biasAmount {0.0};
        double evenBlend {0.5};
        double tiltAmount {0.0};
        double mix {1.0};
        double outputGain {1.0};
        double dynamics {0.3};
        double transient {0.5};
        int mode {0};

        std::vector<double> tempLeft;
        std::vector<double> tempRight;

        Biquad preHighPass[2];
        Biquad postLowPass[2];
        AdaptiveBiasFilter adaptiveBias[2];
        NonLinearStageChain nonlinear[2];
        TiltEQ tilt;
    };
} // namespace gptaudio::dsp
