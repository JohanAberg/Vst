#include "AnalogSaturationProcessor.h"

#include <algorithm>
#include <cmath>

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstparameters.h"

#include "AnalogSaturationIDs.h"

namespace gptaudio
{
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace
{
constexpr double kDriveMin = -6.0;
constexpr double kDriveMax = 36.0;
constexpr double kOutputMin = -24.0;
constexpr double kOutputMax = 12.0;

inline double normToRange(double norm, double minValue, double maxValue)
{
    return minValue + (maxValue - minValue) * norm;
}
} // namespace

AnalogSaturationProcessor::AnalogSaturationProcessor()
{
    setControllerClass(saturation::kControllerUID);
}

FUnknown* AnalogSaturationProcessor::createInstance(void*)
{
    return (IAudioProcessor*)new AnalogSaturationProcessor();
}

tresult PLUGIN_API AnalogSaturationProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultOk)
        return result;

    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);

    return kResultOk;
}

tresult PLUGIN_API AnalogSaturationProcessor::setActive(TBool state)
{
    if (state)
    {
        model.prepare(sampleRate, maxBlockSize);
        syncModel();
    }
    else
    {
        model.reset();
    }
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API AnalogSaturationProcessor::setupProcessing(ProcessSetup& setup)
{
    sampleRate = setup.sampleRate;
    maxBlockSize = setup.maxSamplesPerBlock;

    for (auto& buffer : scratchIn)
    {
        buffer.resize(maxBlockSize);
    }
    for (auto& buffer : scratchOut)
    {
        buffer.resize(maxBlockSize);
    }

    model.prepare(sampleRate, static_cast<int>(maxBlockSize));
    return AudioEffect::setupProcessing(setup);
}

void AnalogSaturationProcessor::handleParameterChanges(ProcessData& data)
{
    if (!data.inputParameterChanges)
        return;

    const int32 numParamsChanged = data.inputParameterChanges->getParameterCount();
    for (int32 i = 0; i < numParamsChanged; ++i)
    {
        if (auto* queue = data.inputParameterChanges->getParameterData(i))
        {
            int32 sampleOffset = 0;
            double value = 0.0;
            queue->getPoint(queue->getPointCount() - 1, sampleOffset, value);

            switch (queue->getParameterId())
            {
                case saturation::kParamDrive:
                    params.driveDb = normToRange(value, kDriveMin, kDriveMax);
                    break;
                case saturation::kParamBias:
                    params.bias = normToRange(value, -1.0, 1.0);
                    break;
                case saturation::kParamEvenOdd:
                    params.evenOdd = value;
                    break;
                case saturation::kParamMode:
                    params.mode = std::round(normToRange(value, 0.0, 2.0));
                    break;
                case saturation::kParamTone:
                    params.tone = normToRange(value, -1.0, 1.0);
                    break;
                case saturation::kParamMix:
                    params.mix = value;
                    break;
                case saturation::kParamOutput:
                    params.outputDb = normToRange(value, kOutputMin, kOutputMax);
                    break;
                case saturation::kParamDynamics:
                    params.dynamics = value;
                    break;
                case saturation::kParamTransient:
                    params.transient = value;
                    break;
                default:
                    break;
            }
        }
    }

    syncModel();
}

void AnalogSaturationProcessor::syncModel()
{
    model.setDriveDb(params.driveDb);
    model.setBias(params.bias);
    model.setEvenOdd(params.evenOdd);
    model.setMode(static_cast<int>(params.mode));
    model.setTone(params.tone);
    model.setMix(params.mix);
    model.setOutputDb(params.outputDb);
    model.setDynamics(params.dynamics);
    model.setTransient(params.transient);
}

tresult PLUGIN_API AnalogSaturationProcessor::process(ProcessData& data)
{
    if (data.numInputs == 0 || data.numOutputs == 0)
        return kResultOk;

    handleParameterChanges(data);

    AudioBusBuffers& input = data.inputs[0];
    AudioBusBuffers& output = data.outputs[0];
    output.silenceFlags = 0;

    if (data.symbolicSampleSize == kSample32)
    {
        model.process(input.channelBuffers32, output.channelBuffers32, output.numChannels, data.numSamples);
    }
    else if (data.symbolicSampleSize == kSample64)
    {
        auto** in = input.channelBuffers64;
        auto** out = output.channelBuffers64;
        const int32 channels = std::min<int32>(output.numChannels, 2);
        for (int32 ch = 0; ch < channels; ++ch)
        {
            scratchIn[ch].assign(data.numSamples, 0.0f);
            scratchOut[ch].assign(data.numSamples, 0.0f);
            for (int32 i = 0; i < data.numSamples; ++i)
            {
                scratchIn[ch][i] = static_cast<float>(in[ch][i]);
            }
        }

        float* inputPtrs[2] = {nullptr, nullptr};
        float* outputPtrs[2] = {nullptr, nullptr};
        for (int32 ch = 0; ch < channels; ++ch)
        {
            inputPtrs[ch] = scratchIn[ch].data();
            outputPtrs[ch] = scratchOut[ch].data();
        }

        model.process(inputPtrs, outputPtrs, output.numChannels, data.numSamples);

        for (int32 ch = 0; ch < channels; ++ch)
        {
            for (int32 i = 0; i < data.numSamples; ++i)
            {
                out[ch][i] = static_cast<double>(scratchOut[ch][i]);
            }
        }
    }

    return kResultOk;
}

tresult PLUGIN_API AnalogSaturationProcessor::setState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);
    streamer.readDouble(params.driveDb);
    streamer.readDouble(params.bias);
    streamer.readDouble(params.evenOdd);
    streamer.readDouble(params.mode);
    streamer.readDouble(params.tone);
    streamer.readDouble(params.mix);
    streamer.readDouble(params.outputDb);
    streamer.readDouble(params.dynamics);
    streamer.readDouble(params.transient);

    syncModel();
    return kResultOk;
}

tresult PLUGIN_API AnalogSaturationProcessor::getState(IBStream* state)
{
    IBStreamer streamer(state, kLittleEndian);
    streamer.writeDouble(params.driveDb);
    streamer.writeDouble(params.bias);
    streamer.writeDouble(params.evenOdd);
    streamer.writeDouble(params.mode);
    streamer.writeDouble(params.tone);
    streamer.writeDouble(params.mix);
    streamer.writeDouble(params.outputDb);
    streamer.writeDouble(params.dynamics);
    streamer.writeDouble(params.transient);
    return kResultOk;
}
} // namespace gptaudio
