#include "dsp/SaturationModel.h"

#include <algorithm>

namespace analog::dsp {
namespace {
constexpr float kMaxSlewHz = 300000.0F;
constexpr float kMinSlewHz = 8000.0F;
}

void SaturationModel::prepare(double sampleRate, int maxBlockSize)
{
    sampleRate_ = sampleRate;
    reset();
}

void SaturationModel::reset()
{
    for (auto& s : slew_) {
        s.prev = 0.0F;
    }
    for (auto& h : hysteresis_) {
        h.memory = 0.0F;
    }
    for (auto& in : lastInput_) {
        in = 0.0F;
    }
}

void SaturationModel::setSettings(const SaturationSettings& s)
{
    settings_ = s;
    oversampleFactor_ = (settings_.quality >= 0.5F) ? 4 : 2;
}

void SaturationModel::process(float** inputs, float** outputs, int32_t numChannels, int32_t numSamples)
{
    if (!inputs || !outputs) {
        return;
    }

    for (int32_t c = 0; c < numChannels; ++c) {
        float* in = inputs[c];
        float* out = outputs[c];
        if (!in || !out) {
            continue;
        }

        for (int32_t i = 0; i < numSamples; ++i) {
            float dry = in[i];
            float wet = processSample(dry, static_cast<size_t>(c));
            float mix = std::clamp(settings_.mix, 0.0F, 1.0F);
            float blended = dry + (wet - dry) * mix;
            float trim = std::pow(10.0F, settings_.outputTrim / 20.0F);
            out[i] = blended * trim;
        }
    }
}

float SaturationModel::processSample(float in, size_t channel)
{
    const float preEmphasis = 0.6F + settings_.color * 0.8F;
    float emphasized = in * preEmphasis;

    const float drive = std::exp2(settings_.drive * 4.5F);
    emphasized *= drive;

    float previousInput = lastInput_[channel % lastInput_.size()];
    float accum = 0.0F;
    for (int f = 1; f <= oversampleFactor_; ++f) {
        float frac = static_cast<float>(f) / static_cast<float>(oversampleFactor_);
        float upsampled = previousInput + (emphasized - previousInput) * frac;
        upsampled = waveshaper(upsampled, channel);
        upsampled = slewLimit(upsampled, channel);
        accum += upsampled;
        previousInput = upsampled;
    }

    lastInput_[channel % lastInput_.size()] = emphasized;
    const float downsampled = accum / static_cast<float>(oversampleFactor_);
    return downsampled;
}

float SaturationModel::waveshaper(float x, size_t channel)
{
    auto& hyst = hysteresis_[channel % hysteresis_.size()];
    const float biasAmount = settings_.bias * 0.8F;
    const float dynamicMemory = hyst.memory * (0.15F + settings_.dynamics * 0.75F);
    float biased = x + biasAmount + dynamicMemory;

    const float asym = 0.4F + settings_.color * 0.6F;
    const float shaper1 = std::tanh(biased);
    const float shaper2 = std::atan(biased * (1.0F + asym * 2.0F));

    const float oddContribution = shaper1;
    const float evenContribution = shaper2 * asym;
    float combined = oddContribution * (1.0F - settings_.color) + evenContribution * settings_.color;

    const float memoryBlend = 0.35F + settings_.dynamics * 0.4F;
    hyst.memory = std::clamp(hyst.memory * (1.0F - memoryBlend) + combined * memoryBlend, -1.0F, 1.0F);

    const float parallelSoftClip = combined / (1.0F + std::fabs(combined));
    const float triodeEmu = 0.8F * combined + 0.2F * parallelSoftClip;

    return triodeEmu;
}

float SaturationModel::slewLimit(float x, size_t channel)
{
    auto& state = slew_[channel % slew_.size()];
    const float slewHz = kMinSlewHz + (kMaxSlewHz - kMinSlewHz) * settings_.slew;
    const float maxStep = slewHz / static_cast<float>(sampleRate_);
    float delta = x - state.prev;
    delta = std::clamp(delta, -maxStep, maxStep);
    state.prev += delta;
    return state.prev;
}

} // namespace analog::dsp
