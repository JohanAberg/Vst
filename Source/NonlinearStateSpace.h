#pragma once

#include <JuceHeader.h>
#include <array>

/**
 * Nonlinear State-Space model for analog saturation circuits.
 * Models circuits with nonlinear elements (diodes, transistors) using
 * state-space formulations for accurate dynamic behavior.
 */
class NonlinearStateSpace
{
public:
    enum class CircuitType
    {
        TubeTriode,      // Vacuum tube triode
        TransistorBJT,   // Bipolar junction transistor
        DiodeClipper,    // Diode-based clipper
        OpAmpSaturation  // Operational amplifier saturation
    };
    
    NonlinearStateSpace();
    ~NonlinearStateSpace() = default;
    
    void prepare(double sampleRate);
    void reset();
    
    float processSample(float input);
    
    void setCircuitType(CircuitType type);
    void setDrive(double drive);
    void setTone(double tone);
    void setBias(double bias);
    
private:
    double sampleRate = 44100.0;
    CircuitType circuitType = CircuitType::TubeTriode;
    
    // State variables
    std::array<double, 4> x;  // State vector
    std::array<double, 4> xPrev;  // Previous state
    
    // Parameters
    double drive = 1.0;
    double tone = 0.5;
    double bias = 0.0;
    
    // Circuit-specific parameters
    double Vt = 26e-3;  // Thermal voltage (26mV at room temp)
    double Is = 1e-12;  // Saturation current
    
    // Nonlinear functions for different circuit types
    double tubeTriodeNonlinearity(double v);
    double transistorBJTCurrent(double v);
    double diodeClipperNonlinearity(double v);
    double opAmpSaturationNonlinearity(double v);
    
    // State-space update
    void updateState(double input);
    
    // Helper functions
    double softClip(double x, double threshold);
    double asymmetricSaturation(double x);
};
