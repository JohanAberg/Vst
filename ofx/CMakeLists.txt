cmake_minimum_required(VERSION 3.15)
project(IntensityProfilePlotter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# OFX SDK path - use parent's definition if available, otherwise default to local
if(NOT DEFINED OFX_SDK_PATH)
    set(OFX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ofx-sdk" CACHE PATH "Path to OpenFX SDK")
endif()

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
    find_library(OPENGL_FRAMEWORK OpenGL)
elseif(WIN32)
    set(PLATFORM_WINDOWS ON)
elseif(UNIX)
    set(PLATFORM_LINUX ON)
endif()

# OpenCL support (optional GPU acceleration)
set(HAVE_OPENCL OFF)

# Set default OpenCL SDK path for Windows (SYCL SDK location)
if(WIN32 AND NOT DEFINED OpenCL_INCLUDE_DIR)
    set(OPENCL_SDK_PATH "C:/Users/aberg/Documents/OpenCL-Setup/sycl_windows")
    if(EXISTS "${OPENCL_SDK_PATH}/include/CL/cl.h" AND EXISTS "${OPENCL_SDK_PATH}/lib/ur_loader.lib")
        set(OpenCL_INCLUDE_DIR "${OPENCL_SDK_PATH}/include" CACHE PATH "OpenCL include directory")
        set(OpenCL_LIBRARY "${OPENCL_SDK_PATH}/lib/ur_loader.lib" CACHE FILEPATH "OpenCL library")
        message(STATUS "✓ Using local SYCL/OpenCL SDK: ${OPENCL_SDK_PATH}")
    endif()
endif()

# Try to find OpenCL
find_package(OpenCL QUIET)

if(OpenCL_FOUND)
    set(HAVE_OPENCL ON)
    message(STATUS "✓ OpenCL found: ${OpenCL_LIBRARIES}")
    message(STATUS "  Include: ${OpenCL_INCLUDE_DIRS}")
elseif(DEFINED OpenCL_INCLUDE_DIR AND DEFINED OpenCL_LIBRARY)
    # Manual paths provided but find_package failed - set it up manually
    set(HAVE_OPENCL ON)
    set(OpenCL_INCLUDE_DIRS "${OpenCL_INCLUDE_DIR}")
    set(OpenCL_LIBRARIES "${OpenCL_LIBRARY}")
    message(STATUS "✓ OpenCL configured manually: ${OpenCL_LIBRARY}")
else()
    message(STATUS "⊘ OpenCL not found - CPU fallback will be used (still fast for <512 samples)")
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
    ${OFX_SDK_PATH}/Support/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Build OFX Support library
add_subdirectory(${OFX_SDK_PATH}/Support/Library ${CMAKE_BINARY_DIR}/ofx_support EXCLUDE_FROM_ALL)

# Metal-specific setup (macOS)
set(HAVE_METAL OFF)
if(PLATFORM_MACOS)
    # Check if xcrun is available
    find_program(XCRUN_EXECUTABLE xcrun)
    if(XCRUN_EXECUTABLE)
        # Check if Metal toolchain is actually available by testing execution
        execute_process(
            COMMAND ${XCRUN_EXECUTABLE} -sdk macosx metal -version
            OUTPUT_VARIABLE METAL_VERSION_OUTPUT
            ERROR_VARIABLE METAL_VERSION_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE METAL_CHECK_RESULT
        )
        if(METAL_CHECK_RESULT EQUAL 0)
            # Metal toolchain is available - compile Metal shaders
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
            set(HAVE_METAL ON)
            message(STATUS "Metal shader compilation enabled")
        else()
            message(WARNING "Metal toolchain not available - Metal shader compilation will be skipped")
            message(STATUS "To enable Metal support, run: xcodebuild -downloadComponent MetalToolchain")
            set(HAVE_METAL OFF)
        endif()
    else()
        message(WARNING "xcrun not found - Metal shader compilation will be skipped")
        set(HAVE_METAL OFF)
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
    # Set Objective-C++ for files that use Metal/Objective-C
    set_source_files_properties(
        src/GPURenderer.cpp
        PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )
    add_library(IntensityProfilePlotter MODULE ${PLUGIN_SOURCES})
    set_target_properties(IntensityProfilePlotter PROPERTIES
        BUNDLE ON
        BUNDLE_EXTENSION "ofx.bundle"
        PREFIX ""
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
    )
    
    # Post-build: rename the binary inside the bundle to have .ofx extension
    # This is required for Resolve to recognize the plugin
    add_custom_command(TARGET IntensityProfilePlotter POST_BUILD
        COMMAND mv "$<TARGET_BUNDLE_DIR:IntensityProfilePlotter>/Contents/MacOS/IntensityProfilePlotter" 
                   "$<TARGET_BUNDLE_DIR:IntensityProfilePlotter>/Contents/MacOS/IntensityProfilePlotter.ofx"
        COMMENT "Renaming binary to IntensityProfilePlotter.ofx"
    )
    
    # Add Metal library if it will be generated
    if(HAVE_METAL)
        target_sources(IntensityProfilePlotter PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/intensitySampler.metallib)
    endif()
    
    target_link_libraries(IntensityProfilePlotter
        OfxSupport
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
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
    
    target_link_libraries(IntensityProfilePlotter
        OfxSupport
        opengl32
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
    
    target_link_libraries(IntensityProfilePlotter
        OfxSupport
        GL
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

# Benchmark executable (standalone, no OFX dependency)
add_executable(benchmark_harness ${CMAKE_CURRENT_SOURCE_DIR}/benchmark_harness.cpp)
set_target_properties(benchmark_harness PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(benchmark_harness PRIVATE -O2 -Wall -Wextra)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(benchmark_harness PRIVATE /O2 /W4)
endif()
