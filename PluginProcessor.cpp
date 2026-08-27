/*
  ==============================================================================
  AlphaAudio Passive Component - Processor
  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ComponentPluginAudioProcessor::ComponentPluginAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    dryWetSmoothed.setCurrentAndTargetValue(1.0f);
}

ComponentPluginAudioProcessor::~ComponentPluginAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout ComponentPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // NOTE: the choice list here is deliberately generic ("Type A".."Type E") because
    // AudioParameterChoice text is what gets saved into automation/session data and
    // must stay stable even though the *meaning* of "Type C" changes depending on
    // which category is selected. The editor re-labels the combo box with real
    // component names (see updateModelComboBox()) without touching this parameter.
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::componentCategory, "Component Category",
        juce::StringArray{ "Resistor", "Capacitor", "Inductor" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::componentModel, "Component Model",
        juce::StringArray{ "Type A", "Type B", "Type C", "Type D", "Type E" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::componentValue, "Value",
        juce::NormalisableRange<float>(1.0f, 10000000.0f, 0.01f, 0.25f),
        1000.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::operatingMode, "Real-World Mode", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::dryWet, "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String ComponentPluginAudioProcessor::getName() const { return JucePlugin_Name; }

bool ComponentPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool ComponentPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

double ComponentPluginAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  ComponentPluginAudioProcessor::getNumPrograms() { return 1; }
int  ComponentPluginAudioProcessor::getCurrentProgram() { return 0; }
void ComponentPluginAudioProcessor::setCurrentProgram(int) {}
const juce::String ComponentPluginAudioProcessor::getProgramName(int) { return {}; }
void ComponentPluginAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void ComponentPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;
    dryWetSmoothed.reset(sampleRate, 0.02); // 20ms ramp - avoids zipper noise on the mix knob

    // Force a full rebuild of the model on (re)start.
    lastModelIndex = -1;
    updateModel();
    if (currentModel)
        currentModel->reset();
}

void ComponentPluginAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ComponentPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto mainIn  = layouts.getMainInputChannelSet();
    auto mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != juce::AudioChannelSet::stereo() && mainIn != juce::AudioChannelSet::mono())
        return false;
    if (mainOut != juce::AudioChannelSet::stereo() && mainOut != juce::AudioChannelSet::mono())
        return false;

    return mainIn == mainOut;
}
#endif

//==============================================================================
void ComponentPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    // Real-time-safe: only ever READ parameters and currentModel here. If a
    // category/model change is detected, hand the actual (allocating) rebuild
    // off to the message thread via triggerAsyncUpdate() -> handleAsyncUpdate()
    // rather than allocating on the audio thread.
    auto cat   = getComponentCategory();
    int  model = getComponentModelIndex();
    auto mode  = getOperatingMode();
    float value = getComponentValue();

    if (cat != lastCategory || model != lastModelIndex || !currentModel)
    {
        triggerAsyncUpdate();
    }
    else if (currentModel)
    {
        // Same component type -- push value/mode in-place, no allocation, no click.
        currentModel->setValue(value);
        if (mode != lastMode)
        {
            currentModel->setMode(mode);
            lastMode = mode;
        }
    }

    dryWetSmoothed.setTargetValue(getDryWet());

    if (!currentModel)
        return; // bypass (dry passthrough) until the async rebuild lands

    // Zero-crossing frequency estimate on channel 0, used only for HF roll-off effects.
    if (buffer.getNumChannels() > 0 && buffer.getNumSamples() > 1)
    {
        const auto* data = buffer.getReadPointer(0);
        int zeroCrossings = 0;
        for (int i = 1; i < buffer.getNumSamples(); ++i)
            if ((data[i - 1] <= 0.0f) != (data[i] <= 0.0f))
                ++zeroCrossings;

        if (zeroCrossings > 0)
            lastDetectedFrequency.store(static_cast<float>(zeroCrossings * currentSampleRate
                                                            / (2.0 * buffer.getNumSamples())));
    }

    const float freqHint = lastDetectedFrequency.load();
    const int numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mix = dryWetSmoothed.getNextValue(); // advance once per sample, not per channel
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            const float dry = channelData[sample];
            const float wet = currentModel->process(dry, freqHint);
            channelData[sample] = dry + (wet - dry) * mix;
        }
    }
}

//==============================================================================
bool ComponentPluginAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* ComponentPluginAudioProcessor::createEditor()
{
    return new ComponentPluginAudioProcessorEditor(*this);
}

//==============================================================================
void ComponentPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ComponentPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

    lastModelIndex = -1; // force rebuild with restored parameter values
    updateModel();
}

//==============================================================================
ComponentCategory ComponentPluginAudioProcessor::getComponentCategory() const
{
    auto* p = apvts.getRawParameterValue(ParamIDs::componentCategory);
    return p ? static_cast<ComponentCategory>(static_cast<int>(p->load())) : ComponentCategory::Resistor;
}

int ComponentPluginAudioProcessor::getComponentModelIndex() const
{
    auto* p = apvts.getRawParameterValue(ParamIDs::componentModel);
    return p ? static_cast<int>(p->load()) : 0;
}

OperatingMode ComponentPluginAudioProcessor::getOperatingMode() const
{
    auto* p = apvts.getRawParameterValue(ParamIDs::operatingMode);
    return (p && p->load() > 0.5f) ? OperatingMode::RealWorld : OperatingMode::Ideal;
}

float ComponentPluginAudioProcessor::getComponentValue() const
{
    auto* p = apvts.getRawParameterValue(ParamIDs::componentValue);
    return p ? p->load() : 1000.0f;
}

float ComponentPluginAudioProcessor::getDryWet() const
{
    auto* p = apvts.getRawParameterValue(ParamIDs::dryWet);
    return p ? p->load() : 1.0f;
}

//==============================================================================
void ComponentPluginAudioProcessor::updateModel()
{
    auto cat   = getComponentCategory();
    int  model = getComponentModelIndex();
    auto mode  = getOperatingMode();
    float value = getComponentValue();

    if (cat != lastCategory || model != lastModelIndex || !currentModel)
    {
        auto newModel = createComponentModel(cat, model, value, mode, currentSampleRate);
        if (newModel)
            newModel->prepare(currentSampleRate);

        const juce::SpinLock::ScopedLockType lock(modelSwapLock);
        currentModel = std::move(newModel);

        lastCategory = cat;
        lastModelIndex = model;
        lastMode = mode;
    }
    else
    {
        // Same component type -- push the new value/mode in-place so there is
        // no filter-state reset (no click) while a knob is being turned live.
        currentModel->setValue(value);
        if (mode != lastMode)
        {
            currentModel->setMode(mode);
            lastMode = mode;
        }
    }
}

//==============================================================================
juce::String ComponentPluginAudioProcessor::getFormattedValue() const
{
    const juce::SpinLock::ScopedLockType lock(modelSwapLock);
    if (!currentModel) return "1.00 k";
    return formatValueWithPrefix(getComponentValue(), currentModel->getUnit());
}

juce::String ComponentPluginAudioProcessor::getModelName() const
{
    const juce::SpinLock::ScopedLockType lock(modelSwapLock);
    return currentModel ? currentModel->getName() : "Resistor";
}

juce::String ComponentPluginAudioProcessor::getModelDescription() const
{
    const juce::SpinLock::ScopedLockType lock(modelSwapLock);
    return currentModel ? currentModel->getDescription() : "";
}

juce::String ComponentPluginAudioProcessor::getRealWorldStatusText() const
{
    if (getOperatingMode() == OperatingMode::Ideal)
        return "Ideal Mode: Perfect textbook behavior, no parasitics";

    const juce::SpinLock::ScopedLockType lock(modelSwapLock);
    return currentModel ? ("Real-World: " + juce::String(currentModel->getDescription())) : "Real-World Mode";
}

float ComponentPluginAudioProcessor::getReactanceAtFrequency(float frequency) const
{
    const juce::SpinLock::ScopedLockType lock(modelSwapLock);
    return currentModel ? currentModel->getReactance(frequency) : 0.0f;
}

float ComponentPluginAudioProcessor::getImpedanceAtFrequency(float frequency) const
{
    const juce::SpinLock::ScopedLockType lock(modelSwapLock);
    return currentModel ? currentModel->getImpedance(frequency) : 0.0f;
}

//==============================================================================
juce::String ComponentPluginAudioProcessor::formatValueWithPrefix(float value, const juce::String& unit)
{
    juce::String prefix;
    float scaled = value;

    if (value >= 1.0e6f)      { prefix = "M";  scaled = value / 1.0e6f; }
    else if (value >= 1.0e3f) { prefix = "k";  scaled = value / 1.0e3f; }
    else if (value >= 1.0f)   { prefix = "";   scaled = value; }
    else if (value >= 1.0e-3f){ prefix = "m";  scaled = value * 1.0e3f; }
    else if (value >= 1.0e-6f){ prefix = juce::String(juce::CharPointer_UTF8("\xc2\xb5")); scaled = value * 1.0e6f; }
    else if (value >= 1.0e-9f){ prefix = "n";  scaled = value * 1.0e9f; }
    else                      { prefix = "p";  scaled = value * 1.0e12f; }

    juce::String numStr;
    if (scaled >= 100.0f)      numStr = juce::String(scaled, 1);
    else if (scaled >= 10.0f)  numStr = juce::String(scaled, 2);
    else                       numStr = juce::String(scaled, 3);

    return numStr + " " + prefix + unit;
}

float ComponentPluginAudioProcessor::parseEngineeringValue(const juce::String& text, const juce::String& unit)
{
    juce::String clean = text.trim();
    if (clean.endsWithIgnoreCase(unit))
        clean = clean.dropLastCharacters(unit.length()).trim();
    if (clean.endsWith("F") || clean.endsWith("H") || clean.endsWith("R") || clean.endsWith(juce::CharPointer_UTF8("\xce\xa9")))
        clean = clean.dropLastCharacters(1).trim();

    double multiplier = 1.0;
    if (clean.endsWithIgnoreCase("k"))      { multiplier = 1.0e3;  clean = clean.dropLastCharacters(1); }
    else if (clean.endsWithIgnoreCase("M")) { multiplier = 1.0e6;  clean = clean.dropLastCharacters(1); }
    else if (clean.endsWithIgnoreCase("m")) { multiplier = 1.0e-3; clean = clean.dropLastCharacters(1); }
    else if (clean.endsWithIgnoreCase("u") || clean.endsWith(juce::CharPointer_UTF8("\xc2\xb5")))
                                             { multiplier = 1.0e-6; clean = clean.dropLastCharacters(1); }
    else if (clean.endsWithIgnoreCase("n")) { multiplier = 1.0e-9; clean = clean.dropLastCharacters(1); }
    else if (clean.endsWithIgnoreCase("p")) { multiplier = 1.0e-12;clean = clean.dropLastCharacters(1); }

    return static_cast<float>(clean.getDoubleValue() * multiplier);
}

//==============================================================================
// Required factory function for the JUCE plugin wrapper (VST3/AU/Standalone).
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ComponentPluginAudioProcessor();
}
