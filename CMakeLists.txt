cmake_minimum_required(VERSION 3.24)
project(AnalogCircuitSaturation VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchVST3SDK)

add_library(analog_saturation_core
    src/dsp/SaturationModel.cpp)

target_include_directories(analog_saturation_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include)

target_compile_definitions(analog_saturation_core
    PUBLIC ANALOG_SATURATION_VERSION="${PROJECT_VERSION}")

set(PLUGIN_SOURCES
    src/AnalogSaturationProcessor.cpp
    src/AnalogSaturationController.cpp
    src/AnalogSaturationFactory.cpp)

smtg_add_vst3plugin(AnalogCircuitSaturation
    SOURCES
        ${PLUGIN_SOURCES})

set_target_properties(AnalogCircuitSaturation
    PROPERTIES
        VST3_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin"
        VST3_COPY_DIR "${CMAKE_BINARY_DIR}/bin")

target_link_libraries(AnalogCircuitSaturation
    PRIVATE
        analog_saturation_core)
