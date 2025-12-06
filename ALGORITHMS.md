# Algorithm Documentation

## Overview

This VST3 plugin implements state-of-the-art algorithms for modeling analog circuit saturation. The plugin uses two complementary approaches:

1. **Wave Digital Filters (WDF)** - For accurate circuit topology modeling
2. **Nonlinear State-Space Models** - For dynamic nonlinear behavior

## Wave Digital Filters (WDF)

### Theory

Wave Digital Filters provide a powerful framework for modeling analog circuits in the digital domain. They maintain the topology and behavior of the original circuit while being computationally efficient for real-time processing.

### Implementation

The WDF implementation models a series RC circuit with nonlinear elements:

- **Wave Variables**: The circuit is described using incident and reflected waves
- **Adaptor Scattering**: Series and parallel connections are modeled using scattering matrices
- **Nonlinearity**: Soft saturation using hyperbolic tangent with adjustable drive
- **Asymmetry**: Subtle asymmetric behavior for more analog character

### Key Features

- Maintains circuit topology
- Efficient real-time processing
- Accurate frequency response modeling
- Configurable nonlinearity

## Nonlinear State-Space Models

### Theory

State-space formulations allow for accurate modeling of circuits with nonlinear components (diodes, transistors, vacuum tubes). The state-space representation captures both the linear dynamics and nonlinear behavior of the circuit.

### Circuit Types

#### 1. Tube Triode
- Models vacuum tube triode behavior
- Uses Child-Langmuir law approximation: I = k * (Vg + μ*Vp)^(3/2)
- Includes grid current modeling for soft clipping
- Characteristic warm, musical saturation

#### 2. Transistor BJT
- Models bipolar junction transistor
- Uses Ebers-Moll model: I = Is * (exp(V/Vt) - 1)
- Includes collector saturation effects
- Provides transistor-like harmonic content

#### 3. Diode Clipper
- Models diode-based clipping circuits
- Uses Shockley diode equation for forward and reverse bias
- Exponential forward current, small reverse leakage
- Classic diode clipper sound

#### 4. Op-Amp Saturation
- Models operational amplifier saturation
- Hard clipping with soft edges
- Symmetric saturation characteristics
- Clean, controlled saturation

### Implementation Details

The state-space model uses a second-order system with nonlinear feedback:

```
x' = Ax + Bu + f(x)
y = Cx + Du
```

Where:
- `x` is the state vector
- `f(x)` is the nonlinear function based on circuit type
- The system includes low-pass filtering to model circuit dynamics

## Hybrid Model

The plugin can combine both WDF and state-space approaches:

- **WDF Component (60%)**: Provides circuit topology and frequency response
- **State-Space Component (40%)**: Adds nonlinear dynamic behavior
- **Blended Output**: Combines both for rich, complex saturation

## Parameters

### Drive
Controls the input gain and saturation amount. Higher values increase harmonic content.

### Tone
Adjusts the frequency response:
- Lower values: More high-frequency content preserved
- Higher values: More high-frequency roll-off (warmer sound)

### Mix
Blends between dry (original) and wet (saturated) signal.

### Circuit Type
Selects the analog circuit model:
- Tube Triode: Warm, musical saturation
- Transistor BJT: Transistor-like harmonics
- Diode Clipper: Classic diode clipping
- Op-Amp Saturation: Clean, controlled saturation

### Model Type
Selects the algorithm approach:
- WDF Based: Pure Wave Digital Filter modeling
- State-Space: Pure nonlinear state-space modeling
- Hybrid: Combination of both (recommended)

## Performance Considerations

- **Latency**: Near-zero latency (sample-accurate processing)
- **CPU Usage**: Optimized for real-time performance
- **Memory**: Minimal memory footprint
- **Stability**: All algorithms are numerically stable

## Future Enhancements

Potential improvements:
- Additional circuit models (JFET, MOSFET, etc.)
- Adaptive parameter estimation
- Machine learning-based circuit modeling
- Multi-stage saturation chains
- Oversampling for aliasing reduction
