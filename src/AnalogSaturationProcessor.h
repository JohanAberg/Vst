#pragma once

#include <vector>

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/vst/vstaudioeffect.h"

#include "AnalogSaturationIDs.h"
#include "dsp/AnalogSaturationModel.h"

namespace gptaudio
{
class AnalogSaturationProcessor : public Steinberg::Vst::AudioEffect
{
public:
    AnalogSaturationProcessor();

    static Steinberg::FUnknown* createInstance(void*);

    // AudioEffect overrides
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

private:
    void handleParameterChanges(Steinberg::Vst::ProcessData& data);
    void syncModel();

    dsp::AnalogSaturationModel model;
    double sampleRate {48000.0};
    Steinberg::int32 maxBlockSize {512};
    std::vector<float> scratchIn[2];
    std::vector<float> scratchOut[2];

    struct ParameterState
    {
        double driveDb {12.0};
        double bias {0.0};
        double evenOdd {0.5};
        double mode {0.0};
        double tone {0.0};
        double mix {1.0};
        double outputDb {0.0};
        double dynamics {0.4};
        double transient {0.5};
    } params;
};
} // namespace gptaudio
