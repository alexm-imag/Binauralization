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

    // maybe add a way so the position data gets updated during longer drags and not just at the end?
    HRTF_Slider.onDragEnd = [this] {audioProcessor.hrtf_buffer.sel = HRTF_Slider.getValue(); };
    HRTF_Slider.setSliderStyle(Slider::Rotary);
    HRTF_Slider.setRange(0, 359, 1);
    HRTF_Slider.setTextBoxStyle(Slider::TextBoxBelow, 1, 50, 20);
    addAndMakeVisible(HRTF_Slider);

    OpenButton.onClick = [this] {openIRfile(); };
    OpenButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    OpenButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(OpenButton);

    SineButton.onClick = [this] {toggleSine(); };
    SineButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    SineButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(SineButton);

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
    g.drawFittedText ("Binauralization 0.8", getLocalBounds(), juce::Justification::centredTop, 1);
}

void BinauralizationAudioProcessorEditor::resized()
{

    // Xpos, Ypos, Xdim, Ydim
    ConvButton.setBounds(200, 75, 100, 50);
    DirButton.setBounds(100, 75, 100, 50);
    HRTF_Slider.setBounds(150, 125, 100, 100);
    OpenButton.setBounds(200, 230, 100, 50);
    SineButton.setBounds(100, 230, 100, 50);

}

void BinauralizationAudioProcessorEditor::openIRdirectory() {

    // let user select an IR dir / or select all files (not very user friendly...)
    FileChooser selector("Choose IR directory", File::getSpecialLocation(File::userDesktopDirectory));

    // check if something has been selected
    if (selector.browseForMultipleFilesToOpen()) {

        int i = 0;
        Array<File> files;
        AudioFormatManager ir_manager;
        AudioFormatReader* ir_reader = NULL;

        // get files from FileChooser
        files = selector.getResults();

        // write first files entry onto file_ptr
        File* file_ptr = files.begin();

        // register .wav and .aiff format
        ir_manager.registerBasicFormats();
        // create audio reader according to first audio file
        ir_reader = ir_manager.createReaderFor(*file_ptr);

        // if an invalid file format has been read, ir_manager returns a NULL pointer
        if (ir_reader == NULL) {
            DBG("Invalid format");
            return;
        }

        // only set flag, after it has been verified, that a file has been selected and the format is supported.
        // Usage of FFT is deactivated in ProcessBlock during loading of new HRTF, so new collisons occur
        audioProcessor.ir_ready = false;

        audioProcessor.hrtf_buffer.num_samples = ir_reader->lengthInSamples;
        audioProcessor.hrtf_buffer.num_hrtfs = files.size();
        
        // free allocated space for hrtf_buffer
        if (audioProcessor.hrtf_buffer.left != NULL) {
            free(audioProcessor.hrtf_buffer.left);
            free(audioProcessor.hrtf_buffer.right);
        }

        // k >= M + N - 1 (length IR, length data - 1)
        // k is a class member of audioProcessor
        audioProcessor.set_padding_size(audioProcessor.n, audioProcessor.hrtf_buffer.num_samples);

        // allocate space for x HRFT spectra
        audioProcessor.hrtf_buffer.left = (fftwf_complex**)malloc(sizeof(fftwf_complex*) * files.size());
        audioProcessor.hrtf_buffer.right = (fftwf_complex**)malloc(sizeof(fftwf_complex*) * files.size());
        for (i = 0; i < audioProcessor.hrtf_buffer.num_hrtfs; i++) {
            audioProcessor.hrtf_buffer.left[i] = fftwf_alloc_complex(audioProcessor.k);
            audioProcessor.hrtf_buffer.right[i] = fftwf_alloc_complex(audioProcessor.k);
        }       
      

        // create temporary AudioBuffer of approriate size
        AudioBuffer<float> tmp_buffer(2, audioProcessor.k);
        tmp_buffer.clear();
        
        // iterate through array of files
        i = 0;
        do {
            // first reader gets written twice, which isn't pretty but ok
            ir_reader = ir_manager.createReaderFor(*file_ptr);

            // copy reader data to float AudioBuffer
            ir_reader->read(&tmp_buffer, 0, ir_reader->lengthInSamples, 0, 1, 1);

            // perform fft on both channels for each HRTF file and store result in hrtf_buffer
            audioProcessor.perform_fft(audioProcessor.k, tmp_buffer.getWritePointer(0), audioProcessor.hrtf_buffer.left[i]);
            audioProcessor.perform_fft(audioProcessor.k, tmp_buffer.getWritePointer(1), audioProcessor.hrtf_buffer.right[i]);

        } while (*(file_ptr + i++) != files.getLast());

        DBG("dir loaded");

        audioProcessor.ir_ready = true;
        audioProcessor.ir_update = true;

        return;
    }

    DBG("loading failed");

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
        SineButton.setButtonText("Sine Active");
    }

    else {
        audioProcessor.sineFlag = true;
        SineButton.setButtonText("Sine Inactive");
    }
        
}


void BinauralizationAudioProcessorEditor::openIRfile() {

    DBG("clicked");

    // let user select an IR file
    FileChooser selector("Choose IR file", File::getSpecialLocation(File::userDesktopDirectory), "*wav");

    // check if something has been selected
    if (selector.browseForFileToOpen()) {
        File ir_file;
        // store file in IRfile
        ir_file = selector.getResult();

        // allow .wav and .aiff formats
        AudioFormatManager ir_manager;

        ir_manager.registerBasicFormats();

        AudioFormatReader* ir_reader = ir_manager.createReaderFor(ir_file);

        AudioBuffer<float> tmp_buffer(2, ir_reader->lengthInSamples);

        ir_reader->read(&tmp_buffer, 0, ir_reader->lengthInSamples, 0, 1, 1);

        // copy AudioBuffer content into audioProcessor context
        audioProcessor.ir_buffer = tmp_buffer;
        // set IRhasbeenLoaded flag
        audioProcessor.ir_flag = true;

        DBG("file loaded");
        return;
    }

    DBG("loading failed");
}