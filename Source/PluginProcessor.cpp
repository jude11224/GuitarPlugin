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
    loadAmpModel(static_cast<int>(ampModelParameter->load()));
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

    const auto inputGain = juce::Decibels::decibelsToGain(inputParameter->load());
    const auto outputGain = juce::Decibels::decibelsToGain(outputParameter->load());
    const auto driveDb = driveParameter->load();
    const auto driveGain = juce::Decibels::decibelsToGain(driveDb);

    const auto gateThreshold = juce::Decibels::decibelsToGain(gateParameter->load());
    const auto masterGain = juce::Decibels::decibelsToGain(masterParameter->load());

    const auto bass = bassParameter->load();
    const auto middle = middleParameter->load();
    const auto treble = trebleParameter->load();

    const auto cabTone = cabToneParameter->load();

    const auto postEqGain =
        juce::Decibels::decibelsToGain(
            (eqLowParameter->load() * 0.18f)
            + (eqMidParameter->load() * 0.12f)
            + (eqHighParameter->load() * 0.18f));

    const auto prePedalGain =
        juce::Decibels::decibelsToGain(
            prePedalLevelParameter->load());

    const auto postPedalGain =
        juce::Decibels::decibelsToGain(
            postPedalLevelParameter->load());

    const auto ampVoiceGain =
        ampModel == 1 ? 1.25f :
        (ampModel == 2 ? 0.85f : 1.0f);

    const auto toneGain =
        juce::Decibels::decibelsToGain(
            (bass * 0.20f)
            + (middle * 0.12f)
            + (treble * 0.20f));

    const auto cabBaseCutoff =
        cabModel == 1 ? 5600.0f :
        (cabModel == 2 ? 7400.0f : 6500.0f);

    const auto cabCutoff =
        juce::jlimit(
            1800.0f,
            11000.0f,
            cabBaseCutoff
            + juce::jmap(
                cabTone,
                0.0f,
                10.0f,
                -2200.0f,
                2600.0f));

    const auto lowpassAmount =
        1.0f - std::exp(
            -2.0f
            * juce::MathConstants<float>::pi
            * cabCutoff
            / static_cast<float>(currentSampleRate));


// ============================================================
// LOAD AMP / CAB
// ============================================================

    if (cabModel != loadedCabModel)
        loadCabImpulse(cabModel);

    if (ampModel != loadedAmpModel)
        loadAmpModel(ampModel);


// ============================================================
// INPUT / GATE / PRE-PEDAL / DRIVE
// ============================================================

    for (auto channel = 0;
         channel < getTotalNumInputChannels();
         ++channel)
    {
        auto* samples = buffer.getWritePointer(channel);

        for (auto sample = 0;
             sample < buffer.getNumSamples();
             ++sample)
        {
            float value = samples[sample] * inputGain;

            if (mode == 1)
            {
                // Noise gate
                if (std::abs(value) < gateThreshold)
                    value = 0.0f;

                // Pre-amp pedal level
                value *= prePedalGain;

                // Drive knob pushes the NAM capture harder
                value *= driveGain;
            }

            samples[sample] = value;
        }
    }


// ============================================================
// AMP
// ============================================================

    if (mode == 1)
    {
        if (ampNamModel.isLoaded())
        {
            // Process through the selected NAM capture.
            // The Gain knob has already been applied above.
            ampNamModel.process(buffer);
        }
        else
        {
            // Fallback only if NAM failed to load.
            const auto fallbackGain = ampVoiceGain;

            for (auto channel = 0;
                 channel < getTotalNumInputChannels();
                 ++channel)
            {
                auto* samples = buffer.getWritePointer(channel);

                for (auto sample = 0;
                     sample < buffer.getNumSamples();
                     ++sample)
                {
                    samples[sample] =
                        std::tanh(samples[sample] * fallbackGain);
                }
            }
        }
    }


// ============================================================
// AMP TONE / MASTER / EQ / POST PEDAL
// ============================================================

    if (mode == 1)
    {
        for (auto channel = 0;
             channel < getTotalNumInputChannels();
             ++channel)
        {
            auto* samples = buffer.getWritePointer(channel);

            for (auto sample = 0;
                 sample < buffer.getNumSamples();
                 ++sample)
            {
                samples[sample] *= outputGain;
            }
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
    const auto executable =
        juce::File::getSpecialLocation(
            juce::File::currentExecutableFile);

    // Standalone:
    // GuitarPlugin.exe
    // Assets/
    const auto standaloneAssets =
        executable.getParentDirectory()
                 .getChildFile("Assets");

    if (standaloneAssets.isDirectory())
    {
        DBG("Assets found (Standalone):");
        DBG(standaloneAssets.getFullPathName());

        return standaloneAssets;
    }

    // VST3:
    // GuitarPlugin.vst3
    //   Contents/
    //     x86_64-win/
    //     Resources/
    //       Assets/
    const auto vst3ResourcesAssets =
        executable.getParentDirectory()
                 .getParentDirectory()
                 .getChildFile("Resources")
                 .getChildFile("Assets");

    if (vst3ResourcesAssets.isDirectory())
    {
        DBG("Assets found (VST3 Resources):");
        DBG(vst3ResourcesAssets.getFullPathName());

        return vst3ResourcesAssets;
    }

    DBG("NAM ERROR: Could not find Assets directory");

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

juce::File GuitarPluginAudioProcessor::getAmpCaptureFile(int ampModel) const
{
    const auto assets = findAssetsDirectory();

    if (ampModel == 1)
    {
        return assets.getChildFile("NAM")
            .getChildFile("JCM800")
            .getChildFile("AMP - JCM800 Gain 6.nam");
    }

    if (ampModel == 2)
    {
        return assets.getChildFile("NAM")
            .getChildFile("TwinReverb")
            .getChildFile("01 Fender Ch2 Vib.nam");
    }

    return assets.getChildFile("NAM")
        .getChildFile("VoxAC30")
        .getChildFile("AC30 TBL Capture 06 V13.5 DI.nam");
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

void GuitarPluginAudioProcessor::loadAmpModel(int ampModel)
{
    if (ampNamModel.load(getAmpCaptureFile(ampModel)))
    {
        loadedAmpModel = ampModel;
        return;
    }

    loadedAmpModel = -1;
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
