#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AnalogSaturationAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AnalogSaturationAudioProcessorEditor (AnalogSaturationAudioProcessor&);
    ~AnalogSaturationAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AnalogSaturationAudioProcessor& audioProcessor;
    
    juce::Slider driveSlider;
    juce::Label driveLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    
    juce::Slider toneSlider;
    juce::Label toneLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
    
    juce::Slider mixSlider;
    juce::Label mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    juce::ComboBox circuitTypeCombo;
    juce::Label circuitTypeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> circuitTypeAttachment;
    
    juce::ComboBox modelTypeCombo;
    juce::Label modelTypeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelTypeAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogSaturationAudioProcessorEditor)
};
