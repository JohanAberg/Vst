#include "SaturationProcessor.h"

#include <algorithm>

#include "SaturationParameters.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstsinglecomponenteffect.h"

namespace analog::sat {

SaturationProcessor::SaturationProcessor() {
    setControllerClass(kControllerUID);
    for (std::size_t i = 0; i < kParameterCount; ++i) {
        paramValues_[i] = kParameterSpecs[i].defaultNormalized;
    }
}

Steinberg::tresult PLUGIN_API SaturationProcessor::initialize(Steinberg::FUnknown* context) {
    const auto result = AudioEffect::initialize(context);
    if (result != Steinberg::kResultTrue) {
        return result;
    }

    addAudioInput(STR16("Input"), Steinberg::Vst::SpeakerArr::kStereo);
    addAudioOutput(STR16("Output"), Steinberg::Vst::SpeakerArr::kStereo);
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SaturationProcessor::setState(Steinberg::IBStream* state) {
    if (!state) {
        return Steinberg::kResultFalse;
    }
    Steinberg::IBStreamer streamer(state, kLittleEndian);
    for (std::size_t i = 0; i < kParameterCount; ++i) {
        double value = 0.0;
        if (!streamer.readDouble(value)) {
            return Steinberg::kResultFalse;
        }
        paramValues_[i] = value;
    }
    syncParameters();
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SaturationProcessor::getState(Steinberg::IBStream* state) {
    if (!state) {
        return Steinberg::kResultFalse;
    }
    Steinberg::IBStreamer streamer(state, kLittleEndian);
    for (const auto value : paramValues_) {
        if (!streamer.writeDouble(value)) {
            return Steinberg::kResultFalse;
        }
    }
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SaturationProcessor::setupProcessing(Steinberg::Vst::ProcessSetup& setup) {
    sampleRate_ = setup.sampleRate;
    model_.prepare(sampleRate_, static_cast<int>(setup.maxSamplesPerBlock));
    syncParameters();
    return AudioEffect::setupProcessing(setup);
}

void SaturationProcessor::syncParameters() {
    const auto block = computeParameterBlock(paramValues_, sampleRate_);
    model_.setParameters(block);
}

Steinberg::tresult PLUGIN_API SaturationProcessor::process(Steinberg::Vst::ProcessData& data) {
    if (data.symbolicSampleSize != Steinberg::Vst::kSample32) {
        return Steinberg::kResultFalse;
    }

    if (data.inputParameterChanges) {
        const auto paramCount = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < paramCount; ++i) {
            auto* queue = data.inputParameterChanges->getParameterData(i);
            if (!queue) {
                continue;
            }

            double value = 0.0;
            int32 offset = 0;
            if (queue->getPoint(queue->getPointCount() - 1, offset, value) == Steinberg::kResultTrue) {
                setParamNormalized(queue->getParameterId(), value);
            }
        }
    }

    if (data.numSamples == 0 || data.numOutputs == 0) {
        return Steinberg::kResultTrue;
    }

    if (data.numInputs == 0) {
        auto& output = data.outputs[0];
        for (int32 ch = 0; ch < output.numChannels; ++ch) {
            std::fill_n(output.channelBuffers32[ch], data.numSamples, 0.0f);
        }
        return Steinberg::kResultTrue;
    }

    auto& input = data.inputs[0];
    auto& output = data.outputs[0];

    if (output.numChannels < input.numChannels) {
        return Steinberg::kResultFalse;
    }

    model_.process(input.channelBuffers32, output.channelBuffers32, input.numChannels, data.numSamples);

    for (int32 ch = input.numChannels; ch < output.numChannels; ++ch) {
        std::fill_n(output.channelBuffers32[ch], data.numSamples, 0.0f);
    }
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API SaturationProcessor::setParamNormalized(Steinberg::Vst::ParamID tag,
                                                                      double value) {
    std::size_t index = 0;
    if (!findParameterIndex(tag, index)) {
        return Steinberg::kResultFalse;
    }

    const double clamped = std::clamp(value, 0.0, 1.0);
    if (paramValues_[index] != clamped) {
        paramValues_[index] = clamped;
        syncParameters();
    }
    return Steinberg::kResultTrue;
}

}  // namespace analog::sat
