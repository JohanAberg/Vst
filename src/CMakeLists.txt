include(SMTG_Global)
include(SMTG_AddSubDirectories)
include(SMTG_AddVST3Plugin)

smtg_add_subdirectories("${VST3_SDK_ROOT}")

set(PLUGIN_NAME AnalogSaturation)
set(PLUGIN_BUNDLE_NAME "AnalogSaturation")
set(PLUGIN_COMPANY_NAME "GPT Audio Labs")

smtg_add_vst3plugin(${PLUGIN_NAME}
    BUNDLE_NAME ${PLUGIN_BUNDLE_NAME}
    COMPANY_NAME ${PLUGIN_COMPANY_NAME}
    SOURCES
        AnalogSaturationProcessor.cpp
        AnalogSaturationProcessor.h
        AnalogSaturationController.cpp
        AnalogSaturationController.h
        AnalogSaturationIDs.h
        dsp/AnalogSaturationModel.cpp
        dsp/AnalogSaturationModel.h
        dsp/AdaptiveBiasFilter.h
        dsp/AdaptiveBiasFilter.cpp
        dsp/NonLinearStages.h
        dsp/NonLinearStages.cpp
        dsp/TiltEQ.h
        dsp/TiltEQ.cpp
        dsp/Utilities.h
        dsp/Utilities.cpp
        version.h
        factory.cpp
)

target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        sdk
)

target_include_directories(${PLUGIN_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/dsp
        ${VST3_SDK_ROOT}
)

set_target_properties(${PLUGIN_NAME}
    PROPERTIES
        OUTPUT_NAME ${PLUGIN_BUNDLE_NAME}
)
