#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace gptaudio
{
namespace saturation
{
    using Steinberg::FUID;
    using Steinberg::Vst::ParamID;

    // Processor / controller unique identifiers
    static constexpr FUID kProcessorUID {0x2E7A9028, 0x30BF4B5C, 0xA508CB54, 0x1789A9CD};
    static constexpr FUID kControllerUID {0x7E2544D1, 0x3F784AC8, 0xB4E0A037, 0x79CA4B51};

    enum ParameterID : ParamID
    {
        kParamDrive = 100,
        kParamBias = 101,
        kParamEvenOdd = 102,
        kParamMode = 103,
        kParamTone = 104,
        kParamMix = 105,
        kParamOutput = 106,
        kParamDynamics = 107,
        kParamTransient = 108
    };
} // namespace saturation
} // namespace gptaudio
