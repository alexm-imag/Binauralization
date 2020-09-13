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

        audioProcessor.hrtf_buffer.num_samples = ir_reader->lengthInSamples;
        audioProcessor.hrtf_buffer.num_hrtfs = files.size();
        
        // currently old data will simply be overwritten (which of course would lead to problmems with the addition of multiple HRTF datasets)
        // WARNING: re-opening because of incomplete load (e.g 357 instead of 360) leads to stack overflow > re-allocate mem at every "open dir" issue, or add mem to buffer!
        if (audioProcessor.hrtf_buffer.left != NULL) {
            for (int i = 0; i < audioProcessor.hrtf_buffer.num_hrtfs; i++) {
                free(audioProcessor.hrtf_buffer.left[i]);
                free(audioProcessor.hrtf_buffer.right[i]);
            }
            free(audioProcessor.hrtf_buffer.left);
            free(audioProcessor.hrtf_buffer.right);
        }

        audioProcessor.k = audioProcessor.get_padding_size(audioProcessor.n, audioProcessor.hrtf_buffer.num_samples);

        // allocate space for x HRFT spectra
        audioProcessor.hrtf_buffer.left = (fftwf_complex**)malloc(sizeof(fftwf_complex*) * files.size());
        audioProcessor.hrtf_buffer.right = (fftwf_complex**)malloc(sizeof(fftwf_complex*) * files.size());
        for (i = 0; i < audioProcessor.hrtf_buffer.num_hrtfs; i++) {
            audioProcessor.hrtf_buffer.left[i] = fftwf_alloc_complex(audioProcessor.k);
            audioProcessor.hrtf_buffer.right[i] = fftwf_alloc_complex(audioProcessor.k);
        }       
      

        // create AudioBuffer of approriate size
        // WARNING: this also only works with HRFTs of constant length!
        AudioBuffer<float> tmp_buffer(2, audioProcessor.k);
        
        // iterate through array of files
        i = 0;
        do {
            // first reader gets written twice, which isn't pretty but ok
            ir_reader = ir_manager.createReaderFor(*file_ptr);

            // copy reader data to float AudioBuffer
            ir_reader->read(&tmp_buffer, 0, ir_reader->lengthInSamples, 0, 1, 1);

            //// NOTE: it seems that Processor and Editor run in different threads
            //// Make sure that perform_fft can't collide
            //// Deactivate fft in Processor while loading new files
            // perform fft on both channels for each HRTF file and store result in hrtf_buffer

            // PERFORM ZERO PADDING BEFORE FFT!!!
            // from where can I get the correct K?
            // Assuming n is constant, this should work for now
            audioProcessor.perform_fft(ir_reader->lengthInSamples, tmp_buffer.getWritePointer(0), audioProcessor.hrtf_buffer.left[i]);
            audioProcessor.perform_fft(ir_reader->lengthInSamples, tmp_buffer.getWritePointer(1), audioProcessor.hrtf_buffer.right[i]);

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