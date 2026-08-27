/*
  ==============================================================================
  AlphaAudio Passive Component - Editor
  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/** Custom value entry: slider + text editor with engineering notation. */
class ValueEntryComponent : public juce::Component,
                            private juce::Slider::Listener,
                            private juce::TextEditor::Listener
{
public:
    ValueEntryComponent(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID, const juce::String& unit);
    ~ValueEntryComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void setUnit(const juce::String& newUnit);

private:
    void sliderValueChanged(juce::Slider* slider) override;
    void textEditorTextChanged(juce::TextEditor&) override {}
    void textEditorReturnKeyPressed(juce::TextEditor&) override;
    void textEditorFocusLost(juce::TextEditor&) override;

    void updateTextFromSlider();
    void updateSliderFromText();

    juce::Slider slider;
    juce::TextEditor textEditor;
    juce::Label unitLabel;
    juce::String currentUnit;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    bool updating = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ValueEntryComponent)
};

//==============================================================================
/** Category selector button with schematic icon. */
class CategoryButton : public juce::Button
{
public:
    CategoryButton(const juce::String& name, ComponentCategory cat);
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    ComponentCategory category;
    juce::Colour accentColour;
};

//==============================================================================
/** Main editor: professional schematic display, typed value entry, and a real
    drawn signal-path circuit (source -> component -> load) rather than a
    disconnected symbol. */
class ComponentPluginAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::ComboBox::Listener,
                                            private juce::Button::Listener,
                                            private juce::Timer
{
public:
    ComponentPluginAudioProcessorEditor(ComponentPluginAudioProcessor&);
    ~ComponentPluginAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

    void updateCategoryUI();
    void updateModelComboBox();   // repopulates combo box text with real model names for the current category
    void updateDisplays();

    void drawCircuitPath(juce::Graphics& g, juce::Rectangle<int> area);   // IN -> component -> OUT, wires and terminals
    void drawComponentSymbol(juce::Graphics& g, juce::Rectangle<float> bounds, bool isSelected);

    void drawResistorSymbol(juce::Graphics& g, juce::Rectangle<float> bounds, bool isSelected);
    void drawCapacitorSymbol(juce::Graphics& g, juce::Rectangle<float> bounds, bool isSelected);
    void drawInductorSymbol(juce::Graphics& g, juce::Rectangle<float> bounds, bool isSelected);

    void drawResistorZigzag(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, float thickness);
    void drawCapacitorLines(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, float thickness, int modelIndex);
    void drawInductorLoops(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, float thickness);

    juce::Colour getCategoryColour() const;
    juce::Colour getCategoryColourDark() const;
    juce::Font getPluginFont(float size, bool bold = false) const;

    //==========================================================================
    ComponentPluginAudioProcessor& audioProcessor;

    CategoryButton resistorButton{ "Resistor", ComponentCategory::Resistor };
    CategoryButton capacitorButton{ "Capacitor", ComponentCategory::Capacitor };
    CategoryButton inductorButton{ "Inductor", ComponentCategory::Inductor };

    juce::ComboBox modelComboBox;
    juce::Label modelLabel;

    std::unique_ptr<ValueEntryComponent> valueEntry;
    juce::Label valueEntryLabel;

    juce::TextButton modeButton;
    juce::Label modeLabel;

    juce::Slider dryWetSlider;
    juce::Label dryWetLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;

    juce::Label valueDisplayLabel;
    juce::Label reactanceLabel;
    juce::Label impedanceLabel;
    juce::Label frequencyLabel;
    juce::Label descriptionLabel;
    juce::Label formulaLabel;

    juce::TextButton infoButton;

    juce::Rectangle<int> schematicArea;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentPluginAudioProcessorEditor)
};
