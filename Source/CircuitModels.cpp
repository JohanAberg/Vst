#include "CircuitModels.h"

CircuitModels::CircuitModels()
{
}

void CircuitModels::prepare(double sampleRate)
{
    wdf.prepare(sampleRate);
    stateSpace.prepare(sampleRate);
}

void CircuitModels::reset()
{
    wdf.reset();
    stateSpace.reset();
    drySample = 0.0f;
}

float CircuitModels::processSample(float input)
{
    drySample = input;
    
    float output = 0.0f;
    
    switch (modelType)
    {
        case ModelType::WDFBased:
        {
            wdf.setNonlinearity(drive);
            output = wdf.processSample(input);
            break;
        }
        case ModelType::StateSpace:
        {
            stateSpace.setDrive(drive);
            stateSpace.setTone(tone);
            stateSpace.setCircuitType(static_cast<NonlinearStateSpace::CircuitType>(circuitType));
            output = stateSpace.processSample(input);
            break;
        }
        case ModelType::Hybrid:
        {
            // Process through both and blend
            wdf.setNonlinearity(drive);
            float wdfOut = wdf.processSample(input);
            
            stateSpace.setDrive(drive * 0.7);
            stateSpace.setTone(tone);
            stateSpace.setCircuitType(static_cast<NonlinearStateSpace::CircuitType>(circuitType));
            float ssOut = stateSpace.processSample(input);
            
            // Blend WDF and state-space outputs
            output = wdfOut * 0.6f + ssOut * 0.4f;
            break;
        }
    }
    
    // Apply tone control (simple EQ)
    static float toneState = 0.0f;
    float toneAlpha = static_cast<float>(tone);
    toneState = toneAlpha * output + (1.0f - toneAlpha) * toneState;
    output = output * (1.0f - toneAlpha * 0.2f) + toneState * (toneAlpha * 0.2f);
    
    // Mix dry/wet
    float wetMix = static_cast<float>(mix);
    output = drySample * (1.0f - wetMix) + output * wetMix;
    
    return output;
}

void CircuitModels::setModelType(ModelType type)
{
    modelType = type;
}

void CircuitModels::setDrive(double drive)
{
    this->drive = juce::jlimit(0.0, 1.0, drive);
}

void CircuitModels::setTone(double tone)
{
    this->tone = juce::jlimit(0.0, 1.0, tone);
}

void CircuitModels::setMix(double mix)
{
    this->mix = juce::jlimit(0.0, 1.0, mix);
}

void CircuitModels::setCircuitType(int type)
{
    this->circuitType = juce::jlimit(0, 3, type);
}