cmake_minimum_required(VERSION 3.15)
project(IntensityProfilePlotter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OFX SDK path - adjust as needed
set(OFX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ofx-sdk" CACHE PATH "Path to OpenFX SDK")

# Verify OFX SDK path exists
if(NOT EXISTS "${OFX_SDK_PATH}")
    message(FATAL_ERROR "OFX SDK path does not exist: ${OFX_SDK_PATH}")
endif()

if(NOT EXISTS "${OFX_SDK_PATH}/include")
    message(FATAL_ERROR "OFX SDK include directory not found at: ${OFX_SDK_PATH}/include")
endif()

# Platform detection
if(APPLE)
    set(PLATFORM_MACOS ON)
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    find_library(FOUNDATION_FRAMEWORK Foundation)
elseif(WIN32)
    set(PLATFORM_WINDOWS ON)
elseif(UNIX)
    set(PLATFORM_LINUX ON)
endif()

# OpenCL
find_package(OpenCL QUIET)
if(OpenCL_FOUND)
    set(HAVE_OPENCL ON)
    message(STATUS "OpenCL found: ${OpenCL_LIBRARIES}")
else()
    message(WARNING "OpenCL not found - OpenCL support will be disabled")
    set(HAVE_OPENCL OFF)
endif()

# Source files
set(PLUGIN_SOURCES
    src/IntensityProfilePlotterPlugin.cpp
    src/IntensityProfilePlotterInteract.cpp
    src/IntensitySampler.cpp
    src/ProfilePlotter.cpp
    src/GPURenderer.cpp
    src/CPURenderer.cpp
)

# GPU kernel sources
set(METAL_KERNELS
    kernels/intensitySampler.metal
)

set(OPENCL_KERNELS
    kernels/intensitySampler.cl
)

# Include directories
include_directories(
    ${OFX_SDK_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Metal-specific setup (macOS)
if(PLATFORM_MACOS)
    # Check if xcrun is available
    find_program(XCRUN_EXECUTABLE xcrun)
    if(XCRUN_EXECUTABLE)
        # Compile Metal shaders
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/intensitySampler.metallib
            COMMAND ${XCRUN_EXECUTABLE} -sdk macosx metal -c ${CMAKE_CURRENT_SOURCE_DIR}/kernels/intensitySampler.metal -o ${CMAKE_CURRENT_BINARY_DIR}/intensitySampler.air
            COMMAND ${XCRUN_EXECUTABLE} -sdk macosx metallib ${CMAKE_CURRENT_BINARY_DIR}/intensitySampler.air -o ${CMAKE_CURRENT_BINARY_DIR}/intensitySampler.metallib
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kernels/intensitySampler.metal
            COMMENT "Compiling Metal shaders"
            VERBATIM
        )
        
        # Embed Metal library
        set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/intensitySampler.metallib PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
    else()
        message(WARNING "xcrun not found - Metal shader compilation will be skipped")
    endif()
endif()

# OpenCL kernel embedding
if(HAVE_OPENCL)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/kernels/intensitySampler.cl
        ${CMAKE_CURRENT_BINARY_DIR}/intensitySampler.cl
        COPYONLY
    )
endif()

# Create plugin bundle/library
if(PLATFORM_MACOS)
    add_library(IntensityProfilePlotter MODULE ${PLUGIN_SOURCES})
    set_target_properties(IntensityProfilePlotter PROPERTIES
        BUNDLE ON
        BUNDLE_EXTENSION "ofx"
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    )
    
    # Add Metal library if it will be generated
    if(XCRUN_EXECUTABLE)
        target_sources(IntensityProfilePlotter PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/intensitySampler.metallib)
    endif()
    
    target_link_libraries(IntensityProfilePlotter
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
    
    if(HAVE_OPENCL)
        target_link_libraries(IntensityProfilePlotter ${OpenCL_LIBRARIES})
        target_include_directories(IntensityProfilePlotter PRIVATE ${OpenCL_INCLUDE_DIRS})
    endif()
    
elseif(PLATFORM_WINDOWS)
    add_library(IntensityProfilePlotter MODULE ${PLUGIN_SOURCES})
    set_target_properties(IntensityProfilePlotter PROPERTIES
        PREFIX ""
        SUFFIX ".ofx"
    )
    
    if(HAVE_OPENCL)
        target_link_libraries(IntensityProfilePlotter ${OpenCL_LIBRARIES})
        target_include_directories(IntensityProfilePlotter PRIVATE ${OpenCL_INCLUDE_DIRS})
    endif()
    
elseif(PLATFORM_LINUX)
    add_library(IntensityProfilePlotter MODULE ${PLUGIN_SOURCES})
    set_target_properties(IntensityProfilePlotter PROPERTIES
        PREFIX ""
        SUFFIX ".ofx"
    )
    
    if(HAVE_OPENCL)
        target_link_libraries(IntensityProfilePlotter ${OpenCL_LIBRARIES})
        target_include_directories(IntensityProfilePlotter PRIVATE ${OpenCL_INCLUDE_DIRS})
    endif()
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(IntensityProfilePlotter PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
        -fPIC
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(IntensityProfilePlotter PRIVATE
        /W4
        /wd4100  # Unused parameter warning
    )
endif()

# Installation
install(TARGETS IntensityProfilePlotter
    LIBRARY DESTINATION lib
    BUNDLE DESTINATION .
)

# Copy OpenCL kernel to build directory for runtime access
if(HAVE_OPENCL)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/kernels/intensitySampler.cl
        DESTINATION .
    )
endif()
