cmake_minimum_required(VERSION 3.22)

project(AnalogSaturation VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE from parent directory
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../JUCE ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# Plugin target
juce_add_plugin(AnalogSaturation
    COMPANY_NAME "AnalogCircuit"
    PLUGIN_MANUFACTURER_CODE Acct
    PLUGIN_CODE Asat
    FORMATS VST3 Standalone
    PRODUCT_NAME "Analog Saturation"
    DESCRIPTION "Advanced analog circuit saturation modeling"
    VERSION 1.0.0
)

target_sources(AnalogSaturation PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginProcessor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginProcessor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/PluginEditor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/SaturationEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/SaturationEngine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/WaveDigitalFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/WaveDigitalFilter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/NonlinearStateSpace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/NonlinearStateSpace.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/CircuitModels.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../Source/CircuitModels.h
)

target_compile_definitions(AnalogSaturation
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AnalogSaturation
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_plugin_client
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

