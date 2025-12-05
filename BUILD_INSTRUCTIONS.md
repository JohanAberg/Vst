# Build Instructions

## Prerequisites

### Required
- **CMake 3.15+**: Download from [cmake.org](https://cmake.org/download/)
- **C++17 Compiler**:
  - macOS: Xcode Command Line Tools (Clang)
  - Windows: Visual Studio 2019+ (MSVC) or MinGW-w64 (GCC)
  - Linux: GCC 7+ or Clang 5+
- **OpenFX SDK**: Download from [OpenFX website](https://openeffects.org/)

### Optional (for GPU acceleration)
- **OpenCL SDK**:
  - macOS: Included with Xcode
  - Windows: [Intel OpenCL SDK](https://www.intel.com/content/www/us/en/developer/tools/opencl-sdk/overview.html) or vendor-specific SDKs
  - Linux: `sudo apt-get install opencl-headers` (Ubuntu/Debian)

## Setting Up the OpenFX SDK

1. Download the OpenFX SDK (version 1.4 or later)
2. Extract to a location of your choice (e.g., `~/ofx-sdk`)
3. Note the path for CMake configuration

## Building on macOS

### Using Xcode Command Line Tools
```bash
# Create build directory
mkdir build
cd build

# Configure with OFX SDK path
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DOFX_SDK_PATH=~/ofx-sdk \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"  # Universal binary

# Build
cmake --build . --config Release

# The plugin bundle will be at:
# build/IntensityProfilePlotter.ofx
```

### Using Xcode IDE
```bash
mkdir build
cd build
cmake .. -G Xcode -DOFX_SDK_PATH=~/ofx-sdk
open IntensityProfilePlotter.xcodeproj
```

Then build from Xcode (Product → Build).

## Building on Windows

### Using Visual Studio
```bash
# Create build directory
mkdir build
cd build

# Configure for Visual Studio 2019
cmake .. \
  -G "Visual Studio 16 2019" \
  -A x64 \
  -DOFX_SDK_PATH=C:/path/to/ofx-sdk

# Build
cmake --build . --config Release

# The plugin DLL will be at:
# build/Release/IntensityProfilePlotter.ofx
```

### Using MinGW-w64
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles" -DOFX_SDK_PATH=C:/path/to/ofx-sdk
cmake --build . --config Release
```

## Building on Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential cmake libopencl-dev

# Create build directory
mkdir build
cd build

# Configure
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DOFX_SDK_PATH=~/ofx-sdk

# Build
cmake --build . --config Release

# The plugin library will be at:
# build/libIntensityProfilePlotter.ofx
```

## CMake Configuration Options

### OFX_SDK_PATH
Path to the OpenFX SDK directory. Required.
```bash
-DOFX_SDK_PATH=/path/to/ofx-sdk
```

### CMAKE_BUILD_TYPE
Build type: `Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`
```bash
-DCMAKE_BUILD_TYPE=Release
```

### CMAKE_OSX_ARCHITECTURES (macOS only)
Target architectures: `x86_64`, `arm64`, or both
```bash
-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"
```

## Troubleshooting

### OpenFX SDK Not Found
**Error**: `Could not find OpenFX SDK`

**Solution**: 
1. Verify the SDK path is correct
2. Ensure the SDK directory contains `include/` and `lib/` subdirectories
3. Use absolute paths if relative paths fail

### Metal Compilation Fails (macOS)
**Error**: `xcrun: error: unable to find utility "metal"`

**Solution**:
1. Install Xcode Command Line Tools: `xcode-select --install`
2. Ensure Xcode is properly installed and licensed

### OpenCL Not Found
**Warning**: `OpenCL not found - OpenCL support will be disabled`

**Solution**:
- This is not critical; the plugin will use CPU fallback
- To enable OpenCL, install the appropriate SDK for your platform
- On Linux, install `libopencl-dev` package

### Compilation Errors
**Error**: Various C++ compilation errors

**Solution**:
1. Ensure your compiler supports C++17
2. Check that all required headers are available
3. Verify OFX SDK version compatibility (1.4+)

### Link Errors
**Error**: Undefined references to OFX functions

**Solution**:
1. Verify OFX SDK library path is correct
2. Ensure OFX libraries are built for your platform
3. Check that library files exist in `$OFX_SDK_PATH/lib/`

## Verifying the Build

### Check Plugin Bundle (macOS)
```bash
# Verify bundle structure
ls -la build/IntensityProfilePlotter.ofx/Contents/
ls -la build/IntensityProfilePlotter.ofx/Contents/Resources/

# Check for Metal library
ls -la build/IntensityProfilePlotter.ofx/Contents/Resources/*.metallib
```

### Check Plugin Library (Linux/Windows)
```bash
# Verify library exists
ls -la build/*.ofx

# Check dependencies (Linux)
ldd build/libIntensityProfilePlotter.ofx
```

## Installation

After successful build, install the plugin to your OFX host's plugin directory:

### DaVinci Resolve
- **macOS**: `~/Library/OFX/Plugins/` or `/Library/OFX/Plugins/`
- **Windows**: `C:\ProgramData\Blackmagic Design\DaVinci Resolve\Support\OFX\Plugins\`
- **Linux**: `/opt/resolve/OFX/Plugins/` or user-specific location

### Other OFX Hosts
Consult your host application's documentation for plugin installation locations.

## Development Build

For development with debugging symbols:
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DOFX_SDK_PATH=~/ofx-sdk
cmake --build . --config Debug
```

## Continuous Integration

Example CI configuration files are available for:
- GitHub Actions (`.github/workflows/`)
- GitLab CI (`.gitlab-ci.yml`)
- Jenkins (Jenkinsfile)

See project documentation for CI-specific build instructions.
