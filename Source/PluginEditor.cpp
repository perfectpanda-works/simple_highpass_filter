#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
_01_panda_filterAudioProcessorEditor::_01_panda_filterAudioProcessorEditor (_01_panda_filterAudioProcessor& p,
                                                                            juce::AudioProcessorValueTreeState& vts,
                                                                            juce::UndoManager& um)
    : AudioProcessorEditor (&p), valueTreeState(vts)
    , forwardFFT(p.fftOrder),
    window(p.fftSize, juce::dsp::WindowingFunction<float>::hann)
    ,audioProcessor(p)
    , undoButton("Undo"), redoButton("Redo")
    , undoManager(um)
{
    //cutoff slider settings
    addAndMakeVisible(cutoffSlider);
    cutoffAttachment.reset(new SliderAttachment(valueTreeState, "cutoff", cutoffSlider));
    cutoffSlider.setRange(20.0, 22000.0);
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);
    cutoffSlider.setSliderStyle(juce::Slider::Rotary);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 20);
    cutoffSlider.setTextBoxIsEditable(true);
    cutoffSlider.setLookAndFeel(&dialLookAndFeel01);

    //Q slider setting
    addAndMakeVisible(QParamSlider);
    QParamAttachment.reset(new SliderAttachment(valueTreeState, "qParam", QParamSlider));
    QParamSlider.setSliderStyle(juce::Slider::Rotary);
    QParamSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 20);
    QParamSlider.setLookAndFeel(&dialLookAndFeel01);
    QParamSlider.setTextBoxIsEditable(true);

    addAndMakeVisible(SlopeComboBox);
    SlopeComboAttachment.reset(new ComboBoxAttachment(valueTreeState, "slope", SlopeComboBox));
    SlopeComboBox.addItem("-6db", 1);
    SlopeComboBox.addItem("-12db", 2);
    SlopeComboBox.addItem("-18db", 3);
    //DBG("slope param" << *(vts.getRawParameterValue("slope")));
    SlopeComboBox.setSelectedId(*(vts.getRawParameterValue("slope")));

    addAndMakeVisible(inputGainSlider);
    inputGainAttachment.reset(new SliderAttachment(valueTreeState, "inputGain", inputGainSlider));
    inputGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    inputGainSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, true, 65, 20);
    inputGainSlider.setTextBoxIsEditable(true);
    inputGainSlider.setRange(-100.0f, 12.0f);
    inputGainSlider.setSkewFactorFromMidPoint(-6.0f);
    inputGainSlider.setLookAndFeel(&gainSliderLookAndFeel01);

    addAndMakeVisible(outputGainSlider);
    outputGainAttachment.reset(new SliderAttachment(valueTreeState, "outputGain", outputGainSlider));
    outputGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxAbove, true, 65, 20);
    outputGainSlider.setTextBoxIsEditable(true);
    outputGainSlider.setRange(-100.0f, 12.0f);
    outputGainSlider.setSkewFactorFromMidPoint(-6.0f);
    outputGainSlider.setLookAndFeel(&gainSliderLookAndFeel01);

    addAndMakeVisible(undoButton);
    addAndMakeVisible(redoButton);
    undoButton.onClick = [this] {
                                    undoManager.undo();
                                    DBG("undo");
                                };
    redoButton.onClick = [this] {
                                    undoManager.redo();
                                    DBG("redo");
                                };

    
    for (int i = 0; i < diaplayWidth; i++) {
        auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / diaplayWidth) * 0.009f);
        skewedVector.push_back(skewedProportionX * sampleRateDiv2);
    }
    //DBG("skewdVectorSize:" << skewedVector.size());

    setOpaque(true);
    startTimerHz(30);

    setSize (500, 300);
}

_01_panda_filterAudioProcessorEditor::~_01_panda_filterAudioProcessorEditor()
{
    cutoffSlider.setLookAndFeel(nullptr);
    QParamSlider.setLookAndFeel(nullptr);
    SlopeComboBox.setLookAndFeel(nullptr);

    inputGainSlider.setLookAndFeel(nullptr);
    outputGainSlider.setLookAndFeel(nullptr);
}

//==============================================================================
void _01_panda_filterAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //draw speana background
    g.setColour(juce::Colour::fromRGBA(128, 128, 128, 64));
    g.fillRect(75.0f,0.0f,350.0f,150.0f);

    //draw speana criteria lines
    g.setColour(juce::Colour::fromRGBA(128,128,128,128));
    g.drawLine(108.0f,0.0f,108.0f,150.0f,2.0f);
    g.drawLine(206.0f, 0.0f, 206.0f, 150.0f, 2.0f);
    g.drawLine(392.0f, 0.0f, 392.0f, 150.0f, 2.0f);

    //draw labels
    g.setColour(juce::Colours::white);
    g.drawText("Input", 0.0f, 5.0f, 75.0f, 10.0f, juce::Justification::centred);
    g.drawText("Output", 425.0f, 5.0f, 75.0f, 10.0f, juce::Justification::centred);
    g.drawText("Cutoff", 80.0f, 280.0f, 100.0f, 10.0f, juce::Justification::centred);
    g.drawText("Q", 197.0f, 280.0f, 100.0f, 10.0f, juce::Justification::centred);
    g.drawText("Slope", 315.0f, 235.0f, 100.0f, 10.0f, juce::Justification::centred);

    //Drawing a spectrum analyzer
    g.setOpacity(1.0f);
    g.setColour(juce::Colours::white);
    drawFrame(g);

    //Drawing filter curves
    g.setColour(juce::Colours::yellow);
    preMagnitude = 0;
    currentMagnitude = 0;
    
    auto slope_Param = valueTreeState.getParameter("slope")->getValue();

    for (int i = 0; i < diaplayWidth; i++) {
        auto filter1_magnitude = audioProcessor.getMyFilter().getFilterCoef().getMagnitudeForFrequency(skewedVector[i], currentSampleRate);
        auto filter2_magnitude = audioProcessor.getMyFilter2().getFilterCoef().getMagnitudeForFrequency(skewedVector[i], currentSampleRate);
        auto filter3_magnitude = audioProcessor.getMyFilter3().getFilterCoef().getMagnitudeForFrequency(skewedVector[i], currentSampleRate);

        if (slope_Param == 0.5f)
            currentMagnitude = filter1_magnitude * filter2_magnitude;
        else if (slope_Param == 1.0f)
            currentMagnitude = filter1_magnitude * filter2_magnitude * filter3_magnitude;
        else
            currentMagnitude = filter1_magnitude;
      
        g.drawLine(  i + displayMargin //start X
                   , (preMagnitude * displayHeight)/ maxGain * -1 + displayHeight //start Y
                   , i + displayMargin + 1 //end X
                   , (currentMagnitude * displayHeight)/ maxGain * -1 + displayHeight //end Y
                   , 1.0f); // Line thickness
        preMagnitude = currentMagnitude;
    }

    g.setColour(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    //Hide the bottom part of the spectrum analyzer
    g.fillRect(75.0f, 125.0f, 350.0f, 27.0f);

    //Drawing Frequency Labels for Spectrum Analyzer
    g.setColour(juce::Colours::white);
    g.drawText("20Hz", 83, 135.0f, 50.0f, 10.0f, juce::Justification::centred);
    g.drawText("100Hz", 181, 135.0f, 50.0f, 10.0f, juce::Justification::centred);
    g.drawText("500Hz", 367, 135.0f, 50.0f, 10.0f, juce::Justification::centred);
}

void _01_panda_filterAudioProcessorEditor::resized()
{
    cutoffSlider.setBounds(80, 175, 100, 100);
    QParamSlider.setBounds(197, 175, 100, 100);
    SlopeComboBox.setBounds(315, 210, 100, 20);
    inputGainSlider.setBounds(5, 25, 65 ,265);
    outputGainSlider.setBounds(430, 25, 65, 265);
    undoButton.setBounds(315,260,50,30);
    redoButton.setBounds(365, 260, 50, 30);
}
