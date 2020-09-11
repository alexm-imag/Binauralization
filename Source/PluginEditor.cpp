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

        int i = 0;
        AudioFormatManager ir_manager;
        AudioFormatReader* ir_reader;
        AudioBuffer<float> tmp;

        // get files from FileChooser
        Array<File> files = selector.getResults();

        // write first files entry onto file_ptr
        File* file_ptr = files.begin();

        // reference first element of hrtf_buffer array
        Array<AudioBuffer<float>>* hrtf_buffer_ptr = &audioProcessor.hrtf_buffer;

        // clear HRTF buffer array
        hrtf_buffer_ptr->clear();

        // register .wav and .aiff format
        ir_manager.registerBasicFormats();

        // create audio reader according to first audio file
        ir_reader = ir_manager.createReaderFor(*file_ptr);

        audioProcessor.hrtf_len = ir_reader->lengthInSamples;

        tmp.setSize(2, ir_reader->lengthInSamples);

        // iterate through array of files
        i = 0;
        do {
            // first reader gets written twice, which isn't pretty but ok
            ir_reader = ir_manager.createReaderFor(*file_ptr);

            // copy reader data to float AudioBuffer
            ir_reader->read(&tmp, 0, ir_reader->lengthInSamples, 0, 1, 1);

            // add new AudioBuffer to array
            hrtf_buffer_ptr->add(tmp);


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