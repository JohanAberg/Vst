cmake_minimum_required(VERSION 3.22)

project(AnalogSaturation VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(JUCE)

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
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/SaturationEngine.cpp
    Source/SaturationEngine.h
    Source/WaveDigitalFilter.cpp
    Source/WaveDigitalFilter.h
    Source/NonlinearStateSpace.cpp
    Source/NonlinearStateSpace.h
    Source/CircuitModels.cpp
    Source/CircuitModels.h
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
