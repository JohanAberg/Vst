#pragma once

#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

#include "AnalogSaturationIDs.h"

namespace gptaudio
{
class AnalogSaturationController : public Steinberg::Vst::EditControllerEx1
{
public:
    AnalogSaturationController() = default;

    static Steinberg::FUnknown* createInstance(void*);

    // EditController overrides
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
};
} // namespace gptaudio
