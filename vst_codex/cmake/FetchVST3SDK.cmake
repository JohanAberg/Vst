include(FetchContent)

if(NOT DEFINED VST3_SDK_ROOT OR VST3_SDK_ROOT STREQUAL "")
    if(DEFINED ENV{VST3_SDK_ROOT})
        set(VST3_SDK_ROOT $ENV{VST3_SDK_ROOT})
    endif()
endif()

if(NOT DEFINED VST3_SDK_ROOT OR VST3_SDK_ROOT STREQUAL "")
    set(VST3_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../third_party/vst3sdk")
endif()

if(NOT EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG v3.7.11_build_11
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(vst3sdk)
    set(VST3_SDK_ROOT ${vst3sdk_SOURCE_DIR} CACHE PATH "Path to the VST3 SDK" FORCE)
else()
    set(VST3_SDK_ROOT ${VST3_SDK_ROOT} CACHE PATH "Path to the VST3 SDK" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${VST3_SDK_ROOT}/cmake/modules")
include(SMTG_VST3_SDK)
include(SMTG_AddVST3Library)
