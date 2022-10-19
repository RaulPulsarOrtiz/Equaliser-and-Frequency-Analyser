/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin editor.
  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
struct CustomRotarySlider : juce::Slider   //To dont type this base class initialisation for every slider, this class do this in the constructor. So I can use this for every slider in the GUI
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
    }
};
//==============================================================================
/**
*/
class SimpleEQAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;
    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;
    CustomRotarySlider peakFreakSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider,
        lowCutSlopeSlider, highCutSlopeSlider;

    //using APVTS = juce::AudioProcessorValueTreeState;
    //using Attachment = APVTS::SliderAttachment;
    //Attachment peakFreakSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment, lowCutFreqSliderAttachment, highCutFreqSliderAttachment, lowCutSlopeSliderAttachment, highCutSlopeSliderAttachment;
    //juce::AupeakGainSliderAttachment;dioProcessorValueTreeState::SliderAttachment peakFreakSliderAttachment;
    //juce::AupeakQualitySliderAttachment;dioProcessorValueTreeState::SliderAttachment peakGainSliderAttachment;
    //juce::Au lowCutFreqSliderAttachment;dioProcessorValueTreeState::SliderAttachment peakQualitySliderAttachment;
    //juce::Au highCutFreqSliderAttachment;dioProcessorValueTreeState::SliderAttachment  lowCutFreqSliderAttachment;
     //juce::AudioProcessorValueTreeState::SliderAttachment  highCutFreqSliderAttachment;
     //    juce::AudioProcessorValueTreeState::SliderAttachment  lowCutSlopeSliderAttachment;
     //        juce::AudioProcessorValueTreeState::SliderAttachment   highCutSlopeSliderAttachment;
   // using APVTS = juce::AudioProcessorValueTreeState;
   // using Attachment = APVTS::SliderAttachment;
    //Attachment needs to be below of the sliders(CustomRotarySlider)
  //  Attachment peakFreakSliderAttachment, peakGainSliderAttachment, peakQualitySliderAttachment, lowCutFreqSliderAttachment, highCutFreqSliderAttachment, lowCutSlopeSliderAttachment, highCutSlopeSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakGainSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakFreakSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakQualitySliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowCutFreqSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highCutFreqSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowCutSlopeSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps(); //To have all the slider in a vector because I want to have done the same all the time to them (like makethemVisible)

    MonoChain monoChain;
    //The function to connect the slider to the audio parameters is called attachment in hte apvts class, but the name is so long so I created an alias for that function:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEQAudioProcessorEditor)
};