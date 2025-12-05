#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace analog::sat {

enum ParameterID : Steinberg::Vst::ParamID {
    kParamDrive = 100,
    kParamBias,
    kParamEvenOdd,
    kParamTone,
    kParamDynamics,
    kParamMix,
    kParamOutput,
    kParamTransientReactance,
    kParamOversample,
};

struct ParameterSpec {
    ParameterID id;
    std::string_view title;
    std::string_view units;
    double defaultNormalized;
};

inline constexpr std::array<ParameterSpec, 9> kParameterSpecs = {{
    {kParamDrive, "Drive", "dB", 0.55},
    {kParamBias, "Bias", "%", 0.5},
    {kParamEvenOdd, "EvenOdd", "mix", 0.4},
    {kParamTone, "Tone", "Hz", 0.45},
    {kParamDynamics, "Dynamics", "ratio", 0.5},
    {kParamMix, "Mix", "%", 0.8},
    {kParamOutput, "Output", "dB", 0.5},
    {kParamTransientReactance, "Reactance", "ratio", 0.5},
    {kParamOversample, "Oversample", "x", 0.66},
}};

inline constexpr Steinberg::FUID kProcessorUID(0x2AB9A4B4, 0x1C57462F, 0x9CCE36E0, 0x98AF5196);
inline constexpr Steinberg::FUID kControllerUID(0x6B73F98A, 0x89424288, 0xAF65B6BC, 0x2749A52C);

inline constexpr std::size_t kParameterCount = kParameterSpecs.size();

inline constexpr double kMinDriveDb = -6.0;
inline constexpr double kMaxDriveDb = 42.0;
inline constexpr double kMinToneHz = 80.0;
inline constexpr double kMaxToneHz = 18000.0;
inline constexpr double kMinOutputDb = -18.0;
inline constexpr double kMaxOutputDb = 18.0;

inline constexpr double lerp(double a, double b, double t) { return a + (b - a) * t; }
inline constexpr double clamp(double v, double lo, double hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}
inline constexpr double normToRange(double norm, double min, double max) {
    return lerp(min, max, clamp(norm, 0.0, 1.0));
}
inline constexpr double dbToGain(double db) {
    return std::pow(10.0, db / 20.0);
}
inline constexpr double gainToDb(double gain) {
    return 20.0 * std::log10(gain);
}

constexpr std::size_t parameterIndex(ParameterID id) {
    for (std::size_t i = 0; i < kParameterSpecs.size(); ++i) {
        if (kParameterSpecs[i].id == id) {
            return i;
        }
    }
    return 0;
}

inline bool findParameterIndex(Steinberg::Vst::ParamID tag, std::size_t& index) {
    for (std::size_t i = 0; i < kParameterSpecs.size(); ++i) {
        if (kParameterSpecs[i].id == tag) {
            index = i;
            return true;
        }
    }
    return false;
}

struct ParameterBlock {
    double driveGain = dbToGain(12.0);
    double bias = 0.0;
    double evenOddBlend = 0.5;
    double toneHz = 2000.0;
    double dynamics = 0.5;
    double mix = 1.0;
    double makeupGain = 1.0;
    double reactance = 0.5;
    int oversampleFactor = 2;
};

ParameterBlock computeParameterBlock(const std::array<double, kParameterCount>& normalizedValues, double sampleRate);

}  // namespace analog::sat
