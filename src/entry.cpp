#include "PluginVersion.h"
#include "SaturationController.h"
#include "SaturationProcessor.h"
#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF(ANALOG_SATURATION_VENDOR,
                  ANALOG_SATURATION_URL,
                  ANALOG_SATURATION_EMAIL)

DEF_CLASS2(INLINE_UID_FROM_FUID(analog::sat::kProcessorUID),
           PClassInfo::kManyInstances,
           kVstAudioEffectClass,
           "AnalogCircuitSaturation",
           Vst::kDistributable,
           Vst::PlugType::kFxDistortion,
           ANALOG_SATURATION_VERSION_STR,
           kVstVersionString,
           analog::sat::SaturationProcessor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(analog::sat::kControllerUID),
           PClassInfo::kManyInstances,
           kVstComponentControllerClass,
           "AnalogCircuitSaturationController",
           0,
           "",
           ANALOG_SATURATION_VERSION_STR,
           kVstVersionString,
           analog::sat::SaturationController::createInstance)

END_FACTORY
