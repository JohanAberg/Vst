# Intensity Profile Plotter - Architectural Blueprint

## Overview

The Intensity Profile Plotter is a high-performance OpenFX (OFX) plugin designed for DaVinci Resolve and other OFX-compatible hosts. It provides GPU-accelerated visualization of intensity profiles along user-defined scan lines, enabling professional colorists to analyze color transforms and LUTs in real-time.

## Architecture

### Core Components

#### 1. Plugin Entry Point (`IntensityProfilePlotterPlugin`)
- **Location**: `src/IntensityProfilePlotterPlugin.cpp`
- **Responsibility**: Main OFX plugin class implementing the plugin interface
- **Key Features**:
  - Parameter management (scan line endpoints, data source selection, visualization options)
  - Clip management (primary input, optional auxiliary input)
  - Render orchestration
  - Integration with On-Screen Manipulator (OSM)

#### 2. On-Screen Manipulator (`IntensityProfilePlotterInteract`)
- **Location**: `src/IntensityProfilePlotterInteract.cpp`
- **Responsibility**: Interactive scan line definition via drag-and-drop endpoints
- **Key Features**:
  - Normalized coordinate system (resolution-independent)
  - Visual feedback (endpoints, scan line overlay)
  - Hit testing for endpoint selection
  - Real-time parameter updates

#### 3. Intensity Sampler (`IntensitySampler`)
- **Location**: `src/IntensitySampler.cpp`
- **Responsibility**: Samples RGB intensity values along scan line
- **Key Features**:
  - GPU/CPU abstraction layer
  - Automatic fallback from GPU to CPU
  - Bilinear interpolation for sub-pixel accuracy
  - Support for multiple data sources

#### 4. Profile Plotter (`ProfilePlotter`)
- **Location**: `src/ProfilePlotter.cpp`
- **Responsibility**: Renders intensity curves and reference ramp overlay
- **Key Features**:
  - OFX DrawSuite integration
  - RGB curve visualization with customizable colors
  - Reference grayscale ramp background
  - Grid overlay for visual reference

#### 5. GPU Renderer (`GPURenderer`)
- **Location**: `src/GPURenderer.cpp`
- **Responsibility**: GPU-accelerated intensity sampling
- **Key Features**:
  - Metal framework support (macOS priority)
  - OpenCL support (cross-platform fallback)
  - Direct GPU image sharing on macOS
  - Automatic backend selection

#### 6. CPU Renderer (`CPURenderer`)
- **Location**: `src/CPURenderer.cpp`
- **Responsibility**: CPU fallback implementation
- **Key Features**:
  - Bilinear interpolation
  - Thread-safe implementation
  - High-quality sampling

### GPU Kernels

#### Metal Kernel (`kernels/intensitySampler.metal`)
- **Platform**: macOS
- **Features**:
  - Parallel sampling across scan line
  - Bilinear interpolation
  - Optimized for Apple Silicon and discrete GPUs
  - Embedded as `.metallib` in plugin bundle

#### OpenCL Kernel (`kernels/intensitySampler.cl`)
- **Platform**: Windows, Linux, macOS (fallback)
- **Features**:
  - Cross-platform GPU acceleration
  - Compatible with NVIDIA, AMD, and Intel GPUs
  - Runtime kernel loading

## Data Flow

```
User Interaction (OSM)
    ↓
Parameter Updates (P1, P2)
    ↓
Render Request
    ↓
Data Source Selection
    ├─→ Input Clip → IntensitySampler → GPU/CPU Sampling
    ├─→ Auxiliary Clip → IntensitySampler → GPU/CPU Sampling
    └─→ Built-in Ramp → Direct Generation
    ↓
Intensity Samples (R, G, B vectors)
    ↓
ProfilePlotter → OFX DrawSuite
    ↓
Overlay Rendering (Curves + Reference Ramp)
    ↓
Output Image
```

## Parameter System

### Scan Line Definition
- **Point 1** (`point1`): Double2D parameter, normalized [0-1, 0-1]
- **Point 2** (`point2`): Double2D parameter, normalized [0-1, 0-1]

### Data Source Selection
- **Data Source** (`dataSource`): Choice parameter
  - 0: Input Clip (post-LUT analysis)
  - 1: Auxiliary Clip (comparative analysis)
  - 2: Built-in Ramp (LUT testing)

### Visualization Controls
- **Sample Count** (`sampleCount`): Integer, 64-2048 samples
- **Plot Height** (`plotHeight`): Double, normalized [0.1-0.8]
- **Curve Colors**: RGBA parameters for R, G, B curves
- **Show Reference Ramp**: Boolean toggle

## GPU Acceleration Strategy

### macOS (Primary)
1. **Metal Framework**: Native Apple API
2. **Direct GPU Image Sharing**: Zero-copy memory access in DaVinci Resolve
3. **Embedded Shaders**: `.metallib` compiled at build time
4. **Automatic Device Selection**: System default GPU

### Cross-Platform (Fallback)
1. **OpenCL**: Industry-standard compute API
2. **Runtime Kernel Loading**: Kernel loaded from resource file
3. **Multi-Vendor Support**: NVIDIA, AMD, Intel GPUs
4. **CPU Fallback**: Automatic if no GPU available

### Performance Characteristics
- **Metal (macOS)**: ~10-50x faster than CPU for 512+ samples
- **OpenCL**: ~5-20x faster than CPU (varies by GPU)
- **CPU**: Reliable fallback, suitable for <256 samples

## Build System

### CMake Configuration
- **Minimum Version**: CMake 3.15
- **C++ Standard**: C++17
- **Platform Detection**: Automatic (macOS/Windows/Linux)
- **Dependency Management**: 
  - OFX SDK (configurable path)
  - OpenCL (optional, auto-detected)
  - Metal (macOS, system framework)

### Build Targets
- **macOS**: `.ofx` bundle with embedded Metal library
- **Windows**: `.ofx` DLL with OpenCL support
- **Linux**: `.ofx` shared library with OpenCL support

### Installation
- Plugin bundles installed to host-specific OFX plugin directories
- Metal library embedded in macOS bundle Resources
- OpenCL kernel copied to plugin directory for runtime loading

## Integration Points

### OFX Host Requirements
- **OFX API Version**: 1.4+
- **Required Suites**:
  - ImageEffect
  - Interact (for OSM)
  - DrawSuite (for overlay rendering)
  - Param (for parameter management)
- **Supported Contexts**: Filter, General
- **Pixel Depths**: Float (32-bit)
- **Components**: RGB, RGBA

### DaVinci Resolve Specific
- **Direct GPU Image Sharing**: Leverages Metal on macOS
- **Real-time Preview**: GPU acceleration enables interactive feedback
- **Color Space**: Works with any color space (linear, log, etc.)

## Extension Points

### Custom Data Sources
The architecture supports adding new data sources by:
1. Extending `dataSource` choice parameter
2. Adding sampling logic in `render()` method
3. Implementing custom generation functions

### Custom Visualization
The `ProfilePlotter` class can be extended to:
- Add additional curve types (luminance, saturation, etc.)
- Implement different plot styles (histogram, waveform, etc.)
- Add annotation features (markers, labels, etc.)

### Additional GPU Backends
New GPU backends can be added by:
1. Implementing backend detection in `GPURenderer`
2. Adding kernel compilation/loading logic
3. Implementing sampling function with backend-specific API

## Performance Considerations

### Memory Management
- **Zero-copy**: Metal on macOS enables direct GPU memory access
- **Buffer Reuse**: GPU buffers allocated once, reused across frames
- **CPU Fallback**: Minimal memory overhead

### Sampling Optimization
- **Bilinear Interpolation**: High-quality sub-pixel sampling
- **Parallel Processing**: GPU kernels process all samples simultaneously
- **Vectorization**: CPU implementation uses SIMD where available

### Rendering Optimization
- **DrawSuite**: Hardware-accelerated overlay rendering
- **Selective Updates**: Only redraws when parameters change
- **Tile Support**: Works with tiled rendering for large images

## Testing Strategy

### Unit Tests
- Parameter validation
- Coordinate transformation (normalized ↔ pixel)
- Bilinear interpolation accuracy
- Sample count edge cases

### Integration Tests
- OFX host compatibility
- GPU/CPU fallback behavior
- Multi-resolution support
- Color space handling

### Performance Tests
- GPU vs CPU benchmarks
- Memory usage profiling
- Real-time playback performance
- Large image handling (4K, 8K)

## Deployment

### Distribution
- **macOS**: Universal binary (Intel + Apple Silicon)
- **Windows**: x64 binary
- **Linux**: x64 binary

### Dependencies
- **Runtime**: OFX host application
- **Optional**: OpenCL runtime (for GPU acceleration on non-macOS)

### Versioning
- **Plugin Version**: 1.0.0
- **API Compatibility**: OFX 1.4+
- **Host Compatibility**: DaVinci Resolve 17+, other OFX hosts

## Future Enhancements

### Potential Features
1. **Multi-line Support**: Multiple scan lines with comparison
2. **Temporal Analysis**: Frame-to-frame intensity tracking
3. **Export Functionality**: Save profile data to CSV/JSON
4. **Custom Transfer Functions**: User-defined LUT testing curves
5. **3D Visualization**: Interactive 3D color space plots

### Performance Improvements
1. **Async GPU Operations**: Overlap sampling with rendering
2. **Caching**: Cache samples for static scan lines
3. **Progressive Rendering**: Lower quality preview, full quality on pause
