#pragma once

#include <JuceHeader.h>
#include "NamModel.h"

class GuitarPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    GuitarPluginAudioProcessor();
    ~GuitarPluginAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void loadCabImpulseForCurrentSelection();
    bool isNamEngineAvailable() const;
    bool isNamModelLoaded() const;
    juce::String getLoadedNamModelName() const;

    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    std::atomic<float>* modeParameter = nullptr;
    std::atomic<float>* ampModelParameter = nullptr;
    std::atomic<float>* cabModelParameter = nullptr;
    std::atomic<float>* inputParameter = nullptr;
    std::atomic<float>* driveParameter = nullptr;
    std::atomic<float>* gateParameter = nullptr;
    std::atomic<float>* masterParameter = nullptr;
    std::atomic<float>* bassParameter = nullptr;
    std::atomic<float>* middleParameter = nullptr;
    std::atomic<float>* trebleParameter = nullptr;
    std::atomic<float>* cabToneParameter = nullptr;
    std::atomic<float>* eqLowParameter = nullptr;
    std::atomic<float>* eqMidParameter = nullptr;
    std::atomic<float>* eqHighParameter = nullptr;
    std::atomic<float>* prePedalLevelParameter = nullptr;
    std::atomic<float>* postPedalLevelParameter = nullptr;
    std::atomic<float>* outputParameter = nullptr;

    std::vector<float> cabLowpassState;
    juce::dsp::Convolution cabConvolution;
    NamModel ampNamModel;
    double currentSampleRate = 44100.0;
    int loadedCabModel = -1;
    int loadedAmpModel = -1;

    juce::File getAmpCaptureFile(int ampModel) const;
    void loadAmpModel(int ampModel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarPluginAudioProcessor)
};
