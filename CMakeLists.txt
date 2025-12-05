cmake_minimum_required(VERSION 3.24)

project(AnalogCircuitSaturation VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VST3_SDK_ROOT "" CACHE PATH "Absolute path to the Steinberg VST3 SDK checkout")

if(NOT VST3_SDK_ROOT)
    message(FATAL_ERROR "Set VST3_SDK_ROOT to a local VST3 SDK clone before configuring.")
endif()

if(NOT EXISTS "${VST3_SDK_ROOT}/public.sdk/source/main/pluginfactory.cpp")
    message(FATAL_ERROR "The provided VST3_SDK_ROOT does not look like a valid VST3 SDK checkout.")
endif()

list(APPEND CMAKE_MODULE_PATH "${VST3_SDK_ROOT}/cmake/modules")

include(GNUInstallDirs)

add_subdirectory(src)
