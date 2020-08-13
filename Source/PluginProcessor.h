/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "fftw3.h"

//==============================================================================
/**
*/
class BinauralizationAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    BinauralizationAudioProcessor();
    ~BinauralizationAudioProcessor() override;

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


    //---------- Binauralization --------------------------------------------------
    void loadIRfile();
    void fftw_convolution(juce::AudioBuffer<float>&);
    void perform_fft(int n, float* input, fftwf_complex* output);
    void perform_ifft(int n, fftwf_complex* input, float* output);
    void normalize(int n, float* data);

    juce::AudioBuffer<float> IRbufferPtr;
    bool ir_update = false;
    bool ir_ready = false;
    bool performConv = false;
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralizationAudioProcessor)

   fftwf_plan plan;
   fftwf_complex** ir_spectrum;
    
};
