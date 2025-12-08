cmake_minimum_required(VERSION 3.22)
project(AnalogCircuitSaturation VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(FetchVST3SDK)

# Set VST3 plugin target path (required by SDK before smtg_add_vst3plugin)
# The SDK checks if the path EXISTS, so we need to create it first
if(NOT DEFINED SMTG_PLUGIN_TARGET_PATH)
    set(SMTG_PLUGIN_TARGET_PATH "${CMAKE_BINARY_DIR}/VST3/Release" CACHE PATH "VST3 plugin installation path" FORCE)
endif()
# Ensure the directory exists (SDK checks for existence)
file(MAKE_DIRECTORY "${SMTG_PLUGIN_TARGET_PATH}")
# Set in parent scope so SDK can see it
set(SMTG_PLUGIN_TARGET_PATH "${SMTG_PLUGIN_TARGET_PATH}" PARENT_SCOPE)

add_library(analog_saturation_core
    src/dsp/SaturationModel.cpp)

target_include_directories(analog_saturation_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_definitions(analog_saturation_core
    PUBLIC ANALOG_SATURATION_VERSION="${PROJECT_VERSION}")

set(PLUGIN_SOURCES
    src/AnalogSaturationProcessor.cpp
    src/AnalogSaturationController.cpp
    src/AnalogSaturationFactory.cpp)

# Add platform-specific main file for macOS bundle entry
if(APPLE)
    list(APPEND PLUGIN_SOURCES ${VST3_SDK_ROOT}/public.sdk/source/main/macmain.cpp)
elseif(WIN32)
    list(APPEND PLUGIN_SOURCES ${VST3_SDK_ROOT}/public.sdk/source/main/dllmain.cpp)
elseif(UNIX)
    list(APPEND PLUGIN_SOURCES ${VST3_SDK_ROOT}/public.sdk/source/main/linuxmain.cpp)
endif()

# Disable plugin link creation to avoid symlink errors
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "Create symbolic link to VST3 plugin" FORCE)

smtg_add_vst3plugin(AnalogCircuitSaturation
    SOURCES_LIST ${PLUGIN_SOURCES})

set_target_properties(AnalogCircuitSaturation
    PROPERTIES
        VST3_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin"
        VST3_COPY_DIR "${CMAKE_BINARY_DIR}/bin")

target_include_directories(AnalogCircuitSaturation
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${VST3_SDK_ROOT})

# Define build configuration macros for VST3 SDK
target_compile_definitions(AnalogCircuitSaturation
    PRIVATE
        $<$<CONFIG:Debug>:_DEBUG=1>
        $<$<CONFIG:Release>:RELEASE=1>
        $<$<CONFIG:RelWithDebInfo>:RELEASE=1>
        $<$<CONFIG:MinSizeRel>:RELEASE=1>)

# Link VST3 SDK libraries
smtg_target_configure_version_file(AnalogCircuitSaturation)

target_link_libraries(AnalogCircuitSaturation
    PRIVATE
        analog_saturation_core
        sdk)
