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
        File IRfile;
        // store file in IRfile
        IRfile = selector.getResult();

        // allow .wav and .aiff formats
        AudioFormatManager IRmanager;
        IRmanager.registerBasicFormats();

        AudioFormatReader* IRreader = IRmanager.createReaderFor(IRfile);
        AudioBuffer<float> IRbuffer(2, IRreader->lengthInSamples);

        IRreader->read(&IRbuffer, 0, IRreader->lengthInSamples, 0, 1, 1);
        // copy AudioBuffer content into audioProcessor context
        audioProcessor.IRbufferPtr = IRbuffer;
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