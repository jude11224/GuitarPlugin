#include "PluginEditor.h"

namespace
{
juce::Colour getGearColour(GearButton::Kind kind)
{
    switch (kind)
    {
        case GearButton::Kind::section: return juce::Colour::fromRGB(38, 43, 48);
        case GearButton::Kind::pedal:   return juce::Colour::fromRGB(39, 87, 79);
        case GearButton::Kind::amp:     return juce::Colour::fromRGB(86, 58, 34);
        case GearButton::Kind::cab:     return juce::Colour::fromRGB(55, 49, 43);
    }

    return juce::Colours::darkgrey;
}
}

GearButton::GearButton(const juce::String& title, const juce::String& subtitle, Kind buttonKind)
    : juce::TextButton(title), subtitleText(subtitle), kind(buttonKind)
{
    setClickingTogglesState(false);
    setWantsKeyboardFocus(false);
}

void GearButton::paintButton(juce::Graphics& graphics, bool isMouseOverButton, bool isButtonDown)
{
    const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    auto base = getGearColour(kind);

    if (getToggleState())
        base = base.brighter(0.35f);
    if (isMouseOverButton)
        base = base.brighter(0.12f);
    if (isButtonDown)
        base = base.darker(0.18f);

    graphics.setColour(base);
    graphics.fillRoundedRectangle(bounds, 7.0f);

    graphics.setColour(getToggleState() ? juce::Colour::fromRGB(236, 206, 126)
                                        : juce::Colour::fromRGB(82, 88, 94));
    graphics.drawRoundedRectangle(bounds, 7.0f, getToggleState() ? 2.0f : 1.0f);

    if (kind == Kind::pedal)
    {
        graphics.setColour(juce::Colour::fromRGB(24, 25, 27).withAlpha(0.75f));
        graphics.fillEllipse(bounds.getCentreX() - 8.0f, bounds.getBottom() - 24.0f, 16.0f, 16.0f);
    }
    else if (kind == Kind::amp)
    {
        graphics.setColour(juce::Colour::fromRGB(212, 181, 116).withAlpha(0.38f));
        for (auto x = bounds.getX() + 12.0f; x < bounds.getRight() - 10.0f; x += 12.0f)
            graphics.drawVerticalLine(static_cast<int>(x), bounds.getY() + 10.0f, bounds.getBottom() - 10.0f);
    }
    else if (kind == Kind::cab)
    {
        graphics.setColour(juce::Colour::fromRGB(20, 21, 22).withAlpha(0.45f));
        graphics.fillEllipse(bounds.getCentreX() - 24.0f, bounds.getCentreY() - 20.0f, 40.0f, 40.0f);
        graphics.drawEllipse(bounds.getCentreX() - 18.0f, bounds.getCentreY() - 14.0f, 28.0f, 28.0f, 2.0f);
    }

    graphics.setColour(juce::Colours::white.withAlpha(0.94f));
    graphics.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    graphics.drawFittedText(getButtonText(), getLocalBounds().reduced(8).removeFromTop(24),
                            juce::Justification::centred, 1);

    graphics.setColour(juce::Colours::white.withAlpha(0.64f));
    graphics.setFont(juce::FontOptions(11.0f));
    graphics.drawFittedText(subtitleText, getLocalBounds().reduced(8).withTrimmedTop(26),
                            juce::Justification::centred, 2);
}

GuitarPluginAudioProcessorEditor::GuitarPluginAudioProcessorEditor(GuitarPluginAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor),
      inputAttachment(audioProcessor.parameters, "input", inputSlider),
      driveAttachment(audioProcessor.parameters, "drive", driveSlider),
      gateAttachment(audioProcessor.parameters, "gate", gateSlider),
      masterAttachment(audioProcessor.parameters, "master", masterSlider),
      bassAttachment(audioProcessor.parameters, "bass", bassSlider),
      middleAttachment(audioProcessor.parameters, "middle", middleSlider),
      trebleAttachment(audioProcessor.parameters, "treble", trebleSlider),
      cabToneAttachment(audioProcessor.parameters, "cabTone", cabToneSlider),
      eqLowAttachment(audioProcessor.parameters, "eqLow", eqLowSlider),
      eqMidAttachment(audioProcessor.parameters, "eqMid", eqMidSlider),
      eqHighAttachment(audioProcessor.parameters, "eqHigh", eqHighSlider),
      prePedalLevelAttachment(audioProcessor.parameters, "prePedalLevel", prePedalLevelSlider),
      postPedalLevelAttachment(audioProcessor.parameters, "postPedalLevel", postPedalLevelSlider),
      outputAttachment(audioProcessor.parameters, "output", outputSlider)
{
    setSize(820, 410);

    titleLabel.setText("Guitar Plugin", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);

    for (auto* button : { &prePedalsButton, &ampButton, &cabButton, &eqButton, &postPedalsButton })
        addAndMakeVisible(*button);

    prePedalsButton.onClick = [this] { selectSection(Section::prePedals); };
    ampButton.onClick = [this] { selectSection(Section::amp); };
    cabButton.onClick = [this] { selectSection(Section::cab); };
    eqButton.onClick = [this] { selectSection(Section::eq); };
    postPedalsButton.onClick = [this] { selectSection(Section::postPedals); };

    for (auto* button : { &compressorButton, &driveButton, &fuzzButton,
                          &chimeAmpButton, &leadAmpButton, &twinAmpButton,
                          &blueCabButton, &angledCabButton, &openCabButton,
                          &chorusButton, &delayButton, &reverbButton })
        addAndMakeVisible(*button);

    compressorButton.onClick = [this] { toggleBoolParameter("preCompressor"); };
    driveButton.onClick = [this] { toggleBoolParameter("preDrive"); };
    fuzzButton.onClick = [this] { toggleBoolParameter("preFuzz"); };

    chimeAmpButton.onClick = [this]
    {
        setChoiceParameter("ampModel", 0);
        if (getBoolParameter("cabLinked"))
            setChoiceParameter("cabModel", 0);
        updateGearButtons();
    };

    leadAmpButton.onClick = [this]
    {
        setChoiceParameter("ampModel", 1);
        if (getBoolParameter("cabLinked"))
            setChoiceParameter("cabModel", 1);
        updateGearButtons();
    };

    twinAmpButton.onClick = [this]
    {
        setChoiceParameter("ampModel", 2);
        if (getBoolParameter("cabLinked"))
            setChoiceParameter("cabModel", 2);
        updateGearButtons();
    };

    blueCabButton.onClick = [this]
    {
        if (getBoolParameter("cabLinked"))
            setBoolParameter("cabLinked", false);
        setChoiceParameter("cabModel", 0);
        updateGearButtons();
    };

    angledCabButton.onClick = [this]
    {
        if (getBoolParameter("cabLinked"))
            setBoolParameter("cabLinked", false);
        setChoiceParameter("cabModel", 1);
        updateGearButtons();
    };

    openCabButton.onClick = [this]
    {
        if (getBoolParameter("cabLinked"))
            setBoolParameter("cabLinked", false);
        setChoiceParameter("cabModel", 2);
        updateGearButtons();
    };

    chorusButton.onClick = [this] { toggleBoolParameter("postChorus"); };
    delayButton.onClick = [this] { toggleBoolParameter("postDelay"); };
    reverbButton.onClick = [this] { toggleBoolParameter("postReverb"); };

    cabLinkButton.onClick = [this]
    {
        setBoolParameter("cabLinked", cabLinkButton.getToggleState());
        if (getBoolParameter("cabLinked"))
            setChoiceParameter("cabModel", getChoiceParameter("ampModel"));
        updateGearButtons();
    };
    addAndMakeVisible(cabLinkButton);

    configureSlider(inputSlider, inputLabel, "Input");
    configureSlider(driveSlider, driveLabel, "Gain");
    configureSlider(gateSlider, gateLabel, "Gate");
    configureSlider(masterSlider, masterLabel, "Master");
    configureSlider(bassSlider, bassLabel, "Bass");
    configureSlider(middleSlider, middleLabel, "Middle");
    configureSlider(trebleSlider, trebleLabel, "Treble");
    configureSlider(cabToneSlider, cabToneLabel, "Cab");
    configureSlider(eqLowSlider, eqLowLabel, "Low");
    configureSlider(eqMidSlider, eqMidLabel, "Mid");
    configureSlider(eqHighSlider, eqHighLabel, "High");
    configureSlider(prePedalLevelSlider, prePedalLevelLabel, "Level");
    configureSlider(postPedalLevelSlider, postPedalLevelLabel, "Level");
    configureSlider(outputSlider, outputLabel, "Output");

    statusLabel.setText("Signal chain: pre pedals into amp, cab, EQ, then post pedals.", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);

    selectSection(Section::amp);
    updateGearButtons();
}

void GuitarPluginAudioProcessorEditor::configureSlider(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void GuitarPluginAudioProcessorEditor::selectSection(Section section)
{
    currentSection = section;
    updateSectionVisibility();
    updateGearButtons();
    resized();
}

void GuitarPluginAudioProcessorEditor::setChoiceParameter(const juce::String& parameterId, int choiceIndex)
{
    if (auto* parameter = audioProcessor.parameters.getParameter(parameterId))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(choiceIndex)));
        parameter->endChangeGesture();
    }
}

int GuitarPluginAudioProcessorEditor::getChoiceParameter(const juce::String& parameterId) const
{
    if (auto* value = audioProcessor.parameters.getRawParameterValue(parameterId))
        return static_cast<int>(value->load());

    return 0;
}

void GuitarPluginAudioProcessorEditor::setBoolParameter(const juce::String& parameterId, bool enabled)
{
    if (auto* parameter = audioProcessor.parameters.getParameter(parameterId))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
        parameter->endChangeGesture();
    }
}

bool GuitarPluginAudioProcessorEditor::getBoolParameter(const juce::String& parameterId) const
{
    if (auto* value = audioProcessor.parameters.getRawParameterValue(parameterId))
        return value->load() > 0.5f;

    return false;
}

void GuitarPluginAudioProcessorEditor::toggleBoolParameter(const juce::String& parameterId)
{
    setBoolParameter(parameterId, ! getBoolParameter(parameterId));
    updateGearButtons();
}

void GuitarPluginAudioProcessorEditor::setComponentGroupVisible(bool shouldBeVisible, std::initializer_list<juce::Component*> components)
{
    for (auto* component : components)
        component->setVisible(shouldBeVisible);
}

void GuitarPluginAudioProcessorEditor::updateSectionVisibility()
{
    prePedalsButton.setToggleState(currentSection == Section::prePedals, juce::dontSendNotification);
    ampButton.setToggleState(currentSection == Section::amp, juce::dontSendNotification);
    cabButton.setToggleState(currentSection == Section::cab, juce::dontSendNotification);
    eqButton.setToggleState(currentSection == Section::eq, juce::dontSendNotification);
    postPedalsButton.setToggleState(currentSection == Section::postPedals, juce::dontSendNotification);

    setComponentGroupVisible(currentSection == Section::prePedals,
        { &compressorButton, &driveButton, &fuzzButton, &prePedalLevelSlider, &prePedalLevelLabel });
    setComponentGroupVisible(currentSection == Section::amp,
        { &chimeAmpButton, &leadAmpButton, &twinAmpButton, &inputSlider, &inputLabel, &driveSlider, &driveLabel, &gateSlider, &gateLabel,
          &masterSlider, &masterLabel, &bassSlider, &bassLabel, &middleSlider, &middleLabel,
          &trebleSlider, &trebleLabel, &outputSlider, &outputLabel });
    setComponentGroupVisible(currentSection == Section::cab,
        { &blueCabButton, &angledCabButton, &openCabButton, &cabLinkButton, &cabToneSlider, &cabToneLabel });
    setComponentGroupVisible(currentSection == Section::eq,
        { &eqLowSlider, &eqLowLabel, &eqMidSlider, &eqMidLabel, &eqHighSlider, &eqHighLabel });
    setComponentGroupVisible(currentSection == Section::postPedals,
        { &chorusButton, &delayButton, &reverbButton, &postPedalLevelSlider, &postPedalLevelLabel });
}

void GuitarPluginAudioProcessorEditor::updateGearButtons()
{
    prePedalsButton.setToggleState(currentSection == Section::prePedals, juce::dontSendNotification);
    ampButton.setToggleState(currentSection == Section::amp, juce::dontSendNotification);
    cabButton.setToggleState(currentSection == Section::cab, juce::dontSendNotification);
    eqButton.setToggleState(currentSection == Section::eq, juce::dontSendNotification);
    postPedalsButton.setToggleState(currentSection == Section::postPedals, juce::dontSendNotification);

    compressorButton.setToggleState(getBoolParameter("preCompressor"), juce::dontSendNotification);
    driveButton.setToggleState(getBoolParameter("preDrive"), juce::dontSendNotification);
    fuzzButton.setToggleState(getBoolParameter("preFuzz"), juce::dontSendNotification);

    const auto ampModel = getChoiceParameter("ampModel");
    chimeAmpButton.setToggleState(ampModel == 0, juce::dontSendNotification);
    leadAmpButton.setToggleState(ampModel == 1, juce::dontSendNotification);
    twinAmpButton.setToggleState(ampModel == 2, juce::dontSendNotification);

    const auto cabModel = getChoiceParameter("cabModel");
    blueCabButton.setToggleState(cabModel == 0, juce::dontSendNotification);
    angledCabButton.setToggleState(cabModel == 1, juce::dontSendNotification);
    openCabButton.setToggleState(cabModel == 2, juce::dontSendNotification);

    cabLinkButton.setToggleState(getBoolParameter("cabLinked"), juce::dontSendNotification);

    chorusButton.setToggleState(getBoolParameter("postChorus"), juce::dontSendNotification);
    delayButton.setToggleState(getBoolParameter("postDelay"), juce::dontSendNotification);
    reverbButton.setToggleState(getBoolParameter("postReverb"), juce::dontSendNotification);

    repaint();
}

void GuitarPluginAudioProcessorEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(juce::Colour::fromRGB(18, 19, 21));

    auto bounds = getLocalBounds().toFloat().reduced(14.0f);
    juce::ColourGradient panelGradient(juce::Colour::fromRGB(43, 45, 48), bounds.getTopLeft(),
                                       juce::Colour::fromRGB(24, 25, 27), bounds.getBottomRight(), false);
    graphics.setGradientFill(panelGradient);
    graphics.fillRoundedRectangle(bounds, 8.0f);

    graphics.setColour(juce::Colour::fromRGB(75, 77, 80));
    graphics.drawRoundedRectangle(bounds, 8.0f, 1.5f);

    auto faceplate = bounds.reduced(18.0f).withTrimmedTop(108.0f).withHeight(230.0f);
    graphics.setColour(juce::Colour::fromRGB(34, 31, 28));
    graphics.fillRoundedRectangle(faceplate, 7.0f);
    graphics.setColour(juce::Colour::fromRGB(209, 177, 109).withAlpha(0.40f));
    graphics.drawRoundedRectangle(faceplate, 7.0f, 1.2f);
}

void GuitarPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);

    titleLabel.setBounds(area.removeFromTop(42));
    area.removeFromTop(10);

    auto chainArea = area.removeFromTop(36);
    const auto buttonWidth = chainArea.getWidth() / 5;
    prePedalsButton.setBounds(chainArea.removeFromLeft(buttonWidth).reduced(3));
    ampButton.setBounds(chainArea.removeFromLeft(buttonWidth).reduced(3));
    cabButton.setBounds(chainArea.removeFromLeft(buttonWidth).reduced(3));
    eqButton.setBounds(chainArea.removeFromLeft(buttonWidth).reduced(3));
    postPedalsButton.setBounds(chainArea.reduced(3));
    area.removeFromTop(20);

    auto gearArea = area.removeFromTop(96);
    const auto gearWidth = gearArea.getWidth() / 3;

    auto placeGear = [gearWidth](juce::Rectangle<int>& bounds, juce::Component& component)
    {
        component.setBounds(bounds.removeFromLeft(gearWidth).reduced(5));
    };

    if (currentSection == Section::prePedals)
    {
        placeGear(gearArea, compressorButton);
        placeGear(gearArea, driveButton);
        placeGear(gearArea, fuzzButton);
    }
    else if (currentSection == Section::amp)
    {
        placeGear(gearArea, chimeAmpButton);
        placeGear(gearArea, leadAmpButton);
        placeGear(gearArea, twinAmpButton);
    }
    else if (currentSection == Section::cab)
    {
        placeGear(gearArea, blueCabButton);
        placeGear(gearArea, angledCabButton);
        placeGear(gearArea, openCabButton);
    }
    else if (currentSection == Section::postPedals)
    {
        placeGear(gearArea, chorusButton);
        placeGear(gearArea, delayButton);
        placeGear(gearArea, reverbButton);
    }

    if (currentSection == Section::cab)
    {
        cabLinkButton.setBounds(area.removeFromTop(30).withSizeKeepingCentre(190, 26));
        area.removeFromTop(8);
    }
    else
    {
        area.removeFromTop(38);
    }

    auto knobArea = area.removeFromTop(150);
    auto visibleKnobs = 1;

    if (currentSection == Section::amp)
        visibleKnobs = 8;
    else if (currentSection == Section::eq)
        visibleKnobs = 3;

    const auto knobWidth = knobArea.getWidth() / visibleKnobs;

    auto placeKnob = [knobWidth](juce::Rectangle<int>& bounds, juce::Slider& slider, juce::Label& label)
    {
        auto cell = bounds.removeFromLeft(knobWidth).reduced(4);
        label.setBounds(cell.removeFromTop(20));
        slider.setBounds(cell);
    };

    if (currentSection == Section::prePedals)
    {
        placeKnob(knobArea, prePedalLevelSlider, prePedalLevelLabel);
        statusLabel.setText("Pre pedals happen before the amp. This placeholder level will become drives, boosts, wah, and compression.", juce::dontSendNotification);
    }
    else if (currentSection == Section::amp)
    {
        placeKnob(knobArea, inputSlider, inputLabel);
        placeKnob(knobArea, driveSlider, driveLabel);
        placeKnob(knobArea, gateSlider, gateLabel);
        placeKnob(knobArea, masterSlider, masterLabel);
        placeKnob(knobArea, bassSlider, bassLabel);
        placeKnob(knobArea, middleSlider, middleLabel);
        placeKnob(knobArea, trebleSlider, trebleLabel);
        placeKnob(knobArea, outputSlider, outputLabel);
        statusLabel.setText("Amp controls: input, gain, gate, master, amp EQ, and output.", juce::dontSendNotification);
    }
    else if (currentSection == Section::cab)
    {
        placeKnob(knobArea, cabToneSlider, cabToneLabel);
        statusLabel.setText("Cab section is a starter cab tone now. Later this becomes IR or NAM-related cab loading.", juce::dontSendNotification);
    }
    else if (currentSection == Section::eq)
    {
        placeKnob(knobArea, eqLowSlider, eqLowLabel);
        placeKnob(knobArea, eqMidSlider, eqMidLabel);
        placeKnob(knobArea, eqHighSlider, eqHighLabel);
        statusLabel.setText("EQ section is after the cab for final tone shaping.", juce::dontSendNotification);
    }
    else
    {
        placeKnob(knobArea, postPedalLevelSlider, postPedalLevelLabel);
        statusLabel.setText("Post pedals happen after amp, cab, and EQ. This placeholder can become delay, reverb, chorus, and more.", juce::dontSendNotification);
    }

    statusLabel.setBounds(area.removeFromTop(32));
}
