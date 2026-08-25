#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GearButton final : public juce::TextButton
{
public:
    enum class Kind
    {
        section,
        pedal,
        amp,
        cab
    };

    GearButton(const juce::String& title, const juce::String& subtitle, Kind buttonKind);

    void paintButton(juce::Graphics& graphics, bool isMouseOverButton, bool isButtonDown) override;

private:
    juce::String subtitleText;
    Kind kind;
};

class GuitarPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit GuitarPluginAudioProcessorEditor(GuitarPluginAudioProcessor& processor);
    ~GuitarPluginAudioProcessorEditor() override = default;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    enum class Section
    {
        prePedals,
        amp,
        cab,
        eq,
        postPedals
    };

    void configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void selectSection(Section section);
    void setChoiceParameter(const juce::String& parameterId, int choiceIndex);
    int getChoiceParameter(const juce::String& parameterId) const;
    void setBoolParameter(const juce::String& parameterId, bool enabled);
    bool getBoolParameter(const juce::String& parameterId) const;
    void toggleBoolParameter(const juce::String& parameterId);
    void updateSectionVisibility();
    void updateGearButtons();
    void setComponentGroupVisible(bool shouldBeVisible, std::initializer_list<juce::Component*> components);

    GuitarPluginAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label statusLabel;
    GearButton prePedalsButton { "Pre Pedals", "stompboxes before amp", GearButton::Kind::section };
    GearButton ampButton { "Amp", "head controls", GearButton::Kind::section };
    GearButton cabButton { "Cab", "speaker pairing", GearButton::Kind::section };
    GearButton eqButton { "EQ", "final tone shape", GearButton::Kind::section };
    GearButton postPedalsButton { "Post Pedals", "space and modulation", GearButton::Kind::section };

    GearButton compressorButton { "Compressor", "studio squeeze", GearButton::Kind::pedal };
    GearButton driveButton { "Green Drive", "mid-push overdrive", GearButton::Kind::pedal };
    GearButton fuzzButton { "Heavy Fuzz", "sustain wall", GearButton::Kind::pedal };

    GearButton chimeAmpButton { "AC30 C2X", "7 NAM captures", GearButton::Kind::amp };
    GearButton leadAmpButton { "JCM800 Mod", "7 NAM captures", GearButton::Kind::amp };
    GearButton twinAmpButton { "Twin Reverb", "7 NAM captures", GearButton::Kind::amp };

    GearButton blueCabButton { "Blue 2x12", "57 + R121 IR", GearButton::Kind::cab };
    GearButton angledCabButton { "1960A 4x12", "Greenbacks IR", GearButton::Kind::cab };
    GearButton openCabButton { "Twin 2x12", "3 cab IRs", GearButton::Kind::cab };

    GearButton chorusButton { "Chorus", "wide modulation", GearButton::Kind::pedal };
    GearButton delayButton { "Delay", "tempo echoes", GearButton::Kind::pedal };
    GearButton reverbButton { "Reverb", "studio space", GearButton::Kind::pedal };

    juce::ToggleButton cabLinkButton { "Link amp and cab" };

    juce::Slider inputSlider;
    juce::Slider driveSlider;
    juce::Slider gateSlider;
    juce::Slider masterSlider;
    juce::Slider bassSlider;
    juce::Slider middleSlider;
    juce::Slider trebleSlider;
    juce::Slider cabToneSlider;
    juce::Slider eqLowSlider;
    juce::Slider eqMidSlider;
    juce::Slider eqHighSlider;
    juce::Slider prePedalLevelSlider;
    juce::Slider postPedalLevelSlider;
    juce::Slider outputSlider;

    juce::Label inputLabel;
    juce::Label driveLabel;
    juce::Label gateLabel;
    juce::Label masterLabel;
    juce::Label bassLabel;
    juce::Label middleLabel;
    juce::Label trebleLabel;
    juce::Label cabToneLabel;
    juce::Label eqLowLabel;
    juce::Label eqMidLabel;
    juce::Label eqHighLabel;
    juce::Label prePedalLevelLabel;
    juce::Label postPedalLevelLabel;
    juce::Label outputLabel;

    SliderAttachment inputAttachment;
    SliderAttachment driveAttachment;
    SliderAttachment gateAttachment;
    SliderAttachment masterAttachment;
    SliderAttachment bassAttachment;
    SliderAttachment middleAttachment;
    SliderAttachment trebleAttachment;
    SliderAttachment cabToneAttachment;
    SliderAttachment eqLowAttachment;
    SliderAttachment eqMidAttachment;
    SliderAttachment eqHighAttachment;
    SliderAttachment prePedalLevelAttachment;
    SliderAttachment postPedalLevelAttachment;
    SliderAttachment outputAttachment;

    Section currentSection = Section::amp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GuitarPluginAudioProcessorEditor)
};
