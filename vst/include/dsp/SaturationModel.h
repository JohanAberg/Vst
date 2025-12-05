#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace analog::dsp {

struct SaturationSettings {
    float drive = 0.5F;
    float bias = 0.0F;
    float color = 0.5F;
    float mix = 1.0F;
    float outputTrim = 0.0F;
    float dynamics = 0.5F;
    float slew = 0.5F;
    float quality = 1.0F; // 0 = eco, 1 = high
};

class SaturationModel {
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setSettings(const SaturationSettings& s);
    const SaturationSettings& getSettings() const { return settings_; }

    void process(float** inputs, float** outputs, int32_t numChannels, int32_t numSamples);

private:
    float processSample(float in, size_t channel);
    float waveshaper(float x, size_t channel);
    float slewLimit(float x, size_t channel);

    struct SlewState {
        float prev = 0.0F;
    };

    struct HysteresisState {
        float memory = 0.0F;
    };

    double sampleRate_ = 44100.0;
    int oversampleFactor_ = 2;
    SaturationSettings settings_ {};
    std::array<SlewState, 2> slew_ {};
    std::array<HysteresisState, 2> hysteresis_ {};
    std::array<float, 2> lastInput_ {0.0F, 0.0F};
};

} // namespace analog::dsp
