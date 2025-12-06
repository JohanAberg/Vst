include(FetchContent)

# Check if VST3_SDK_ROOT is provided via environment or cache
if(NOT DEFINED VST3_SDK_ROOT OR VST3_SDK_ROOT STREQUAL "")
    if(DEFINED ENV{VST3_SDK_ROOT})
        set(VST3_SDK_ROOT $ENV{VST3_SDK_ROOT})
    endif()
endif()

# If no SDK root is provided, fetch it
if(NOT DEFINED VST3_SDK_ROOT OR VST3_SDK_ROOT STREQUAL "" OR NOT EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG v3.7.9_build_61
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(vst3sdk)
    # After FetchContent_MakeAvailable, vst3sdk_SOURCE_DIR is available
    if(DEFINED vst3sdk_SOURCE_DIR AND EXISTS "${vst3sdk_SOURCE_DIR}/CMakeLists.txt")
        set(VST3_SDK_ROOT ${vst3sdk_SOURCE_DIR} CACHE PATH "Path to the VST3 SDK" FORCE)
    else()
        message(FATAL_ERROR "Failed to fetch VST3 SDK. vst3sdk_SOURCE_DIR is not defined or invalid.")
    endif()
endif()

# Ensure VST3_SDK_ROOT is set and exists
if(NOT VST3_SDK_ROOT)
    message(FATAL_ERROR "VST3_SDK_ROOT is not set. Please set it or ensure FetchContent works.")
endif()

if(NOT EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "VST3 SDK not found at ${VST3_SDK_ROOT}")
endif()

# Add the VST3 SDK cmake modules to the module path
list(APPEND CMAKE_MODULE_PATH "${VST3_SDK_ROOT}/cmake/modules")

# Include required VST3 SDK modules
if(EXISTS "${VST3_SDK_ROOT}/cmake/modules/SMTG_VST3_SDK.cmake")
    include(SMTG_VST3_SDK)
    include(SMTG_AddVST3Library)
else()
    message(FATAL_ERROR "VST3 SDK cmake modules not found at ${VST3_SDK_ROOT}/cmake/modules")
endif()
