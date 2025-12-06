#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogSaturationAudioProcessor::AnalogSaturationAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters(*this, nullptr, juce::Identifier("AnalogSaturation"),
               {
                   std::make_unique<juce::AudioParameterFloat>(DRIVE_ID, "Drive",
                                                                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                                0.5f),
                   std::make_unique<juce::AudioParameterFloat>(TONE_ID, "Tone",
                                                                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                                0.5f),
                   std::make_unique<juce::AudioParameterFloat>(MIX_ID, "Mix",
                                                                juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                                                1.0f),
                   std::make_unique<juce::AudioParameterInt>(CIRCUIT_TYPE_ID, "Circuit Type",
                                                             0, 3, 0),
                   std::make_unique<juce::AudioParameterInt>(MODEL_TYPE_ID, "Model Type",
                                                             0, 2, 2)
               })
{
}

AnalogSaturationAudioProcessor::~AnalogSaturationAudioProcessor()
{
}

//==============================================================================
const juce::String AnalogSaturationAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnalogSaturationAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AnalogSaturationAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AnalogSaturationAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AnalogSaturationAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AnalogSaturationAudioProcessor::getNumPrograms()
{
    return 1;
}

int AnalogSaturationAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AnalogSaturationAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AnalogSaturationAudioProcessor::getProgramName (int index)
{
    return {};
}

void AnalogSaturationAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AnalogSaturationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    
    saturationEngine.prepare(spec);
}

void AnalogSaturationAudioProcessor::releaseResources()
{
    saturationEngine.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AnalogSaturationAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
  #endif
}
#endif

void AnalogSaturationAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Update parameters
    saturationEngine.setDrive(*parameters.getRawParameterValue(DRIVE_ID));
    saturationEngine.setTone(*parameters.getRawParameterValue(TONE_ID));
    saturationEngine.setMix(*parameters.getRawParameterValue(MIX_ID));
    saturationEngine.setCircuitType(static_cast<int>(*parameters.getRawParameterValue(CIRCUIT_TYPE_ID)));
    saturationEngine.setModelType(static_cast<int>(*parameters.getRawParameterValue(MODEL_TYPE_ID)));

    // Process audio
    saturationEngine.processBlock(buffer);
}

//==============================================================================
bool AnalogSaturationAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AnalogSaturationAudioProcessor::createEditor()
{
    return new AnalogSaturationAudioProcessorEditor (*this);
}

//==============================================================================
void AnalogSaturationAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AnalogSaturationAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogSaturationAudioProcessor();
}
