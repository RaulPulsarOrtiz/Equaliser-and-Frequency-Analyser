/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin processor.
  ==============================================================================
*/
#pragma once
#include <JuceHeader.h>
#include <array>
template<typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        for (auto& buffer : buffers)
        {
            buffer.setSize(numChannels,    
                             numSamples, 
                             false,  //clear everything?
                             true,   //including the extra space?
                             true);  //avoid reallocating if you can?
            buffer.clear();
        }
    }

    bool push(const T& t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }
        return false;
    }

    int getnumAvailableForReading() const
    {
        return fifo.getNumReady();
    }
private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{ Capacity };
};

enum Channel //FFT works for each channel of audio
{
    Right, //effectively 0 
    Left // effectively 1
};
   
template<typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }

    void update(const BlockType& buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto* channelPtr = buffer.getReadPointer(channelToUse);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1,                  //channel
                             bufferSize,         // num samples
                             false,              //KeepExistingContent
                             true,               //clear extra space
                             true);              //avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBufferAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType& buf) { return audioBufferFifo.pull(buf); }

private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo <BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};
struct ChainSettings //Extract our parameters from the AudioProcesorValueTreeState
{
    float peakFreq{ 0 }, peakGainInDecibels{ 0 }, peakQuality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;  // 12dB per octave
using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>; // Mono signal path.  Processor Chain needs a  processor context to be pass to it, in order to run the audio through the links in the Chain.
enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& old, const Coefficients& replacements);
Coefficients makePeakFilter(const ChainSettings& chainSettings, double sampleRate);
// To avoid all the duplication in the switch case . We can use this:
template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain, const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}
template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& leftLowCut, const CoefficientType& cutCoefficients, const Slope& lowCutSlope)

{
    leftLowCut.template setBypassed<0>(true);
    leftLowCut.template setBypassed<1>(true);
    leftLowCut.template setBypassed<2>(true);
    leftLowCut.template setBypassed<3>(true);
    switch (lowCutSlope)  //fallthrough ib the cases because I dont say break
    {
    case Slope_48:
    {
        update<3>(leftLowCut, cutCoefficients);
        //*leftLowCut.template get<3>().coefficients = *cutCoefficients[3];
        // leftLowCut.template setBypassed<3>(false);
    }

    case Slope_36:
    {
        update<2>(leftLowCut, cutCoefficients);
    }

    case Slope_24:
    {
        update<1>(leftLowCut, cutCoefficients);
    }

    case Slope_12:
    {
        update<0>(leftLowCut, cutCoefficients);
    }
    }
}
inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                         sampleRate,
                                                                                         2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter (const ChainSettings& chainSettings, double sampleRate)
{
return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                  sampleRate,
                                                                                  2 * (chainSettings.highCutSlope + 1));
}
//==============================================================================
/**
*/
class SimpleEQAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    SimpleEQAudioProcessor();
    ~SimpleEQAudioProcessor() override;
    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    //==============================================================================
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout(); //needs to be public so the GUI can attach all the knobs and combo boxes etc
    AudioProcessorValueTreeState apvts{ *this, nullptr, "Parameters", createParameterLayout() };
   
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEQAudioProcessor)
        //Declaration of the Filters:
  
    MonoChain leftChain, rightChain;
    
    void updatePeakFilter(const ChainSettings& chainSettings);
   
    
        // case Slope_12:
        // {
        //     *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
        //     leftLowCut.template setBypassed<0>(false);
        //     break;
        // }
        // case Slope_24:
        // {
        //     *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
        //     leftLowCut.template setBypassed<0>(false);
        //     *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
        //     leftLowCut.template setBypassed<1>(false);
        //     break;
        // }
        // case Slope_36:
        // {
        //     *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
        //     leftLowCut.template setBypassed<0>(false);
        //     *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
        //     leftLowCut.template setBypassed<1>(false);
        //     *leftLowCut.template get<2>().coefficients = *cutCoefficients[2];
        //     leftLowCut.template setBypassed<2>(false);
        //     break;
        // }
        // case Slope_48:
        // {
        //     *leftLowCut.template get<0>().coefficients = *cutCoefficients[0];
        //     leftLowCut.template setBypassed<0>(false);
        //     *leftLowCut.template get<1>().coefficients = *cutCoefficients[1];
        //     leftLowCut.template setBypassed<1>(false);
        //     *leftLowCut.template get<2>().coefficients = *cutCoefficients[2];
        //     leftLowCut.template setBypassed<2>(false);
        //     *leftLowCut.template get<3>().coefficients = *cutCoefficients[3];
        //     leftLowCut.template setBypassed<3>(false);
        //     break;
        // }

 
    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);
    void updateFilters();
};