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

    //==============================================================================libfftw3f-3
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
    void fftw_convolution(int n, float* input1, float* input2, float* output);
    void fftw_convolution(int n, float* input1, fftwf_complex* input2, float* output);
    void perform_fft(int n, float* input, fftwf_complex* output);
    void perform_ifft(int n, fftwf_complex* input, float* output);
    void normalize(int n, float* data);
    int get_padding_size(int n, int m);


    bool ir_update = false;
    bool ir_ready = false;
    bool performConv = false;

    int n = 0;
    int k = 0;

    struct hrtf_buffer_sc {
        fftwf_complex** left = NULL;
        fftwf_complex** right = NULL;
        int num_hrtfs;
        int num_samples;
        int sel;
    } hrtf_buffer;
    
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BinauralizationAudioProcessor)

   // [MEM][SAMPLES]
   float** overlap_buffer_left = NULL;
   float** overlap_buffer_right = NULL;

   int MEM = 0;
    
};
