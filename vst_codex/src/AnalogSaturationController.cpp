#include "AnalogSaturationController.h"

#include "dsp/SaturationModel.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"

namespace analog {

using namespace Steinberg;
using namespace Steinberg::Vst;

tresult PLUGIN_API AnalogSaturationController::initialize(FUnknown* context)
{
    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    auto* drive = new RangeParameter(USTRING("Drive"), ids::kDrive, nullptr, 0.0, 1.0, 0.5);
    drive->setPrecision(2);
    parameters.addParameter(drive);

    auto* bias = new RangeParameter(USTRING("Bias"), ids::kBias, nullptr, 0.0, 1.0, 0.5);
    bias->setPrecision(2);
    parameters.addParameter(bias);

    auto* color = new RangeParameter(USTRING("Color"), ids::kColor, nullptr, 0.0, 1.0, 0.5);
    color->setPrecision(2);
    parameters.addParameter(color);

    auto* mix = new RangeParameter(USTRING("Mix"), ids::kMix, nullptr, 0.0, 1.0, 1.0);
    mix->setPrecision(2);
    parameters.addParameter(mix);

    auto* trim = new RangeParameter(USTRING("Output Trim"), ids::kOutputTrim, nullptr, 0.0, 1.0, 0.5);
    trim->setPrecision(2);
    parameters.addParameter(trim);

    auto* dynamics = new RangeParameter(USTRING("Dynamics"), ids::kDynamics, nullptr, 0.0, 1.0, 0.5);
    dynamics->setPrecision(2);
    parameters.addParameter(dynamics);

    auto* slew = new RangeParameter(USTRING("Slew"), ids::kSlew, nullptr, 0.0, 1.0, 0.5);
    slew->setPrecision(2);
    parameters.addParameter(slew);

    auto* quality = new StringListParameter(USTRING("Quality"), ids::kQuality);
    quality->appendString(USTRING("Eco"));
    quality->appendString(USTRING("High"));
    parameters.addParameter(quality);

    auto* bypass = new RangeParameter(USTRING("Bypass"), ids::kBypass, nullptr, 0.0, 1.0, 0.0, 0, ParameterInfo::kIsBypass);
    parameters.addParameter(bypass);

    return kResultOk;
}

tresult PLUGIN_API AnalogSaturationController::terminate()
{
    return EditControllerEx1::terminate();
}

tresult PLUGIN_API AnalogSaturationController::setComponentState(IBStream* state)
{
    if (!state) {
        return kResultOk;
    }

    dsp::SaturationSettings settings {};
    if (state->read(&settings, sizeof(settings)) == kResultTrue) {
        setParamNormalized(ids::kDrive, settings.drive);
        setParamNormalized(ids::kBias, (settings.bias + 1.0F) * 0.5F);
        setParamNormalized(ids::kColor, settings.color);
        setParamNormalized(ids::kMix, settings.mix);
        setParamNormalized(ids::kOutputTrim, (settings.outputTrim + 12.0F) / 24.0F);
        setParamNormalized(ids::kDynamics, settings.dynamics);
        setParamNormalized(ids::kSlew, settings.slew);
        setParamNormalized(ids::kQuality, settings.quality);
    }

    return kResultOk;
}

} // namespace analog
