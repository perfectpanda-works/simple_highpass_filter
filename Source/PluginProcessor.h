#pragma once

#include <JuceHeader.h>

//ProcessorBase class
class ProcessorBase : public juce::AudioProcessor
{
public:
    //==============================================================================
    ProcessorBase() {}

    //==============================================================================
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioSampleBuffer&, juce::MidiBuffer&) override {}

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    //==============================================================================
    const juce::String getName() const override { return {}; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }

    //==============================================================================
    int getNumPrograms() override { return 0; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessorBase)
};

//FilterProcessor class
class FilterProcessor : public ProcessorBase
{
public:
    FilterProcessor(): filter_coef_ptr(new juce::dsp::IIR::Coefficients<float>()) {}//test 20210713

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        //Keep the sample rate
        samplerate = sampleRate; 
        
        *filter_coef_ptr = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 1000.0f);
        filter.state = *filter_coef_ptr;

        juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32> (samplesPerBlock), 2 };
        filter.prepare(spec);
    }

    void processBlock(juce::AudioSampleBuffer& buffer, juce::MidiBuffer&) override
    {
        float cutoff_val = *cutoffParam_filter;
        float qParam_val = *QParam_filter;
        
        *filter_coef_ptr = *juce::dsp::IIR::Coefficients<float>::makeHighPass(samplerate, cutoff_val, qParam_val);
        filter.state = *filter_coef_ptr;
        filter_coef = *filter_coef_ptr.get();

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        filter.process(context);
    }

    void reset() override
    {
        filter.reset();
    }

    void setCutoffParam(std::atomic<float>* param) {
        cutoffParam_filter = param;
    }

    void setQParam(std::atomic<float>* param) {
        QParam_filter = param;
    }

    const juce::String getName() const override { return "Filter"; }

    juce::dsp::IIR::Coefficients<float> getFilterCoef() {
        return *filter_coef_ptr;
    }

private:
    std::atomic<float>* cutoffParam_filter = nullptr;
    float samplerate;
    std::atomic<float>* QParam_filter = nullptr;

    juce::dsp::IIR::Coefficients<float>::Ptr filter_coef_ptr;
    juce::dsp::IIR::Coefficients<float> filter_coef;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> filter;
};

//==============================================================================

class _01_panda_filterAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    _01_panda_filterAudioProcessor();
    ~_01_panda_filterAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //accessors
    bool getFFTReady() {
        return nextFFTBlockReady;
    }

    void setFFTReady(bool b) {
        nextFFTBlockReady = b;
    }

    float* getFFTDataPtr() {
        return fftData.data();
    }

    float getFFTDataSample(int sample) {
        return fftData[sample];
    }

    //Spectrum Analyzer Functions
    void pushNextSampleIntoFifo(float sample) noexcept
    {
        if (fifoIndex == fftSize)
        {
            if (!nextFFTBlockReady)
            {
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                std::copy(fifo.begin(), fifo.end(), fftData.begin());
                nextFFTBlockReady = true;
            }

            fifoIndex = 0;
        }

        fifo[fifoIndex++] = sample;
    }

    //Spectrum Analyzer Related Variables
    static constexpr auto fftOrder = 14;//power of two
    //10: 1024
    //11: 2048
    //12: 4096
    static constexpr auto fftSize = 1 << fftOrder;

    //Accessor to the filter module instance
    FilterProcessor& getMyFilter() {
        return MyFilter;
    }
    FilterProcessor& getMyFilter2() {
        return MyFilter2;
    }
    FilterProcessor& getMyFilter3() {
        return MyFilter3;
    }

private:
    //Members related to Spectrum Analyzer
    std::array<float, fftSize> fifo;
    std::array<float, fftSize * 2> fftData;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    //Parameters of AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* cutoffParam = nullptr;
    std::atomic<float>* QParam = nullptr;
    std::atomic<float>* slope = nullptr;
    std::atomic<float>* inputGainDb = nullptr;
    std::atomic<float>* outputGainDb = nullptr;

    //3 filters
    FilterProcessor MyFilter;
    FilterProcessor MyFilter2;
    FilterProcessor MyFilter3;

    //Input/output gain related members
    float previousInputGain;
    float previousOutputGain;

    //Undo and redo object
    juce::UndoManager undoManager;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (_01_panda_filterAudioProcessor)
};
