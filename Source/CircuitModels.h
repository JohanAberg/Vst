#pragma once

#include <JuceHeader.h>
#include "WaveDigitalFilter.h"
#include "NonlinearStateSpace.h"

/**
 * CircuitModels combines WDF and state-space models to create
 * sophisticated analog saturation effects.
 */
class CircuitModels
{
public:
    enum class ModelType
    {
        WDFBased,        // Wave Digital Filter based
        StateSpace,      // Nonlinear state-space based
        Hybrid          // Combination of both
    };
    
    CircuitModels();
    ~CircuitModels() = default;
    
    void prepare(double sampleRate);
    void reset();
    
    float processSample(float input);
    
    void setModelType(ModelType type);
    void setDrive(double drive);
    void setTone(double tone);
    void setMix(double mix);
    void setCircuitType(int type);
    
private:
    ModelType modelType = ModelType::Hybrid;
    
    WaveDigitalFilter wdf;
    NonlinearStateSpace stateSpace;
    
    double drive = 1.0;
    double tone = 0.5;
    double mix = 1.0;
    int circuitType = 0;
    
    // Dry/wet mixing
    float drySample = 0.0f;
    
    // Tone control state (per-instance)
    float toneState = 0.0f;
};
