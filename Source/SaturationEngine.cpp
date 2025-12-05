#include "SaturationEngine.h"

SaturationEngine::SaturationEngine()
{
}

void SaturationEngine::prepare(const juce::dsp::ProcessSpec& spec)
{
    processSpec = spec;
    circuitModels.prepare(spec.sampleRate);
}

void SaturationEngine::reset()
{
    circuitModels.reset();
}

void SaturationEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    auto numChannels = buffer.getNumChannels();
    auto numSamples = buffer.getNumSamples();
    
    // Update parameters
    circuitModels.setDrive(static_cast<double>(drive));
    circuitModels.setTone(static_cast<double>(tone));
    circuitModels.setMix(static_cast<double>(mix));
    circuitModels.setCircuitType(circuitType);
    circuitModels.setModelType(static_cast<CircuitModels::ModelType>(modelType));
    
    // Process each sample
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] = circuitModels.processSample(channelData[sample]);
        }
    }
}

void SaturationEngine::setDrive(float drive)
{
    this->drive = juce::jlimit(0.0f, 1.0f, drive);
}

void SaturationEngine::setTone(float tone)
{
    this->tone = juce::jlimit(0.0f, 1.0f, tone);
}

void SaturationEngine::setMix(float mix)
{
    this->mix = juce::jlimit(0.0f, 1.0f, mix);
}

void SaturationEngine::setCircuitType(int type)
{
    this->circuitType = juce::jlimit(0, 3, type);
}

void SaturationEngine::setModelType(int type)
{
    this->modelType = juce::jlimit(0, 2, type);
}
