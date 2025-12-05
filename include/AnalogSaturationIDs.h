#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace analog::ids {

inline constexpr Steinberg::FUID kProcessorUID(0x89F9F0CB, 0x216344F2, 0xABFC2F40, 0x7B915B12);
inline constexpr Steinberg::FUID kControllerUID(0x6C7DD1AD, 0xA3DE463C, 0x9A4F8D23, 0xAD7A2FAA);

// Parameter IDs
enum ParamIds : Steinberg::Vst::ParamID {
    kDrive = 0,
    kBias,
    kColor,
    kMix,
    kOutputTrim,
    kDynamics,
    kSlew,
    kQuality,
    kBypass
};

inline constexpr Steinberg::int32 kNumParameters = 8;

} // namespace analog::ids
