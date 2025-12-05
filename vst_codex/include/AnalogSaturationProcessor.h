#pragma once

#include <array>
#include <vector>

#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "public.sdk/source/vst/vstaudioeffect.h"

#include "AnalogSaturationIDs.h"
#include "dsp/SaturationModel.h"

namespace analog {

class AnalogSaturationProcessor final : public Steinberg::Vst::AudioEffect {
public:
    AnalogSaturationProcessor();

    //--- from AudioEffect ---
    Steinberg::tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

    void PLUGIN_API setSampleRate(Steinberg::SampleRate sampleRate) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs,
                                                     Steinberg::int32 numIns,
                                                     Steinberg::Vst::SpeakerArrangement* outputs,
                                                     Steinberg::int32 numOuts) SMTG_OVERRIDE;

    static Steinberg::FUnknown* createInstance(void*) { return static_cast<Steinberg::Vst::IAudioProcessor*>(new AnalogSaturationProcessor()); }

private:
    void syncModelWithParameters();
    void updateSmoothing(Steinberg::SampleRate sampleRate);

    dsp::SaturationModel model_;

    struct SmoothedValue {
        void setTime(double timeMs, double sampleRate);
        void setCurrent(float value);
        void setTarget(float value);
        float getNext();

        double coeff {0.0};
        float current {0.0F};
        float target {0.0F};
    };

    SmoothedValue drive_;
    SmoothedValue bias_;
    SmoothedValue color_;
    SmoothedValue mix_;
    SmoothedValue outputTrim_;
    SmoothedValue dynamics_;
    SmoothedValue slew_;

    float bypass_ {0.0F};
    double sampleRate_ {44100.0};
    Steinberg::Vst::ProcessSetup setup_ {};

    std::array<std::vector<float>, 2> tempIn_ {};
    std::array<std::vector<float>, 2> tempOut_ {};
};

} // namespace analog
