/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BinauralizationAudioProcessor::BinauralizationAudioProcessor()
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
{
}

BinauralizationAudioProcessor::~BinauralizationAudioProcessor()
{
}

//==============================================================================
const juce::String BinauralizationAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BinauralizationAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BinauralizationAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BinauralizationAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BinauralizationAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BinauralizationAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BinauralizationAudioProcessor::getCurrentProgram()
{
    return 0;
}

void BinauralizationAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String BinauralizationAudioProcessor::getProgramName (int index)
{
    return {};
}

void BinauralizationAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void BinauralizationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void BinauralizationAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BinauralizationAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
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

void BinauralizationAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();


    int filter_sel = 0;

    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        juce::dsp::Complex<float> tmp;

        int n = buffer.getNumSamples();
        static int current_hrtf = 0;

        static bool init = true;

        static dsp::Convolution conv;
        static dsp::ProcessSpec conv_spec;
        dsp::AudioBlock<float> conv_block(buffer);
        dsp::ProcessorChain<juce::dsp::Convolution> conv_chain;

        // init stuff
        if (init) {
            conv_spec.numChannels = 2;
            conv_spec.sampleRate = 48000;
            conv_spec.maximumBlockSize = 1024;  // estimation...

            init = false;
        }

        if (performConv) {
            // if hrtf_sel has been altered, load new HRTF
            if (current_hrtf != hrtf_sel) {
                current_hrtf = hrtf_sel;
                conv.prepare(conv_spec);
                conv.loadImpulseResponse(hrtf_buffer[hrtf_sel], 48000., dsp::Convolution::Stereo::yes, dsp::Convolution::Trim::no, dsp::Convolution::Normalise::no);
                DBG("SWAPPED HRTF");
            }
            conv.process(dsp::ProcessContextReplacing<float>(conv_block));
            conv.reset();
        }
    }
}

//==============================================================================
bool BinauralizationAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* BinauralizationAudioProcessor::createEditor()
{
    return new BinauralizationAudioProcessorEditor (*this);
}

//==============================================================================
void BinauralizationAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BinauralizationAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BinauralizationAudioProcessor();
}
