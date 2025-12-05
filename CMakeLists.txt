cmake_minimum_required(VERSION 3.20)
project(AnalogCircuitSaturation VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ANALOG_SAT_BUILD_TESTS "Build unit tests" OFF)

if(NOT DEFINED VST3_SDK_ROOT)
    if(DEFINED ENV{VST3_SDK_ROOT})
        set(VST3_SDK_ROOT $ENV{VST3_SDK_ROOT})
    endif()
endif()

if(NOT VST3_SDK_ROOT)
    message(FATAL_ERROR "VST3_SDK_ROOT is not set. Point it to the VST3 SDK checkout.")
endif()

if(NOT EXISTS "${VST3_SDK_ROOT}/pluginterfaces")
    message(FATAL_ERROR "${VST3_SDK_ROOT} does not look like a VST3 SDK checkout.")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

file(GLOB ANALOG_SATURATION_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(AnalogCircuitSaturation MODULE ${ANALOG_SATURATION_SOURCES}
    ${VST3_SDK_ROOT}/public.sdk/source/main/pluginfactory.cpp
)

if(WIN32)
    target_sources(AnalogCircuitSaturation PRIVATE ${VST3_SDK_ROOT}/public.sdk/source/main/dllmain.cpp)
endif()

if(APPLE)
    set_target_properties(AnalogCircuitSaturation PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION "vst3"
        MACOSX_BUNDLE_GUI_IDENTIFIER "audio.lab.analogcircuitsaturation"
        MACOSX_BUNDLE_BUNDLE_NAME "AnalogCircuitSaturation"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
    )
endif()

if(MSVC)
    target_compile_options(AnalogCircuitSaturation PRIVATE /W4 /WX)
else()
    target_compile_options(AnalogCircuitSaturation PRIVATE -Wall -Wextra -Wpedantic)
endif()

set(VST3_INCLUDE_DIRS
    ${VST3_SDK_ROOT}
    ${VST3_SDK_ROOT}/pluginterfaces
    ${VST3_SDK_ROOT}/base
    ${VST3_SDK_ROOT}/public.sdk/source
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.json.in
               ${CMAKE_CURRENT_BINARY_DIR}/generated/Info.json
               @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/PluginVersion.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/generated/PluginVersion.h
               @ONLY)

target_include_directories(AnalogCircuitSaturation
    PRIVATE
        include
        ${VST3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_compile_definitions(AnalogCircuitSaturation PRIVATE NOMINMAX=1)

set_target_properties(AnalogCircuitSaturation PROPERTIES
    OUTPUT_NAME "AnalogCircuitSaturation"
)

include(Vst3Packaging)
vst3_configure_target(AnalogCircuitSaturation)

if(ANALOG_SAT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
