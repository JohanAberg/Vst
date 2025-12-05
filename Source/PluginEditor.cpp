#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogSaturationAudioProcessorEditor::AnalogSaturationAudioProcessorEditor (AnalogSaturationAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Drive slider
    driveSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    driveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    driveSlider.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(driveSlider);
    
    driveLabel.setText("Drive", juce::dontSendNotification);
    driveLabel.attachToComponent(&driveSlider, false);
    driveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(driveLabel);
    
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "drive", driveSlider);
    
    // Tone slider
    toneSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    toneSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    toneSlider.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(toneSlider);
    
    toneLabel.setText("Tone", juce::dontSendNotification);
    toneLabel.attachToComponent(&toneSlider, false);
    toneLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(toneLabel);
    
    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "tone", toneSlider);
    
    // Mix slider
    mixSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    mixSlider.setRange(0.0, 1.0, 0.01);
    addAndMakeVisible(mixSlider);
    
    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.attachToComponent(&mixSlider, false);
    mixLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mixLabel);
    
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "mix", mixSlider);
    
    // Circuit type combo (0-indexed for parameter)
    circuitTypeCombo.addItem("Tube Triode", 1);
    circuitTypeCombo.addItem("Transistor BJT", 2);
    circuitTypeCombo.addItem("Diode Clipper", 3);
    circuitTypeCombo.addItem("Op-Amp Saturation", 4);
    circuitTypeCombo.setSelectedId(1);
    addAndMakeVisible(circuitTypeCombo);
    
    circuitTypeLabel.setText("Circuit Type", juce::dontSendNotification);
    circuitTypeLabel.attachToComponent(&circuitTypeCombo, false);
    addAndMakeVisible(circuitTypeLabel);
    
    // Note: ComboBoxAttachment automatically handles 0-based indexing
    circuitTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "circuitType", circuitTypeCombo);
    
    // Model type combo (0-indexed for parameter)
    modelTypeCombo.addItem("WDF Based", 1);
    modelTypeCombo.addItem("State-Space", 2);
    modelTypeCombo.addItem("Hybrid", 3);
    modelTypeCombo.setSelectedId(3);
    addAndMakeVisible(modelTypeCombo);
    
    modelTypeLabel.setText("Model Type", juce::dontSendNotification);
    modelTypeLabel.attachToComponent(&modelTypeCombo, false);
    addAndMakeVisible(modelTypeLabel);
    
    // Note: ComboBoxAttachment automatically handles 0-based indexing  
    modelTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "modelType", modelTypeCombo);
    
    setSize (600, 400);
}

AnalogSaturationAudioProcessorEditor::~AnalogSaturationAudioProcessorEditor()
{
}

//==============================================================================
void AnalogSaturationAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (24.0f);
    g.drawFittedText ("Analog Saturation", getLocalBounds().removeFromTop(40),
                      juce::Justification::centred, 1);
    
    g.setFont (14.0f);
    g.setColour (juce::Colours::lightgrey);
    g.drawFittedText ("Advanced Circuit Modeling", 
                      getLocalBounds().removeFromTop(60).removeFromTop(20),
                      juce::Justification::centred, 1);
}

void AnalogSaturationAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(80);
    
    auto sliderArea = area.removeFromTop(200);
    auto sliderWidth = sliderArea.getWidth() / 3;
    
    driveSlider.setBounds(sliderArea.removeFromLeft(sliderWidth).reduced(10));
    toneSlider.setBounds(sliderArea.removeFromLeft(sliderWidth).reduced(10));
    mixSlider.setBounds(sliderArea.reduced(10));
    
    area.removeFromTop(20);
    
    auto comboArea = area.removeFromTop(60);
    auto comboWidth = comboArea.getWidth() / 2;
    
    circuitTypeCombo.setBounds(comboArea.removeFromLeft(comboWidth).reduced(10, 20));
    modelTypeCombo.setBounds(comboArea.reduced(10, 20));
}
