cmake_minimum_required(VERSION 3.22)
project(PluginCollection VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set VST3 plugin target path early (required by VST3 SDK)
# The SDK uses SMTG_PLUGIN_TARGET_USER_PATH if set, otherwise defaults
set(SMTG_PLUGIN_TARGET_USER_PATH "${CMAKE_BINARY_DIR}/VST3/Release" CACHE PATH "VST3 plugin installation path")
# Ensure the directory exists (SDK checks for existence)
file(MAKE_DIRECTORY "${SMTG_PLUGIN_TARGET_USER_PATH}")
# Disable plugin link creation (causes issues when directory is not empty)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "Create symbolic link to VST3 plugin")

# Build options for each plugin
option(BUILD_OFX_PLUGIN "Build the Intensity Profile Plotter OFX plugin" ON)
option(BUILD_VST3_CIRCUIT_SATURATION "Build the Analog Circuit Saturation VST3 plugin" ON)
option(BUILD_VST3_ANALOG_SATURATION "Build the Analog Saturation VST3 plugin (JUCE-based)" ON)

# OFX SDK path (can be set from command line: -DOFX_SDK_PATH=/path/to/ofx-sdk)
set(OFX_SDK_PATH "/Users/johan/openfx" CACHE PATH "Path to OpenFX SDK")

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

# VST3 Plugin 1: Analog Circuit Saturation (VST3 SDK)
if(BUILD_VST3_CIRCUIT_SATURATION)
    message(STATUS "Building VST3 plugin: Analog Circuit Saturation")
    # Ensure VST3 plugin target path directory exists
    file(MAKE_DIRECTORY "${SMTG_PLUGIN_TARGET_PATH}")
    # Fetch VST3 SDK once at root level
    include(${CMAKE_CURRENT_SOURCE_DIR}/vst_codex/cmake/FetchVST3SDK.cmake)
    # Add VST3 SDK as subdirectory to build SDK libraries
    if(NOT TARGET base)
        add_subdirectory(${VST3_SDK_ROOT} ${CMAKE_BINARY_DIR}/vst3sdk EXCLUDE_FROM_ALL)
    endif()
    add_subdirectory(vst_codex)
endif()

# VST3 Plugin 2: Analog Saturation (JUCE-based)
if(BUILD_VST3_ANALOG_SATURATION)
    message(STATUS "Building VST3 plugin: Analog Saturation (JUCE-based)")
    # Check if JUCE exists
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE")
        message(WARNING "JUCE not found. Run ./setup_juce.sh to download JUCE framework.")
        message(STATUS "Skipping JUCE-based VST3 plugin build.")
        set(BUILD_VST3_ANALOG_SATURATION OFF)
    else()
        # Create a separate CMakeLists for the JUCE-based plugin
        add_subdirectory(vst_juce)
    endif()
endif()

