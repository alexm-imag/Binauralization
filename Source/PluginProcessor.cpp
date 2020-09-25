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


    // when a new set of IRs is loaded, the overlap_add_buffer has to be allocated accordingly
    if (ir_update) {
        // free previously allocated memory (not really efficient but ok for this case, since this should only occur rarely)
        // only check one buffer, because there is no case where only one of them gets allocated
        if (overlap_buffer_left != NULL) {
            for (int i = 0; i < MEM; i++) {
                free(overlap_buffer_left[i]);
                free(overlap_buffer_right[i]);
            }
            free(overlap_buffer_left);
            free(overlap_buffer_right);
        }

        // k >= M + N - 1 (gets set in openIRdirectory())
        // MEM holds the number of iterations, for which an old convolution result has to be hold to perform the overlap add convolution
        //MEM = k / n;
        MEM = (k / n > 2) ? k / n : 2;

        // allocate space for overlap add buffer
        overlap_buffer_left = (float**)malloc(sizeof(float*) * MEM);
        overlap_buffer_right = (float**)malloc(sizeof(float*) * MEM);

        for (int i = 0; i < MEM; i++) {
             overlap_buffer_left[i] = (float*)malloc(sizeof(float) * k);
             overlap_buffer_right[i] = (float*)malloc(sizeof(float) * k);
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

        // some selections lead to a lot of noise... (e.g. 0, 4)

        // perform fft-based convolution
        fftw_convolution(k, overlap_buffer_left[0], hrtf_buffer.left[filter_sel], overlap_buffer_left[0]);
        fftw_convolution(k, overlap_buffer_right[0], hrtf_buffer.right[filter_sel], overlap_buffer_right[0]);

        // write first block into channel Data
        memcpy(channelLeft, overlap_buffer_left[0], (sizeof(float) * n));
        memcpy(channelRight, overlap_buffer_right[0], (sizeof(float) * n));
                
        // overlap and add
        for (int i = 1; i < MEM; i++) {
            for (int j = 0; j < n; j++) {
                channelLeft[j] += overlap_buffer_left[i][j + (n * i)];
                channelRight[j] += overlap_buffer_right[i][j + (n * i)];
            }
            // shuffle through overlap_buffer
            memcpy(overlap_buffer_left[i], overlap_buffer_left[i - 1], sizeof(float) * k);
            memcpy(overlap_buffer_right[i], overlap_buffer_right[i - 1], sizeof(float) * k);
        } 
       
    }
    
    else {
        if (ir_flag) {
            ir_flag = false;
            int m = ir_buffer.getNumSamples();
            //k = get_padding_size(n, m);
            //MEM = (k / n > 2) ? k / n : 2;
            int spec_len = k / 2 + 1;

            ir_left = fftwf_alloc_complex(spec_len);   // (k/2 + 1)
            ir_right = fftwf_alloc_complex(spec_len);

            ir_buffer.setSize(2, k, true);

            for (int i = m; i < k; i++) {
                ir_buffer.getWritePointer(0)[i] = 0;
                ir_buffer.getWritePointer(1)[i] = 0;
            }

            current_left = (float*)malloc(sizeof(float) * k);
            current_right = (float*)malloc(sizeof(float) * k);
            previous_left = (float*)malloc(sizeof(float) * k);
            previous_right = (float*)malloc(sizeof(float) * k);

            for (int i = 0; i < k; i++) {
                previous_left[i] = 0;
                previous_right[i] = 0;
            }

            perform_fft(k, ir_buffer.getWritePointer(0), ir_left);
            perform_fft(k, ir_buffer.getWritePointer(1), ir_right);
            //conv_flag = true;
        }

        int m = 128;
        // make k an even number
        k = (n + m - 1) % 2 ? n + m : n + m - 1;

        float* data1 = (float*)malloc(sizeof(float) * (k+2));
        float* data2 = (float*)malloc(sizeof(float) * (k+2));

        // use sine test-tone
        if (sineFlag) {
            if (!sineInit) {
                sine = (float*)malloc(sizeof(float) * k);
                double f = 375;
                for (int i = 0; i < n; i++) {
                    sine[i] = 0.473 * cos(2 * juce::double_Pi * f * i / 48000.);
                }
                sineInit = true;
            }
            memcpy(data1, sine, sizeof(float)* n);
            memcpy(data2, sine, sizeof(float)* n);
        }

        else {
            memcpy(data1, channelData, sizeof(float) * n);
            memcpy(data2, channelData, sizeof(float) * n);
        }


        for (int i = n; i < k+2; i++) {
            data1[i] = 0;
            data2[i] = 0;
        }
        int b = k / 2 + 1;
        fftwf_complex* tmp = fftwf_alloc_complex(b);
        fftwf_complex* tmp2 = fftwf_alloc_complex(b);
        perform_fft(k, data1, tmp);
        perform_fft(k, data2, tmp2);

        if (performConv) {
            for (int i = 0; i < k/2 + 1; i++) {
                tmp[i][REAL] = tmp[i][REAL] * ir_left[i][REAL] - tmp[i][IMAG] * ir_left[i][IMAG];
                tmp[i][IMAG] = tmp[i][REAL] * ir_left[i][IMAG] + tmp[i][IMAG] * ir_left[i][REAL];
                tmp2[i][REAL] = tmp2[i][REAL] * ir_right[i][REAL] - tmp2[i][IMAG] * ir_right[i][IMAG];
                tmp2[i][IMAG] = tmp2[i][REAL] * ir_right[i][IMAG] + tmp2[i][IMAG] * ir_right[i][REAL];
            }    
            perform_ifft(k, tmp, current_left);
            perform_ifft(k, tmp2, current_right);
            normalize(k, current_left);
            normalize(k, current_right);

            memcpy(data1, current_left, sizeof(float)* n);
            memcpy(data2, current_right, sizeof(float)* n);

            for (int i = 0; i < n; i++) {
                data1[i] += previous_left[i+n];
                data2[i] += previous_right[i+n];
            }

            memcpy(previous_left, current_left, sizeof(float) * k);
            memcpy(previous_right, current_right, sizeof(float) * k);
        }
        else {
            perform_ifft(k, tmp, data1);
            perform_ifft(k, tmp2, data2);
            normalize(k, data1);
            normalize(k, data2);
        }

        memcpy(channelLeft, data1, sizeof(float) * n);
        memcpy(channelRight, data2, sizeof(float) * n);

        free(data1);
        free(data2);
        fftwf_free(tmp);
        fftwf_free(tmp2);


        /*
        if (conv_flag) {
            float* data = (float*)malloc(sizeof(float) * k);
            memcpy(data, channelData, sizeof(float) * n);
            for (int i = n; i < k; i++)
                data[i] = 0;

            fftw_convolution(k, data, ir_left, current_left);
            fftw_convolution(k, data, ir_right, current_right);

            for (int i = 0; i < n; i++) {
                channelLeft[i] = current_left[i];
                channelRight[i] = current_right[i];
                channelLeft[i] += previous_left[i+n];
                channelRight[i] += previous_right[i+n];
            }

            memcpy(previous_left, current_left, sizeof(float)* k);
            memcpy(previous_right, current_right, sizeof(float)* k);
        }
        */

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

int BinauralizationAudioProcessor::get_padding_size(int n, int m) {

    int k = n + m - 1;
    // leaving this out leads to problems... probably within adding...
    if (k % n != 0)
        k += n - (k % n);
    
    return k;
}

void BinauralizationAudioProcessor::fftw_convolution(int n ,float* input1, float* input2, float* output) {

    int m = n / 2 + 1;

    fftwf_complex* spec1 = fftwf_alloc_complex(m);
    fftwf_complex* spec2 = fftwf_alloc_complex(m);

    perform_fft(n, input1, spec1);
    perform_fft(n, input2, spec2);

    for (int i = 0; i < m; i++) {
        spec1[i][REAL] = spec1[i][REAL] * spec2[i][REAL] - spec1[i][IMAG] * spec2[i][IMAG];
        spec1[i][IMAG] = spec1[i][REAL] * spec2[i][IMAG] + spec1[i][IMAG] * spec2[i][REAL];
    }

    perform_ifft(n, spec1, output);

    normalize(n, output);

    fftwf_free(spec1);
    fftwf_free(spec2);

}

void BinauralizationAudioProcessor::fftw_convolution(int n ,float* input1, fftwf_complex* input2, float* output) {

    int m = n / 2 + 1;

    fftwf_complex* spec1 = fftwf_alloc_complex(m);

    perform_fft(n, input1, spec1);
    
    for (int i = 0; i < m; i++) {       
        spec1[i][REAL] = spec1[i][REAL] * input2[i][REAL] - spec1[i][IMAG] * input2[i][IMAG];
        spec1[i][IMAG] = spec1[i][REAL] * input2[i][IMAG] + spec1[i][IMAG] * input2[i][REAL];
    }

    
    perform_ifft(n, spec1, output);

    normalize(n, output);

    fftwf_free(spec1);

}
