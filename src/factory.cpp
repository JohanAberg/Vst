#include "AnalogSaturationController.h"
#include "AnalogSaturationProcessor.h"
#include "AnalogSaturationIDs.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF(
    "GPT Audio Labs",
    "https://gpt-audio.example.com",
    "support@gpt-audio.example.com")

DEF_CLASS2(INLINE_UID_FROM_FUID(gptaudio::saturation::kProcessorUID),
    PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    stringPluginName,
    Vst::kDistributable,
    Vst::PlugType::kFx,
    FULL_VERSION_STR,
    kVstVersionString,
    gptaudio::AnalogSaturationProcessor::createInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(gptaudio::saturation::kControllerUID),
    PClassInfo::kManyInstances,
    kVstComponentControllerClass,
    stringPluginName " Controller",
    0,
    "",
    FULL_VERSION_STR,
    kVstVersionString,
    gptaudio::AnalogSaturationController::createInstance)

END_FACTORY
