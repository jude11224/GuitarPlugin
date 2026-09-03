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

    model.reset();

    if (!modelFile.existsAsFile())
    {
        DBG("NAM ERROR: File does not exist:");
        DBG(modelFile.getFullPathName());
        return false;
    }

    DBG("NAM: Loading model:");
    DBG(modelFile.getFullPathName());

    try
    {
        auto newModel =
            nam::get_dsp(modelFile.getFullPathName().toStdString());

        if (newModel == nullptr)
        {
            DBG("NAM ERROR: get_dsp() returned nullptr");
            return false;
        }

        newModel->Reset(currentSampleRate, maxBlockSize);

        model = std::move(newModel);

        loadedModelName = modelFile.getFileNameWithoutExtension();

        DBG("NAM: Successfully loaded:");
        DBG(loadedModelName);

        return true;
    }
    catch (const std::exception& e)
    {
        DBG("NAM ERROR:");
        DBG(e.what());

        model.reset();
        return false;
    }
    catch (...)
    {
        DBG("NAM ERROR: Unknown exception");

        model.reset();
        return false;
    }

#else

    DBG("NAM ERROR: GUITARPLUGIN_HAS_NAM is disabled");
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
    const auto channels = buffer.getNumChannels();

    if (samples > monoInput.getNumSamples())
    {
        monoInput.setSize(1, samples, false, false, true);
        monoOutput.setSize(1, samples, false, false, true);
    }

    monoInput.copyFrom(0, 0, buffer, 0, 0, samples);

    for (int channel = 1; channel < channels; ++channel)
        monoInput.addFrom(0, 0, buffer, channel, 0, samples);

    if (channels > 1)
        monoInput.applyGain(1.0f / static_cast<float>(channels));

    monoOutput.clear();

    float* inputChannels[] =
    {
        monoInput.getWritePointer(0)
    };

    float* outputChannels[] =
    {
        monoOutput.getWritePointer(0)
    };

    model->process(inputChannels, outputChannels, samples);

    for (int channel = 0; channel < channels; ++channel)
        buffer.copyFrom(channel, 0, monoOutput, 0, 0, samples);

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
