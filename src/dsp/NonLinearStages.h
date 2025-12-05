#pragma once

#include <array>
#include <cmath>

namespace gptaudio::dsp
{
    class AdaaTanh
    {
    public:
        void setGain(double newGain);
        double process(double input);
        void reset(double value = 0.0);

    private:
        double gain {1.0};
        double lastInput {0.0};
    };

    class AdaaSineFold
    {
    public:
        void setAmount(double newAmount);
        double process(double input);
        void reset(double value = 0.0);

    private:
        double amount {0.5};
        double lastInput {0.0};
    };

    class ZeroDelayFeedbackStage
    {
    public:
        void setFeedback(double feedback);
        void setSoftness(double softness);
        double process(double input);
        void reset(double value = 0.0);

    private:
        double fb {0.3};
        double softness {1.0};
        double lastOutput {0.0};
    };

    class NonLinearStageChain
    {
    public:
        void setDrive(double drive);
        void setEvenOdd(double evenOdd);
        void setAsymmetry(double bias);
        void setFeedback(double feedback);
        void setSineAmount(double sineAmount);
        double process(double input);
        void reset(double value = 0.0);

    private:
        AdaaTanh tanhStage;
        AdaaSineFold sineStage;
        ZeroDelayFeedbackStage feedbackStage;
        double evenRatio {0.6};
        double asym {0.0};
    };
} // namespace gptaudio::dsp
