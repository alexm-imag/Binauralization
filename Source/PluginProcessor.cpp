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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        // move outside of for loop
        int n = buffer.getNumSamples();
        auto* channelData = buffer.getWritePointer (channel);

        // pre-transform ir and store
        if (ir_update) {
            
            if (overlap_buffer != NULL)
                free(overlap_buffer);

            K = store_ir_spectrum(n, ir_buffer.getNumSamples());

            MEM = K / n;
            
            // allocate space for overlap add buffer
            overlap_buffer = (float***)malloc(sizeof(float**) * MEM);
            for (int i = 0; i < MEM; i++) {
                overlap_buffer[i] = (float**)malloc(sizeof(float*) * 2);
                for (int j = 0; j < 2; j++) {
                    overlap_buffer[i][j] = (float*)malloc(sizeof(float) * K);
                }
            }
        }
            
        // perform convolution with loaded impulse response
        if (ir_ready && performConv) {

            // perform fft-based convolution
            for (int ch = 0; ch < 2; ch++) {
                // make sure over_lap_buffer is initialized with zeroes...
                memcpy(overlap_buffer[0][ch], channelData, sizeof(float) * n);

                for (int i = n; i < K; i++) {
                    overlap_buffer[0][ch][i] = 0.;
                }

                channelData = buffer.getWritePointer(ch);

                fftw_convolution(K, overlap_buffer[0][ch], ir_spectrum[ch], overlap_buffer[0][ch]);

                // write first block into channel Data
                memcpy(channelData, overlap_buffer[0][ch], sizeof(float) * n);

                // overlap add
                for (int i = 1; i < MEM; i++) {
                    for (int j = 0; j < n; j++) {
                        channelData[j] += overlap_buffer[i][ch][j + (n * i)];
                    }
                    // shuffle through overlap_buffer
                    memcpy(overlap_buffer[i][ch], overlap_buffer[i - 1][ch], sizeof(float) * K);
                }
            }

        }

        // perform fft and ifft (unaltered signal)
        else {
            fftwf_complex* tmp = fftwf_alloc_complex(n);

            perform_fft(n, channelData, tmp);
            perform_ifft(n, tmp, channelData);
            normalize(n, channelData);

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


void BinauralizationAudioProcessor::perform_fft(int n, float* input, fftwf_complex* output) {

    fftwf_plan plan = fftwf_plan_dft_r2c_1d(n, input, output, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
    fftwf_cleanup();

}

void BinauralizationAudioProcessor::perform_ifft(int n, fftwf_complex* input, float* output) {

    fftwf_plan plan = fftwf_plan_dft_c2r_1d(n, input, output, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
    fftwf_cleanup();

}

void BinauralizationAudioProcessor::normalize(int n, float* data) {

    for (int i = 0; i < n; i++) {
        data[i] /= n;
    }
}

int BinauralizationAudioProcessor::store_ir_spectrum(int n, int m) {

    float* tmp_padded;
    int K = n + m - 1;

    ir_update = false;

    if (K % n != 0)
        K += n - (K % n);

    if (ir_spectrum != NULL)
        free(ir_spectrum);

    ir_spectrum = new fftwf_complex * [2];
    ir_spectrum[0] = fftwf_alloc_complex(K);
    ir_spectrum[1] = fftwf_alloc_complex(K);

    tmp_padded = new float[K] {0};

    for (int i = 0; i < 2; i++) {
        memcpy(tmp_padded, ir_buffer.getWritePointer(i), sizeof(float)*m);
        perform_fft(K, tmp_padded, ir_spectrum[i]);
    }

    delete[] tmp_padded;

    ir_ready = true;

    return K;

}

void BinauralizationAudioProcessor::fftw_convolution(int n, float* input1, float* input2, float* output) {


    fftwf_complex* spec1 = fftwf_alloc_complex(n);
    fftwf_complex* spec2 = fftwf_alloc_complex(n);

    perform_fft(n, input1, spec1);
    perform_fft(n, input2, spec2);

    for (int i = 0; i < n; i++) {
        spec1[i][REAL] = spec1[REAL][i] * spec2[REAL][i] - spec1[IMAG][i] * spec2[IMAG][i];
        spec1[i][IMAG] = spec1[REAL][i] * spec2[IMAG][i] + spec1[IMAG][i] * spec2[REAL][i];
    }

    perform_ifft(n, spec1, output);

    normalize(n, output);

}

void BinauralizationAudioProcessor::fftw_convolution(int n, float* input1, fftwf_complex* input2, float* output) {

    fftwf_complex* spec1 = fftwf_alloc_complex(n);

    perform_fft(n, input1, spec1);

    for (int i = 0; i < n; i++) {
        spec1[i][REAL] = spec1[REAL][i] * input2[REAL][i] - spec1[IMAG][i] * input2[IMAG][i];
        spec1[i][IMAG] = spec1[REAL][i] * input2[IMAG][i] + spec1[IMAG][i] * input2[REAL][i];
    }

    perform_ifft(n, spec1, output);

    normalize(n, output);

}
