# Building the Analog Saturation VST3 Plugin

## Prerequisites

- **CMake** 3.22 or higher
- **C++17 compatible compiler**:
  - Windows: Visual Studio 2019 or later (MSVC)
  - macOS: Xcode 12 or later (Clang)
  - Linux: GCC 7+ or Clang 8+
- **Git** (for downloading JUCE)

## Setup Instructions

### 1. Download JUCE Framework

Run the setup script:

```bash
./setup_juce.sh
```

Or manually:

```bash
git clone --depth 1 --branch 8.0.0 https://github.com/juce-framework/JUCE.git
```

### 2. Build the Plugin

#### Linux/macOS:

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

#### Windows:

```cmd
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 3. Install the Plugin

The built plugin will be located in:
- **Linux**: `build/AnalogSaturation_artefacts/Release/VST3/`
- **macOS**: `build/AnalogSaturation_artefacts/Release/VST3/`
- **Windows**: `build/AnalogSaturation_artefacts/Release/VST3/`

Copy the `.vst3` file to your DAW's VST3 plugin directory:
- **Linux**: `~/.vst3/` or `/usr/local/lib/vst3/`
- **macOS**: `~/Library/Audio/Plug-Ins/VST3/`
- **Windows**: `C:\Program Files\Common Files\VST3\`

## Development

### Project Structure

```
.
├── CMakeLists.txt          # Main CMake configuration
├── Source/
│   ├── PluginProcessor.*   # Main plugin processor
│   ├── PluginEditor.*      # Plugin UI
│   ├── SaturationEngine.* # DSP engine wrapper
│   ├── CircuitModels.*     # Circuit modeling interface
│   ├── WaveDigitalFilter.*# WDF implementation
│   └── NonlinearStateSpace.* # State-space models
└── JUCE/                   # JUCE framework (after setup)
```

### Algorithm Details

#### Wave Digital Filters (WDF)
- Models analog circuits using wave variables
- Maintains circuit topology digitally
- Efficient real-time processing

#### Nonlinear State-Space Models
- Models nonlinear components (diodes, transistors, tubes)
- Captures dynamic behavior
- Multiple circuit types supported

## Troubleshooting

### CMake can't find JUCE
- Ensure JUCE directory exists in the project root
- Run `setup_juce.sh` if not already done

### Build errors
- Ensure C++17 is supported by your compiler
- Check that all dependencies are installed
- Try cleaning the build directory and rebuilding

### Plugin not loading in DAW
- Verify the plugin is in the correct VST3 directory
- Check DAW's plugin scan settings
- Ensure the plugin architecture matches your system (64-bit)
