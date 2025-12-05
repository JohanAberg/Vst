#include "SaturationController.h"

#include "SaturationParameters.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"

namespace analog::sat {

Steinberg::tresult PLUGIN_API SaturationController::initialize(Steinberg::FUnknown* context) {
    const auto result = EditControllerEx1::initialize(context);
    if (result != Steinberg::kResultTrue) {
        return result;
    }

    for (const auto& spec : kParameterSpecs) {
        Steinberg::Vst::String128 title;
        Steinberg::Vst::UString(title).fromAscii(spec.title.data());

        Steinberg::Vst::String128 units;
        Steinberg::Vst::UString(units).fromAscii(spec.units.data());

        parameters.addParameter(title, units, 0, spec.defaultNormalized,
                                Steinberg::Vst::ParameterInfo::kCanAutomate, spec.id);
    }

    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SaturationController::setComponentState(Steinberg::IBStream* state) {
    if (!state) {
        return Steinberg::kResultTrue;
    }

    Steinberg::IBStreamer streamer(state, kLittleEndian);
    for (const auto& spec : kParameterSpecs) {
        double value = 0.0;
        if (!streamer.readDouble(value)) {
            return Steinberg::kResultFalse;
        }
        setParamNormalized(spec.id, value);
    }

    return Steinberg::kResultTrue;
}

}  // namespace analog::sat
