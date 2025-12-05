#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <complex>

/**
 * Wave Digital Filter (WDF) implementation for analog circuit modeling.
 * WDFs provide a powerful framework for modeling analog circuits digitally
 * while maintaining their topology and behavior.
 */
class WaveDigitalFilter
{
public:
    WaveDigitalFilter();
    ~WaveDigitalFilter() = default;

    void prepare(double sampleRate);
    void reset();
    
    // Process sample through WDF circuit
    float processSample(float input);
    
    // Set circuit parameters
    void setResistance(double R);
    void setCapacitance(double C);
    void setInductance(double L);
    void setNonlinearity(double nonlinearity);

private:
    double sampleRate = 44100.0;
    
    // WDF adaptor parameters
    double R = 1000.0;  // Resistance
    double C = 1e-6;    // Capacitance
    double L = 1e-3;    // Inductance
    
    // State variables
    double a1 = 0.0;  // Incident wave
    double b1 = 0.0;  // Reflected wave
    
    // Nonlinearity parameter
    double nonlinearity = 0.5;
    
    // Helper functions
    double adaptorScattering(double incident);
    double nonlinearFunction(double x);
};
