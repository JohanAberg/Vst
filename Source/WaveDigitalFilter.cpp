#include "WaveDigitalFilter.h"

WaveDigitalFilter::WaveDigitalFilter()
{
}

void WaveDigitalFilter::prepare(double sampleRate)
{
    this->sampleRate = sampleRate;
    reset();
}

void WaveDigitalFilter::reset()
{
    a1 = 0.0;
    b1 = 0.0;
    capacitorState = 0.0;
}

float WaveDigitalFilter::processSample(float input)
{
    // Convert voltage to wave variable
    double voltage = static_cast<double>(input);
    
    // Incident wave: a = v + i*R
    // For simplicity, we model a series RC circuit
    double R_eff = R;
    double Z = R_eff;  // Impedance
    
    // Calculate incident wave
    a1 = voltage + (voltage / Z) * R_eff;
    
    // Apply nonlinearity to the wave
    a1 = nonlinearFunction(a1);
    
    // Scattering operation (reflection)
    b1 = adaptorScattering(a1);
    
    // Convert back to voltage: v = (a + b) / 2
    double output = (a1 + b1) * 0.5;
    
    // Apply capacitor smoothing (low-pass effect)
    double alpha = 1.0 / (1.0 + 2.0 * juce::MathConstants<double>::pi * C * R * sampleRate);
    capacitorState = alpha * output + (1.0 - alpha) * capacitorState;
    output = capacitorState;
    
    return static_cast<float>(output);
}

void WaveDigitalFilter::setResistance(double R)
{
    this->R = R;
}

void WaveDigitalFilter::setCapacitance(double C)
{
    this->C = C;
}

void WaveDigitalFilter::setInductance(double L)
{
    this->L = L;
}

void WaveDigitalFilter::setNonlinearity(double nonlinearity)
{
    this->nonlinearity = juce::jlimit(0.0, 1.0, nonlinearity);
}

double WaveDigitalFilter::adaptorScattering(double incident)
{
    // Series adaptor scattering matrix
    // For a series connection, reflection coefficient depends on impedances
    double Z1 = R;
    double Z2 = 1.0 / (2.0 * juce::MathConstants<double>::pi * C * sampleRate);
    
    double gamma = (Z1 - Z2) / (Z1 + Z2);
    return gamma * incident;
}

double WaveDigitalFilter::nonlinearFunction(double x)
{
    // Soft saturation using hyperbolic tangent with adjustable curve
    double drive = 1.0 + nonlinearity * 9.0;  // Drive from 1 to 10
    double saturated = std::tanh(x * drive);
    
    // Add subtle asymmetry for more analog character
    if (x > 0.0)
    {
        saturated = std::tanh(x * drive * 0.95);
    }
    else
    {
        saturated = std::tanh(x * drive * 1.05);
    }
    
    return saturated;
}
