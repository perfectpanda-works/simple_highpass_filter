#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//Gain slider look and feel
class gainSliderLookAndFeelStyle01 : public juce::LookAndFeel_V4
{
public:
    gainSliderLookAndFeelStyle01()
    {
        setColour(juce::Slider::ColourIds::textBoxOutlineColourId, juce::Colour::fromRGBA(0, 0, 0, 0));
    }

    void drawLinearSlider(juce::Graphics& g,
        int 	x,
        int 	y,
        int 	width,
        int 	height,
        float 	sliderPos,
        float 	minSliderPos,
        float 	maxSliderPos,
        const juce::Slider::SliderStyle,
        juce::Slider&
    ) override
    {
        g.setColour(juce::Colour::fromRGBA(128,128,128,128));
        g.fillRect(x + width / 2, y, 3, height);
        
        g.setColour(juce::Colours::white);
        g.fillRect(x + width / 2 - 25, (int)sliderPos - 1, 50, 3);
        //DBG("SliderPos:" << sliderPos);
    }
};

//dial slider look and feel
class dialLookAndFeelStyle01 : public juce::LookAndFeel_V4
{
public:
    dialLookAndFeelStyle01()
    {
        //setColour(juce::Slider::ColourIds::textBoxOutlineColourId, juce::Colour::fromRGBA(0, 0, 0, 0));//deleted
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        //Numerical value for drawing the dial
        int offset = 10;// add this param
        auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto rx = centreX - radius + offset/2;
        auto ry = centreY - radius + offset/2;
        auto rw = radius * 2.0f - offset;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        //draw variable frame around the dial 
        juce::Path p2;
        p2.clear();
        p2.startNewSubPath(centreX, centreY);
        p2.addArc(rx, ry, rw, rw, rotaryStartAngle, angle);
        g.setColour(juce::Colour::fromRGBA(152, 251, 152, 254));
        g.strokePath(p2, juce::PathStrokeType(10.0f));

        //draw dial body
        g.setColour(juce::Colours::grey);
        g.fillEllipse(rx, ry, rw, rw);

        //draw black line on the frame of the dial body
        g.setColour(juce::Colours::black);
        g.drawEllipse(rx, ry, rw, rw, 1.0f);

        //draw dial current value scale
        juce::Path p;
        auto pointerLength = radius * 0.7f;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        g.setColour(juce::Colours::black);
        g.fillPath(p);
    }
};

//Combo box look and feel
class ComboBoxLookAndFeelStyle01 : public juce::LookAndFeel_V4
{
public:
    ComboBoxLookAndFeelStyle01()
    {
        setColour(juce::Slider::thumbColourId, juce::Colours::red);
    }

    void drawComboBox(juce::Graphics& g,
        int width,
        int height,
        bool isButtonDown,
        int buttonX,
        int buttonY,
        int buttonW,
        int buttonH,
        juce::ComboBox&) override
    {
        
    }

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override
    {
        g.fillAll(juce::Colour::fromFloatRGBA(0.0f, 0.0f, 0.0f,0.0f));
    }
};


class _01_panda_filterAudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    _01_panda_filterAudioProcessorEditor (_01_panda_filterAudioProcessor&, juce::AudioProcessorValueTreeState& vts, juce::UndoManager& um);
    ~_01_panda_filterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;

    //timer
    void timerCallback() override
    {
        auto flg = audioProcessor.getFFTReady();
        if (flg)
        {
            drawNextFrameOfSpectrum();
            audioProcessor.setFFTReady(false);
            
        }
        repaint();
    }


    //Spectrum analyzer drawing function
    void drawNextFrameOfSpectrum()
    {
        window.multiplyWithWindowingTable(audioProcessor.getFFTDataPtr(), audioProcessor.fftSize);
        forwardFFT.performFrequencyOnlyForwardTransform(audioProcessor.getFFTDataPtr());

        auto mindB = -100.0f;
        auto maxdB = 12.0f;

        for (int i = 0; i < scopeSize; ++i)
        {
            auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * 0.009f);
            auto fftDataIndex = juce::jlimit(0, audioProcessor.fftSize / 2, (int)(skewedProportionX * (float)audioProcessor.fftSize * 0.5f));
            auto level = juce::jmap(juce::jlimit(mindB, maxdB, juce::Decibels::gainToDecibels(audioProcessor.getFFTDataSample(fftDataIndex))
                - juce::Decibels::gainToDecibels((float)audioProcessor.fftSize)),
                mindB, maxdB, 0.0f, 1.0f);

            scopeData[i] = level;
        }
    }
    //Spectrum analyzer drawing function
    void drawFrame(juce::Graphics& g)
    {
        auto width = getLocalBounds().getWidth() - drawingOffsetX;
        auto height = getLocalBounds().getHeight() - drawingOffsetY;

        auto same_level_count = 0;

        for (int i = 1; i < scopeSize; ++i)
        {
            auto previous_level = juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f);
            auto current_level = juce::jmap(scopeData[i], 0.0f, 1.0f, (float)height, 0.0f);

            if (previous_level == current_level) {
                same_level_count++;
            }
            else {

                if (same_level_count > 10) {//If the same value continues for a width of 10 pixels or more
                    
                    g.drawLine({ (float)juce::jmap(i - 1 - same_level_count, 0, scopeSize - 1, 0, width) + (drawingOffsetX / 2),
                            juce::jmap(scopeData[i - 1 - same_level_count], 0.0f, 1.0f, (float)height, 0.0f),
                            (float)juce::jmap(i-1,     0, scopeSize - 1, 0, width) + (drawingOffsetX / 2),
                            juce::jmap(scopeData[i-1],     0.0f, 1.0f, (float)height, 0.0f) }, 2.0f);

                    g.drawLine({ (float)juce::jmap(i - 1, 0, scopeSize - 1, 0, width) + (drawingOffsetX / 2),//20210623
                                  juce::jmap(scopeData[i - 1], 0.0f, 1.0f, (float)height, 0.0f),
                          (float)juce::jmap(i,     0, scopeSize - 1, 0, width) + (drawingOffsetX / 2),//20210623
                                  juce::jmap(scopeData[i],     0.0f, 1.0f, (float)height, 0.0f) }, 2.0f);
                }
                else {
                    g.drawLine({ (float)juce::jmap(i - 1 - same_level_count, 0, scopeSize - 1, 0, width) + (drawingOffsetX / 2),
                            juce::jmap(scopeData[i - 1 - same_level_count], 0.0f, 1.0f, (float)height, 0.0f),
                            (float)juce::jmap(i,     0, scopeSize - 1, 0, width) + (drawingOffsetX / 2),
                            juce::jmap(scopeData[i],     0.0f, 1.0f, (float)height, 0.0f) }, 2.0f);
                }
                
                same_level_count = 0;
            }

            //if the same volume continues, draw the part of the line that is not drawn.
            if (i == scopeSize-1) {
                g.drawLine({ (float)juce::jmap(i - same_level_count, 0, scopeSize - 1, 0, width) + (drawingOffsetX / 2),
                        juce::jmap(scopeData[i - same_level_count], 0.0f, 1.0f, (float)height, 0.0f),
                        (float)juce::jmap(i,     0, scopeSize - 1, 0, width) + (drawingOffsetX / 2),
                        juce::jmap(scopeData[i],     0.0f, 1.0f, (float)height, 0.0f) }, 2.0f);

                same_level_count = 0;
            }
        }
    }

    static constexpr auto scopeSize = 512;

private:
    _01_panda_filterAudioProcessor& audioProcessor;

    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;
    float scopeData[scopeSize];

    juce::AudioProcessorValueTreeState& valueTreeState;
    juce::Slider cutoffSlider;
    std::unique_ptr<SliderAttachment> cutoffAttachment;

    juce::Slider QParamSlider;
    std::unique_ptr<SliderAttachment> QParamAttachment;

    juce::ComboBox SlopeComboBox;
    std::unique_ptr<ComboBoxAttachment> SlopeComboAttachment;

    dialLookAndFeelStyle01 dialLookAndFeel01;
    ComboBoxLookAndFeelStyle01 ComboBoxLookAndFeel01;

    const int drawingOffsetX = 150;
    const int drawingOffsetY = 150;

    juce::Slider inputGainSlider;
    std::unique_ptr<SliderAttachment> inputGainAttachment;
    juce::Slider outputGainSlider;
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    gainSliderLookAndFeelStyle01 gainSliderLookAndFeel01;

    juce::TextButton undoButton, redoButton;

    juce::UndoManager& undoManager;

    double preMagnitude = 0;
    double currentMagnitude = 0;
    const double maxGain = 2.0;
    const double displayHeight = 150.0;
    const int diaplayWidth = 350;
    const int displayMargin = 75;
    const float currentSampleRate = (float)audioProcessor.getSampleRate();
    const float sampleRateDiv2 = (float)audioProcessor.getSampleRate() / 2;

    std::vector<double> skewedVector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (_01_panda_filterAudioProcessorEditor)
};
