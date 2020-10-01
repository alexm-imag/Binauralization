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
    
     // number of samples inside each buffer
     n = buffer.getNumSamples();
     // input data
     auto* channelData = buffer.getWritePointer (0); 
     // output left
     auto* channelLeft = buffer.getWritePointer(0);
     // outpur right
     auto* channelRight = buffer.getWritePointer(1);

     // use sine test-tone
     if (sineFlag) {
         if (!sineInit) {
             sine = (float*)malloc(sizeof(float) * n);
             // set f to n * 93.75 to have a complete wave inside sine
             double f = 375;
             for (int i = 0; i < n; i++) {
                 sine[i] = 0.473 * cos(2 * juce::double_Pi * f * i / 48000.);   
             }
             sineInit = true;
         }
         memcpy(channelData, sine, sizeof(float) * n);
     }

    // when a new set of IRs is loaded, the overlap_add_buffer has to be allocated accordingly
    if (ir_update) {
        // free previously allocated memory (not really efficient but ok for this case, since this should only occur rarely)
        if (overlap_buffer_left != NULL) {
            for (int i = 0; i < MEM; i++) {
                free(overlap_buffer_left[i]);
                free(overlap_buffer_right[i]);
            }
            free(overlap_buffer_left);
            free(overlap_buffer_right);
        }

        // MEM holds the number of iterations, for which an old convolution result has to be hold to perform the overlap add convolution
        MEM = (k / n > 2) ? k / n : 2;

        // allocate space for overlap add buffer
        overlap_buffer_left = (float**)malloc(sizeof(float*) * MEM);
        overlap_buffer_right = (float**)malloc(sizeof(float*) * MEM);

        for (int i = 0; i < MEM; i++) {
             overlap_buffer_left[i] = (float*)malloc(sizeof(float) * (k+2));
             overlap_buffer_right[i] = (float*)malloc(sizeof(float) * (k+2));
        }

        DBG("IR UPDATE DONE");
        ir_update = false;
    }
            
    // perform convolution with loaded impulse response
    if (!ir_update && ir_ready && performConv) {

        // get current HRTF selection from UI-Slider
        filter_sel = hrtf_buffer.sel;
        
        // perform fft-based convolution
        // write inputData into overlap_buffers
        memcpy(overlap_buffer_left[0], channelData, (sizeof(float) * n));
        memcpy(overlap_buffer_right[0], channelData, (sizeof(float) * n));

        // fill space from n to k with zeroes
        for (int i = n; i < k; i++) {
            overlap_buffer_left[0][i] = 0.;
            overlap_buffer_right[0][i] = 0.;
        }
        
        // perform fft-based convolution
        fftw_convolution(k, overlap_buffer_left[0], hrtf_buffer.left[filter_sel], overlap_buffer_left[0]);
        fftw_convolution(k, overlap_buffer_right[0], hrtf_buffer.right[filter_sel], overlap_buffer_right[0]);
        
        // write first block into channel Data
        memcpy(channelLeft, overlap_buffer_left[0], (sizeof(float) * n));
        memcpy(channelRight, overlap_buffer_right[0], (sizeof(float) * n));
                
        // overlap and add
        for (int i = 1; i < MEM; i++) {
            for (int j = 0; j < k-n; j++) {
                channelLeft[j] += overlap_buffer_left[i][j + (n * i)];
                channelRight[j] += overlap_buffer_right[i][j + (n * i)];
            }
            // shuffle through overlap_buffer
            memcpy(overlap_buffer_left[i], overlap_buffer_left[i - 1], sizeof(float) * k);
            memcpy(overlap_buffer_right[i], overlap_buffer_right[i - 1], sizeof(float) * k);
        }    
    }
    else {
        memcpy(channelLeft, channelData, sizeof(float) * n);
        memcpy(channelRight, channelData, sizeof(float) * n);
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

    // faster if one plan of this param set (no input/output) already exists?
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

int BinauralizationAudioProcessor::set_padding_size(int n, int m) {

    // FFTW is more efficient with a power of 2, 3, 5, ... (using 2 for simplicity)
    //k = (n + m - 1) % 2 ? n + m : n + m - 1;

    int p = log2(m + n - 1);
    k = 1 << (p + 1);
    
    return k;
}

void BinauralizationAudioProcessor::fftw_convolution(int n ,float* input1, float* input2, float* output) {

    // FFTW gives N/2+1 complex values as a result of a N-sized real-valued FFT
    int m = n / 2 + 1;

    fftwf_complex* spec1 = fftwf_alloc_complex(m);
    fftwf_complex* spec2 = fftwf_alloc_complex(m);
    fftwf_complex* result = fftwf_alloc_complex(m);

    perform_fft(n, input1, spec1);
    perform_fft(n, input2, spec2);

    for (int i = 0; i < m; i++) {
        result[i][REAL] = spec1[i][REAL] * spec2[i][REAL] - spec1[i][IMAG] * spec2[i][IMAG];
        result[i][IMAG] = spec1[i][REAL] * spec2[i][IMAG] + spec1[i][IMAG] * spec2[i][REAL];
    }

    perform_ifft(n, result, output);

    normalize(n, output);

    fftwf_free(spec1);
    fftwf_free(spec2);
    fftwf_free(result);

}

void BinauralizationAudioProcessor::fftw_convolution(int n ,float* input1, fftwf_complex* input2, float* output) {

    // FFTW gives N/2+1 complex values as a result of a N-sized real-valued FFT
    int m = n / 2 + 1;

    fftwf_complex* spec1 = fftwf_alloc_complex(m);
    fftwf_complex* result = fftwf_alloc_complex(m);

    perform_fft(n, input1, spec1);
    
    for (int i = 0; i < m; i++) {       
        result[i][REAL] = spec1[i][REAL] * input2[i][REAL] - spec1[i][IMAG] * input2[i][IMAG];
        result[i][IMAG] = spec1[i][REAL] * input2[i][IMAG] + spec1[i][IMAG] * input2[i][REAL];
    }

    
    perform_ifft(n, result, output);

    normalize(n, output);

    fftwf_free(spec1);
    fftwf_free(result);

}

void BinauralizationAudioProcessor::fftw_convolution(int n, fftwf_complex* input1, fftwf_complex* input2, float* output) {

    // FFTW gives N/2+1 complex values as a result of a N-sized real-valued FFT
    int m = n / 2 + 1;

    fftwf_complex* result = fftwf_alloc_complex(m);

    for (int i = 0; i < m; i++) {
        result[i][REAL] = input1[i][REAL] * input2[i][REAL] - input1[i][IMAG] * input2[i][IMAG];
        result[i][IMAG] = input1[i][REAL] * input2[i][IMAG] + input1[i][IMAG] * input2[i][REAL];
    }

    perform_ifft(n, result, output);

    normalize(n, output);

    fftwf_free(result);


}
