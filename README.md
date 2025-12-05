# Analog Circuit Saturation VST3

A cross-platform VST3 plug-in that recreates analog saturation using novel, anti-aliased circuit models. The processor blends adaptive triode drive, diode symmetry control, transformer hysteresis, and dynamic biasing to deliver musical distortion with minimal aliasing.

## Highlights
- **State-of-the-art signal path** – cascaded ADAA waveshapers, adaptive pre/de-emphasis filters, and transformer hysteresis modeling reduce digital harshness while retaining analog character.
- **Dynamic circuit response** – bias, even/odd harmonic balance, reactance, and transient-dependent drive continuously adapt to the incoming material.
- **Cross-platform build** – CMake-based project targeting Windows, macOS, and Linux VST3 hosts. Output bundles follow Steinberg’s directory layout.

## Building
1. Download the official [Steinberg VST3 SDK](https://developer.steinberg.help/display/VST) and set `VST3_SDK_ROOT` to the extracted directory (environment variable or CMake cache entry).
2. Configure the project:
   ```
   cmake -S . -B build -DVST3_SDK_ROOT=/path/to/VST3_SDK
   ```
3. Build the plug-in:
   ```
   cmake --build build --config Release
   ```
4. The finished bundle is placed in `build/AnalogCircuitSaturation.vst3`. Copy it to your host’s VST3 directory (e.g., `~/Library/Audio/Plug-Ins/VST3` on macOS or `%ProgramFiles%/Common Files/VST3` on Windows).

## Parameters
| Control | Description |
| --- | --- |
| Drive | Gain into the triode stage (−6 dB to +42 dB). |
| Bias | DC bias injected into the ADAA stage for asymmetry. |
| Even/Odd | Morph between symmetrical (odd) and biased (even) harmonics. |
| Tone | Sets the post-emphasis cutoff (80 Hz – 18 kHz). |
| Dynamics | Governs how much the bias follows the envelope. |
| Mix | Dry/wet blend for parallel saturation. |
| Output | Makeup gain (−18 dB to +18 dB). |
| Reactance | Controls transformer hysteresis depth. |
| Oversample | Selects 1×, 2×, or 4× oversampling via multi-step ADAA interpolation. |

## Architecture
- `AnalogCircuitModel` orchestrates pre/de-emphasis filters, ADAA triode/diode stages, and transformer hysteresis with adaptive biasing.
- `AdaaWaveshaper` implements antiderivative anti-aliased processing for tanh and soft clip transfer curves plus a responsive envelope follower.
- `SaturationProcessor` (Steinberg `AudioEffect`) wires the DSP into the VST3 audio pipeline.
- `SaturationController` registers automatable parameters and handles state syncing with the host.

## Testing & Validation
- Unit tests can be added under `tests/` by enabling `-DANALOG_SAT_BUILD_TESTS=ON`.
- For manual validation, load the plug-in in a VST3 host (REAPER, Cubase, etc.) and sweep parameters while feeding sine sweeps or drum loops to inspect harmonic content and alias suppression.

## License
The project source is MIT licensed. You must separately comply with Steinberg’s VST3 SDK license when building or distributing the plug-in.
