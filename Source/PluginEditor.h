/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

using namespace juce;

//==============================================================================
/**
*/
class BinauralizationAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    BinauralizationAudioProcessorEditor (BinauralizationAudioProcessor&);
    ~BinauralizationAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BinauralizationAudioProcessor& audioProcessor;

    TextButton DirButton{ "Open IR dir" };
    TextButton ConvButton{ "Conv Inactive" };
    TextButton SineButton{ "Sine Inactive" };
    TextButton NoiseButton{ "Noise Inactive" };
    Slider     HRTF_Slider;

    void openIRdirectory();
    void toggleConvolution();
    void toggleSine();
    void toggleNoise();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BinauralizationAudioProcessorEditor)


};
