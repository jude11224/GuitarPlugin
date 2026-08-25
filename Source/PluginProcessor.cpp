#include "PluginProcessor.h"
#include "PluginEditor.h"

GuitarPluginAudioProcessor::GuitarPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    modeParameter = parameters.getRawParameterValue("mode");
    ampModelParameter = parameters.getRawParameterValue("ampModel");
    cabModelParameter = parameters.getRawParameterValue("cabModel");
    inputParameter = parameters.getRawParameterValue("input");
    driveParameter = parameters.getRawParameterValue("drive");
    gateParameter = parameters.getRawParameterValue("gate");
    masterParameter = parameters.getRawParameterValue("master");
    bassParameter = parameters.getRawParameterValue("bass");
    middleParameter = parameters.getRawParameterValue("middle");
    trebleParameter = parameters.getRawParameterValue("treble");
    cabToneParameter = parameters.getRawParameterValue("cabTone");
    eqLowParameter = parameters.getRawParameterValue("eqLow");
    eqMidParameter = parameters.getRawParameterValue("eqMid");
    eqHighParameter = parameters.getRawParameterValue("eqHigh");
    prePedalLevelParameter = parameters.getRawParameterValue("prePedalLevel");
    postPedalLevelParameter = parameters.getRawParameterValue("postPedalLevel");
    outputParameter = parameters.getRawParameterValue("output");
}

void GuitarPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    cabLowpassState.assign(static_cast<size_t>(juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels())), 0.0f);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(juce::jmax(1, getTotalNumOutputChannels()));
    cabConvolution.prepare(spec);
    ampNamModel.prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
    loadAmpModel(static_cast<int>(ampModelParameter->load()), driveParameter->load());
    loadCabImpulseForCurrentSelection();
}

void GuitarPluginAudioProcessor::releaseResources()
{
    cabConvolution.reset();
}

bool GuitarPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainInput = layouts.getMainInputChannelSet();
    const auto& mainOutput = layouts.getMainOutputChannelSet();

    return mainInput == mainOutput
        && (mainInput == juce::AudioChannelSet::mono()
            || mainInput == juce::AudioChannelSet::stereo());
}

void GuitarPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    const auto mode = static_cast<int>(modeParameter->load());
    const auto ampModel = static_cast<int>(ampModelParameter->load());
    const auto cabModel = static_cast<int>(cabModelParameter->load());
    const auto driveDb = driveParameter->load();
    const auto ampCaptureSlot = getAmpCaptureSlot(ampModel, driveDb);

    if (cabModel != loadedCabModel)
        loadCabImpulse(cabModel);
    if (ampModel != loadedAmpModel || ampCaptureSlot != loadedAmpCaptureSlot)
        loadAmpModel(ampModel, driveDb);

    const auto inputGain = juce::Decibels::decibelsToGain(inputParameter->load());
    const auto outputGain = juce::Decibels::decibelsToGain(outputParameter->load());
    const auto driveGain = juce::Decibels::decibelsToGain(driveDb);
    const auto gateThreshold = juce::Decibels::decibelsToGain(gateParameter->load());
    const auto masterGain = juce::Decibels::decibelsToGain(masterParameter->load());
    const auto bass = bassParameter->load();
    const auto middle = middleParameter->load();
    const auto treble = trebleParameter->load();
    const auto cabTone = cabToneParameter->load();
    const auto postEqGain = juce::Decibels::decibelsToGain((eqLowParameter->load() * 0.18f)
                                                           + (eqMidParameter->load() * 0.12f)
                                                           + (eqHighParameter->load() * 0.18f));
    const auto prePedalGain = juce::Decibels::decibelsToGain(prePedalLevelParameter->load());
    const auto postPedalGain = juce::Decibels::decibelsToGain(postPedalLevelParameter->load());

    const auto ampVoiceGain = ampModel == 1 ? 1.25f : (ampModel == 2 ? 0.85f : 1.0f);
    const auto toneGain = juce::Decibels::decibelsToGain((bass * 0.20f) + (middle * 0.12f) + (treble * 0.20f));
    const auto cabBaseCutoff = cabModel == 1 ? 5600.0f : (cabModel == 2 ? 7400.0f : 6500.0f);
    const auto cabCutoff = juce::jlimit(1800.0f, 11000.0f, cabBaseCutoff + juce::jmap(cabTone, 0.0f, 10.0f, -2200.0f, 2600.0f));
    const auto lowpassAmount = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cabCutoff
                                               / static_cast<float>(currentSampleRate));

    for (auto channel = 0; channel < getTotalNumInputChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);
        auto lowpassState = cabLowpassState[static_cast<size_t>(channel)];

        for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto value = samples[sample] * inputGain;

            if (mode == 1)
            {
                if (std::abs(value) < gateThreshold)
                    value = 0.0f;

                value *= prePedalGain;

                if (! ampNamModel.isLoaded())
                    value = std::tanh(value * driveGain * ampVoiceGain);

                value *= toneGain;
                value *= masterGain;

                value *= postEqGain;
                value *= postPedalGain;
            }

            samples[sample] = value * outputGain;
        }

        cabLowpassState[static_cast<size_t>(channel)] = lowpassState;
    }

    if (mode == 1)
    {
        ampNamModel.process(buffer);

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        cabConvolution.process(context);

        for (auto channel = 0; channel < getTotalNumInputChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer(channel);
            auto lowpassState = cabLowpassState[static_cast<size_t>(channel)];

            for (auto sample = 0; sample < buffer.getNumSamples(); ++sample)
            {
                lowpassState += lowpassAmount * (samples[sample] - lowpassState);
                samples[sample] = lowpassState;
            }

            cabLowpassState[static_cast<size_t>(channel)] = lowpassState;
        }
    }
}

juce::AudioProcessorEditor* GuitarPluginAudioProcessor::createEditor()
{
    return new GuitarPluginAudioProcessorEditor(*this);
}

bool GuitarPluginAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String GuitarPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GuitarPluginAudioProcessor::acceptsMidi() const
{
    return false;
}

bool GuitarPluginAudioProcessor::producesMidi() const
{
    return false;
}

bool GuitarPluginAudioProcessor::isMidiEffect() const
{
    return false;
}

double GuitarPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GuitarPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int GuitarPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GuitarPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String GuitarPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void GuitarPluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void GuitarPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState(); auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void GuitarPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
        {
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
            loadCabImpulseForCurrentSelection();
        }
}

void GuitarPluginAudioProcessor::loadCabImpulseForCurrentSelection()
{
    if (cabModelParameter != nullptr)
        loadCabImpulse(static_cast<int>(cabModelParameter->load()));
}

bool GuitarPluginAudioProcessor::isNamEngineAvailable() const
{
#if GUITARPLUGIN_HAS_NAM
    return true;
#else
    return false;
#endif
}

bool GuitarPluginAudioProcessor::isNamModelLoaded() const
{
    return ampNamModel.isLoaded();
}

juce::String GuitarPluginAudioProcessor::getLoadedNamModelName() const
{
    return ampNamModel.getLoadedModelName();
}

juce::File GuitarPluginAudioProcessor::findAssetsDirectory() const
{
    const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);

    const auto standaloneAssets = executable.getParentDirectory().getChildFile("Assets");
    if (standaloneAssets.isDirectory())
        return standaloneAssets;

    const auto vst3ResourcesAssets = executable.getParentDirectory()
        .getParentDirectory()
        .getChildFile("Resources")
        .getChildFile("Assets");
    if (vst3ResourcesAssets.isDirectory())
        return vst3ResourcesAssets;

    const auto sourceAssets = juce::File("C:/Users/jude0/Documents/GuitarPlugin/Assets");
    if (sourceAssets.isDirectory())
        return sourceAssets;

    return {};
}

juce::File GuitarPluginAudioProcessor::getCabImpulseFile(int cabModel) const
{
    const auto assets = findAssetsDirectory();

    if (cabModel == 1)
    {
        return assets.getChildFile("IR")
            .getChildFile("Marshall1960A")
            .getChildFile("Marshall 1972 1960a 4x12 custom - Celestian Greenbacks -Airsong.wav");
    }

    if (cabModel == 2)
    {
        return assets.getChildFile("IR")
            .getChildFile("TwinReverb")
            .getChildFile("TWIN REVERB __ BALANCED.wav");
    }

    return assets.getChildFile("IR")
        .getChildFile("VoxBlue212")
        .getChildFile("TF 64 AC30 2X12 BLUE - 57 R121 70-30.wav");
}

int GuitarPluginAudioProcessor::getAmpCaptureSlot(int ampModel, float gainDb) const
{
    if (ampModel == 1)
        return juce::jlimit(0, 6, static_cast<int>(std::round(juce::jmap(gainDb, 0.0f, 30.0f, 0.0f, 6.0f))));

    if (ampModel == 2)
        return juce::jlimit(0, 6, static_cast<int>(std::round(juce::jmap(gainDb, 0.0f, 30.0f, 0.0f, 6.0f))));

    return juce::jlimit(0, 6, static_cast<int>(std::round(juce::jmap(gainDb, 0.0f, 30.0f, 0.0f, 6.0f))));
}

juce::File GuitarPluginAudioProcessor::getAmpCaptureFile(int ampModel, float gainDb) const
{
    const auto assets = findAssetsDirectory();
    const auto slot = getAmpCaptureSlot(ampModel, gainDb);

    if (ampModel == 1)
    {
        const juce::StringArray captures {
            "AMP - JCM800 Gain 3.nam",
            "AMP - JCM800 Gain 4.nam",
            "AMP - JCM800 Gain 5.nam",
            "AMP - JCM800 Gain 6.nam",
            "AMP - JCM800 Gain 7.nam",
            "AMP - JCM800 Gain 8.nam",
            "AMP - JCM800 Gain 9.nam"
        };

        return assets.getChildFile("NAM").getChildFile("JCM800").getChildFile(captures[slot]);
    }

    if (ampModel == 2)
    {
        const juce::StringArray captures {
            "01 Fender Ch2 Vib.nam",
            "05 Fender Twin Reverb TS808 g-05 di half.nam",
            "08 Fender Twin Reverb TS808 g-5 di 75.nam",
            "10 Fender Twin Reverb TS10 g0 difull.nam",
            "05 Fender Twin Reverb TS808 g-11 di half.nam",
            "02 Fender Twin Reverb TS808 g-17 di 5.nam",
            "11 Fender Twin Reverb Bogner Blue High.nam"
        };

        return assets.getChildFile("NAM").getChildFile("TwinReverb").getChildFile(captures[slot]);
    }

    const juce::StringArray captures {
        "AC30 TBL Capture 03 V08 DI.nam",
        "AC30 TBL Capture 02 V09 DI.nam",
        "AC30 TBL Capture 05 V10.5 DI.nam",
        "AC30 TBL Capture 01 V12 DI.nam",
        "AC30 TBL Capture 06 V13.5 DI.nam",
        "AC30 TBL Capture 04 V15 DI.nam",
        "AC30 TBL Capture 07 V17 DI.nam"
    };

    return assets.getChildFile("NAM").getChildFile("VoxAC30").getChildFile(captures[slot]);
}

void GuitarPluginAudioProcessor::loadCabImpulse(int cabModel)
{
    const auto impulse = getCabImpulseFile(cabModel);

    if (! impulse.existsAsFile())
        return;

    cabConvolution.loadImpulseResponse(impulse,
                                       juce::dsp::Convolution::Stereo::yes,
                                       juce::dsp::Convolution::Trim::yes,
                                       0);
    loadedCabModel = cabModel;
}

void GuitarPluginAudioProcessor::loadAmpModel(int ampModel, float gainDb)
{
    const auto slot = getAmpCaptureSlot(ampModel, gainDb);

    if (ampNamModel.load(getAmpCaptureFile(ampModel, gainDb)))
    {
        loadedAmpModel = ampModel;
        loadedAmpCaptureSlot = slot;
        return;
    }

    loadedAmpModel = -1;
    loadedAmpCaptureSlot = -1;
}

juce::AudioProcessorValueTreeState::ParameterLayout GuitarPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "mode", "Mode", juce::StringArray { "Clean Input", "Simple Amp + Cab" }, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "ampModel", "Amp Model", juce::StringArray { "AC30 C2X NAM", "JCM800 Modded NAM", "Twin Reverb NAM" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "cabModel", "Cab Model", juce::StringArray { "AC30 Blue 2x12 IR", "Marshall 1960A 4x12 IR", "Twin Reverb Cab IR" }, 0));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "cabLinked", "Link Amp and Cab", true));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "preCompressor", "Pre Compressor", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "preDrive", "Pre Drive", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "preFuzz", "Pre Fuzz", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "postChorus", "Post Chorus", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "postDelay", "Post Delay", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "postReverb", "Post Reverb", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "input", "Input", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Gain", juce::NormalisableRange<float>(0.0f, 30.0f, 0.1f), 6.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gate", "Gate", juce::NormalisableRange<float>(-90.0f, -20.0f, 0.1f), -70.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "master", "Master", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bass", "Bass", juce::NormalisableRange<float>(-10.0f, 10.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "middle", "Middle", juce::NormalisableRange<float>(-10.0f, 10.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "treble", "Treble", juce::NormalisableRange<float>(-10.0f, 10.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "cabTone", "Cab Tone", juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f), 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eqLow", "EQ Low", juce::NormalisableRange<float>(-10.0f, 10.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eqMid", "EQ Mid", juce::NormalisableRange<float>(-10.0f, 10.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "eqHigh", "EQ High", juce::NormalisableRange<float>(-10.0f, 10.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "prePedalLevel", "Pre Pedal Level", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "postPedalLevel", "Post Pedal Level", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GuitarPluginAudioProcessor();
}
