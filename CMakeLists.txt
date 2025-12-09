cmake_minimum_required(VERSION 3.22)
project(PluginCollection VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define DEVELOPMENT for VST3 SDK for all configurations
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEVELOPMENT)
else()
    add_compile_definitions(RELEASE)
endif()

# Suppress warnings-as-errors for external dependencies
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=unused-but-set-variable")
elseif(MSVC)
    # MSVC: disable specific warnings or use /WX- to turn off warnings-as-errors
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4101")
endif()

# Build options for each plugin
option(BUILD_OFX_PLUGIN "Build the Intensity Profile Plotter OFX plugin" ON)
# option(BUILD_OFX_MINIMAL "Build the minimal test OFX plugin" ON)
# option(BUILD_VST3_CIRCUIT_SATURATION "Build the Analog Circuit Saturation VST3 plugin" ON)
# option(BUILD_VST3_ANALOG_SATURATION "Build the Analog Saturation VST3 plugin (JUCE-based)" ON)

# OFX SDK path (can be set from command line: -DOFX_SDK_PATH=/path/to/ofx-sdk)
if(WIN32)
    set(OFX_SDK_PATH "C:/Users/aberg/Documents/openfx" CACHE PATH "Path to OpenFX SDK")
elseif(APPLE)
    set(OFX_SDK_PATH "/Users/johan/openfx" CACHE PATH "Path to OpenFX SDK")
else()
    set(OFX_SDK_PATH "/home/user/openfx" CACHE PATH "Path to OpenFX SDK")
endif()

# OFX Plugin
if(BUILD_OFX_PLUGIN)
    # Check if OFX SDK exists
    if(NOT EXISTS "${OFX_SDK_PATH}")
        message(WARNING "OFX SDK not found at: ${OFX_SDK_PATH}")
        message(STATUS "Skipping OFX plugin build. Set -DOFX_SDK_PATH=/path/to/ofx-sdk to build it.")
        set(BUILD_OFX_PLUGIN OFF)
    elseif(NOT EXISTS "${OFX_SDK_PATH}/include")
        message(WARNING "OFX SDK include directory not found at: ${OFX_SDK_PATH}/include")
        message(STATUS "Skipping OFX plugin build.")
        set(BUILD_OFX_PLUGIN OFF)
    else()
        message(STATUS "Building OFX plugin: Intensity Profile Plotter")
        message(STATUS "OFX SDK path: ${OFX_SDK_PATH}")
        add_subdirectory(ofx)
    endif()
endif()

# # VST3 Plugin 1: Analog Circuit Saturation (VST3 SDK)
# if(BUILD_VST3_CIRCUIT_SATURATION)
#     message(STATUS "Building VST3 plugin: Analog Circuit Saturation")
#     add_subdirectory(vst_codex)
# endif()

# # VST3 Plugin 2: Analog Saturation (JUCE-based)
# if(BUILD_VST3_ANALOG_SATURATION)
#     message(STATUS "Building VST3 plugin: Analog Saturation (JUCE-based)")
#     # Check if JUCE exists
#     if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
#         message(WARNING "JUCE not found. Run ./setup_juce.sh to download JUCE framework.")
#         message(STATUS "Skipping JUCE-based VST3 plugin build.")
#         set(BUILD_VST3_ANALOG_SATURATION OFF)
#     else()
#         # Create a separate CMakeLists for the JUCE-based plugin
#         add_subdirectory(vst_juce)
#     endif()
# endif()
