/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "fftw3.h"

#define REAL 0
#define IMAG 1

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
    int  store_ir_spectrum(int n, int m);
    void fftw_convolution(int n, float* input1, float* input2, float* output);
    void fftw_convolution(int n, float* input1, fftwf_complex* input2, float* output);
    void perform_fft(int n, float* input, fftwf_complex* output);
    void perform_ifft(int n, fftwf_complex* input, float* output);
    void normalize(int n, float* data);

    juce::AudioBuffer<float> ir_buffer;
    bool ir_update = false;
    bool ir_ready = false;
    bool performConv = false;
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralizationAudioProcessor)

   fftwf_plan plan;
   fftwf_complex** ir_spectrum = NULL;
   // [MEM][CHANNEL][SAMPLES]
   float*** overlap_buffer = NULL;
   float* test_buf = NULL;
   int K = 0;
   int MEM = 0;
    
};
