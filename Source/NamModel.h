#pragma once

#include <JuceHeader.h>

#if GUITARPLUGIN_HAS_NAM
#include <memory>
#include <NAM/dsp.h>
#endif

class NamModel final
{
public:
    void prepare(double sampleRate, int maximumBlockSize, int channels);
    bool load(const juce::File& modelFile);
    void process(juce::AudioBuffer<float>& buffer);
    bool isLoaded() const;
    juce::String getLoadedModelName() const;

private:
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
    int numChannels = 2;
    juce::String loadedModelName;

#if GUITARPLUGIN_HAS_NAM
    std::unique_ptr<nam::DSP> model;
    juce::AudioBuffer<float> monoInput;
    juce::AudioBuffer<float> monoOutput;
#endif
};
