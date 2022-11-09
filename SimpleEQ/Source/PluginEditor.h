/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin editor.
  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeels : juce::LookAndFeel_V4 
{
    void drawRotarySlider(Graphics&, int x, int y, int width, int height,
        float sliderPosProportional, float rotaryStartAngle,
        float rotaryEndAngle, Slider&) override;
};

struct RotarySliderWithLabels : juce::Slider   //To dont type this base class initialisation for every slider, this class do this in the constructor. So I can use this for every slider in the GUI
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
        juce::Slider::TextEntryBoxPosition::NoTextBox), 
        param(&rap), suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float Pos;         //Normalised value
        juce::String label; //String that is going to be displayed
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;

private: juce::RangedAudioParameter* param;
       juce::String suffix;
       LookAndFeels lnf;
};

struct ResponseCurveComponent: juce::Component,
                               juce::AudioProcessorParameter::Listener,
                               juce::Timer
{
    ResponseCurveComponent(SimpleEQAudioProcessor&);
    ~ResponseCurveComponent();
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void timerCallback() override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    SimpleEQAudioProcessor& audioProcessor;
    //In this timer callback we are going to query and atomic flag to decide if the chain needs updating and our component needs to be repainted
    juce::Atomic<bool> parametersChanged{ false };
    MonoChain monoChain;

    void updateChain();

    juce::Image background; //Resize() is a good place to do this because is called each time that component bounds is changed and is called before the first time that paint() is called

    juce::Rectangle<int> getRenderArea();
    juce::Rectangle<int> getAnalysisArea(); //Is going to be a little bit smaller than the RenderArea
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

    //In this timer callback we are going to query and atomic flag to decide if the chain needs updating and our component needs to be repainted
   // juce::Atomic<bool> parametersChanged{ false };

    RotarySliderWithLabels peakFreqSlider, peakGainSlider, peakQualitySlider, lowCutFreqSlider, highCutFreqSlider,
        lowCutSlopeSlider, highCutSlopeSlider;

    ResponseCurveComponent responseCurveComponent;
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
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakFreqSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakQualitySliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowCutFreqSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highCutFreqSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowCutSlopeSliderAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highCutSlopeSliderAttachment;

    std::vector<juce::Component*> getComps(); //To have all the slider in a vector because I want to have done the same all the time to them (like makethemVisible)

   // MonoChain monoChain;
    //The function to connect the slider to the audio parameters is called attachment in hte apvts class, but the name is so long so I created an alias for that function:
   
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEQAudioProcessorEditor)
};