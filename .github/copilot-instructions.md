# Plugin Collection: OFX Video + VST3 Audio

## Project Overview
Multi-plugin C++ repository containing:
1. **OFX Plugin** (`ofx/`): Intensity Profile Plotter for DaVinci Resolve - GPU-accelerated video analysis with Metal/OpenCL
2. **VST3 Plugins** (commented out): Audio saturation effects using JUCE (`vst_juce/`) and VST3 SDK (`vst_codex/`)

**Active development**: OFX video plugin. VST3 builds disabled in root `CMakeLists.txt` (see `BUILD_VST3_*` options).

## Quick Start

### Build OFX Plugin (macOS)
```bash
cd /Users/johan/Vst
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build . --config Release
sudo cp -r ofx/IntensityProfilePlotter.ofx.bundle /Library/OFX/Plugins/
```

### Enable VST3 Builds
Uncomment `BUILD_VST3_*` options in root `CMakeLists.txt`, then run JUCE setup:
```bash
./setup_juce.sh  # Downloads JUCE framework for vst_juce/
cmake .. -DBUILD_VST3_ANALOG_SATURATION=ON
```

## Project Structure

```
/Users/johan/Vst/
├── ofx/                          # IntensityProfilePlotter OFX plugin (ACTIVE)
│   ├── src/                      # Plugin implementation
│   │   ├── IntensityProfilePlotterPlugin.cpp    # OFX entry point, parameters
│   │   ├── IntensityProfilePlotterInteract.cpp  # Viewport overlay (drag endpoints)
│   │   ├── IntensitySampler.cpp                 # GPU/CPU sampling coordinator
│   │   ├── GPURenderer.cpp                      # Metal (macOS) / OpenCL dispatcher
│   │   ├── CPURenderer.cpp                      # Fallback bilinear sampling
│   │   └── ProfilePlotter.cpp                   # RGB curve rendering via DrawSuite
│   ├── kernels/
│   │   ├── intensitySampler.metal               # Metal compute shader (compiled to .metallib)
│   │   └── intensitySampler.cl                  # OpenCL kernel (runtime loaded)
│   ├── include/                  # Headers (matching src/)
│   ├── CMakeLists.txt            # OFX-specific build rules
│   └── *.md                      # Architecture docs (ARCHITECTURE.md, IMPLEMENTATION_NOTES.md)
│
├── vst_juce/                     # Analog saturation VST3 (JUCE-based, disabled)
│   └── Source/                   # PluginProcessor, CircuitModels, WaveDigitalFilter
├── vst_codex/                    # Analog saturation VST3 (VST3 SDK-based, disabled)
│   ├── src/dsp/SaturationModel.cpp
│   └── cmake/FetchVST3SDK.cmake  # Auto-downloads VST3 SDK
│
├── CMakeLists.txt                # Root: controls which plugins build
├── BUILD.md                      # JUCE setup instructions
└── ALGORITHMS.md                 # WDF + state-space saturation theory
```

## Architecture Essentials

### OFX Plugin Data Flow
```
DaVinci Resolve Viewer
    ↓ (Mouse drag endpoints)
IntensityProfilePlotterInteract → Updates P1/P2 parameters
    ↓
IntensityProfilePlotterPlugin::render()
    ├─→ Select data source: Input clip / Auxiliary clip / Built-in ramp
    ├─→ IntensitySampler → GPURenderer (Metal/OpenCL) OR CPURenderer
    │       ↓ Parallel sampling along scan line [P1→P2]
    │       ↓ Returns RGB intensity vectors
    └─→ ProfilePlotter → Draws curves + reference ramp via OFX DrawSuite
            ↓
        Overlay rendered in Resolve viewport
```

**Key Design**: GPU backend selected at runtime (Metal > OpenCL > CPU). No CUDA - OpenCL ensures NVIDIA/AMD/Intel cross-compatibility.

### VST3 Plugins (Currently Disabled)
- **vst_juce**: Uses JUCE's `AudioProcessor`, relies on WDF (Wave Digital Filters) for circuit modeling
- **vst_codex**: Native VST3 SDK via `smtg_add_vst3plugin()`, implements `IAudioProcessor` directly
- See `ALGORITHMS.md` for WDF theory and nonlinear state-space models (tube triode, BJT)

## Build System

### Root CMakeLists.txt Pattern
```cmake
# Key toggle: Plugins are opt-in via BUILD_* options
option(BUILD_OFX_PLUGIN "Build Intensity Profile Plotter" ON)
# option(BUILD_VST3_CIRCUIT_SATURATION "Build VST3 via SDK" ON)  # Commented out
# option(BUILD_VST3_ANALOG_SATURATION "Build VST3 via JUCE" ON)  # Commented out

# Platform-specific SDK paths (user-overridable)
if(APPLE)
    set(OFX_SDK_PATH "/Users/johan/openfx" CACHE PATH "...")
elseif(WIN32)
    set(OFX_SDK_PATH "C:/Users/aberg/Documents/openfx" CACHE PATH "...")
```

**Critical**: Root CMakeLists checks SDK existence before `add_subdirectory()` - emits warnings instead of fatal errors if missing.

### CMake Anti-Patterns to Avoid
❌ `// comments` → Use `#` (CMake != C++)  
❌ Hardcoded paths → Use `CACHE PATH` for SDK locations  
❌ `FATAL_ERROR` for optional deps → Emit `WARNING` and skip subdirectory  

### Platform-Specific Output

**macOS Bundle Structure**:
```
IntensityProfilePlotter.ofx.bundle/
└── Contents/
    ├── Info.plist
    ├── MacOS/
    │   └── IntensityProfilePlotter.ofx  # Binary (post-build renamed from .dylib)
    └── Resources/
        └── intensitySampler.metallib    # Compiled Metal kernels
```
Post-build command: `mv .../MacOS/IntensityProfilePlotter .../MacOS/IntensityProfilePlotter.ofx`

**Windows/Linux**: Single file `IntensityProfilePlotter.ofx`, OpenCL kernels copied alongside.

### Metal Shader Compilation (macOS)
```cmake
# ofx/CMakeLists.txt checks if Metal toolchain available
execute_process(COMMAND xcrun -sdk macosx metal -version ...)
# Compile .metal → .air → .metallib at build time
# Embedded in bundle via MACOSX_PACKAGE_LOCATION Resources
```
**Fallback**: If `xcrun` fails, `HAVE_METAL` set to OFF, plugin still builds without Metal support.

## GPU Backend Development

### Runtime Backend Selection
```cpp
// GPURenderer.cpp initialization order (Metal > OpenCL > CPU)
if (macOS && Metal available) {
    if (Apple Silicon) → Metal (unified memory, best perf)
    else if (AMD GPU) → Metal or OpenCL
    else → OpenCL (NVIDIA on Intel Mac)
}
else if (OpenCL available) {
    // Query devices: prefer NVIDIA > AMD > Intel
    clGetDeviceInfo(device, CL_DEVICE_NAME, ...)  // Log device selection
}
else → CPURenderer with bilinear interpolation
```

### Adding New GPU Kernels

**1. Prototype in CPU** (`CPURenderer.cpp`):
```cpp
// Verify algorithm correctness before GPU port
for (int i = 0; i < sampleCount; ++i) {
    float t = i / (float)(sampleCount - 1);
    float x = p1.x + t * (p2.x - p1.x);
    float y = p1.y + t * (p2.y - p1.y);
    // Bilinear sample at (x, y)...
}
```

**2. Translate to OpenCL** (`kernels/*.cl`):
```opencl
__kernel void sample_intensity(__global const float4* input, ...) {
    int gid = get_global_id(0);  // Parallel across samples
    // ... kernel logic, mirrors CPU version
}
```

**3. Port to Metal** (`kernels/*.metal`):
```metal
kernel void sample_intensity(texture2d<float, access::read> input [[texture(0)]],
                              device float* output [[buffer(1)]],
                              uint gid [[thread_position_in_grid]]) {
    // Use texture2d.read() for bilinear sampling
}
```

**Pro tip**: Profile CPU vs GPU - for <256 samples, CPU may be faster due to kernel launch overhead.

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

### SDK Setup by Platform

| SDK | Windows | macOS | Linux |
|-----|---------|-------|-------|
| **OpenFX** | `C:/Users/aberg/Documents/openfx` | `/Users/johan/openfx` | `~/openfx` |
| **OpenCL** | Intel/AMD/NVIDIA SDK | System framework (Xcode) | `apt install ocl-icd-opencl-dev` |
| **JUCE** (VST3) | Run `./setup_juce.sh` → clones JUCE 8.0 | Same | Same |
| **VST3 SDK** | Auto-fetched by `vst_codex/cmake/FetchVST3SDK.cmake` | Auto | Auto |

### First-Time Setup
```bash
# 1. Clone OpenFX SDK (required for OFX plugin)
git clone https://github.com/AcademySoftwareFoundation/openfx.git /Users/johan/openfx

# 2. Optional: Enable VST3 builds
./setup_juce.sh  # Downloads JUCE framework
# Then uncomment BUILD_VST3_* in root CMakeLists.txt
```

### Windows-Specific: OpenCL SDK
Download Intel SDK (recommended): https://github.com/intel/llvm/releases  
Extract to `C:/Users/aberg/Documents/OpenCL-SDK/`:
```
OpenCL-SDK/
├── include/CL/{cl.h, opencl.h}
└── lib/OpenCL.lib
```
Pass to CMake: `-DOpenCL_INCLUDE_DIR=... -DOpenCL_LIBRARY=...`

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

---

**Last Updated**: December 9, 2025 (auto-generated from codebase analysis)
