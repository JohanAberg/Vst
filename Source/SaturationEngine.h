#pragma once

#include <JuceHeader.h>
#include "CircuitModels.h"

/**
 * Main saturation engine that manages the DSP processing.
 */
class SaturationEngine
{
public:
    SaturationEngine();
    ~SaturationEngine() = default;
    
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();
    
    void processBlock(juce::AudioBuffer<float>& buffer);
    
    void setDrive(float drive);
    void setTone(float tone);
    void setMix(float mix);
    void setCircuitType(int type);
    void setModelType(int type);
    
private:
    CircuitModels circuitModels;
    juce::dsp::ProcessSpec processSpec;
    
    float drive = 0.5f;
    float tone = 0.5f;
    float mix = 1.0f;
    int circuitType = 0;
    int modelType = 2;  // Default to Hybrid
};
