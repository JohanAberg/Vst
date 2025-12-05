# Analog Saturation Plugin

A cross-platform audio plugin that models analog circuits for saturation using state-of-the-art algorithms including Wave Digital Filters (WDF) and nonlinear state-space models.

**This repository contains implementations for both VST3 and OFX (OpenFX) plugin formats**, allowing the same advanced saturation algorithms to be used in both audio DAWs (VST3) and video/compositing software (OFX).

## Features

- **Dual Plugin Format Support**: 
  - **VST3**: For audio DAWs (Ableton Live, Pro Tools, Reaper, etc.)
  - **OFX (OpenFX)**: For video/compositing software (DaVinci Resolve, Nuke, Fusion, etc.)
- **Advanced Circuit Modeling**: Uses Wave Digital Filters (WDF) for accurate analog circuit simulation
- **Nonlinear State-Space Models**: Implements sophisticated saturation algorithms based on real analog circuits
- **Multiple Saturation Types**: Various circuit models including tube, transistor, and diode-based saturation
- **Hybrid Modeling**: Combines WDF and state-space approaches for rich, complex saturation
- **Cross-Platform**: Builds on Windows, macOS, and Linux
- **Low Latency**: Optimized DSP algorithms for real-time processing
- **Full Automation**: All parameters support automation in both formats

## Quick Start

### 1. Setup JUCE Framework

```bash
./setup_juce.sh
```

### 2. Build the Plugins

The build system will compile both VST3 and OFX versions:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

**Output locations:**
- **VST3**: `build/AnalogSaturation_artefacts/Release/VST3/`
- **OFX**: `build/AnalogSaturation_artefacts/Release/OFX/`

See [BUILD.md](BUILD.md) for detailed build instructions for each format.

## Algorithm Details

### Wave Digital Filters (WDF)

WDFs provide a powerful framework for modeling analog circuits in the digital domain. They maintain the topology and behavior of the original circuit while being computationally efficient.

### Nonlinear State-Space Models

State-space formulations allow for accurate modeling of nonlinear components like diodes and transistors, capturing their dynamic behavior and saturation characteristics.

### Supported Circuit Types

1. **Tube Triode**: Warm, musical saturation using Child-Langmuir law
2. **Transistor BJT**: Transistor-like harmonics using Ebers-Moll model
3. **Diode Clipper**: Classic diode clipping using Shockley equation
4. **Op-Amp Saturation**: Clean, controlled saturation

See [ALGORITHMS.md](ALGORITHMS.md) for detailed algorithm documentation.

## Parameters

- **Drive** (0.0 - 1.0): Controls the input gain and saturation amount
- **Tone** (0.0 - 1.0): Adjusts the frequency response (higher = warmer)
- **Mix** (0.0 - 1.0): Blend between dry and saturated signal
- **Circuit Type**: Selects analog circuit model (Tube, Transistor, Diode, Op-Amp)
- **Model Type**: Selects algorithm (WDF, State-Space, or Hybrid)

## Project Structure

This repository is organized to support both VST3 and OFX plugin formats:

```
.
├── CMakeLists.txt          # Main CMake configuration (builds both VST3 and OFX)
├── Source/
│   ├── PluginProcessor.*   # VST3 plugin processor
│   ├── PluginEditor.*      # VST3 plugin UI
│   ├── OFX/                 # OFX plugin implementation (OpenFX format)
│   ├── SaturationEngine.* # Shared DSP engine wrapper
│   ├── CircuitModels.*     # Shared circuit modeling interface
│   ├── WaveDigitalFilter.*# Shared WDF implementation
│   └── NonlinearStateSpace.* # Shared state-space models
├── BUILD.md                # Detailed build instructions
└── ALGORITHMS.md           # Algorithm documentation
```

### Shared DSP Code

The core saturation algorithms (`SaturationEngine`, `CircuitModels`, `WaveDigitalFilter`, `NonlinearStateSpace`) are shared between both plugin formats, ensuring consistent audio processing regardless of the host application.

### Plugin Format Details

- **VST3**: Audio plugin format for music production DAWs
- **OFX**: OpenFX plugin format for video post-production and compositing software

## Requirements

- CMake 3.22+
- C++17 compatible compiler
- JUCE 8.0+ (automatically downloaded)

## License

This project is provided as-is for educational and development purposes.
