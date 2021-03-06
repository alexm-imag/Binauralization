/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BinauralizationAudioProcessorEditor::BinauralizationAudioProcessorEditor (BinauralizationAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    DirButton.onClick = [this] {openIRdirectory(); };
    DirButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    DirButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(DirButton);

    ConvButton.onClick = [this] {toggleConvolution(); };
    ConvButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    ConvButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(ConvButton);

    // onDrag only changes slider value after releasing the slider
    // for a continuous filter change a different approach has to be chosen, but this would also require
    // are more complex filter transition process
    HRTF_Slider.onDragEnd = [this] {audioProcessor.hrtf_buffer.sel = HRTF_Slider.getValue(); };
    HRTF_Slider.setSliderStyle(Slider::Rotary);
    HRTF_Slider.setRange(0, 359, 1);
    HRTF_Slider.setTextBoxStyle(Slider::TextBoxBelow, 1, 50, 20);
    addAndMakeVisible(HRTF_Slider);

    SineButton.onClick = [this] {toggleSine(); };
    SineButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    SineButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(SineButton);

    NoiseButton.onClick = [this] {toggleNoise(); };
    NoiseButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    NoiseButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(NoiseButton);

}

BinauralizationAudioProcessorEditor::~BinauralizationAudioProcessorEditor()
{
}

//==============================================================================
void BinauralizationAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Binauralization 1.0", getLocalBounds(), juce::Justification::centredTop, 1);
}

void BinauralizationAudioProcessorEditor::resized()
{

    // Xpos, Ypos, Xdim, Ydim
    ConvButton.setBounds(200, 75, 100, 50);
    DirButton.setBounds(100, 75, 100, 50);
    HRTF_Slider.setBounds(150, 125, 100, 100);
    SineButton.setBounds(100, 230, 100, 50);
    NoiseButton.setBounds(200, 230, 100, 50);

}

void BinauralizationAudioProcessorEditor::openIRdirectory() {

    // let user select an IR dir / or select all files (not very user friendly...)
    FileChooser selector("Choose IR directory", File::getSpecialLocation(File::userDesktopDirectory));

    // check if something has been selected
    if (selector.browseForMultipleFilesToOpen()) {

        Array<File> files;
        AudioFormatManager ir_manager;
        AudioFormatReader* ir_reader = NULL;

        // get files from FileChooser
        files = selector.getResults();

        // write first files entry onto file_ptr
        File currentFile = files.getFirst();

        // register .wav and .aiff format
        ir_manager.registerBasicFormats();
        // create audio reader according to first audio file
        ir_reader = ir_manager.createReaderFor(currentFile);

        // if an invalid file format has been read, ir_manager returns a NULL pointer
        if (ir_reader == NULL) {
            DBG("Invalid format");
            return;
        }

        // only set flag, after it has been verified, that a file has been selected and the format is supported.
        // usage of FFT is deactivated in ProcessBlock during loading of new HRTF, so new collisons occur
        audioProcessor.ir_ready = false;

        audioProcessor.hrtf_buffer.num_samples = ir_reader->lengthInSamples;
        audioProcessor.hrtf_buffer.num_hrtfs = files.size();
        
        // free allocated space for hrtf_buffer
        if (audioProcessor.hrtf_buffer.left != NULL) {
            free(audioProcessor.hrtf_buffer.left);
            free(audioProcessor.hrtf_buffer.right);
        }

        // set k to a power of 2 while fulfilling k >= M + N - 1 
        audioProcessor.set_padding_size(audioProcessor.n, audioProcessor.hrtf_buffer.num_samples);

        // allocate space for x HRFT spectra
        audioProcessor.hrtf_buffer.left = (fftwf_complex**)malloc(sizeof(fftwf_complex*) * files.size());
        audioProcessor.hrtf_buffer.right = (fftwf_complex**)malloc(sizeof(fftwf_complex*) * files.size());
        for (int i = 0; i < audioProcessor.hrtf_buffer.num_hrtfs; i++) {
            audioProcessor.hrtf_buffer.left[i] = fftwf_alloc_complex(audioProcessor.k);
            audioProcessor.hrtf_buffer.right[i] = fftwf_alloc_complex(audioProcessor.k);
        }       
      

        // create temporary AudioBuffer of approriate size
        AudioBuffer<float> tmp_buffer(2, audioProcessor.k);
        tmp_buffer.clear();
        
        // iterate through array of files
        for (int i = 0; i < files.size(); i++) {
            currentFile = files.getReference(i);

            // first reader gets written twice, which isn't pretty but ok
            ir_reader = ir_manager.createReaderFor(currentFile);

            // copy reader data to float AudioBuffer
            ir_reader->read(&tmp_buffer, 0, ir_reader->lengthInSamples, 0, 1, 1);

            // perform fft on both channels for each HRTF file and store result in hrtf_buffer
            audioProcessor.perform_fft(audioProcessor.k, tmp_buffer.getWritePointer(0), audioProcessor.hrtf_buffer.left[i]);
            audioProcessor.perform_fft(audioProcessor.k, tmp_buffer.getWritePointer(1), audioProcessor.hrtf_buffer.right[i]);

        }

        DBG("Dir loaded");

        audioProcessor.ir_ready = true;
        audioProcessor.ir_update = true;

        return;
    }

    DBG("Loading failed");

}

void BinauralizationAudioProcessorEditor::toggleConvolution() {
    
    if (!audioProcessor.ir_ready) {
        DBG("NO IR AVAILABLE!");
        return;
    }
    
    if (!audioProcessor.performConv) {
        audioProcessor.performConv = true;
        ConvButton.setButtonText("Conv Active");
    }
    else {
        audioProcessor.performConv = false;
        ConvButton.setButtonText("Conv Inactive");
    }

}

void BinauralizationAudioProcessorEditor::toggleSine() {

    if (audioProcessor.sineFlag) {
        audioProcessor.sineFlag = false;
        SineButton.setButtonText("Sine Inactive");
    }

    else {
        audioProcessor.sineFlag = true;
        audioProcessor.noiseFlag = false;
        SineButton.setButtonText("Sine Active");
        NoiseButton.setButtonText("Noise Inactive");
    }
        
}

void BinauralizationAudioProcessorEditor::toggleNoise() {

    if (audioProcessor.noiseFlag) {
        audioProcessor.noiseFlag = false;
        NoiseButton.setButtonText("Noise Inactive");
    }

    else {
        audioProcessor.noiseFlag = true;
        audioProcessor.sineFlag = false;
        NoiseButton.setButtonText("Noise Active");
        SineButton.setButtonText("Sine Inactive");
    }

}