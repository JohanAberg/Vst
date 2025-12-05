#pragma once

#include <array>
#include <cstdint>

#include "AdaaWaveshaper.h"
#include "Biquad.h"
#include "SaturationParameters.h"
#include "pluginterfaces/base/types.h"

namespace analog::sat {

class AnalogCircuitModel {
   public:
    void prepare(double sampleRate, int maxBlockSize);
    void setParameters(const ParameterBlock& block);
    void reset();

    void process(float** inputs, float** outputs, int32_t numChannels, int32_t numSamples);

   private:
    struct ChannelState {
        Biquad preEmphasis;
        Biquad deEmphasis;
        AdaTanhStage triode;
        AdaSoftClipStage diode;
        HysteresisStage transformer;
        EnvelopeFollower envelope;
        double dcOffset = 0.0;
        double lastInput = 0.0;
    };

    double processSample(double input, ChannelState& state);
    void updateFilters(ChannelState& state);

    ParameterBlock parameters_{};
    std::array<ChannelState, 2> channels_{};
    double sampleRate_ = 48000.0;
};

}  // namespace analog::sat
