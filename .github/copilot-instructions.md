# Cross-Platform OFX Plugin Development Agent

## Agent Purpose & Capabilities
I am a specialized C++ development agent for creating OpenFX (OFX) video plugins targeting DaVinci Resolve, Nuke, and similar hosts across **Windows 11, macOS, and Linux**. I focus on GPU-accelerated image processing with multi-vendor GPU support:
- **NVIDIA GPUs**: OpenCL on all platforms
- **AMD GPUs**: OpenCL on Windows/Linux, Metal on macOS
- **Apple Silicon (M1/M2/M3)**: Metal on macOS (native performance)
- **Intel GPUs**: OpenCL fallback on all platforms

### When to Invoke This Agent
- Writing cross-platform OFX plugin C++ code (Windows/macOS/Linux)
- Implementing GPU kernels for NVIDIA, AMD, or Apple Silicon
- Designing custom OpenGL overlay interactions in host viewport
- Debugging CMake cross-platform build issues
- Troubleshooting plugin loading on different OS/GPU combinations
- Optimizing GPU rendering for specific hardware (NVIDIA RTX, AMD RX, Apple M-series)
- Setting up CI/CD for multi-platform builds

### Agent Boundaries
✅ **I WILL**: Write platform-conditional code, implement OpenCL/Metal kernels, configure CMake for cross-compilation, debug platform-specific issues, optimize for specific GPUs, handle endianness differences

❌ **I WON'T**: Write CUDA code (OpenCL is priority), support DirectX compute, implement Vulkan backends, debug vendor driver issues, provide distro-specific Linux packaging

## Project Overview
C++ OpenFX (OFX) video plugin collection targeting DaVinci Resolve, Nuke, and similar hosts. Primary focus: GPU-accelerated image processing with Metal (macOS) and OpenCL (Windows/Linux) backends across multiple platforms and GPU vendors.

OpenFX developer documentation: https://openfx.readthedocs.io/ and "C:\ProgramData\Blackmagic Design\DaVinci Resolve\Support\Developer\OpenFX"


## Architecture

### Component Hierarchy
```
PluginCollection (root)
├── ofx/                          # OFX plugin implementations
│   ├── src/                      # C++ plugin logic
│   │   ├── *Plugin.cpp          # OFX entry points & parameter definitions
│   │   ├── *Interact.cpp        # Custom overlay UI in host viewport
│   │   ├── *Sampler.cpp         # Image sampling logic
│   │   ├── GPURenderer.cpp      # Metal/OpenCL dispatcher
│   │   └── CPURenderer.cpp      # Fallback implementation
│   ├── kernels/                 # GPU compute kernels
│   │   ├── *.metal              # Metal shaders (macOS)
│   │   └── *.cl                 # OpenCL kernels (cross-platform)
│   └── include/                 # Plugin headers
└── vst_*/                       # Future: VST3 plugins (commented out)
```

### Data Flow
1. **Host → Plugin**: Image buffers + parameter values via OFX API
2. **Plugin Logic**: Determine GPU backend availability (Metal > OpenCL > CPU)
3. **GPU Kernel**: Process image data in parallel
4. **Plugin → Host**: Modified image buffer returned

### Critical Design Decisions
- **Multi-backend strategy**: Single codebase supports Metal, OpenCL, and CPU - runtime detection ensures graceful degradation
- **OFX over VST3**: OFX provides better cross-host compatibility (Resolve, Nuke, Flame) vs VST3 (audio-focused)
- **CMake over Xcode/VS projects**: Enables cross-platform builds from single configuration

## Build System

### CMake Configuration
```bash
# Standard build flow
cd c:\Users\aberg\Documents\Vst
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Windows-specific: Point to OpenCL SDK if not system-installed
cmake .. -DOpenCL_INCLUDE_DIR="C:/Users/aberg/Documents/OpenCL-SDK/include" \
         -DOpenCL_LIBRARY="C:/Users/aberg/Documents/OpenCL-SDK/lib/OpenCL.lib"
```

### CMake Conventions
- **Comments**: Use `#` not `//` (common mistake - CMake != C++)
- **Platform detection**: `if(WIN32)`, `if(APPLE)`, `if(UNIX)` - set `PLATFORM_*` flags
- **Optional dependencies**: Check existence before `add_subdirectory()` - emit warnings, don't fail
- **SDK paths**: Use `CACHE PATH` for user-overridable locations

### Build Targets
- `IntensityProfilePlotter`: Main OFX plugin (`.ofx.bundle` on macOS, `.ofx` on Windows/Linux)
- `OfxSupport`: OFX SDK support library (from `${OFX_SDK_PATH}/Support/Library`)

### Platform-Specific Output
**macOS**: 
- Bundle structure: `IntensityProfilePlotter.ofx.bundle/Contents/MacOS/IntensityProfilePlotter.ofx`
- Post-build renames binary to `.ofx` extension (DaVinci Resolve requirement)
- Metal shaders compiled to `.metallib` and embedded in bundle Resources

**Windows/Linux**:
- Single file: `IntensityProfilePlotter.ofx`
- OpenCL kernels copied alongside binary for runtime loading

## GPU Backend Development

### Backend Selection Logic
```cpp
// Runtime priority chain (implement in GPURenderer.cpp)
if (macOS && Metal available) {
    if (Apple Silicon) → use Metal (best performance)
    else if (AMD GPU) → use Metal or OpenCL
    else → OpenCL (NVIDIA on Intel Mac)
}
else if (OpenCL available) {
    // Auto-detect: NVIDIA > AMD > Intel
    if (NVIDIA GPU detected) → use OpenCL (CUDA-accelerated)
    else if (AMD GPU detected) → use OpenCL (ROCm/APP)
    else if (Intel GPU detected) → use OpenCL (Neo)
}
else → fallback to CPU (SIMD if available)
```

### OpenCL Kernel Development
**File**: `ofx/kernels/intensitySampler.cl`

```opencl
__kernel void sample_intensity(
    __global const float4* input,
    __global float4* output,
    const int width,
    const int height
) {
    int x = get_global_id(0);
    int y = get_global_id(1);
    // ... processing logic
}
```

**Host code pattern** (in `GPURenderer.cpp`):
```cpp
cl_kernel kernel = clCreateKernel(program, "sample_intensity", &err);
clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
size_t globalWorkSize[2] = {width, height};
clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
```

### Metal Shader Development
**File**: `ofx/kernels/intensitySampler.metal`

```metal
kernel void sample_intensity(
    texture2d<float, access::read> input [[texture(0)]],
    texture2d<float, access::write> output [[texture(1)]],
    uint2 gid [[thread_position_in_grid]]
) {
    // ... processing logic
}
```

**Build integration**: Shaders compile at CMake build time via `xcrun metal`

## OFX Plugin Development

### Plugin Structure Pattern
**Entry point** (`*Plugin.cpp`):
```cpp
class IntensityProfilePlotterPlugin : public OFX::ImageEffect {
    // Define parameters in constructor
    IntensityProfilePlotterPlugin(OfxImageEffectHandle handle);
    
    // Main processing entry point
    virtual void render(const OFX::RenderArguments &args) override;
};

// Factory functions for OFX host
OfxExport int OfxGetNumberOfPlugins(void) { return 1; }
OfxExport OfxPlugin* OfxGetPlugin(int nth) { /* ... */ }
```

### Parameter Definition
```cpp
// In plugin constructor
OFX::IntParam* threshold = fetchIntParam("threshold");
OFX::RGBAParam* color = fetchRGBAParam("plotColor");
```

### Custom Overlay UI
**File**: `*Interact.cpp` - implements `OFX::OverlayInteract`
- Draw directly in host viewport using OpenGL
- Handle mouse events for parameter control
- Useful for visual tools (scopes, masks, tracking)

### Image Processing Pattern
```cpp
void render(const RenderArguments &args) {
    // 1. Fetch input/output clips
    auto src = _srcClip->fetchImage(args.time);
    auto dst = _dstClip->fetchImage(args.time);
    
    // 2. Get image properties
    OfxRectI renderWindow = args.renderWindow;
    int width = renderWindow.x2 - renderWindow.x1;
    
    // 3. Process via GPU or CPU
    if (gpuRenderer->isAvailable()) {
        gpuRenderer->process(src, dst, renderWindow);
    } else {
        cpuRenderer->process(src, dst, renderWindow);
    }
}
```

## Dependencies Management

### Required SDKs
| SDK | Purpose | Windows Path | macOS Path | Linux Path |
|-----|---------|--------------|------------|------------|
| OpenFX | Plugin API | `C:/Users/aberg/Documents/openfx` | `/Users/johan/openfx` | `~/openfx` |
| OpenCL | GPU compute | `C:/Users/aberg/Documents/OpenCL-SDK` | System (via Xcode) | `/usr/include/CL` |

### Platform-Specific SDK Paths

#### Windows 11
```cmake
set(OFX_SDK_PATH "C:/Users/aberg/Documents/openfx")
set(OpenCL_INCLUDE_DIR "C:/Users/aberg/Documents/OpenCL-SDK/include")
set(OpenCL_LIBRARY "C:/Users/aberg/Documents/OpenCL-SDK/lib/OpenCL.lib")
```

#### macOS (Intel + Apple Silicon)
```cmake
set(OFX_SDK_PATH "/Users/johan/openfx")
# Metal framework auto-detected by CMake
# OpenCL deprecated but available at /System/Library/Frameworks/OpenCL.framework
```

#### Linux (Ubuntu/Debian)
```cmake
set(OFX_SDK_PATH "/home/user/openfx")
# OpenCL via package manager:
# apt install ocl-icd-opencl-dev  # Headers
# NVIDIA: apt install nvidia-opencl-dev
# AMD: Install ROCm from https://rocm.docs.amd.com/
```

### OpenCL SDK Setup (Windows)
Download options:
1. **Intel OpenCL SDK**: https://github.com/intel/llvm/releases (recommended)
2. **AMD APP SDK**: https://github.com/GPUOpen-LibrariesAndSDKs/OCL-SDK
3. **Khronos Headers + ICD**: https://github.com/KhronosGroup/OpenCL-Headers

Extract structure:
```
C:/Users/aberg/Documents/OpenCL-SDK/
├── include/
│   └── CL/
│       ├── cl.h
│       └── opencl.h
└── lib/
    └── OpenCL.lib
```

### OFX SDK Setup
```bash
# Clone official SDK
git clone https://github.com/AcademySoftwareFoundation/openfx.git
# Windows: C:/Users/aberg/Documents/openfx
# macOS: /Users/johan/openfx
```

## Debugging Workflows

### DaVinci Resolve Plugin Debugging

**Installation paths**:
- **Windows**: `C:\Program Files\Common Files\OFX\Plugins\`
- **macOS**: `/Library/OFX/Plugins/`
- **Linux**: `/usr/OFX/Plugins/`

**Debug build workflow**:
```bash
# 1. Build with debug symbols
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug

# 2. Copy plugin to Resolve's plugin directory
# Windows:
copy build\ofx\IntensityProfilePlotter.ofx "C:\Program Files\Common Files\OFX\Plugins\"

# macOS:
cp -r build/ofx/IntensityProfilePlotter.ofx.bundle /Library/OFX/Plugins/

# 3. Attach debugger to Resolve process
# Visual Studio: Debug → Attach to Process → resolve.exe
# Xcode: Debug → Attach to Process → DaVinci Resolve
```

**Common debugging patterns**:
```cpp
// Log to host's message log (visible in Resolve)
sendMessage(OFX::Message::eMessageError, "", "GPU initialization failed");

// Assert during development (disabled in release builds)
assert(width > 0 && "Invalid image width");

// Platform-specific logging
#ifdef _WIN32
    OutputDebugString("Kernel execution time: 2.3ms\n");
#else
    NSLog(@"Kernel execution time: 2.3ms");
#endif
```

### GPU Debugging

**OpenCL debugging**:
```cpp
// Always check error codes
cl_int err;
cl_kernel kernel = clCreateKernel(program, "sample_intensity", &err);
if (err != CL_SUCCESS) {
    sendMessage(OFX::Message::eMessageError, "", 
                "Kernel creation failed: " + std::to_string(err));
    return;
}

// Query device info
char deviceName[128];
clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL);
// Log: "Using device: NVIDIA GeForce RTX 3080"
```

**Metal debugging** (macOS):
- Enable Metal API Validation in Xcode scheme
- Use Xcode's GPU Frame Capture (Product → Perform Action → Capture GPU Frame)
- Check for shader compilation errors in console

### Resolve-Specific Debugging Tips
- **Plugin not loading**: Check binary has `.ofx` extension, verify bundle structure on macOS
- **Crashes on parameter change**: Ensure thread-safety in `render()` - Resolve may call from multiple threads
- **Black output**: Verify pixel format compatibility (RGBA, float vs uint8)
- **Performance issues**: Use Resolve's timeline ruler to measure frame times, profile GPU vs CPU

## Qt Integration (Future)

### When to Use Qt
- **Complex parameter UI**: Beyond simple sliders (e.g., curve editors, color wheels)
- **Standalone tools**: Export/import utilities, batch processors
- **Plugin settings UI**: Preferences dialogs outside host

### Qt + OFX Pattern
```cpp
// Separate Qt widget from OFX plugin
// Plugin opens Qt dialog via button parameter
class SettingsDialog : public QDialog {
    // Qt UI logic here
};

// In plugin's parameterChangedAction
if (paramName == "openSettings") {
    QApplication app(argc, argv);
    SettingsDialog dialog;
    dialog.exec();
}
```

**CMake integration**:
```cmake
find_package(Qt6 COMPONENTS Widgets REQUIRED)
target_link_libraries(IntensityProfilePlotter Qt6::Widgets)
```

## Code Conventions

### C++ Style
- **C++17 standard**: Enables `std::filesystem`, structured bindings
- **Headers**: `#pragma once` for include guards
- **Namespacing**: Wrap plugin code in project namespace to avoid collisions
- **RAII**: Use smart pointers for GPU resources (`std::unique_ptr<cl_mem, ClMemDeleter>`)

### Error Handling
```cpp
// OFX convention: return status codes
OfxStatus render(const RenderArguments &args) {
    try {
        // ... processing
        return kOfxStatOK;
    } catch (const std::exception& e) {
        sendMessage(OFX::Message::eMessageError, "", e.what());
        return kOfxStatFailed;
    }
}
```

### Performance Guidelines
- **Avoid allocations in render()**: Pre-allocate buffers in constructor
- **Cache compiled kernels**: Build once, reuse across frames
- **Respect renderWindow**: Only process requested region, not full frame
- **Use host threading**: Set `kOfxImageEffectPluginRenderThreadSafety = kOfxImageEffectRenderFullySafe`

## Common Issues

### Build Issues
❌ **"cl.h not found"** → Set `OpenCL_INCLUDE_DIR` CMake variable
❌ **"OfxSupport library not found"** → Verify `OFX_SDK_PATH` points to complete SDK (not just headers)
❌ **Metal compilation fails** → Run `xcodebuild -downloadComponent MetalToolchain` on macOS
❌ **MSVC linker errors** → Ensure `/MD` runtime library flag (not `/MT`) for OFX plugins

### Runtime Issues
❌ **Plugin not visible in Resolve** → Check installed to correct path, verify `.ofx` extension
❌ **Crashes on launch** → Missing dependencies (OpenCL runtime DLL on Windows)
❌ **Slow performance** → Verify GPU backend is actually being used (add logging)
❌ **Wrong colors** → Check byte order (RGBA vs BGRA) and pixel type (float vs uint8)

## Testing Strategy

### Unit Testing
```cpp
// Test kernels independently
TEST(IntensitySampler, CalculatesCorrectAverage) {
    float input[4] = {1.0f, 0.5f, 0.25f, 1.0f}; // RGBA
    float result = calculateIntensity(input);
    EXPECT_NEAR(result, 0.5833f, 0.001f);
}
```

### Integration Testing
1. **Load in Resolve**: Place test footage in timeline, apply plugin
2. **Parameter sweep**: Automate parameter changes via scripting API
3. **Performance profiling**: Compare frame times GPU vs CPU

### Validation
- **Visual tests**: Render known inputs, compare against reference images
- **Cross-platform**: Build + test on Windows + macOS
- **GPU vendors**: Test NVIDIA, AMD, Intel (different OpenCL implementations)

## AI Agent Development Notes

When assisting with this codebase:

1. **Always verify file paths** before suggesting changes - user paths are user-specific
2. **Prefer OpenCL examples** over CUDA - cross-platform is priority
3. **Include error checking** in all GPU code - silent failures are hard to debug
4. **Reference OFX SDK docs** for API questions: https://openfx.readthedocs.io/
5. **Test plugin loading** before complex logic - get basic plugin recognized first
6. **Consider performance** - video plugins process 24-60fps, every millisecond counts

### Generating New Plugins
When creating a new OFX plugin:
1. Copy `ofx/` directory structure as template
2. Rename all classes (`IntensityProfilePlotter` → `YourPluginName`)
3. Update `CMakeLists.txt` plugin name and sources
4. Implement minimum: `getPluginID()`, `render()`, parameter definitions
5. Test in Resolve before adding GPU acceleration

### Adding GPU Kernels
When implementing new image processing:
1. Prototype in CPU version first (`CPURenderer.cpp`)
2. Translate to OpenCL kernel (`.cl` file)
3. Test OpenCL on Windows
4. Port to Metal if macOS support needed (`.metal` file)
5. Profile: ensure GPU is faster than CPU (sometimes CPU wins for small images)

## GPU Vendor-Specific Notes

### NVIDIA (GeForce RTX, Quadro, Tesla)
- **Best on**: Windows/Linux with latest CUDA drivers
- **OpenCL optimization**: Use `__local` memory, coalesce global memory access
- **Debugging**: NVIDIA Nsight Compute for kernel profiling
- **Work group size**: Multiples of 32 (warp size)

### AMD (Radeon RX, Radeon Pro)
- **Best on**: Linux (ROCm) or Windows (Adrenalin drivers)
- **OpenCL optimization**: Wave size 64 (RDNA), tune LDS usage
- **Debugging**: AMD Radeon GPU Profiler (RGP)
- **macOS Metal**: Use `threadgroup_barrier()` for synchronization

### Apple Silicon (M1/M2/M3)
- **Best on**: macOS with Metal API
- **Metal optimization**: Leverage unified memory, use `simdgroup` functions
- **Debugging**: Xcode GPU Frame Capture, Metal Debugger
- **Neural Engine**: Can offload ML operations via Core ML

### Intel (Iris Xe, Arc)
- **Best on**: Linux (Neo driver) or Windows (Intel SDK)
- **OpenCL optimization**: Sub-group size 8-16, avoid register spilling
- **Debugging**: Intel VTune Profiler
- **Fallback role**: Often used when dedicated GPU unavailable

## Cross-Platform Build Instructions

### Windows 11 (Visual Studio 2022)
```powershell
cd C:\Users\aberg\Documents\Vst
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
copy build\ofx\Release\*.ofx "C:\Program Files\Common Files\OFX\Plugins\"
```

### macOS (Universal Binary)
```bash
cd /Users/johan/Documents/Vst
mkdir build && cd build
cmake .. -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
sudo cp -r build/ofx/IntensityProfilePlotter.ofx.bundle /Library/OFX/Plugins/
```

### Linux (GCC/Clang)
```bash
cd ~/Documents/Vst
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
sudo cp build/ofx/*.ofx /usr/OFX/Plugins/
# Or user-local: cp build/ofx/*.ofx ~/.local/OFX/Plugins/
```

## Testing Matrix

### Recommended Test Combinations
| OS | GPU | Driver | Priority |
|----|-----|--------|----------|
| Windows 11 | NVIDIA RTX | CUDA 12.x | High |
| Windows 11 | AMD RX 6000 | Adrenalin | High |
| macOS 14+ | Apple M2 | Metal | High |
| macOS 13 | AMD Radeon Pro | Metal | Medium |
| Ubuntu 22.04 | NVIDIA | CUDA 11.8+ | High |
| Ubuntu 22.04 | AMD | ROCm 5.7+ | Medium |
| Windows/Linux | Intel Arc | Level Zero | Low |

## Tool Sets

### ofx-development
Multi-platform CMake setup, compiler configuration, SDK management
- **Windows**: Visual Studio 2022, build to `C:\Program Files\Common Files\OFX\Plugins\`
- **macOS**: Xcode, universal binaries (arm64 + x86_64), install to `/Library/OFX/Plugins/`
- **Linux**: GCC/Clang, RPATH handling, install to `/usr/OFX/Plugins/`

### gpu-debugging
OpenCL/Metal validation and vendor-specific profilers
- **NVIDIA**: Nsight Compute (profiler), Nsight Graphics (debugger)
- **AMD**: Radeon GPU Profiler, GPU Detective
- **Apple**: Xcode Instruments (GPU Timeline), Metal Debugger
- **Intel**: VTune Profiler, Graphics Performance Analyzers

### resolve-integration
DaVinci Resolve plugin testing and debugging workflow
- Build with Debug config
- Copy to platform-specific OFX directory
- Attach debugger to Resolve process
- Set breakpoints in render() or parameter handlers

### opencl-setup
OpenCL SDK setup and validation across platforms
- **Windows**: Intel/AMD/NVIDIA SDK options
- **macOS**: Built-in OpenCL framework (deprecated, prefer Metal)
- **Linux**: Package manager installation (ocl-icd-opencl-dev + vendor ICD)

---

**Last Updated**: December 9, 2025 (auto-generated from codebase analysis)
