## Analog Circuit Saturation VST3

Analog Saturation is a cross-platform VST3 plug-in that recreates multiple analog saturation topologies using antiderivative antialiasing (ADAA), zero-delay feedback (ZDF) and adaptive biasing. It runs as a stereo insert/FX plug-in and ships with a parameter-only interface that works in any VST3 host. The processing core is platform-agnostic modern C++ and does not depend on JUCE or other frameworks.

### DSP highlights
- **Topology-agnostic chain** – switch between Triode Class-A, Push-Pull bus and Tape Fuse behaviors without reloading the plug-in.
- **ADAA + ZDF stages** – reduces aliasing even at high drive by solving the nonlinear feedback loop per sample.
- **Adaptive bias & supply droop** – models power rail sag and asymmetric bias shifts using dynamic envelope tracking.
- **Transient tilt and spectral tone** – configurable pre-emphasis and pivoted tilt EQ to match different sources.

### Repository layout
```
.
├── CMakeLists.txt            # root build script
├── src
│   ├── AnalogSaturationProcessor.{h,cpp}
│   ├── AnalogSaturationController.{h,cpp}
│   ├── AnalogSaturationIDs.h
│   ├── dsp/                  # ADAA, bias and tone building blocks
│   ├── factory.cpp           # VST3 factory exports
│   └── version.h
```

### Building the plug-in
1. Clone the [Steinberg VST3 SDK](https://github.com/steinbergmedia/vst3sdk) next to this repository or anywhere on your disk.
2. Configure CMake with the SDK path:
   ```bash
   cmake -B build -S . -DVST3_SDK_ROOT=/absolute/path/to/vst3sdk
   cmake --build build
   ```
3. The resulting binary is written to `build/bin/AnalogSaturation.vst3` (on macOS this is a bundle inside `AnalogSaturation.vst3`).

### Parameter overview
| Parameter   | Range                | Description |
|-------------|----------------------|-------------|
| Drive       | -6 – +36 dB          | Input drive hitting the nonlinear stages |
| Bias        | -1 – +1              | Static asymmetry that shifts the ADAA/ZDF stages |
| Even/Odd    | 0 – 100 %            | Cross-fades between triode (even) and tape fold (odd) contributions |
| Topology    | Triode / Push-Pull / Tape | Selects the macro analog model |
| Tilt        | -1 – +1              | Broad spectral tilt around 1.4 kHz |
| Mix         | 0 – 100 %            | Wet/dry control |
| Output      | -24 – +12 dB         | Make-up gain |
| Dynamics    | 0 – 100 %            | Amount of adaptive bias/sag |
| Transient   | 0 – 100 %            | Strength of transient emphasis entering the drive stage |

### Testing
Use any VST3-compatible host (Cubase, Reaper, Bitwig, Live, etc.) and drop the compiled `AnalogSaturation.vst3` into the appropriate VST3 folder:
- Windows: `%COMMONPROGRAMFILES%/VST3`
- macOS: `/Library/Audio/Plug-Ins/VST3`
- Linux: `~/.vst3` or `/usr/lib/vst3`

Automated unit tests are not included because the Steinberg SDK handles most infrastructure. For DSP verification you can render sweeps through the different topologies and analyze spectra to confirm the low aliasing behavior.