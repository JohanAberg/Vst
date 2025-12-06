#include "AnalogSaturationController.h"
#include "AnalogSaturationIDs.h"
#include "AnalogSaturationProcessor.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringCompanyName "Analog Research Lab"
#define stringCompanyWeb "https://example.com"
#define stringCompanyEmail "contact@example.com"
#define stringSubcategory Steinberg::Vst::PlugType::kFx

BEGIN_FACTORY_DEF(stringCompanyName,
                  stringCompanyWeb,
                  stringCompanyEmail)

    DEF_CLASS2(INLINE_UID_FROM_FUID(analog::ids::kProcessorUID),
               Steinberg::PClassInfo::kManyInstances,
               kVstAudioEffectClass,
               "Analog Circuit Saturation",
               Steinberg::Vst::kDistributable,
               stringSubcategory,
               "1.0.0",
               kVstVersionString,
               analog::AnalogSaturationProcessor::createInstance)

    DEF_CLASS2(INLINE_UID_FROM_FUID(analog::ids::kControllerUID),
               Steinberg::PClassInfo::kManyInstances,
               kVstComponentControllerClass,
               "Analog Circuit Saturation Controller",
               0,
               "",
               "1.0.0",
               kVstVersionString,
               analog::AnalogSaturationController::createInstance)

END_FACTORY
