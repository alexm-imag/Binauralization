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
    g.drawFittedText ("Binauralization 0.5", getLocalBounds(), juce::Justification::centred, 1);
}

void BinauralizationAudioProcessorEditor::resized()
{

    // Xpos, Ypos, Xdim, Ydim
    OpenButton.setBounds(100, 200, 100, 50);
    ConvButton.setBounds(200, 200, 100, 50);

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