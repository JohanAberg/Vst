#include "AnalogCircuitModel.h"

#include <algorithm>
#include <cmath>

namespace analog::sat {

namespace {
constexpr double kPreEmphasisHz = 180.0;
constexpr double kDeEmphasisHz = 16000.0;
}

void AnalogCircuitModel::prepare(double sampleRate, int /*maxBlockSize*/) {
    sampleRate_ = sampleRate;
    reset();
}

void AnalogCircuitModel::setParameters(const ParameterBlock& block) {
    parameters_ = block;
    for (auto& channel : channels_) {
        channel.triode.setGain(parameters_.driveGain);
        channel.diode.setDrive(parameters_.driveGain * 0.75);
        channel.transformer.setReactance(parameters_.reactance);
        updateFilters(channel);
    }
}

void AnalogCircuitModel::reset() {
    for (auto& channel : channels_) {
        channel.preEmphasis.reset();
        channel.deEmphasis.reset();
        channel.triode.reset();
        channel.diode.reset();
        channel.transformer.reset();
        channel.envelope.reset();
        channel.dcOffset = 0.0;
        channel.lastInput = 0.0;
        channel.envelope.setCoefficients(0.5, 60.0, sampleRate_);
        updateFilters(channel);
    }
}

void AnalogCircuitModel::updateFilters(ChannelState& state) {
    state.preEmphasis.setHighpass(sampleRate_, kPreEmphasisHz, 0.707);
    const double tone = std::clamp(parameters_.toneHz, 200.0, 18000.0);
    state.deEmphasis.setLowpass(sampleRate_, tone, 0.707);
}

void AnalogCircuitModel::process(float** inputs, float** outputs, int32_t numChannels, int32_t numSamples) {
    if (!inputs || !outputs) {
        return;
    }
    const auto channels = std::min<int32_t>(numChannels, static_cast<int32_t>(channels_.size()));
    for (int32_t ch = 0; ch < channels; ++ch) {
        auto* in = inputs[ch];
        auto* out = outputs[ch];
        auto& state = channels_[ch];
        for (int32_t n = 0; n < numSamples; ++n) {
            out[n] = static_cast<float>(processSample(in[n], state));
        }
        state.lastInput = in[numSamples - 1];
    }
}

double AnalogCircuitModel::processSample(double input, ChannelState& state) {
    const double envelope = state.envelope.process(input);
    const double adaptiveBias = parameters_.bias + parameters_.dynamics * (envelope - 0.5);

    const double oversampleFactor = static_cast<double>(std::max(1, parameters_.oversampleFactor));
    double accumulated = 0.0;
    double previous = state.lastInput;

    for (int i = 0; i < parameters_.oversampleFactor; ++i) {
        const double t = (i + 1.0) / oversampleFactor;
        const double interpolated = previous + (input - previous) * t;
        double pre = state.preEmphasis.process(interpolated * parameters_.driveGain + adaptiveBias);
        double triode = state.triode.process(pre);
        double evenComponent = state.diode.process(triode + adaptiveBias * 0.35);
        double oddComponent = state.diode.process(triode);
        double blended = std::lerp(oddComponent, evenComponent, parameters_.evenOddBlend);
        double transformer = state.transformer.process(blended);
        double post = state.deEmphasis.process(transformer);
        accumulated += post;
    }

    state.lastInput = input;
    const double oversampled = accumulated / oversampleFactor;
    state.dcOffset += 0.00005 * (oversampled - state.dcOffset);

    const double wet = (oversampled - state.dcOffset) * parameters_.makeupGain;
    const double dry = input;
    const double mixed = parameters_.mix * wet + (1.0 - parameters_.mix) * dry;
    return mixed;
}

}  // namespace analog::sat
