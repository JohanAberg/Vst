#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

#include "AnalogSaturationIDs.h"

namespace analog {

class AnalogSaturationController final : public Steinberg::Vst::EditControllerEx1 {
public:
    AnalogSaturationController() = default;

    static Steinberg::FUnknown* createInstance(void*) { return static_cast<Steinberg::Vst::IEditController*>(new AnalogSaturationController()); }

    //--- from EditController -----
    Steinberg::tresult PLUGIN_API initialize(FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE;

    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
};

} // namespace analog
