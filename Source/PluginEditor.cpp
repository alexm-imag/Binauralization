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

    HRTF_Slider.onDragEnd = [this] {audioProcessor.hrtf_sel = HRTF_Slider.getValue(); };
    HRTF_Slider.setSliderStyle(Slider::Rotary);
    HRTF_Slider.setRange(0, 359, 1);
    HRTF_Slider.setTextBoxStyle(Slider::TextBoxBelow, 1, 50, 20);
    addAndMakeVisible(HRTF_Slider);

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
    ConvButton.setBounds(200, 75, 100, 50);
    DirButton.setBounds(100, 75, 100, 50);
    HRTF_Slider.setBounds(150, 125, 100, 100);

}

void BinauralizationAudioProcessorEditor::openIRdirectory() {

    DBG("clicked");

    // delete old entries upon re-opening

    audioProcessor.ir_ready = false;

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

        audioProcessor.hrtf_len = ir_reader->lengthInSamples;
        
        // change to if != NULL delete everything and re-allocate (but this will do for now)
        // currently old data will simply be overwritten (which of course would lead to problmems with the addition of multiple HRTF datasets)
        // WARNING: re-opening because of incomplete load (e.g 357 instead of 360) leads to stack overflow > re-allocate mem at every "open dir" issue, or add mem to buffer!
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