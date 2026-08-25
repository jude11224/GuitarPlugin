#include "NamModel.h"

#if GUITARPLUGIN_HAS_NAM
#include <NAM/get_dsp.h>
#endif

void NamModel::prepare(double sampleRate, int maximumBlockSize, int channels)
{
    currentSampleRate = sampleRate;
    maxBlockSize = juce::jmax(1, maximumBlockSize);
    numChannels = juce::jmax(1, channels);

#if GUITARPLUGIN_HAS_NAM
    monoInput.setSize(1, maxBlockSize);
    monoOutput.setSize(1, maxBlockSize);

    if (model != nullptr)
        model->Reset(currentSampleRate, maxBlockSize);
#endif
}

bool NamModel::load(const juce::File& modelFile)
{
    loadedModelName.clear();

#if GUITARPLUGIN_HAS_NAM
    if (! modelFile.existsAsFile())
    {
        model.reset();
        return false;
    }

    try
    {
        model = nam::get_dsp(modelFile.getFullPathName().toStdString());
        model->Reset(currentSampleRate, maxBlockSize);
        loadedModelName = modelFile.getFileNameWithoutExtension();
        return true;
    }
    catch (...)
    {
        model.reset();
        return false;
    }
#else
    juce::ignoreUnused(modelFile);
    return false;
#endif
}

void NamModel::process(juce::AudioBuffer<float>& buffer)
{
#if GUITARPLUGIN_HAS_NAM
    if (model == nullptr)
        return;

    const auto samples = buffer.getNumSamples();

    if (samples > monoInput.getNumSamples())
    {
        monoInput.setSize(1, samples, false, false, true);
        monoOutput.setSize(1, samples, false, false, true);
    }

    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        monoInput.copyFrom(0, 0, buffer, channel, 0, samples);
        monoOutput.clear();

        float* inputChannels[] = { monoInput.getWritePointer(0) };
        float* outputChannels[] = { monoOutput.getWritePointer(0) };
        model->process(inputChannels, outputChannels, samples);

        buffer.copyFrom(channel, 0, monoOutput, 0, 0, samples);
    }
#else
    juce::ignoreUnused(buffer);
#endif
}

bool NamModel::isLoaded() const
{
#if GUITARPLUGIN_HAS_NAM
    return model != nullptr;
#else
    return false;
#endif
}

juce::String NamModel::getLoadedModelName() const
{
    return loadedModelName;
}
