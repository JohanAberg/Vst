#include "NonlinearStateSpace.h"

NonlinearStateSpace::NonlinearStateSpace()
{
    x.fill(0.0);
    xPrev.fill(0.0);
}

void NonlinearStateSpace::prepare(double sampleRate)
{
    this->sampleRate = sampleRate;
    reset();
}

void NonlinearStateSpace::reset()
{
    x.fill(0.0);
    xPrev.fill(0.0);
    toneState = 0.0;
}

float NonlinearStateSpace::processSample(float input)
{
    double inputScaled = static_cast<double>(input) * drive;
    
    // Update state-space model
    updateState(inputScaled);
    
    // Apply nonlinearity based on circuit type
    double output = 0.0;
    switch (circuitType)
    {
        case CircuitType::TubeTriode:
            output = tubeTriodeNonlinearity(x[0]);
            break;
        case CircuitType::TransistorBJT:
            output = transistorBJTCurrent(x[0]);
            break;
        case CircuitType::DiodeClipper:
            output = diodeClipperNonlinearity(x[0]);
            break;
        case CircuitType::OpAmpSaturation:
            output = opAmpSaturationNonlinearity(x[0]);
            break;
    }
    
    // Apply tone control (simple high-frequency roll-off)
    double toneAlpha = tone;
    toneState = toneAlpha * output + (1.0 - toneAlpha) * toneState;
    output = output * (1.0 - tone * 0.3) + toneState * (tone * 0.3);
    
    // Normalize output
    output = juce::jlimit(-1.0, 1.0, output);
    
    return static_cast<float>(output);
}

void NonlinearStateSpace::setCircuitType(CircuitType type)
{
    circuitType = type;
    reset();
}

void NonlinearStateSpace::setDrive(double drive)
{
    this->drive = juce::jlimit(0.1, 10.0, drive);
}

void NonlinearStateSpace::setTone(double tone)
{
    this->tone = juce::jlimit(0.0, 1.0, tone);
}

void NonlinearStateSpace::setBias(double bias)
{
    this->bias = juce::jlimit(-1.0, 1.0, bias);
}

void NonlinearStateSpace::updateState(double input)
{
    // State-space representation: x' = Ax + Bu, y = Cx + Du
    // Simplified second-order system with nonlinear feedback
    
    double dt = 1.0 / sampleRate;
    double alpha = 0.99;  // Damping
    
    // State update (simplified)
    xPrev = x;
    
    // First-order low-pass to model circuit dynamics
    double cutoff = 20000.0 * (1.0 - tone * 0.8);
    double rc = 1.0 / (2.0 * juce::MathConstants<double>::pi * cutoff);
    double alpha_lp = dt / (rc + dt);
    
    x[0] = alpha_lp * (input + bias) + (1.0 - alpha_lp) * x[0];
    
    // Higher-order states for more complex dynamics
    x[1] = alpha * x[1] + (1.0 - alpha) * x[0];
    x[2] = alpha * x[2] + (1.0 - alpha) * x[1];
    x[3] = alpha * x[3] + (1.0 - alpha) * x[2];
}

double NonlinearStateSpace::tubeTriodeNonlinearity(double v)
{
    // Child-Langmuir law approximation for triode
    // I = k * (Vg + mu*Vp)^(3/2)
    // Simplified for audio processing
    
    double vg = v + bias;
    double mu = 100.0;  // Amplification factor
    double k = 0.001;
    
    // Soft saturation with 3/2 power law
    double sign = vg >= 0.0 ? 1.0 : -1.0;
    double absV = std::abs(vg);
    double current = k * std::pow(absV, 1.5) * sign;
    
    // Add grid current (soft clipping)
    if (std::abs(vg) > 0.5)
    {
        current = softClip(current, 0.5);
    }
    
    return current * 10.0;  // Scale for audio range
}

double NonlinearStateSpace::transistorBJTCurrent(double v)
{
    // Ebers-Moll model approximation
    // I = Is * (exp(V/Vt) - 1)
    
    double vbe = v + bias;
    double current = Is * (std::exp(vbe / Vt) - 1.0);
    
    // Add collector saturation
    double maxCurrent = 0.01;
    if (current > maxCurrent)
    {
        current = maxCurrent + (current - maxCurrent) * 0.1;
    }
    
    // Convert to voltage (simplified)
    return std::tanh(current * 1000.0);
}

double NonlinearStateSpace::diodeClipperNonlinearity(double v)
{
    // Shockley diode equation: I = Is * (exp(V/Vt) - 1)
    // For clipping, we model the forward and reverse characteristics
    
    double vd = v + bias;
    
    if (vd > 0.0)
    {
        // Forward bias - exponential current
        double current = Is * (std::exp(vd / Vt) - 1.0);
        return std::tanh(current * 100.0);
    }
    else
    {
        // Reverse bias - small leakage current
        double current = -Is * (1.0 - std::exp(vd / Vt));
        return std::tanh(current * 100.0);
    }
}

double NonlinearStateSpace::opAmpSaturationNonlinearity(double v)
{
    // Operational amplifier saturation
    // Hard clipping with soft edges
    
    double vIn = v + bias;
    double saturationVoltage = 0.9;
    
    if (std::abs(vIn) < saturationVoltage)
    {
        return vIn;
    }
    else
    {
        double sign = vIn >= 0.0 ? 1.0 : -1.0;
        double excess = std::abs(vIn) - saturationVoltage;
        return sign * (saturationVoltage + excess * 0.1);
    }
}

double NonlinearStateSpace::softClip(double x, double threshold)
{
    if (std::abs(x) < threshold)
    {
        return x;
    }
    
    double sign = x >= 0.0 ? 1.0 : -1.0;
    double absX = std::abs(x);
    double excess = absX - threshold;
    
    // Soft compression above threshold
    return sign * (threshold + excess / (1.0 + excess));
}

double NonlinearStateSpace::asymmetricSaturation(double x)
{
    // Asymmetric saturation for more analog character
    if (x > 0.0)
    {
        return std::tanh(x * 0.9);
    }
    else
    {
        return std::tanh(x * 1.1);
    }
}
