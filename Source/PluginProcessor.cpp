#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
_01_panda_filterAudioProcessor::_01_panda_filterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
    , MyFilter()
    , parameters(*this, &undoManager, juce::Identifier("APVTSTutorial"),//20210709_2
        {
            std::make_unique<juce::AudioParameterFloat>("cutoff",  // ID
                                                        "Cutoff",  // name
                                                         juce::NormalisableRange<float>(20.0f, 22000.0f),
                                                         1000.0f,
                                                         "cutoff",
                                                         juce::AudioProcessorParameter::genericParameter,
                                                         [](float value, int) {return juce::String(value, 0) + "Hz";}
                                                         ),
            std::make_unique<juce::AudioParameterFloat>("qParam",  // ID
                                                        "QParam",  // name
                                                        juce::NormalisableRange<float>(0.01f, 4.0f),
                                                        0.70710678f,
                                                        "Qparam",
                                                        juce::AudioProcessorParameter::genericParameter,
                                                        [](float value, int) {return juce::String(value, 3);}
                                                        ),
            std::make_unique<juce::AudioParameterInt>("slope",     // parameterID
                                                        "Slope",   // parameter name
                                                        1,         // min
                                                        3,         // max
                                                        1)         // default
                                                        ,
            std::make_unique<juce::AudioParameterFloat>("inputGain",
                                                        "InputGain",
                                                         juce::NormalisableRange<float>(-100.0f, 12.0f),
                                                         0.0f,
                                                         "input",
                                                         juce::AudioProcessorParameter::genericParameter,
                                                         [](float value, int) {return juce::String(value, 1) + " dB";}
                                                         ),
            std::make_unique<juce::AudioParameterFloat>("outputGain",
                                                        "OutputGain",
                                                         juce::NormalisableRange<float>(-100.0f, 12.0f),
                                                         0.0f,
                                                         "output",
                                                         juce::AudioProcessorParameter::genericParameter,
                                                         [](float value, int) {return juce::String(value, 1) + " dB";}
                                                         ),
        })
{
    cutoffParam = parameters.getRawParameterValue("cutoff");
    QParam = parameters.getRawParameterValue("qParam");
    slope = parameters.getRawParameterValue("slope");

    inputGainDb = parameters.getRawParameterValue("inputGain");
    outputGainDb = parameters.getRawParameterValue("outputGain");

    MyFilter.setCutoffParam(cutoffParam);
    MyFilter.setQParam(QParam);
    MyFilter2.setCutoffParam(cutoffParam);
    MyFilter2.setQParam(QParam);
    MyFilter3.setCutoffParam(cutoffParam);
    MyFilter3.setQParam(QParam);
}

_01_panda_filterAudioProcessor::~_01_panda_filterAudioProcessor()
{
}

//==============================================================================
const juce::String _01_panda_filterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool _01_panda_filterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool _01_panda_filterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool _01_panda_filterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double _01_panda_filterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int _01_panda_filterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int _01_panda_filterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void _01_panda_filterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String _01_panda_filterAudioProcessor::getProgramName (int index)
{
    return {};
}

void _01_panda_filterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void _01_panda_filterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    MyFilter.prepareToPlay(sampleRate, samplesPerBlock);
    MyFilter2.prepareToPlay(sampleRate, samplesPerBlock);
    MyFilter3.prepareToPlay(sampleRate, samplesPerBlock);

    float val = *inputGainDb;
    previousInputGain = juce::Decibels::decibelsToGain(val);
    float val2 = *outputGainDb;
    previousOutputGain = juce::Decibels::decibelsToGain(val2);
}

void _01_panda_filterAudioProcessor::releaseResources()
{
    MyFilter.releaseResources();
    MyFilter2.releaseResources();
    MyFilter3.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool _01_panda_filterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void _01_panda_filterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    //
    float currentInputGain = *inputGainDb;
    float inputGainVal = juce::Decibels::decibelsToGain(currentInputGain);

    if (inputGainVal == previousInputGain)
    {
        buffer.applyGain(inputGainVal);
    }
    else
    {
        //Input gain processing, smooth volume change
        buffer.applyGainRamp(0, buffer.getNumSamples(), previousInputGain, inputGainVal);
        previousInputGain = inputGainVal;
    }

    //Vary the number of filters to process with the slope parameter
    MyFilter.processBlock(buffer, midiMessages);
    if (*slope == 2) {
        MyFilter2.processBlock(buffer, midiMessages);
    }
    else if (*slope == 3) {
        MyFilter2.processBlock(buffer, midiMessages);
        MyFilter3.processBlock(buffer, midiMessages);
    }

    //Spectrum Analyzer Buffer Processing
    if (buffer.getNumChannels() > 0)
    {
        auto* channelData = buffer.getReadPointer(0);

        for (auto i = 0; i < buffer.getNumSamples(); ++i)
            pushNextSampleIntoFifo(channelData[i]);
    }

    //Volume change process for output gain
    float currentOutputGain = *outputGainDb;
    float outputGainVal = juce::Decibels::decibelsToGain(currentOutputGain);
    if (outputGainVal == previousOutputGain)
    {
        buffer.applyGain(outputGainVal);
    }
    else
    {
        //Smoothly change the volume.
        buffer.applyGainRamp(0, buffer.getNumSamples(), previousOutputGain, outputGainVal);
        previousOutputGain = outputGainVal;
    }
}

//==============================================================================
bool _01_panda_filterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* _01_panda_filterAudioProcessor::createEditor()
{
    return new _01_panda_filterAudioProcessorEditor (*this, parameters, undoManager);
}

void _01_panda_filterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    //Process to hold the parameters of the plugin
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void _01_panda_filterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    //Process to read the parameters of the plugin
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new _01_panda_filterAudioProcessor();
}