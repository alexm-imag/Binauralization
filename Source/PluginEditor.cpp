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

    OpenButton.onClick = [this] {openIRfile(); };
    OpenButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    OpenButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(OpenButton);

    DirButton.onClick = [this] {openIRdirectory(); };
    DirButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    DirButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(DirButton);

    ConvButton.onClick = [this] {toggleConvolution(); };
    ConvButton.setColour(TextButton::buttonColourId, Colour(0xff79ed7f));
    ConvButton.setColour(TextButton::textColourOffId, Colours::black);
    addAndMakeVisible(ConvButton);
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
    g.drawFittedText ("Binauralization 0.6", getLocalBounds(), juce::Justification::centredTop, 1);
}

void BinauralizationAudioProcessorEditor::resized()
{

    // Xpos, Ypos, Xdim, Ydim
    OpenButton.setBounds(100, 100, 100, 50);
    DirButton.setBounds(100, 150, 100, 50);
    ConvButton.setBounds(200, 100, 100, 50);

}

void BinauralizationAudioProcessorEditor::openIRfile() {

    DBG("clicked");

    audioProcessor.ir_update = false;

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
        audioProcessor.ir_update = true;

        DBG("file loaded");
        return;
    }

    DBG("loading failed");
}


void BinauralizationAudioProcessorEditor::openIRdirectory() {

    DBG("clicked");

    // delete old entries upon re-opening

    //audioProcessor.ir_update = false;

    // let user select an IR dir / or select all files (not very user friendly...)
    FileChooser selector("Choose IR directory", File::getSpecialLocation(File::userDesktopDirectory));

    // check if something has been selected
    if (selector.browseForMultipleFilesToOpen()) {
        
        Array<File> files;
        AudioFormatManager ir_manager;
        AudioFormatReader* ir_reader;

        // get files from FileChooser
        files = selector.getResults();

        // write first files entry onto file_ptr
        File* file_ptr = files.begin();

        // register .wav and .aiff format
        ir_manager.registerBasicFormats();
        // create audio reader according to first audio file
        // WARNING: this works only audio files are of the same length / dimension!
        ir_reader = ir_manager.createReaderFor(*file_ptr);
        
        // change to if != NULL delete everything and re-allocate (but this will do for now)
        if (audioProcessor.hrtf_buffer == NULL) {
            // allocate space for x HRFT spectra
            audioProcessor.hrtf_buffer = (fftwf_complex***)malloc(sizeof(fftwf_complex**) * files.size());
            // allocate two channles for each spectre (maybe just use 2 instead of ir_reader->numChannels, since other sizes will lead to problems!)
            for (int i = 0; i < files.size(); i++) {
                audioProcessor.hrtf_buffer[i] = (fftwf_complex**)malloc(sizeof(fftwf_complex*) * ir_reader->numChannels);
                for (int j = 0; j < ir_reader->numChannels; j++) {
                    audioProcessor.hrtf_buffer[i][j] = fftwf_alloc_complex(ir_reader->lengthInSamples);
                    audioProcessor.hrtf_buffer[i][j] = fftwf_alloc_complex(ir_reader->lengthInSamples);
                }
            }       
        }

        // create AudioBUffer of approriate size
        // WARNING: this also only works with HRFTs of constant length!
        AudioBuffer<float> tmp_buffer(2, ir_reader->lengthInSamples);
        
        // iterate through array of files
        for (int i = 0; *(file_ptr + i) != files.getLast(); i++) {
            // first reader gets written twice, which isn't pretty but ok
            ir_reader = ir_manager.createReaderFor(*file_ptr);

            // copy reader data to float AudioBuffer
            ir_reader->read(&tmp_buffer, 0, ir_reader->lengthInSamples, 0, 1, 1);


            //// NOTE: it seems that Processor and Editor run in different threads
            //// Make sure that perform_fft can't collide
            //// Deactivate fft in Processor while loading new files
            // perform fft on both channels for each HRTF file and store result in hrtf_buffer
            audioProcessor.perform_fft(ir_reader->lengthInSamples, tmp_buffer.getWritePointer(0), audioProcessor.hrtf_buffer[i][0]);
            audioProcessor.perform_fft(ir_reader->lengthInSamples, tmp_buffer.getWritePointer(1), audioProcessor.hrtf_buffer[i][1]);

        }

        
        //audioProcessor.ir_update = true;

        DBG("dir loaded");

        // clean up old fft-buffer at some point?
        // perform fft here?

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