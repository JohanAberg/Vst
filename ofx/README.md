# Intensity Profile Plotter - OFX Plugin

A high-performance OpenFX plugin for DaVinci Resolve providing GPU-accelerated intensity profile visualization along user-defined scan lines.

## Features

### Interactive Visualization
- **On-Screen Manipulator (OSM)**: Drag two endpoints directly in the viewer to define scan lines
- **RGB Curve Plotting**: Real-time visualization of Red, Green, and Blue intensity values
- **Reference Ramp**: Linear 0-1 grayscale background for visual comparison
- **Resolution Independent**: Normalized coordinates work at any resolution

### Advanced Analytical Modes
- **Input Clip Analysis**: Sample the primary video clip (post-LUT analysis)
- **Auxiliary Clip Comparison**: Sample an optional secondary input for comparative analysis
- **Built-in Ramp (LUT Test)**: Generate linear 0-1 signal to test transfer functions independently

### GPU Acceleration
- **Metal (macOS)**: Native Apple framework with direct GPU image sharing
- **OpenCL (Cross-Platform)**: Fallback for Windows and Linux
- **CPU Fallback**: Automatic fallback when GPU unavailable

## Building

### Prerequisites
- CMake 3.15 or higher
- C++17 compatible compiler (Clang, GCC, MSVC)
- OpenFX SDK (configure path in CMakeLists.txt)
- OpenCL SDK (optional, for GPU acceleration on non-macOS)

### macOS Build
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

The plugin bundle will be created at `build/IntensityProfilePlotter.ofx`.

### Windows Build
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

### Linux Build
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Installation

### DaVinci Resolve
1. **macOS**: Copy `.ofx` bundle to `/Library/OFX/Plugins/` or `~/Library/OFX/Plugins/`
2. **Windows**: Copy `.ofx` DLL to `C:\ProgramData\Blackmagic Design\DaVinci Resolve\Support\OFX\Plugins\`
3. **Linux**: Copy `.ofx` library to `/opt/resolve/OFX/Plugins/` or user-specific location

Restart DaVinci Resolve after installation.

## Usage

### Basic Workflow
1. Add the plugin to a node in the Color page
2. Use the On-Screen Manipulator to position scan line endpoints
3. Select data source (Input Clip, Auxiliary Clip, or Built-in Ramp)
4. Adjust sample count and plot height as needed
5. Observe RGB intensity curves in the plot overlay

### Parameters
- **Point 1 / Point 2**: Normalized coordinates [0-1, 0-1] of scan line endpoints
- **Data Source**: 
  - Input Clip: Samples primary video
  - Auxiliary Clip: Samples connected secondary input
  - Built-in Ramp: Generates linear 0-1 signal for LUT testing
- **Sample Count**: Number of samples along scan line (64-2048)
- **Plot Height**: Height of plot overlay as fraction of image height (0.1-0.8)
- **Curve Colors**: RGBA colors for Red, Green, Blue curves
- **Show Reference Ramp**: Toggle linear grayscale background

### LUT Testing Workflow
1. Set Data Source to "Built-in Ramp"
2. Apply your LUT or color transform to the node
3. Observe how the transfer function affects the linear ramp
4. Compare input vs output to analyze LUT response

## Architecture

See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed architectural documentation.

### Key Components
- **IntensityProfilePlotterPlugin**: Main plugin class
- **IntensityProfilePlotterInteract**: On-Screen Manipulator
- **IntensitySampler**: GPU/CPU sampling abstraction
- **ProfilePlotter**: Overlay rendering with DrawSuite
- **GPURenderer**: Metal/OpenCL acceleration
- **CPURenderer**: CPU fallback implementation

## Performance

### GPU Acceleration
- **Metal (macOS)**: 10-50x faster than CPU
- **OpenCL**: 5-20x faster than CPU (varies by GPU)
- **CPU**: Suitable for <256 samples, reliable fallback

### Optimization
- Direct GPU image sharing on macOS (zero-copy)
- Parallel sampling across all points
- Hardware-accelerated overlay rendering

## Development

### Project Structure
```
.
├── CMakeLists.txt          # Build configuration
├── Info.plist              # macOS bundle metadata
├── include/                # Header files
│   ├── IntensityProfilePlotterPlugin.h
│   ├── IntensityProfilePlotterInteract.h
│   ├── IntensitySampler.h
│   ├── ProfilePlotter.h
│   ├── GPURenderer.h
│   └── CPURenderer.h
├── src/                    # Source files
│   ├── IntensityProfilePlotterPlugin.cpp
│   ├── IntensityProfilePlotterInteract.cpp
│   ├── IntensitySampler.cpp
│   ├── ProfilePlotter.cpp
│   ├── GPURenderer.cpp
│   └── CPURenderer.cpp
└── kernels/                # GPU kernels
    ├── intensitySampler.metal
    └── intensitySampler.cl
```

### Code Style
- C++17 standard
- RAII memory management
- Clear separation of concerns
- Comprehensive error handling

## License

This project is licensed under the MIT License - see below for details:

```
MIT License

Copyright (c) 2025 Johan Aberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Contributing

Contributions are welcome! If you'd like to contribute to this project:

1. **Fork the repository** on GitHub
2. **Create a feature branch** (`git checkout -b feature/your-feature-name`)
3. **Make your changes** and ensure they follow the existing code style
4. **Test your changes** thoroughly on your target platform
5. **Commit your changes** (`git commit -am 'Add some feature'`)
6. **Push to the branch** (`git push origin feature/your-feature-name`)
7. **Open a Pull Request** with a clear description of your changes

### Guidelines
- Follow C++17 standard practices
- Maintain RAII memory management patterns
- Include comments for complex logic
- Test on macOS, Windows, or Linux as applicable
- Ensure GPU acceleration paths remain functional

## Support

### Getting Help

If you encounter issues or have questions:

- **Issues**: Report bugs or request features via [GitHub Issues](https://github.com/JohanAberg/Vst/issues)
- **Discussions**: Ask questions in [GitHub Discussions](https://github.com/JohanAberg/Vst/discussions)
- **Email**: Contact Johan Aberg at johan@aberg.co.nz

### Known Issues

Please check the [Issues](https://github.com/JohanAberg/Vst/issues) page for known bugs and planned enhancements.

### Platform Support

- **macOS**: Fully supported with Metal GPU acceleration
- **Windows**: Supported with OpenCL GPU acceleration
- **Linux**: Supported with OpenCL GPU acceleration

## Version History

### 1.0.0
- Initial release
- GPU-accelerated intensity sampling
- On-Screen Manipulator for scan line definition
- RGB curve visualization
- Reference ramp background
- Multiple data source modes
- Metal (macOS) and OpenCL (cross-platform) support
