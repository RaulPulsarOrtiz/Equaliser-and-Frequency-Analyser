/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin editor.
  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeels::drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                                    float sliderPosProportional, float rotaryStartAngle,
                                    float rotaryEndAngle, Slider& slider)
{
    auto bounds = Rectangle<float>(x, y, width, height);

    g.setColour(Colour(97u, 18u, 167u));
    g.fillEllipse(bounds);

    g.setColour(Colour(255u, 154u, 1u));
    g.drawEllipse(bounds, 1.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();

        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);
        p.addRectangle(r);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));
       
        g.fillPath(p);

        g.setFont(rswl->getTextHeight()); 
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(center);

        g.setColour(Colours::black);
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }

}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    auto startAng = degreesToRadians(-45.f - 90.f);
    auto endAng = degreesToRadians(45.f + 90.f);

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    //g.setColour(Colours::red);
    //g.drawRect(getLocalBounds());
    //g.setColour(Colours::yellow);
    //g.drawRect(sliderBounds);

    getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), sliderBounds.getY(), sliderBounds.getWidth(), sliderBounds.getHeight(), 
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), //Here is where we map our sliders. We turn our slider values into normalised values
                                      startAng, endAng, *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; i++)
    {
        auto pos = labels[i].Pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
               
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
                 //Get centre of the circle                 //Go a little bit pass the circle
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight()); //shift it down from the cicle

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{

    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;

}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    //return juce::String(getValue());
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        return choiceParam->getCurrentChoiceName();

    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val = getValue();

        if (val > 999.f)
        {
            val /= 1000.f; //1001 / 100 = 1.001
                addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    //else
    //{
    //    jassertfalse; //this shouldn't happen!
    //}

    //Quality has no units. so
    if (suffix.isNotEmpty())
    {
        str << " ";
        if (addK)
            str << "k";

        str << suffix;
    }

    return str;
}
//==============================================================================

ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : audioProcessor(p)
{

    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    updateChain(); //Loading the previous configuration I want see that already drawn in the curve. Thats why I use this here

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        updateChain();
        //signal a repaint
        repaint();
    }
}

void ResponseCurveComponent::updateChain() //Helper function to have the curve drawn the first time that the plugin is load. (Because previous configuration keeps on from the previous time)
{
    //update the monochain
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    //Now we can update our chain Coefficients:
    updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());

    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);

    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysisArea(); //getRenderArea(); //getLocalBounds();
   
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
    //Magnitude is expressed as gain units an these ar multiplicative, unlike decibels which are additive. thats why I need a starting gain of one
    for (int i = 0; i < w; i++)
    {
        double mag = 1.f; //Starting gain of One 
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

        //Now we convert this vector of magnitudes into a path and then  draw it:
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
        g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

        g.setColour(Colours::red);
        g.strokePath(responseCurve, PathStrokeType(2.f));

    }
}

void ResponseCurveComponent::resized()
{
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);//Create a new background image.
    Graphics g(background); // Then create a graphics context which draw into the backgorund image
   
    Array<float> freqs
    {
        20, 30, 40, 50, 100,
        200, 300, 400, 500, 1000,
        2000, 3000, 4000, 5000, 10000,
        20000
    };

    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    //g.setColour(Colours::dimgrey);
    Array<float> xs;
    for (auto f : freqs) //Loop through the freqs and draw a vertical line in each freq
    {
        auto normX = mapFromLog10(f, 20.f, 20000.f); //Convert the frequency value into normalised position
        xs.add(left + width * normX); //Compute that to a window x position based on the left edge of the renderArea
       // g.drawVerticalLine(getWidth() * normX, 0.f, getHeight());
    }

    g.setColour(Colours::dimgrey);
    for (auto x : xs)
    {
        g.drawVerticalLine(x, top, bottom);
    }

    Array<float> gain
    {
        -24, -23, 0, 12, 24
    };

    for (auto gDb : gain)
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
       // g.drawHorizontalLine(y, 0, getWidth());
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey); //Draw a green line in the center
        g.drawHorizontalLine(y, left, right);
    }

    g.drawRect(getRenderArea());
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea() //Check how much rectangle I want to reduce to dont cover the -24 and +24 gain in the Response Area
{
    auto bounds = getLocalBounds();
  //  bounds.reduce(15, //JUCE_LIVE_CONSTANT(5),
  //                20); //JUCE_LIVE_CONSTANT(5));
    bounds.removeFromTop(15);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);

    return bounds;

}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor(SimpleEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), 
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
    responseCurveComponent(audioProcessor)
   // peakFreakSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreakSlider),
   // peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
   // peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
   // lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
   // highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
   // lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
   // highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)

   

{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }
     peakFreqSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Freq", peakFreqSlider);
     peakGainSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Gain", peakGainSlider);
     peakQualitySliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "Peak Quality", peakQualitySlider);
     lowCutFreqSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider);
     highCutFreqSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider);
     lowCutSlopeSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider);
     highCutSlopeSliderAttachment = std::make_unique< juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider);
      
     peakFreqSlider.labels.add({ 0.f, "20hz" });
     peakFreqSlider.labels.add({ 1.f, "20KHz" });
     
     peakGainSlider.labels.add({ 0.f, "-24.f" });
     peakGainSlider.labels.add({ 1.f, "+24.f" });

     peakQualitySlider.labels.add({ 0.f, "0.1" });
     peakQualitySlider.labels.add({ 1.f, "10.0" });

     lowCutFreqSlider.labels.add({ 0.f, "20Hz" });
     lowCutFreqSlider.labels.add({ 1.f, "20KHz" });
         
     highCutFreqSlider.labels.add({ 0.f, "20Hz" });
     highCutFreqSlider.labels.add({ 1.f, "20KHz" });
        
     lowCutSlopeSlider.labels.add({ 0.f, "12" });
     lowCutSlopeSlider.labels.add({ 1.f, "48" });
         
     highCutSlopeSlider.labels.add({ 0.f, "12" });
     highCutSlopeSlider.labels.add({ 1.f, "48" });

    setSize(600, 400);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor()
{

}
//==============================================================================
void SimpleEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);
}
void SimpleEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    responseCurveComponent.setBounds(responseArea);
    
    bounds.removeFromTop(5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);
    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}