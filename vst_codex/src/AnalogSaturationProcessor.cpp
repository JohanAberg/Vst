#include "AnalogSaturationProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <type_traits>

#include "pluginterfaces/base/ibstream.h"

namespace analog {

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {
constexpr double kSmoothingTimeMs = 15.0;
}

AnalogSaturationProcessor::AnalogSaturationProcessor()
{
    setControllerClass(ids::kControllerUID);
}

tresult PLUGIN_API AnalogSaturationProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk) {
        return result;
    }

    addAudioInput(STR16("Input"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Output"), SpeakerArr::kStereo);

    model_.prepare(sampleRate_, 512);

    drive_.setCurrent(0.5F);
    bias_.setCurrent(0.0F);
    color_.setCurrent(0.5F);
    mix_.setCurrent(1.0F);
    outputTrim_.setCurrent(0.0F);
    dynamics_.setCurrent(0.5F);
    slew_.setCurrent(0.5F);

    return kResultOk;
}

tresult PLUGIN_API AnalogSaturationProcessor::terminate()
{
    return AudioEffect::terminate();
}

void PLUGIN_API AnalogSaturationProcessor::setSampleRate(SampleRate sampleRate)
{
    AudioEffect::setSampleRate(sampleRate);
    sampleRate_ = sampleRate;
    updateSmoothing(sampleRate_);
}

tresult PLUGIN_API AnalogSaturationProcessor::setupProcessing(ProcessSetup& setup)
{
    setup_ = setup;
    sampleRate_ = setup.sampleRate;
    updateSmoothing(sampleRate_);
    model_.prepare(sampleRate_, static_cast<int>(setup.maxSamplesPerBlock));
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API AnalogSaturationProcessor::setBusArrangements(SpeakerArrangement* inputs,
                                                                 int32 numIns,
                                                                 SpeakerArrangement* outputs,
                                                                 int32 numOuts)
{
    if (numIns != 1 || numOuts != 1) {
        return kResultFalse;
    }

    if (SpeakerArr::getChannelCount(inputs[0]) != 2 || SpeakerArr::getChannelCount(outputs[0]) != 2) {
        return kResultFalse;
    }

    return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

void AnalogSaturationProcessor::syncModelWithParameters()
{
    dsp::SaturationSettings settings = model_.getSettings();
    settings.drive = drive_.target;
    settings.bias = bias_.target;
    settings.color = color_.target;
    settings.mix = mix_.target;
    settings.dynamics = dynamics_.target;
    settings.slew = slew_.target;
    settings.outputTrim = outputTrim_.target;
    settings.quality = std::clamp(settings.quality, 0.0F, 1.0F);
    model_.setSettings(settings);
}

void AnalogSaturationProcessor::updateSmoothing(SampleRate sampleRate)
{
    drive_.setTime(kSmoothingTimeMs, sampleRate);
    bias_.setTime(kSmoothingTimeMs, sampleRate);
    color_.setTime(kSmoothingTimeMs, sampleRate);
    mix_.setTime(kSmoothingTimeMs, sampleRate);
    outputTrim_.setTime(kSmoothingTimeMs, sampleRate);
    dynamics_.setTime(kSmoothingTimeMs, sampleRate);
    slew_.setTime(kSmoothingTimeMs * 2.0, sampleRate);
}

static void readOrWriteState(IBStream* state, void* data, int64 size, bool write)
{
    if (!state) {
        return;
    }
    if (write) {
        state->write(data, size);
    } else {
        state->read(data, size);
    }
}

tresult PLUGIN_API AnalogSaturationProcessor::setState(IBStream* state)
{
    dsp::SaturationSettings settings = model_.getSettings();
    readOrWriteState(state, &settings, sizeof(settings), false);
    model_.setSettings(settings);
    drive_.setCurrent(settings.drive);
    bias_.setCurrent(settings.bias);
    color_.setCurrent(settings.color);
    mix_.setCurrent(settings.mix);
    outputTrim_.setCurrent(settings.outputTrim);
    dynamics_.setCurrent(settings.dynamics);
    slew_.setCurrent(settings.slew);
    return kResultOk;
}

tresult PLUGIN_API AnalogSaturationProcessor::getState(IBStream* state)
{
    auto settings = model_.getSettings();
    readOrWriteState(state, &settings, sizeof(settings), true);
    return kResultOk;
}

static float getNormalizedValue(IParamValueQueue* queue)
{
    if (!queue) {
        return 0.0F;
    }
    int32 lastIndex = queue->getPointCount() - 1;
    if (lastIndex < 0) {
        return 0.0F;
    }
    int32 sampleOffset = 0;
    double value = 0.0;
    if (queue->getPoint(lastIndex, sampleOffset, value) == kResultTrue) {
        return static_cast<float>(value);
    }
    return 0.0F;
}

void AnalogSaturationProcessor::SmoothedValue::setTime(double timeMs, double sampleRate)
{
    if (timeMs <= 0.0 || sampleRate <= 0.0) {
        coeff = 0.0;
        return;
    }
    double tau = timeMs * 0.001 * sampleRate;
    coeff = tau <= 1.0 ? 0.0 : std::exp(-1.0 / tau);
}

void AnalogSaturationProcessor::SmoothedValue::setCurrent(float value)
{
    current = value;
    target = value;
}

void AnalogSaturationProcessor::SmoothedValue::setTarget(float value)
{
    target = value;
}

float AnalogSaturationProcessor::SmoothedValue::getNext()
{
    current += static_cast<float>((1.0 - coeff) * (target - current));
    return current;
}

tresult PLUGIN_API AnalogSaturationProcessor::process(ProcessData& data)
{
    if (data.numInputs == 0 || data.numOutputs == 0) {
        return kResultOk;
    }

    IParameterChanges* params = data.inputParameterChanges;
    if (params) {
        const int32 numParams = params->getParameterCount();
        for (int32 i = 0; i < numParams; ++i) {
            auto* queue = params->getParameterData(i);
            if (!queue) {
                continue;
            }

            ParamID pid = queue->getParameterId();
            float value = getNormalizedValue(queue);
            switch (pid) {
                case ids::kDrive:
                    drive_.setTarget(value);
                    break;
                case ids::kBias:
                    bias_.setTarget(value * 2.0F - 1.0F);
                    break;
                case ids::kColor:
                    color_.setTarget(value);
                    break;
                case ids::kMix:
                    mix_.setTarget(value);
                    break;
                case ids::kOutputTrim:
                    outputTrim_.setTarget(-12.0F + value * 24.0F);
                    break;
                case ids::kDynamics:
                    dynamics_.setTarget(value);
                    break;
                case ids::kSlew:
                    slew_.setTarget(value);
                    break;
                case ids::kQuality:
                {
                    dsp::SaturationSettings settings = model_.getSettings();
                    settings.quality = value;
                    model_.setSettings(settings);
                    break;
                }
                case ids::kBypass:
                    bypass_ = value;
                    break;
                default:
                    break;
            }
        }
    }

    syncModelWithParameters();

    const bool is64Bit = data.symbolicSampleSize == kSample64;

    auto copyBypass = [&](auto** dst, auto** src) {
        if (!dst || !src) {
            return;
        }
        for (int32 ch = 0; ch < 2; ++ch) {
            if (!dst[ch] || !src[ch]) {
                continue;
            }
            if constexpr (std::is_same_v<std::remove_pointer_t<decltype(dst[ch])>, float>) {
                std::memcpy(dst[ch], src[ch], sizeof(float) * data.numSamples);
            } else {
                std::memcpy(dst[ch], src[ch], sizeof(double) * data.numSamples);
            }
        }
    };

    if (bypass_ >= 0.5F) {
        if (is64Bit) {
            copyBypass(data.outputs[0].channelBuffers64, data.inputs[0].channelBuffers64);
        } else {
            copyBypass(data.outputs[0].channelBuffers32, data.inputs[0].channelBuffers32);
        }
        return kResultOk;
    }

    if (is64Bit) {
        for (auto& buf : tempIn_) {
            buf.resize(data.numSamples);
        }
        for (auto& buf : tempOut_) {
            buf.resize(data.numSamples);
        }

        double** in64 = data.inputs[0].channelBuffers64;
        double** out64 = data.outputs[0].channelBuffers64;
        for (int32 ch = 0; ch < 2; ++ch) {
            for (int32 i = 0; i < data.numSamples; ++i) {
                tempIn_[ch][i] = static_cast<float>(in64[ch][i]);
            }
        }

        float* inputChannels[2] = {tempIn_[0].data(), tempIn_[1].data()};
        float* outputChannels[2] = {tempOut_[0].data(), tempOut_[1].data()};
        model_.process(inputChannels, outputChannels, 2, data.numSamples);

        for (int32 ch = 0; ch < 2; ++ch) {
            for (int32 i = 0; i < data.numSamples; ++i) {
                out64[ch][i] = static_cast<double>(tempOut_[ch][i]);
            }
        }
    } else {
        if (!data.inputs[0].channelBuffers32 || !data.outputs[0].channelBuffers32) {
            return kResultOk;
        }
        float* inputChannels[2] = {data.inputs[0].channelBuffers32[0], data.inputs[0].channelBuffers32[1]};
        float* outputChannels[2] = {data.outputs[0].channelBuffers32[0], data.outputs[0].channelBuffers32[1]};
        model_.process(inputChannels, outputChannels, 2, data.numSamples);
    }

    return kResultOk;
}

} // namespace analog
