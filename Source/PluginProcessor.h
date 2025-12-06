#pragma once

#include <JuceHeader.h>
#include "SaturationEngine.h"

//==============================================================================
/**
*/
class AnalogSaturationAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    AnalogSaturationAudioProcessor();
    ~AnalogSaturationAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

private:
    //==============================================================================
    SaturationEngine saturationEngine;
    
    juce::AudioProcessorValueTreeState parameters;
    
    // Parameter IDs
    static constexpr const char* DRIVE_ID = "drive";
    static constexpr const char* TONE_ID = "tone";
    static constexpr const char* MIX_ID = "mix";
    static constexpr const char* CIRCUIT_TYPE_ID = "circuitType";
    static constexpr const char* MODEL_TYPE_ID = "modelType";
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogSaturationAudioProcessor)
};
