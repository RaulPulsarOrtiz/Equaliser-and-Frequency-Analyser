/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin editor.
  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

enum FFTOrder  //FFT Data Generator configuration
{
    order2048 = 11, //at 48000 sampleRate each bin represent 23 Hertz This means a lot of resolution in the upper end and not very good resolution in the bottom end 
    order4096 = 12,
    order8192 = 13
};

template<typename BlockType>
struct FFTDataGenerator
{
    /** produces the FFT data form an audio buffer */
    void produceFFTDataForRendering(const juce::AudioBuffer<float>& audioData, const float negativeInfinity) //Feed audio to the FFT
    {
        const auto fftSize = getFFTSize();

        fftData.assign(fftData.size(), 0);
        auto* readIndex = audioData.getReadPointer(0);
        std::copy(readIndex, readIndex + fftSize, fftData.begin());

        //First apply a windowing function to our data
        window->multiplyWithWindowingTable(fftData.data(), fftSize);

        //then render our FFT data..
        forwardFFT->performFrequencyOnlyForwardTransform(fftData.data());

        int numBins = (int)fftSize / 2;

        //normalise the fft values
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] /= (float)numBins;
        }

        //convert them to decibels
        for (int i = 0; i < numBins; ++i)
        {
            fftData[i] = juce::Decibels::gainToDecibels(fftData[i], negativeInfinity);
        }

        fftDataFifo.push(fftData);
    }

    void changeOrder(FFTOrder newOrder)
    {
        //When you change order, recreate the window, dorwardFFT, fifo, fftData
        //also reset the fifoIndex
        //things that need recreating should be created on the heap via st::make_unique<>

        order = newOrder;
        auto fftSize = getFFTSize();

        forwardFFT = std::make_unique<juce::dsp::FFT>(order);
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fftSize, juce::dsp::WindowingFunction<float>::blackmanHarris);

        fftData.clear();
        fftData.resize(fftSize * 2, 0);

        fftDataFifo.prepare(fftData.size());
    }
    //==============================================================================
    int getFFTSize() const { return 1 << order; } 
    int getNumAvailableFFTDataBlocks() const { return fftDataFifo.getNumAvailableForReading(); } //Here we see ho much FFT data we have
    //==============================================================================
    bool getFFTData(BlockType& fftData) { return fftDataFifo.pull(fftData); } //Here is where we get our FFT data available
private:
    FFTOrder order;
    BlockType fftData;
    std::unique_ptr<juce::dsp::FFT> forwardFFT;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;

    Fifo<BlockType> fftDataFifo;
};


template<typename PathType>
struct AnalyserPathGenerator
{
    /*
    converts 'renderdata[]' into a juce::Path 
    */
    void generatePath(const std::vector<float>& renderData,
                      juce::Rectangle<float> fftBounds,
                      int fftSize, float binWidth,
                      float negativeInfinity)
    {
        auto top = fftBounds.getY();
        auto bottom = fftBounds.getHeight();
        auto width = fftBounds.getWidth();

        int numBins = (int)fftSize / 2;

        PathType p;
        p.preallocateSpace(3 * (int)fftBounds.getWidth());

        auto map = [bottom, top, negativeInfinity](float v)
        {
            return juce::jmap(v, negativeInfinity, 0.f, float(bottom), top);
        };

        auto y = map(renderData[0]);

        jassert(!std::isnan(y) && !std::isinf(y));

        p.startNewSubPath(0, y);

        const int pathResolution = 2; //you can draw line-to's every 'pathResolution' pixels

        for (int binNum = 1; binNum < numBins; binNum += pathResolution)
        {
            y = map(renderData[binNum]);

            jassert(!std::isnan(y) && !std::isinf(y));
            {
                auto binFreq = binNum * binWidth;
                auto normalisedBinX = juce::mapFromLog10(binFreq, 20.f, 20000.f);
                int binX = std::floor(normalisedBinX * width);
                p.lineTo(binX, y);
            }
        }
        pathFifo.push(p);
    }
    
    int getNumPathsAvailable() const
    {
        return pathFifo.getNumAvailableForReading();
    }

    bool getPath(PathType& path)
    {
        return pathFifo.pull(path);
    }

private:
    Fifo<PathType> pathFifo;
};

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

struct PathProducer
{
    PathProducer(SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>& scsf) : leftChannelFifo(&scsf)
    {
        //Split the audio spectrum from 20Hz to 20KHz into 2048 or 4096 or 8192 frequency bins
        leftChannelFFTDataGenerator.changeOrder(FFTOrder::order2048);
        monoBuffer.setSize(1, leftChannelFFTDataGenerator.getFFTSize());
    }
    void process(juce::Rectangle<float> fftBounds, double sameplRate);
    juce::Path getPath() { return leftChannelFFTPath; }
private:
    SingleChannelSampleFifo<SimpleEQAudioProcessor::BlockType>* leftChannelFifo;

    juce::AudioBuffer<float> monoBuffer; //That are going to be send from the SCSF to the FFT Data Generator
    FFTDataGenerator<std::vector<float>> leftChannelFFTDataGenerator;

    AnalyserPathGenerator<juce::Path> pathProducer; //Producing a path in our path generator

    juce::Path leftChannelFFTPath;
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

    PathProducer leftPathProducer, rightPathProducer;
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