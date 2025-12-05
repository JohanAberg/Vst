#pragma once

#include <array>

#include "AnalogCircuitModel.h"
#include "public.sdk/source/vst/vstaudioeffect.h"

namespace analog::sat {

class SaturationProcessor : public Steinberg::Vst::AudioEffect {
   public:
    SaturationProcessor();

    static Steinberg::FUnknown* createInstance(void* /*context*/) {
        return (Steinberg::Vst::IAudioProcessor*)new SaturationProcessor();
    }

    // AudioEffect overrides
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, double value) SMTG_OVERRIDE;

   private:
    void syncParameters();

    AnalogCircuitModel model_;
    std::array<double, kParameterCount> paramValues_{};
    double sampleRate_ = 48000.0;
};

}  // namespace analog::sat
