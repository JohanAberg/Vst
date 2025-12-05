#include "AnalogSaturationController.h"

#include <algorithm>

#include "base/source/fstring.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"

namespace gptaudio
{
using namespace Steinberg;
using namespace Steinberg::Vst;

FUnknown* AnalogSaturationController::createInstance(void*)
{
    return (IEditController*)new AnalogSaturationController();
}

tresult PLUGIN_API AnalogSaturationController::initialize(FUnknown* context)
{
    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk)
        return result;

    auto* drive = new RangeParameter(USTRING("Drive"), saturation::kParamDrive, USTRING(" dB"), -6.0, 36.0, 12.0);
    parameters.addParameter(drive);

    auto* bias = new RangeParameter(USTRING("Bias"), saturation::kParamBias, USTRING(""), -1.0, 1.0, 0.0);
    parameters.addParameter(bias);

    auto* evenOdd = new RangeParameter(USTRING("Even/Odd Blend"), saturation::kParamEvenOdd, USTRING(" %"), 0.0, 1.0, 0.5);
    parameters.addParameter(evenOdd);

    auto* mode = new StringListParameter(USTRING("Topology"), saturation::kParamMode);
    mode->appendString(USTRING("Triode A"));
    mode->appendString(USTRING("Push-Pull Bus"));
    mode->appendString(USTRING("Tape Fuse"));
    parameters.addParameter(mode);

    auto* tone = new RangeParameter(USTRING("Tilt"), saturation::kParamTone, USTRING(""), -1.0, 1.0, 0.0);
    parameters.addParameter(tone);

    auto* mix = new RangeParameter(USTRING("Mix"), saturation::kParamMix, USTRING(" %"), 0.0, 1.0, 1.0);
    parameters.addParameter(mix);

    auto* output = new RangeParameter(USTRING("Output"), saturation::kParamOutput, USTRING(" dB"), -24.0, 12.0, 0.0);
    parameters.addParameter(output);

    auto* dynamics = new RangeParameter(USTRING("Dynamics"), saturation::kParamDynamics, USTRING(" %"), 0.0, 1.0, 0.4);
    parameters.addParameter(dynamics);

    auto* transient = new RangeParameter(USTRING("Transient"), saturation::kParamTransient, USTRING(" %"), 0.0, 1.0, 0.5);
    parameters.addParameter(transient);

    return kResultOk;
}

tresult PLUGIN_API AnalogSaturationController::setComponentState(IBStream* state)
{
    if (!state)
        return kResultFalse;

    IBStreamer streamer(state, kLittleEndian);
    double value = 0.0;

    auto toNorm = [](double x, double min, double max) {
        return std::clamp((x - min) / (max - min), 0.0, 1.0);
    };

    constexpr double driveMin = -6.0;
    constexpr double driveMax = 36.0;
    constexpr double outputMin = -24.0;
    constexpr double outputMax = 12.0;

    streamer.readDouble(value);
    setParamNormalized(saturation::kParamDrive, toNorm(value, driveMin, driveMax));
    streamer.readDouble(value);
    setParamNormalized(saturation::kParamBias, toNorm(value, -1.0, 1.0));
    streamer.readDouble(value);
    setParamNormalized(saturation::kParamEvenOdd, std::clamp(value, 0.0, 1.0));
    streamer.readDouble(value);
    setParamNormalized(saturation::kParamMode, toNorm(value, 0.0, 2.0));
    streamer.readDouble(value);
    setParamNormalized(saturation::kParamTone, toNorm(value, -1.0, 1.0));
    streamer.readDouble(value);
    setParamNormalized(saturation::kParamMix, std::clamp(value, 0.0, 1.0));
    streamer.readDouble(value);
    setParamNormalized(saturation::kParamOutput, toNorm(value, outputMin, outputMax));
    streamer.readDouble(value);
    setParamNormalized(saturation::kParamDynamics, std::clamp(value, 0.0, 1.0));
    streamer.readDouble(value);
    setParamNormalized(saturation::kParamTransient, std::clamp(value, 0.0, 1.0));

    return kResultOk;
}
} // namespace gptaudio
