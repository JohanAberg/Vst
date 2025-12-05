#include "AnalogSaturationModel.h"

#include <algorithm>
#include <cmath>

namespace gptaudio::dsp
{
void AnalogSaturationModel::prepare(double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    blockSize = std::max(maxBlockSize, 32);

    tempLeft.assign(blockSize, 0.0);
    tempRight.assign(blockSize, 0.0);

    for (int ch = 0; ch < 2; ++ch)
    {
        preHighPass[ch].reset(sr);
        postLowPass[ch].reset(sr);
        postLowPass[ch].setLowPass(18000.0, 0.707);
        adaptiveBias[ch].prepare(sr);
        nonlinear[ch].reset();
    }

    tilt.prepare(sr);
    tilt.setPivot(1400.0);

    setDriveDb(0.0);
    setBias(0.0);
    setEvenOdd(0.5);
    setMode(0);
    setTone(0.0);
    setMix(1.0);
    setOutputDb(0.0);
    setDynamics(0.3);
    setTransient(0.5);

    reset();
}

void AnalogSaturationModel::reset()
{
    for (auto& stage : nonlinear)
    {
        stage.reset();
    }
    for (auto& bias : adaptiveBias)
    {
        bias.reset();
    }
    tilt.reset();
}

void AnalogSaturationModel::setDriveDb(double driveDb)
{
    driveLinear = dbToLinear(driveDb);
    applyModeConfiguration();
}

void AnalogSaturationModel::setBias(double bias)
{
    biasAmount = std::clamp(bias, -1.0, 1.0);
    applyModeConfiguration();
}

void AnalogSaturationModel::setEvenOdd(double evenOdd)
{
    evenBlend = std::clamp(evenOdd, 0.0, 1.0);
    applyModeConfiguration();
}

void AnalogSaturationModel::setMode(int modeIndex)
{
    mode = std::clamp(modeIndex, 0, 2);
    applyModeConfiguration();
}

void AnalogSaturationModel::setTone(double tiltValue)
{
    tiltAmount = std::clamp(tiltValue, -1.0, 1.0);
    tilt.setTilt(tiltAmount);
}

void AnalogSaturationModel::setMix(double mixValue)
{
    mix = std::clamp(mixValue, 0.0, 1.0);
}

void AnalogSaturationModel::setOutputDb(double outputDb)
{
    outputGain = dbToLinear(outputDb);
}

void AnalogSaturationModel::setDynamics(double amount)
{
    dynamics = std::clamp(amount, 0.0, 1.0);
    for (auto& stage : adaptiveBias)
    {
        stage.setAmount(dynamics);
    }
}

void AnalogSaturationModel::setTransient(double emphasis)
{
    transient = std::clamp(emphasis, 0.0, 1.0);
    const double cutoff = 40.0 + transient * 1600.0;
    for (auto& filter : preHighPass)
    {
        filter.setHighPass(cutoff, 0.707);
    }
}

void AnalogSaturationModel::process(float** inputs, float** outputs, int32_t numChannels, int32_t numSamples)
{
    if (!inputs || !outputs || numChannels <= 0 || numSamples <= 0)
        return;

    const int32_t channels = std::min<int32_t>(numChannels, 2);

    for (int32_t ch = 0; ch < channels; ++ch)
    {
        float* out = outputs[ch];
        const float* in = inputs[ch];

        for (int32_t i = 0; i < numSamples; ++i)
        {
            const double dry = in[i];
            const double emphasised = transient * preHighPass[ch].process(dry) + (1.0 - transient) * dry;
            const double biased = emphasised * driveLinear;
            const double dynamicBias = adaptiveBias[ch].process(biased) * dynamics + biasAmount;
            nonlinear[ch].setAsymmetry(dynamicBias);

            double shaped = nonlinear[ch].process(biased);
            shaped = postLowPass[ch].process(shaped);
            shaped = tilt.process(shaped, ch);

            const double wet = mix * shaped + (1.0 - mix) * dry;
            out[i] = static_cast<float>(wet * outputGain);
        }
    }

    for (int32_t ch = channels; ch < numChannels; ++ch)
    {
        outputs[ch] = inputs[ch];
    }
}

void AnalogSaturationModel::applyModeConfiguration()
{
    const double driveScale = (mode == 0) ? 1.8 : (mode == 1) ? 1.2 : 0.9;
    const double feedback = (mode == 0) ? 0.35 : (mode == 1) ? 0.25 : 0.15;
    const double sineAmt = (mode == 2) ? 0.85 : 0.35;

    for (auto& stage : nonlinear)
    {
        stage.setDrive(driveLinear * driveScale);
        stage.setEvenOdd(evenBlend);
        stage.setFeedback(feedback);
        stage.setSineAmount(sineAmt);
        stage.setAsymmetry(biasAmount);
    }
}
} // namespace gptaudio::dsp
