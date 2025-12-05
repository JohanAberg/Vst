#include "SaturationParameters.h"

#include <algorithm>

namespace analog::sat {

namespace {
constexpr double kBiasRange = 0.55;
constexpr double kReactanceMin = 0.1;
constexpr double kReactanceMax = 0.95;
constexpr double kDynamicsMin = 0.15;
constexpr double kDynamicsMax = 0.9;
constexpr double kMixMin = 0.15;
constexpr double kMixMax = 1.0;
}  // namespace

ParameterBlock computeParameterBlock(const std::array<double, kParameterCount>& normalizedValues,
                                     double /*sampleRate*/) {
    ParameterBlock block;

    const auto norm = [](double value) {
        return clamp(value, 0.0, 1.0);
    };

    const auto indexed = [&](ParameterID id) {
        return norm(normalizedValues[parameterIndex(id)]);
    };

    block.driveGain = dbToGain(normToRange(indexed(kParamDrive), kMinDriveDb, kMaxDriveDb));
    block.bias = lerp(-kBiasRange, kBiasRange, indexed(kParamBias));
    block.evenOddBlend = indexed(kParamEvenOdd);

    const double toneNorm = indexed(kParamTone);
    const double logMin = std::log(kMinToneHz);
    const double logMax = std::log(kMaxToneHz);
    block.toneHz = std::exp(lerp(logMin, logMax, toneNorm));

    block.dynamics = lerp(kDynamicsMin, kDynamicsMax, indexed(kParamDynamics));
    block.mix = lerp(kMixMin, kMixMax, indexed(kParamMix));
    block.makeupGain = dbToGain(normToRange(indexed(kParamOutput), kMinOutputDb, kMaxOutputDb));
    block.reactance = lerp(kReactanceMin, kReactanceMax, indexed(kParamTransientReactance));

    const double oversampleNorm = indexed(kParamOversample);
    block.oversampleFactor = oversampleNorm < 0.33
                                 ? 1
                                 : (oversampleNorm < 0.66 ? 2 : 4);

    return block;
}

}  // namespace analog::sat
