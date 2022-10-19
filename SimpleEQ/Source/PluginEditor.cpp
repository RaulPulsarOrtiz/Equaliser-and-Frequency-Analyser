/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin editor.
  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"
//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
   // peakFreakSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreakSlider),
   // peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
   // peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
   // lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCutFreq", lowCutFreqSlider),
   // highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
   // lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
   // highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
   
    //peakFreakSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreakSlider),
    //peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    //peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    //lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCutFreq", lowCutFreqSlider),
    //highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    //lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    //highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
    peakFreakSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Freq", peakFreakSlider);
    peakGainSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Gain", peakGainSlider);
    peakQualitySliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Quality", peakQualitySlider);
    lowCutFreqSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider);
    highCutFreqSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider);
    lowCutSlopeSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider);
    highCutSlopeSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider);

    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }
    startTimerHz(60);

    setSize(600, 400);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}
//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    auto w = responseArea.getWidth();

    //Individual chain Elements:
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate = audioProcessor.getSampleRate(); //to use that function I need to know the sampleRate to get magnitude for frequency function

    std::vector<double> mags; //I store all those magnitudes which return form that function as 'doubles'
    //We are computing one magnitude per pixel so let's pre-allocate the space that we need:
    mags.resize(w);

    //We need to iterate through each pixel and compute the magnitude at that frequency.
    //Magnitude is expressed as gain units an these ar multiplicative, unlike decibels which are additive
    for (int i = 0; i < w; i++)
    {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0); //We need to call the magnitude function for a particular pixel, map for pixel space to frequency space. This function do that 

        if (!monoChain.isBypassed<ChainPositions::Peak>()) //If the band is not bypassed  
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        //Convert this magnitude into decibels and store it:
        mags[i] = Decibels::gainToDecibels(mag);

        //Now we convert this vector of magnitudes into a path and the draw it:
        Path responseCurve;

        //Map our decibel value to the response area:
        const double outputMin = responseArea.getBottom(); //Define our maximun and minimun position in the window
        const double outputMax = responseArea.getY();
        auto map = [outputMin, outputMax](double input)
        {
            return jmap(input, -24.0, 24.0, outputMin, outputMax);
        };

        //Start a new subpath with the first magnitude:
        responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

        //Now create lines for every other magnitude:
        for (size_t i = 1; i < mags.size(); i++)
        {
            responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
        }

        g.setColour(Colours::orange);
        g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

        g.setColour(Colours::white);
        g.strokePath(responseCurve, PathStrokeType(2.f));

    }
}
void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    peakFreakSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}

void SimpleEQAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void SimpleEQAudioProcessorEditor::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        //update the monochain
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        //Now we can update our chain Coefficients:
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        //signal a repaint
        repaint();
    }
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreakSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
    };
}