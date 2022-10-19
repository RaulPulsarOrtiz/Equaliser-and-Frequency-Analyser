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
class SimpleEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            juce::AudioProcessorParameter::Listener,
                                            juce::Timer 
{
public:
    SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor&);
    ~SimpleEQAudioProcessorEditor() override;
    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    void parameterValueChanged(int parameterIndex, float newValue) override;

     void timerCallback() override;

    /** Indicates that a parameter change gesture has started.

        E.g. if the user is dragging a slider, this would be called with gestureIsStarting
        being true when they first press the mouse button, and it will be called again with
        gestureIsStarting being false when they release it.

        IMPORTANT NOTE: This will be called synchronously, and many audio processors will
        call it during their audio callback. This means that not only has your handler code
        got to be completely thread-safe, but it's also got to be VERY fast, and avoid
        blocking. If you need to handle this event on your message thread, use this callback
        to trigger an AsyncUpdater or ChangeBroadcaster which you can respond to later on the
        message thread.
    */
     void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SimpleEQAudioProcessor& audioProcessor;

    //In this timer callback we are going to query and atomic flag to decide if the chain needs updating and our component needs to be repainted
    juce::Atomic<bool> parametersChanged{ false };

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