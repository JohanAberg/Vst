cmake_minimum_required(VERSION 3.15)
project(IntensityProfilePlotterRaw VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OFX SDK path
if(NOT DEFINED OFX_SDK_PATH)
    set(OFX_SDK_PATH "C:/Users/aberg/Documents/openfx" CACHE PATH "Path to OpenFX SDK")
endif()

# Verify OFX SDK path exists
if(NOT EXISTS "${OFX_SDK_PATH}")
    message(FATAL_ERROR "OFX SDK path does not exist: ${OFX_SDK_PATH}")
endif()

if(NOT EXISTS "${OFX_SDK_PATH}/include")
    message(FATAL_ERROR "OFX SDK include directory not found at: ${OFX_SDK_PATH}/include")
endif()

# Platform detection
if(WIN32)
    set(PLATFORM_WINDOWS ON)
endif()

# Source files - we'll need to adapt IntensitySampler and ProfilePlotter
# For now, just the main plugin file
set(PLUGIN_SOURCES
    src/IntensityProfilePlotterRaw.cpp
    # We'll add adapted versions of IntensitySampler and CPURenderer later
    # src/IntensitySamplerRaw.cpp
    # src/CPURendererRaw.cpp
)

# Include directories
include_directories(
    ${OFX_SDK_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # Include original headers for now - will need to adapt them
    ${CMAKE_CURRENT_SOURCE_DIR}/../ofx/include
)

# Create plugin library
if(PLATFORM_WINDOWS)
    add_library(IntensityProfilePlotterRaw MODULE ${PLUGIN_SOURCES})
    set_target_properties(IntensityProfilePlotterRaw PROPERTIES
        PREFIX ""
        SUFFIX ".ofx"
    )
    # Set runtime library to dynamic (/MD) for compatibility with DaVinci Resolve
    set_property(TARGET IntensityProfilePlotterRaw PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    # Link OpenGL for on-screen controls
    target_link_libraries(IntensityProfilePlotterRaw opengl32)
endif()

