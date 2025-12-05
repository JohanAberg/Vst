# Analog Circuit Saturation VST3

A cross-platform VST3 audio effect that models analog circuit saturation using a hybrid triode/tape network, adaptive bias, and slew-dependent hysteresis. The plug-in targets creative coloration on busses, synths, and mastering chains while remaining CPU-efficient.

## Key Features
- **Dual topology model** blending memoryful tape-style soft clipping with asymmetric triode transfer curves.
- **Dynamic bias & hysteresis loop** that reacts to the envelope for natural bloom and punch.
- **Adaptive slew limiter** to emulate op-amp slewing and transformer inertia.
- **Quality switch** toggling eco (2×) vs high (4×) oversampling.
- **Thoughtful parameter set** covering drive, color, bias, dynamics, slew, mix, and output trim.

## DSP Architecture
1. **Pre-emphasis & drive staging** – frequency-dependent boost controlled by `color`, followed by exponential drive scaling for musically linear knob travel.
2. **Stateful dual-stage waveshaper** – combines `tanh` (odd harmonics) and `atan` (even harmonics) while feeding a hysteresis memory register influenced by `dynamics` and `bias`.
3. **Adaptive slew limiter** – clamps per-sample deltas according to `slew`, interpolating transformer-style inertia with oversampled resolution.
4. **Mix/trim & quality** – wet/dry crossfade followed by output trim and oversampling factor selection.

## Building
1. **Configure**
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   ```
2. **Build**
   ```bash
   cmake --build build --config Release
   ```

### VST3 SDK
The build downloads the official Steinberg VST3 SDK automatically via `FetchContent`. To use a local copy instead, set `-DVST3_SDK_ROOT=/path/to/vst3sdk` when configuring. By building this plug-in you agree to the [Steinberg VST3 License](https://www.steinberg.net/sdklicenses).

The compiled plug-in (`AnalogCircuitSaturation.vst3`) resides in `build/bin`. Install per-platform:
- **macOS**: copy the bundle to `~/Library/Audio/Plug-Ins/VST3`.
- **Windows**: copy the folder to `%PROGRAMFILES%/Common Files/VST3`.
- **Linux**: copy to `~/.vst3` or `/usr/lib/vst3`.

## Parameters
| ID | Description |
| --- | --- |
| Drive | Input drive with exponential taper. |
| Bias | Shifts operating point for asymmetric saturation. |
| Color | Crossfades tape vs triode curves. |
| Dynamics | Sets amount of memory feedback & adaptive bias. |
| Slew | Controls slew limiter cutoff for transient softening. |
| Mix | Wet/dry blend. |
| Output Trim | -12 dB to +12 dB makeup gain. |
| Quality | Eco (2×) vs High (4×) oversampling. |
| Bypass | Host-manageable hard bypass. |

## Testing
Render tests or creative comparisons can be automated via DAW session bounce. For headless CI, feed test impulses through the plug-in using a lightweight host such as JUCE's AudioPluginHost or clap-launch, then analyze THD+N and overshoot to validate regressions.
