#include "NonLinearStages.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace gptaudio::dsp
{
void AdaaTanh::setGain(double newGain)
{
    gain = std::max(newGain, 1e-6);
}

double AdaaTanh::process(double input)
{
    const double scaled = gain * input;
    const double prevScaled = gain * lastInput;
    const double delta = scaled - prevScaled;

    double output = 0.0;
    if (std::abs(delta) > 1e-9)
    {
        const double F = std::log(std::cosh(scaled));
        const double Fprev = std::log(std::cosh(prevScaled));
        output = (F - Fprev) / delta;
    }
    else
    {
        output = std::tanh(scaled);
    }

    lastInput = input;
    return output;
}

void AdaaTanh::reset(double value)
{
    lastInput = value;
}

void AdaaSineFold::setAmount(double newAmount)
{
    amount = std::clamp(newAmount, 0.0, 1.0);
}

double AdaaSineFold::process(double input)
{
    const double scaled = amount * input;
    const double prevScaled = amount * lastInput;
    const double delta = scaled - prevScaled;

    double output = 0.0;
    if (std::abs(delta) > 1e-9)
    {
        // Antiderivative of sin(x) is -cos(x)
        output = (std::cos(prevScaled) - std::cos(scaled)) / delta;
    }
    else
    {
        output = std::sin(scaled);
    }

    lastInput = input;
    return output;
}

void AdaaSineFold::reset(double value)
{
    lastInput = value;
}

void ZeroDelayFeedbackStage::setFeedback(double feedback)
{
    fb = std::clamp(feedback, 0.0, 0.95);
}

void ZeroDelayFeedbackStage::setSoftness(double softnessIn)
{
    softness = std::clamp(softnessIn, 0.2, 4.0);
}

double ZeroDelayFeedbackStage::process(double input)
{
    double y = lastOutput;
    for (int i = 0; i < 4; ++i)
    {
        const double f = y - input - fb * std::tanh(softness * y);
        const double df = 1.0 - fb * softness * (1.0 - std::tanh(softness * y) * std::tanh(softness * y));
        y -= f / df;
    }
    lastOutput = y;
    return y;
}

void ZeroDelayFeedbackStage::reset(double value)
{
    lastOutput = value;
}

void NonLinearStageChain::setDrive(double drive)
{
    tanhStage.setGain(drive);
}

void NonLinearStageChain::setEvenOdd(double evenOdd)
{
    evenRatio = std::clamp(evenOdd, 0.0, 1.0);
}

void NonLinearStageChain::setAsymmetry(double bias)
{
    asym = std::clamp(bias, -1.0, 1.0);
}

void NonLinearStageChain::setFeedback(double feedback)
{
    feedbackStage.setFeedback(feedback);
}

void NonLinearStageChain::setSineAmount(double sineAmount)
{
    sineStage.setAmount(sineAmount);
}

double NonLinearStageChain::process(double input)
{
    const double biased = input + asym;
    const double triode = tanhStage.process(biased);
    const double tape = sineStage.process(biased * 0.5);
    const double composite = evenRatio * triode + (1.0 - evenRatio) * tape;
    return feedbackStage.process(composite);
}

void NonLinearStageChain::reset(double value)
{
    tanhStage.reset(value);
    sineStage.reset(value);
    feedbackStage.reset(value);
}
} // namespace gptaudio::dsp
