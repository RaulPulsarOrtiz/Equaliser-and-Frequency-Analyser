/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor()
{
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SimpleEQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SimpleEQAudioProcessor::getProgramName (int index)
{
    return {};
}

void SimpleEQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    //Prepare the FIlters before we use them
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate; 

    //Pass those spec especifications to the Filters:
    leftChain.prepare(spec);
    rightChain.prepare(spec);

                                                                            //       auto chainSettings = getChainSettings(apvts); //Settings from the apvts
                                                                            //       
                                                                            //        updatePeakFilter(chainSettings);
                                                                            //       
                                                                            //        auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, 
                                                                            //                                                                                                            sampleRate, 
                                                                            //                                                                                                            2 * (chainSettings.lowCutSlope + 1));
                                                                            //        auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
                                                                            //        updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
                                                                            //       
                                                                            //        auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
                                                                            //        updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
                                                                            //       
                                                                            //      auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, 
                                                                            //                                                                                                              sampleRate, 
                                                                            //                                                                                                             2 * (chainSettings.highCutSlope + 1));
                                                                            //      auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
                                                                            //      auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
                                                                            //     
                                                                            //        updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
                                                                            //        updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
updateFilters();
}

void SimpleEQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SimpleEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) //This is called by the Host
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    updateFilters();
    //Produce Coefficients using the static helper function from the IIR coeficients class
                                                    //      auto chainSettings = getChainSettings(apvts);
                                                    //      updatePeakFilter(chainSettings); //Refactoring the coefficients
                                                    //                                                                      //    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                    //                                                                      //                                                                                chainSettings.peakFreq,
                                                    //                                                                      //                                                                                chainSettings.peakQuality,
                                                    //                                                                      //                                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
                                                    //                                                                      //
                                                    //                                                                      //    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients; //Ac  cess links in the processor chain. selecting an index in the chain inside the <>
                                                    //                                                                      //    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
                                                    //      
                                                    //    
                                                    //      auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                    //                                                                                                         getSampleRate(),
                                                    //                                                                                                        2 * (chainSettings.lowCutSlope + 1));
                                                    //      auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
                                                    //      updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope); //Refactoring the Slope coefficients
                                                    //   //                                                                                            //
                                                    //   //                                                                                            //leftLowCut.setBypassed<0>(true);
                                                    //   //                                                                                            //leftLowCut.setBypassed<1>(true);
                                                    //   //                                                                                            //leftLowCut.setBypassed<2>(true);
                                                    //   //                                                                                            //leftLowCut.setBypassed<3>(true);
                                                    //   //                                                                                            //
                                                    //   //                                                                                            //switch (chainSettings.lowCutSlope)
                                                    //   //                                                                                            //{
                                                    //   //                                                                                            //case Slope_12:
                                                    //   //                                                                                            //{
                                                    //   //                                                                                            //    *leftLowCut.get<0>().coefficients = *cutCoeficcients[0];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<0>(false);
                                                    //   //                                                                                            //    break;
                                                    //   //                                                                                            //}
                                                    //   //                                                                                            //case Slope_24:
                                                    //   //                                                                                            //{
                                                    //   //                                                                                            //    *leftLowCut.get<0>().coefficients = *cutCoeficcients[0];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<0>(false);
                                                    //   //                                                                                            //    *leftLowCut.get<1>().coefficients = *cutCoeficcients[1];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<1>(false);
                                                    //   //                                                                                            //    break;
                                                    //   //                                                                                            //}
                                                    //   //                                                                                            //case Slope_36:
                                                    //   //                                                                                            //{
                                                    //   //                                                                                            //    *leftLowCut.get<0>().coefficients = *cutCoeficcients[0];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<0>(false);
                                                    //   //                                                                                            //    *leftLowCut.get<1>().coefficients = *cutCoeficcients[1];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<1>(false);
                                                    //   //                                                                                            //    *leftLowCut.get<2>().coefficients = *cutCoeficcients[2];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<2>(false);
                                                    //   //                                                                                            //    break;
                                                    //   //                                                                                            //}
                                                    //   //                                                                                            //case Slope_48:
                                                    //   //                                                                                            //{
                                                    //   //                                                                                            //    *leftLowCut.get<0>().coefficients = *cutCoeficcients[0];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<0>(false);
                                                    //   //                                                                                            //    *leftLowCut.get<1>().coefficients = *cutCoeficcients[1];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<1>(false);
                                                    //   //                                                                                            //    *leftLowCut.get<2>().coefficients = *cutCoeficcients[2];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<2>(false);
                                                    //   //                                                                                            //    *leftLowCut.get<3>().coefficients = *cutCoeficcients[3];
                                                    //   //                                                                                            //    leftLowCut.setBypassed<3>(false);
                                                    //   //                                                                                            //    break;
                                                    //   //                                                                                            //}
                                                    //   //                                                                                            //}
                                                    //      auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
                                                    //      updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope); //As I have a functio for do this in the left Channel and can use the same function for the rigt channel. Just change the work rightLowcut
                                                    //   //                                                                                        //rightLowCut.setBypassed<0>(true);
                                                    //   //                                                                                        //rightLowCut.setBypassed<1>(true);
                                                    //   //                                                                                        //rightLowCut.setBypassed<2>(true);
                                                    //   //                                                                                        //rightLowCut.setBypassed<3>(true);
                                                    //   //                                                                                        //
                                                    //   //                                                                                        //switch (chainSettings.lowCutSlope)
                                                    //   //                                                                                        //{
                                                    //   //                                                                                        //
                                                    //   //                                                                                        //case Slope_12:
                                                    //   //                                                                                        //{
                                                    //   //                                                                                        //    *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<0>(false);
                                                    //   //                                                                                        //    break;
                                                    //   //                                                                                        //}
                                                    //   //                                                                                        //case Slope_24:
                                                    //   //                                                                                        //{
                                                    //   //                                                                                        //    *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<0>(false);
                                                    //   //                                                                                        //    *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<1>(false);
                                                    //   //                                                                                        //    break;
                                                    //   //                                                                                        //}
                                                    //   //                                                                                        //case Slope_36:
                                                    //   //                                                                                        //{
                                                    //   //                                                                                        //    *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<0>(false);
                                                    //   //                                                                                        //    *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<1>(false);
                                                    //   //                                                                                        //    *rightLowCut.get<2>().coefficients = *cutCoefficients[2];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<2>(false);
                                                    //   //                                                                                        //    break;
                                                    //   //                                                                                        //}
                                                    //   //                                                                                        //case Slope_48:
                                                    //   //                                                                                        //{
                                                    //   //                                                                                        //    *rightLowCut.get<0>().coefficients = *cutCoefficients[0];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<0>(false);
                                                    //   //                                                                                        //    *rightLowCut.get<1>().coefficients = *cutCoefficients[1];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<1>(false);
                                                    //   //                                                                                        //    *rightLowCut.get<2>().coefficients = *cutCoefficients[2];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<2>(false);
                                                    //   //                                                                                        //    *rightLowCut.get<3>().coefficients = *cutCoefficients[3];
                                                    //   //                                                                                        //    rightLowCut.setBypassed<3>(false);
                                                    //   //                                                                                        //    break;
                                                    //   //                                                                                        //}
                                                    //   //                                                                                        //
                                                    //   //                                                                                        //}
                                                    //   //
                                                    //      auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                    //                                                                                                             getSampleRate(),
                                                    //                                                                                                             2 * (chainSettings.highCutSlope + 1));
                                                    //      auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
                                                    //      auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
                                                    //   
                                                    //      updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
                                                    //      updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);

    // Run Audio Through our plugin
    juce::dsp::AudioBlock<float> block(buffer); //Create an Audio Block that wrap this buffer

    //Extract individual channel from the buffer:
    auto leftBlock = block.getSingleChannelBlock(0); 
    auto rightBlock = block.getSingleChannelBlock(1);

    //Create Processing Context that wrap each individual AudioBlock:
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    //Pass this context to the monoFilters:
    leftChain.process(leftContext);
    rightChain.process(rightContext);

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor()
{
  // return new SimpleEQAudioProcessorEditor (*this);
    return new GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SimpleEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}


ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) //Get the parameters values from the APVTS
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load(); //getParameters gives normalized values. But this function gives real world parameters, that are in the Range we said. They are atomic
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = (Slope)apvts.getRawParameterValue("LowCut Slope")->load();
    settings.highCutSlope = (Slope)apvts.getRawParameterValue("HighCut Slope")->load();

    return settings;
}

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings)
    {
        auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                                       chainSettings.peakFreq,
                                                                                       chainSettings.peakQuality,
                                                                                       juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));

   // *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients; //Access links in the processor chain. selecting an index in the chain inside the <>
   // *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    }

void SimpleEQAudioProcessor::updateCoefficients(Coefficients& old, const Coefficients& replacements)
{
    *old = *replacements;
}

void SimpleEQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq,
                                                                                                         getSampleRate(),
                                                                                                        2 * (chainSettings.lowCutSlope + 1));
    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
   
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope); //Refactoring the Slope coefficients
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope); //As I have a functio for do this in the left Channel and can use the same function for the rigt channel. Just change the work rightLowcut
}

void SimpleEQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq,
                                                                                                           getSampleRate(),
                                                                                                           2 * (chainSettings.highCutSlope + 1));
    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void SimpleEQAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);

    updateLowCutFilters(chainSettings);
    updatePeakFilter(chainSettings);
    updateHighCutFilters(chainSettings);
}

AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout() //Sync Parameters in the GUI and the variale in the DSP
{
    AudioProcessorValueTreeState::ParameterLayout layout;                                      //Creation of the parameters
    
    layout.add(std::make_unique<AudioParameterFloat>("LowCut Freq", "LowCut Freq",
        NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));

    layout.add(std::make_unique<AudioParameterFloat>("HighCut Freq", "HighCut Freq",
        NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));

    layout.add(std::make_unique<AudioParameterFloat>("Peak Freq", "Peak Freq",
        NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));

    layout.add(std::make_unique<AudioParameterFloat>("Peak Gain", "Peak Gain",
        NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));

    layout.add(std::make_unique<AudioParameterFloat>("Peak Quality", "Peak Quality",
        NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));

    StringArray stringArray;
    for (int i = 0; i < 4; ++i)
    {
        String str;
        str << (12 + i * 12);
        str << " dB/Oct";
        stringArray.add(str);
    }

    layout.add(std::make_unique<AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

        return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SimpleEQAudioProcessor();
}
