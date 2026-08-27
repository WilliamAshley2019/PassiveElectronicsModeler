/*
  ==============================================================================
  AlphaAudio Passive Component - Editor Implementation
  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// ValueEntryComponent
//==============================================================================
ValueEntryComponent::ValueEntryComponent(juce::AudioProcessorValueTreeState& apvts,
                                          const juce::String& paramID,
                                          const juce::String& unit)
    : currentUnit(unit)
{
    setOpaque(false);

    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(1.0, 10000000.0, 0.01);
    slider.setSkewFactor(0.25);
    slider.addListener(this);
    addAndMakeVisible(slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider);

    textEditor.setMultiLine(false);
    textEditor.setReturnKeyStartsNewLine(false);
    textEditor.setJustification(juce::Justification::centred);
    textEditor.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    textEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF2A2A2A));
    textEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    textEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xFF444444));
    textEditor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::cyan);
    textEditor.addListener(this);
    addAndMakeVisible(textEditor);

    unitLabel.setText(unit, juce::dontSendNotification);
    unitLabel.setFont(juce::Font(juce::FontOptions(14.0f)));
    unitLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    unitLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(unitLabel);

    updateTextFromSlider();
}

ValueEntryComponent::~ValueEntryComponent() = default;

void ValueEntryComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xFF252525));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1), 6.0f);
}

void ValueEntryComponent::resized()
{
    auto bounds = getLocalBounds().reduced(4);
    auto sliderArea = bounds.removeFromLeft(bounds.getHeight());
    slider.setBounds(sliderArea);

    auto unitW = 40;
    auto textArea = bounds.removeFromLeft(bounds.getWidth() - unitW);
    unitLabel.setBounds(bounds);
    textEditor.setBounds(textArea.reduced(2));
}

void ValueEntryComponent::setUnit(const juce::String& newUnit)
{
    currentUnit = newUnit;
    unitLabel.setText(newUnit, juce::dontSendNotification);
    updateTextFromSlider();
}

void ValueEntryComponent::sliderValueChanged(juce::Slider*)
{
    if (!updating)
    {
        updating = true;
        updateTextFromSlider();
        updating = false;
    }
}

void ValueEntryComponent::textEditorReturnKeyPressed(juce::TextEditor&) { updateSliderFromText(); }
void ValueEntryComponent::textEditorFocusLost(juce::TextEditor&) { updateSliderFromText(); }

void ValueEntryComponent::updateTextFromSlider()
{
    float val = static_cast<float>(slider.getValue());
    textEditor.setText(ComponentPluginAudioProcessor::formatValueWithPrefix(val, currentUnit), false);
}

void ValueEntryComponent::updateSliderFromText()
{
    if (updating) return;
    updating = true;

    float parsed = ComponentPluginAudioProcessor::parseEngineeringValue(textEditor.getText(), currentUnit);
    parsed = juce::jlimit(1.0f, 10000000.0f, parsed);
    slider.setValue(parsed, juce::sendNotificationSync);
    updateTextFromSlider();

    updating = false;
}

//==============================================================================
// CategoryButton
//==============================================================================
CategoryButton::CategoryButton(const juce::String& name, ComponentCategory cat)
    : juce::Button(name), category(cat)
{
    switch (cat)
    {
        case ComponentCategory::Resistor:  accentColour = juce::Colour(0xFFFFA500); break;
        case ComponentCategory::Capacitor: accentColour = juce::Colour(0xFF00BFFF); break;
        case ComponentCategory::Inductor:  accentColour = juce::Colour(0xFF32CD32); break;
    }
    setClickingTogglesState(true);
}

void CategoryButton::paintButton(juce::Graphics& g, bool highlighted, bool /*down*/)
{
    auto bounds = getLocalBounds().toFloat().reduced(4);
    bool selected = getToggleState();

    if (selected)      g.setColour(accentColour.withAlpha(0.25f));
    else if (highlighted) g.setColour(juce::Colour(0xFF333333));
    else               g.setColour(juce::Colour(0xFF222222));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(selected ? accentColour : juce::Colour(0xFF444444));
    g.drawRoundedRectangle(bounds, 8.0f, selected ? 2.0f : 1.0f);

    auto iconArea = bounds.removeFromTop(bounds.getHeight() * 0.55f).reduced(8);
    g.setColour(selected ? accentColour : accentColour.withAlpha(0.6f));

    juce::Path p;
    auto cx = iconArea.getCentreX();
    auto cy = iconArea.getCentreY();
    auto w = iconArea.getWidth();
    auto h = iconArea.getHeight() * 0.5f;

    switch (category)
    {
        case ComponentCategory::Resistor:
        {
            float segW = w / 7.0f;
            p.startNewSubPath(cx - w * 0.4f, cy);
            for (int i = 0; i < 6; ++i)
            {
                float x = cx - w * 0.4f + (i + 1) * segW;
                float y = cy + ((i % 2 == 0) ? -h * 0.5f : h * 0.5f);
                p.lineTo(x, y);
            }
            p.lineTo(cx + w * 0.4f, cy);
            break;
        }
        case ComponentCategory::Capacitor:
        {
            float gap = w * 0.15f;
            p.startNewSubPath(cx - gap, cy - h * 0.5f); p.lineTo(cx - gap, cy + h * 0.5f);
            p.startNewSubPath(cx + gap, cy - h * 0.5f); p.lineTo(cx + gap, cy + h * 0.5f);
            p.startNewSubPath(cx - w * 0.4f, cy);       p.lineTo(cx - gap, cy);
            p.startNewSubPath(cx + gap, cy);            p.lineTo(cx + w * 0.4f, cy);
            break;
        }
        case ComponentCategory::Inductor:
        {
            float loopW = w / 5.0f;
            float r = loopW * 0.4f;
            p.startNewSubPath(cx - w * 0.4f, cy);
            for (int i = 0; i < 4; ++i)
            {
                float lx = cx - w * 0.4f + i * loopW + loopW * 0.5f;
                p.addArc(lx - r, cy - r, r * 2, r * 2, 0.0f, juce::MathConstants<float>::pi, true);
            }
            p.lineTo(cx + w * 0.4f, cy);
            break;
        }
    }

    g.strokePath(p, juce::PathStrokeType(2.0f));

    g.setColour(selected ? juce::Colours::white : juce::Colours::lightgrey);
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawFittedText(getButtonText(), bounds.toNearestInt(), juce::Justification::centred, 1);
}

//==============================================================================
// Main Editor
//==============================================================================
ComponentPluginAudioProcessorEditor::ComponentPluginAudioProcessorEditor(ComponentPluginAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // NOTE: setSize() is deliberately NOT called here. Component::setSize()
    // synchronously fires resized() the moment the size actually changes, and
    // resized() dereferences valueEntry (a unique_ptr not yet constructed at
    // the top of this constructor), several other members, etc. Calling it
    // here was the actual root cause of the crash on every host and the
    // Standalone build alike (confirmed via debugger: access violation in
    // Component::setBounds, called from resized(), called from setSize(),
    // called from line 1 of this constructor, on a still-null valueEntry).
    // setSize() now happens at the very end of this constructor instead, once
    // every component resized() touches actually exists.

    resistorButton.setRadioGroupId(1001);
    capacitorButton.setRadioGroupId(1001);
    inductorButton.setRadioGroupId(1001);
    resistorButton.addListener(this);
    capacitorButton.addListener(this);
    inductorButton.addListener(this);
    addAndMakeVisible(resistorButton);
    addAndMakeVisible(capacitorButton);
    addAndMakeVisible(inductorButton);

    modelLabel.setText("Model:", juce::dontSendNotification);
    modelLabel.setFont(getPluginFont(13.0f, true));
    modelLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(modelLabel);

    modelComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF2A2A2A));
    modelComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    modelComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xFF444444));
    modelComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::lightgrey);
    modelComboBox.addListener(this);
    addAndMakeVisible(modelComboBox);

    valueEntry = std::make_unique<ValueEntryComponent>(audioProcessor.getAPVTS(), ParamIDs::componentValue,
                                                        juce::String(juce::CharPointer_UTF8("\xce\xa9")));
    addAndMakeVisible(*valueEntry);

    valueEntryLabel.setText("Value:", juce::dontSendNotification);
    valueEntryLabel.setFont(getPluginFont(13.0f, true));
    valueEntryLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(valueEntryLabel);

    modeLabel.setText("Mode:", juce::dontSendNotification);
    modeLabel.setFont(getPluginFont(13.0f, true));
    modeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(modeLabel);

    modeButton.setButtonText("Ideal");
    modeButton.setClickingTogglesState(true);
    modeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF2A2A2A));
    modeButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF006400));
    modeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    modeButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(modeButton);

    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParamIDs::operatingMode, modeButton);

    dryWetLabel.setText("Dry / Wet:", juce::dontSendNotification);
    dryWetLabel.setFont(getPluginFont(13.0f, true));
    dryWetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(dryWetLabel);

    dryWetSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    dryWetSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    dryWetSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF444444));
    dryWetSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(dryWetSlider);
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParamIDs::dryWet, dryWetSlider);

    infoButton.setButtonText("?");
    infoButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    infoButton.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    infoButton.addListener(this);
    addAndMakeVisible(infoButton);

    valueDisplayLabel.setFont(getPluginFont(28.0f, true));
    valueDisplayLabel.setJustificationType(juce::Justification::centred);
    valueDisplayLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(valueDisplayLabel);

    reactanceLabel.setFont(getPluginFont(12.0f));
    reactanceLabel.setJustificationType(juce::Justification::centredLeft);
    reactanceLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    addAndMakeVisible(reactanceLabel);

    impedanceLabel.setFont(getPluginFont(12.0f));
    impedanceLabel.setJustificationType(juce::Justification::centredLeft);
    impedanceLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    addAndMakeVisible(impedanceLabel);

    frequencyLabel.setFont(getPluginFont(12.0f));
    frequencyLabel.setJustificationType(juce::Justification::centredLeft);
    frequencyLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFAAAAAA));
    addAndMakeVisible(frequencyLabel);

    descriptionLabel.setFont(getPluginFont(11.0f, false));
    descriptionLabel.setJustificationType(juce::Justification::centred);
    descriptionLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF888888));
    addAndMakeVisible(descriptionLabel);

    formulaLabel.setFont(getPluginFont(13.0f, true));
    formulaLabel.setJustificationType(juce::Justification::centred);
    formulaLabel.setColour(juce::Label::textColourId, juce::Colour(0xFFCCCCCC));
    addAndMakeVisible(formulaLabel);

    modelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParamIDs::componentModel, modelComboBox);

    updateCategoryUI();
    updateModelComboBox();
    updateDisplays();

    // All child components now exist -- safe to trigger the first resized().
    setSize(760, 560);
    setResizable(false, false);

    startTimerHz(30);
}

ComponentPluginAudioProcessorEditor::~ComponentPluginAudioProcessorEditor() = default;

//==============================================================================
juce::Font ComponentPluginAudioProcessorEditor::getPluginFont(float size, bool bold) const
{
    return juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
void ComponentPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF151515));

    g.setColour(juce::Colour(0xFF1E1E1E));
    g.fillRect(0, 0, getWidth(), 50);
    g.setColour(juce::Colour(0xFF333333));
    g.drawLine(0, 50, (float) getWidth(), 50, 1.0f);

    g.setFont(getPluginFont(18.0f, true));
    g.setColour(juce::Colours::white);
    g.drawText("AlphaAudio Passive Component Modeler", 20, 0, 420, 50, juce::Justification::centredLeft);

    if (!schematicArea.isEmpty())
    {
        g.setColour(juce::Colour(0xFF1E1E1E));
        g.fillRoundedRectangle(schematicArea.toFloat(), 12.0f);
        g.setColour(juce::Colour(0xFF333333));
        g.drawRoundedRectangle(schematicArea.toFloat(), 12.0f, 1.5f);

        g.setColour(juce::Colour(0xFF252525));
        for (int x = schematicArea.getX() + 20; x < schematicArea.getRight(); x += 20)
            g.drawVerticalLine(x, (float) schematicArea.getY() + 10, (float) schematicArea.getBottom() - 10);
        for (int y = schematicArea.getY() + 20; y < schematicArea.getBottom(); y += 20)
            g.drawHorizontalLine(y, (float) schematicArea.getX() + 10, (float) schematicArea.getRight() - 10);
    }

    drawCircuitPath(g, schematicArea);

    g.setFont(getPluginFont(20.0f, true));
    g.setColour(getCategoryColour());
    auto valueBounds = schematicArea.toFloat();
    auto textRect = juce::Rectangle<float>(valueBounds.getCentreX() - 100, valueBounds.getBottom() - 40, 200, 30);
    g.drawText(audioProcessor.getFormattedValue(), textRect, juce::Justification::centred, true);
}

//==============================================================================
/** Draws the actual signal path this plugin implements: IN -> the selected
    passive component -> OUT, with the fixed 1k reference load it works
    against shown dropping to ground. This is what the DSP is really doing,
    not just a floating symbol -- so turning the value knob and watching the
    attenuation/roll-off change means something concrete. */
void ComponentPluginAudioProcessorEditor::drawCircuitPath(juce::Graphics& g, juce::Rectangle<int> area)
{
    if (area.isEmpty())
        return;

    auto bounds = area.toFloat().reduced(24.0f, 36.0f);
    float cy = bounds.getCentreY() - 14.0f;

    float nodeR = 5.0f;
    float inX = bounds.getX();
    float outX = bounds.getRight();
    float componentBoundsMargin = bounds.getWidth() * 0.16f;

    juce::Rectangle<float> componentBounds(bounds.getX() + componentBoundsMargin, cy - 34.0f,
                                            bounds.getWidth() - componentBoundsMargin * 2.0f, 68.0f);

    juce::Colour wireColour = juce::Colour(0xFF888888);

    // IN wire + node + label
    g.setColour(wireColour);
    g.drawLine(inX, cy, componentBounds.getX(), cy, 2.0f);
    g.setColour(juce::Colours::lightgrey);
    g.fillEllipse(inX - nodeR, cy - nodeR, nodeR * 2, nodeR * 2);
    g.setFont(getPluginFont(11.0f, true));
    g.drawText("IN", juce::Rectangle<float>(inX - 20.0f, cy - 26.0f, 40.0f, 16.0f), juce::Justification::centred);

    // OUT wire + node + label
    g.setColour(wireColour);
    g.drawLine(componentBounds.getRight(), cy, outX, cy, 2.0f);
    g.setColour(juce::Colours::lightgrey);
    g.fillEllipse(outX - nodeR, cy - nodeR, nodeR * 2, nodeR * 2);
    g.drawText("OUT", juce::Rectangle<float>(outX - 20.0f, cy - 26.0f, 40.0f, 16.0f), juce::Justification::centred);

    // Fixed reference load, drawn dropping to a ground symbol below OUT --
    // this is the load every component in this plugin is working against.
    float loadX = outX - 6.0f;
    float loadTopY = cy + 14.0f;
    float loadBotY = cy + 54.0f;
    g.setColour(wireColour);
    g.drawLine(loadX, cy, loadX, loadTopY, 2.0f);
    juce::Rectangle<float> loadBody(loadX - 7.0f, loadTopY, 14.0f, loadBotY - loadTopY);
    g.drawRect(loadBody, 1.5f);
    g.drawLine(loadX, loadBotY, loadX, loadBotY + 10.0f, 2.0f);

    // Ground symbol
    float gy = loadBotY + 10.0f;
    g.drawLine(loadX - 10.0f, gy, loadX + 10.0f, gy, 2.0f);
    g.drawLine(loadX - 6.0f,  gy + 4.0f, loadX + 6.0f,  gy + 4.0f, 2.0f);
    g.drawLine(loadX - 2.0f,  gy + 8.0f, loadX + 2.0f,  gy + 8.0f, 2.0f);

    g.setFont(getPluginFont(9.0f));
    g.setColour(juce::Colour(0xFF777777));
    g.drawText("1k ref load", juce::Rectangle<float>(loadX - 40.0f, loadBotY + 12.0f, 80.0f, 14.0f), juce::Justification::centred);

    // The component itself, drawn with the real schematic symbol methods.
    bool isRealWorld = audioProcessor.getOperatingMode() == OperatingMode::RealWorld;
    componentBounds.setY(cy - 34.0f);
    drawComponentSymbol(g, componentBounds, isRealWorld);
}

void ComponentPluginAudioProcessorEditor::drawComponentSymbol(juce::Graphics& g, juce::Rectangle<float> bounds, bool isSelected)
{
    switch (audioProcessor.getComponentCategory())
    {
        case ComponentCategory::Resistor:  drawResistorSymbol(g, bounds, isSelected);  break;
        case ComponentCategory::Capacitor: drawCapacitorSymbol(g, bounds, isSelected); break;
        case ComponentCategory::Inductor:  drawInductorSymbol(g, bounds, isSelected);  break;
    }
}

//==============================================================================
void ComponentPluginAudioProcessorEditor::resized()
{
    // Defensive: resized() can in principle fire before every child component
    // exists (e.g. if a future edit reintroduces an early setSize(), or a
    // subclass calls resized() directly). valueEntry is the one component
    // here stored as a unique_ptr rather than a plain member, so it's the one
    // that needs this guard -- this is exactly the shape of bug that crashed
    // every build until the setSize() ordering fix. Kept here as a cheap
    // second line of defense, and as the pattern to repeat in future
    // plugins (e.g. Circuit Builder's array of unique_ptr<ComponentModel>
    // slots will have the same hazard).
    if (!valueEntry)
        return;

    auto area = getLocalBounds();
    auto header = area.removeFromTop(50);
    infoButton.setBounds(header.removeFromRight(50).withSizeKeepingCentre(30, 30));

    auto sidebar = area.removeFromLeft(110);
    auto buttonH = sidebar.getHeight() / 3 - 10;
    resistorButton.setBounds(sidebar.removeFromTop(buttonH).reduced(4));
    capacitorButton.setBounds(sidebar.removeFromTop(buttonH).reduced(4));
    inductorButton.setBounds(sidebar.removeFromTop(buttonH).reduced(4));

    auto controlPanel = area.removeFromRight(240).reduced(10, 10);

    modelLabel.setBounds(controlPanel.removeFromTop(20));
    modelComboBox.setBounds(controlPanel.removeFromTop(32));
    controlPanel.removeFromTop(16);

    valueEntryLabel.setBounds(controlPanel.removeFromTop(20));
    valueEntry->setBounds(controlPanel.removeFromTop(70));
    controlPanel.removeFromTop(16);

    modeLabel.setBounds(controlPanel.removeFromTop(20));
    modeButton.setBounds(controlPanel.removeFromTop(36));
    controlPanel.removeFromTop(16);

    dryWetLabel.setBounds(controlPanel.removeFromTop(20));
    dryWetSlider.setBounds(controlPanel.removeFromTop(28));

    schematicArea = area.reduced(10, 10);
    schematicArea.removeFromBottom(80);

    auto infoStrip = area.removeFromBottom(80).reduced(10, 0);

    valueDisplayLabel.setBounds(infoStrip.removeFromTop(30));

    auto statsRow = infoStrip.removeFromTop(22);
    reactanceLabel.setBounds(statsRow.removeFromLeft(statsRow.getWidth() / 3));
    impedanceLabel.setBounds(statsRow.removeFromLeft(statsRow.getWidth() / 2));
    frequencyLabel.setBounds(statsRow);

    descriptionLabel.setBounds(infoStrip.removeFromTop(28));
}

//==============================================================================
void ComponentPluginAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &resistorButton)
    {
        audioProcessor.getAPVTS().getParameterAsValue(ParamIDs::componentCategory).setValue(0);
        updateCategoryUI();
    }
    else if (button == &capacitorButton)
    {
        audioProcessor.getAPVTS().getParameterAsValue(ParamIDs::componentCategory).setValue(1);
        updateCategoryUI();
    }
    else if (button == &inductorButton)
    {
        audioProcessor.getAPVTS().getParameterAsValue(ParamIDs::componentCategory).setValue(2);
        updateCategoryUI();
    }
    else if (button == &infoButton)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "About Passive Component Modeler",
            "Models a single passive component (R, C, or L) in series with a fixed 1k reference load.\n\n"
            "Component Types:\n"
            "  \xe2\x80\xa2 Resistors: Carbon Comp, Carbon Film, Metal Film, Wirewound, SMD\n"
            "  \xe2\x80\xa2 Capacitors: Ceramic, Electrolytic, Polyester, Polypropylene, Tantalum\n"
            "  \xe2\x80\xa2 Inductors: Air Core, Ferrite, Iron Core, Toroidal, Powdered Iron\n\n"
            "Ideal Mode: perfect theoretical behavior.\n"
            "Real-World Mode: parasitics, non-linearity, noise, and aging.\n\n"
            "Value entry accepts engineering prefixes (4.7k, 10n, 2.2M).\n"
            "Dry/Wet blends the processed path back with the untouched signal."
        );
    }
}

void ComponentPluginAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &modelComboBox)
    {
        updateDisplays();
        repaint();
    }
}

void ComponentPluginAudioProcessorEditor::timerCallback()
{
    updateDisplays();

    bool realWorld = audioProcessor.getOperatingMode() == OperatingMode::RealWorld;
    modeButton.setButtonText(realWorld ? "Real-World" : "Ideal");

    auto cat = audioProcessor.getComponentCategory();
    resistorButton.setToggleState(cat == ComponentCategory::Resistor, juce::dontSendNotification);
    capacitorButton.setToggleState(cat == ComponentCategory::Capacitor, juce::dontSendNotification);
    inductorButton.setToggleState(cat == ComponentCategory::Inductor, juce::dontSendNotification);

    repaint(schematicArea);
}

//==============================================================================
void ComponentPluginAudioProcessorEditor::updateCategoryUI()
{
    auto cat = audioProcessor.getComponentCategory();

    resistorButton.setToggleState(cat == ComponentCategory::Resistor, juce::dontSendNotification);
    capacitorButton.setToggleState(cat == ComponentCategory::Capacitor, juce::dontSendNotification);
    inductorButton.setToggleState(cat == ComponentCategory::Inductor, juce::dontSendNotification);

    juce::String unit;
    switch (cat)
    {
        case ComponentCategory::Resistor:  unit = juce::CharPointer_UTF8("\xce\xa9"); break;
        case ComponentCategory::Capacitor: unit = "F"; break;
        case ComponentCategory::Inductor:  unit = "H"; break;
    }
    valueEntry->setUnit(unit);

    switch (cat)
    {
        case ComponentCategory::Resistor:
            formulaLabel.setText(juce::CharPointer_UTF8("R = V / I    |    P = V\xc2\xb2 / R"), juce::dontSendNotification);
            break;
        case ComponentCategory::Capacitor:
            formulaLabel.setText(juce::CharPointer_UTF8("Xc = 1 / (2" "\xcf\x80" "fC)    |    " "\xcf\x84" " = RC"), juce::dontSendNotification);
            break;
        case ComponentCategory::Inductor:
            formulaLabel.setText(juce::CharPointer_UTF8("Xl = 2" "\xcf\x80" "fL    |    Q = " "\xcf\x89" "L / R"), juce::dontSendNotification);
            break;
    }

    updateModelComboBox();
    updateDisplays();
    repaint();
}

void ComponentPluginAudioProcessorEditor::updateModelComboBox()
{
    modelComboBox.clear();
    auto cat = audioProcessor.getComponentCategory();

    juce::StringArray models;
    switch (cat)
    {
        case ComponentCategory::Resistor:  models = { "Carbon Composition", "Carbon Film", "Metal Film", "Wirewound", "Thick Film SMD" }; break;
        case ComponentCategory::Capacitor: models = { "Ceramic Disc", "Electrolytic", "Polyester Film", "Polypropylene", "Tantalum" }; break;
        case ComponentCategory::Inductor:  models = { "Air Core", "Ferrite Rod", "Iron Core", "Toroidal", "Powdered Iron" }; break;
    }

    for (int i = 0; i < models.size(); ++i)
        modelComboBox.addItem(models[i], i + 1);

    int currentModel = juce::jlimit(1, models.size(), audioProcessor.getComponentModelIndex() + 1);
    modelComboBox.setSelectedId(currentModel, juce::dontSendNotification);
}

void ComponentPluginAudioProcessorEditor::updateDisplays()
{
    valueDisplayLabel.setText(audioProcessor.getFormattedValue(), juce::dontSendNotification);

    // Reactance/impedance at a fixed 1kHz reference point (matches the formula
    // labels) -- the live-audio frequency estimate is shown separately below.
    float refFreq = 1000.0f;
    float reactance = audioProcessor.getReactanceAtFrequency(refFreq);
    float impedance = audioProcessor.getImpedanceAtFrequency(refFreq);
    float liveFreq = audioProcessor.getLastDetectedFrequency();

    reactanceLabel.setText("Reactance: " + juce::String(reactance, 2) + juce::String(juce::CharPointer_UTF8(" \xce\xa9")), juce::dontSendNotification);
    impedanceLabel.setText("Impedance: " + juce::String(impedance, 2) + juce::String(juce::CharPointer_UTF8(" \xce\xa9")), juce::dontSendNotification);
    frequencyLabel.setText("@ 1 kHz (live: " + juce::String(liveFreq, 0) + " Hz)", juce::dontSendNotification);

    descriptionLabel.setText(audioProcessor.getRealWorldStatusText(), juce::dontSendNotification);
}

//==============================================================================
juce::Colour ComponentPluginAudioProcessorEditor::getCategoryColour() const
{
    switch (audioProcessor.getComponentCategory())
    {
        case ComponentCategory::Resistor:  return juce::Colour(0xFFFFA500);
        case ComponentCategory::Capacitor: return juce::Colour(0xFF00BFFF);
        case ComponentCategory::Inductor:  return juce::Colour(0xFF32CD32);
    }
    return juce::Colours::white;
}

juce::Colour ComponentPluginAudioProcessorEditor::getCategoryColourDark() const { return getCategoryColour().withAlpha(0.3f); }

//==============================================================================
void ComponentPluginAudioProcessorEditor::drawResistorSymbol(juce::Graphics& g, juce::Rectangle<float> bounds, bool isSelected)
{
    auto colour = getCategoryColour();
    int modelIdx = audioProcessor.getComponentModelIndex();

    g.setColour(juce::Colours::grey);
    float leadL = bounds.getWidth() * 0.15f;
    float cy = bounds.getCentreY();
    g.drawLine(bounds.getX(), cy, bounds.getX() + leadL, cy, 2.0f);
    g.drawLine(bounds.getRight() - leadL, cy, bounds.getRight(), cy, 2.0f);

    auto body = bounds.reduced(leadL, bounds.getHeight() * 0.3f);
    drawResistorZigzag(g, body, colour, isSelected ? 3.5f : 2.5f);

    if (modelIdx == 3)
    {
        g.setColour(colour.withAlpha(0.5f));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        auto wwBounds = body.withSizeKeepingCentre(30, 16).translated(0, -body.getHeight() * 0.4f);
        g.drawText("WW", wwBounds, juce::Justification::centred);
    }

    if (isSelected)
    {
        g.setColour(juce::Colours::red.withAlpha(0.6f));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        auto textPos = bounds.getBottomLeft().translated(0, -16);
        g.drawText(juce::CharPointer_UTF8("\xc2\xb1THERMAL"), juce::Rectangle<float>(textPos.x, textPos.y, 60.0f, 14.0f), juce::Justification::left);
    }
}

void ComponentPluginAudioProcessorEditor::drawCapacitorSymbol(juce::Graphics& g, juce::Rectangle<float> bounds, bool isSelected)
{
    auto colour = getCategoryColour();
    int modelIdx = audioProcessor.getComponentModelIndex();

    float cy = bounds.getCentreY();
    float plateGap = 18.0f;
    float plateH = bounds.getHeight() * 0.5f;
    float cx = bounds.getCentreX();

    g.setColour(juce::Colours::grey);
    g.drawLine(bounds.getX(), cy, cx - plateGap * 0.5f, cy, 2.0f);
    g.drawLine(cx + plateGap * 0.5f, cy, bounds.getRight(), cy, 2.0f);

    g.setColour(colour);
    float plateThick = (modelIdx == 1 || modelIdx == 4) ? 4.0f : 3.0f;

    g.fillRect(cx - plateGap * 0.5f - 2.0f, cy - plateH * 0.5f, plateThick, plateH);

    if (modelIdx == 1 || modelIdx == 4)
    {
        juce::Path curvedPlate;
        curvedPlate.addArc(cx + plateGap * 0.5f - 2.0f, cy - plateH * 0.5f, plateThick * 2, plateH,
                            -juce::MathConstants<float>::pi * 0.5f, juce::MathConstants<float>::pi * 0.5f, false);
        g.fillPath(curvedPlate);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
        g.drawText("+", juce::Rectangle<float>(cx - 24, cy - 20, 16, 16), juce::Justification::centred);

        if (modelIdx == 1)
        {
            g.setColour(colour.withAlpha(0.4f));
            for (float dy = -plateH * 0.4f; dy < plateH * 0.4f; dy += 6.0f)
                g.drawLine(cx + plateGap * 0.5f + 6.0f, cy + dy, cx + plateGap * 0.5f + 14.0f, cy + dy, 1.0f);
        }
    }
    else
    {
        g.fillRect(cx + plateGap * 0.5f - 2.0f, cy - plateH * 0.5f, plateThick, plateH);
    }

    if (isSelected)
    {
        g.setColour(juce::Colours::red.withAlpha(0.6f));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        juce::String label = (modelIdx == 1) ? juce::String("ESR+DA") : juce::String(juce::CharPointer_UTF8("\xc2\xb5PHONIC"));
        auto textPos = bounds.getBottomLeft().translated(0, -16);
        g.drawText(label, juce::Rectangle<float>(textPos.x, textPos.y, 60.0f, 14.0f), juce::Justification::left);
    }
}

void ComponentPluginAudioProcessorEditor::drawInductorSymbol(juce::Graphics& g, juce::Rectangle<float> bounds, bool isSelected)
{
    auto colour = getCategoryColour();
    int modelIdx = audioProcessor.getComponentModelIndex();

    float cy = bounds.getCentreY();
    float leadL = bounds.getWidth() * 0.12f;

    g.setColour(juce::Colours::grey);
    g.drawLine(bounds.getX(), cy, bounds.getX() + leadL, cy, 2.0f);
    g.drawLine(bounds.getRight() - leadL, cy, bounds.getRight(), cy, 2.0f);

    auto body = bounds.reduced(leadL, bounds.getHeight() * 0.25f);
    drawInductorLoops(g, body, colour, isSelected ? 3.5f : 2.5f);

    if (modelIdx == 2 || modelIdx == 3 || modelIdx == 4)
    {
        g.setColour(juce::Colours::grey);
        float coreY = cy + body.getHeight() * 0.35f;
        float coreW = body.getWidth() * 0.85f;
        float coreX = body.getCentreX() - coreW * 0.5f;

        if (modelIdx == 3)
        {
            for (float dx = 0; dx < coreW; dx += 8.0f)
                g.drawLine(coreX + dx, coreY, coreX + juce::jmin(dx + 4.0f, coreW), coreY, 2.0f);
        }
        else
        {
            g.drawLine(coreX, coreY, coreX + coreW, coreY, 2.0f);
            if (modelIdx == 2)
                g.drawLine(coreX, coreY + 4.0f, coreX + coreW, coreY + 4.0f, 2.0f);
        }
    }

    if (isSelected)
    {
        g.setColour(juce::Colours::red.withAlpha(0.6f));
        g.setFont(juce::Font(juce::FontOptions(10.0f)));
        juce::String label = (modelIdx == 2) ? "SATURATION" : "HYSTERESIS";
        auto textPos = bounds.getBottomLeft().translated(0, -16);
        g.drawText(label, juce::Rectangle<float>(textPos.x, textPos.y, 70.0f, 14.0f), juce::Justification::left);
    }
}

//==============================================================================
void ComponentPluginAudioProcessorEditor::drawResistorZigzag(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                              juce::Colour colour, float thickness)
{
    juce::Path p;
    int segments = 10;
    float segW = bounds.getWidth() / segments;
    float halfH = bounds.getHeight() * 0.35f;
    float x = bounds.getX();
    float cy = bounds.getCentreY();

    p.startNewSubPath(x, cy);
    for (int i = 0; i < segments; ++i)
    {
        float xPos = x + (i + 1) * segW;
        float yPos = (i % 2 == 0) ? cy - halfH : cy + halfH;
        if (i == segments - 1) yPos = cy;
        p.lineTo(xPos, yPos);
    }

    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void ComponentPluginAudioProcessorEditor::drawCapacitorLines(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                              juce::Colour colour, float thickness, int modelIndex)
{
    juce::ignoreUnused(modelIndex);
    float cy = bounds.getCentreY();
    float cx = bounds.getCentreX();
    float gap = 12.0f;
    float h = bounds.getHeight() * 0.6f;

    g.setColour(colour);
    g.drawLine(cx - gap * 0.5f, cy - h * 0.5f, cx - gap * 0.5f, cy + h * 0.5f, thickness);
    g.drawLine(cx + gap * 0.5f, cy - h * 0.5f, cx + gap * 0.5f, cy + h * 0.5f, thickness);
}

void ComponentPluginAudioProcessorEditor::drawInductorLoops(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                             juce::Colour colour, float thickness)
{
    juce::Path p;
    int loops = 5;
    float loopW = bounds.getWidth() / loops;
    float r = loopW * 0.38f;
    float x = bounds.getX();
    float cy = bounds.getCentreY();

    p.startNewSubPath(x, cy);
    for (int i = 0; i < loops; ++i)
    {
        float lx = x + i * loopW + loopW * 0.5f;
        p.addArc(lx - r, cy - r, r * 2.0f, r * 2.0f, 0.0f, juce::MathConstants<float>::pi, true);
    }
    p.lineTo(bounds.getRight(), cy);

    g.setColour(colour);
    g.strokePath(p, juce::PathStrokeType(thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}
