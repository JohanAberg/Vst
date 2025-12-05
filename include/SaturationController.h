#pragma once

#include "SaturationParameters.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace analog::sat {

class SaturationController : public Steinberg::Vst::EditControllerEx1 {
   public:
    SaturationController() = default;

    static Steinberg::FUnknown* createInstance(void* /*context*/) {
        return (Steinberg::Vst::IEditController*)new SaturationController();
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
};

}  // namespace analog::sat
