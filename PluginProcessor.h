/*
  ==============================================================================
  AlphaAudio Passive Component - Processor
  ------------------------------------------------------------------------------
  Consolidated into the standard 4-file JUCE layout (PluginProcessor.h/.cpp,
  PluginEditor.h/.cpp) at William's request, to rule out the extra ParamIDs.h /
  DSP/ComponentModels.h files as a variable while chasing the FL Studio /
  Bitwig load crash. Everything the DSP layer needs now lives at the top of
  this one header.
  ==============================================================================
*/
#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <string>

//==============================================================================
// Parameter IDs
//==============================================================================
namespace ParamIDs
{
    inline const juce::String componentCategory { "componentCategory" };
    inline const juce::String componentModel    { "componentModel" };
    inline const juce::String componentValue    { "componentValue" };
    inline const juce::String operatingMode     { "operatingMode" };
    inline const juce::String dryWet            { "dryWet" };
}

//==============================================================================
// Passive Component DSP models
// ------------------------------------------------------------------------------
// Kept JUCE-independent on purpose (no APVTS, no juce_audio_processors types)
// so this block can be lifted into a future Circuit Builder / RLC Filter plugin
// verbatim, or unit-tested outside of a plugin host. See ComponentModel below
// for the shared interface every future building-block plugin should target.
//==============================================================================
enum class ComponentCategory { Resistor = 0, Capacitor, Inductor };

enum class ResistorModel   { CarbonComposition = 0, CarbonFilm, MetalFilm, Wirewound, ThickFilmSMD };
enum class CapacitorModel  { CeramicDisc = 0, Electrolytic, PolyesterFilm, Polypropylene, Tantalum };
enum class InductorModel   { AirCore = 0, FerriteRod, IronCore, Toroidal, PowderedIron };
enum class OperatingMode   { Ideal = 0, RealWorld };

/** Base interface for all component models. Designed for reuse in circuit builders:
    anything that can process(), report its complex behaviour, and describe itself
    can be dropped into a signal-chain slot without the host caring which passive
    it actually is. */
class ComponentModel
{
public:
    virtual ~ComponentModel() = default;

    virtual void  prepare(double sampleRate) = 0;
    virtual void  reset() = 0;

    /** Process one sample. frequencyHint is the last zero-crossing-estimated
        fundamental, used only for HF roll-off effects (ESL, self-resonance). */
    virtual float process(float input, float frequencyHint = 1000.0f) = 0;

    /** Live-update the component's nominal value without resetting filter state. */
    virtual void setValue(float newValue) = 0;
    virtual void setMode(OperatingMode newMode) = 0;

    virtual float getReactance(float frequency) const = 0;
    virtual float getImpedance(float frequency) const = 0;

    virtual std::string getName() const = 0;
    virtual std::string getDescription() const = 0;
    virtual std::string getUnit() const = 0;

    static constexpr float loadResistanceOhms = 1000.0f; // fixed reference load the component works against
};

//==============================================================================
class ResistorModelImpl : public ComponentModel
{
public:
    ResistorModelImpl(float value, ResistorModel model, OperatingMode m, double sr)
        : nominalValue(value), modelType(model), mode(m), sampleRate(sr) { applyModelConstants(); }

    void prepare(double sr) override { sampleRate = sr; }
    void reset() override { driftPhase = 0.0f; }
    void setValue(float v) override { nominalValue = v; }
    void setMode(OperatingMode m) override { mode = m; }

    float process(float input, float /*frequencyHint*/) override
    {
        float output = input;
        float effectiveR = nominalValue;

        if (mode == OperatingMode::RealWorld)
        {
            float noiseGain = (1.0f - noiseIndex) * 0.0002f * std::sqrt(nominalValue / 1000.0f);
            output += (pseudoRandom() - 0.5f) * noiseGain;
            output += output * output * output * voltageCoeff * 0.01f;

            driftPhase += driftRate * 0.0001f;
            if (driftPhase > 6.2831853f) driftPhase -= 6.2831853f;
            float driftFactor = 1.0f + std::sin(driftPhase) * driftRate * 0.001f;
            effectiveR *= driftFactor;
        }

        float attenuation = loadResistanceOhms / (loadResistanceOhms + effectiveR);
        output *= attenuation;

        if (mode == OperatingMode::RealWorld && parasiticL > 0.0f)
            output *= (1.0f - (parasiticL / 1.0e-6f) * 0.001f);

        return output;
    }

    float getReactance(float) const override { return nominalValue; }

    float getImpedance(float frequency) const override
    {
        if (mode == OperatingMode::Ideal || frequency <= 0.0f) return nominalValue;
        float xl = 2.0f * 3.14159265f * frequency * parasiticL;
        float xc = 1.0f / (2.0f * 3.14159265f * frequency * parasiticC);
        float reactance = xl - xc;
        return std::sqrt(nominalValue * nominalValue + reactance * reactance);
    }

    std::string getName() const override
    {
        switch (modelType)
        {
            case ResistorModel::CarbonComposition: return "Carbon Composition";
            case ResistorModel::CarbonFilm:        return "Carbon Film";
            case ResistorModel::MetalFilm:         return "Metal Film";
            case ResistorModel::Wirewound:         return "Wirewound";
            case ResistorModel::ThickFilmSMD:      return "Thick Film SMD";
        }
        return "Resistor";
    }

    std::string getDescription() const override
    {
        switch (modelType)
        {
            case ResistorModel::CarbonComposition: return "Carbon comp: warm, noisy, slightly microphonic. Vintage mojo.";
            case ResistorModel::CarbonFilm:        return "Carbon film: balanced performance, slight warmth, low cost.";
            case ResistorModel::MetalFilm:         return "Metal film: precision, ultra-low noise, hi-fi transparency.";
            case ResistorModel::Wirewound:         return "Wirewound: power handling, inductive at HF, very stable.";
            case ResistorModel::ThickFilmSMD:      return "Thick film SMD: modern surface mount, compact, moderate performance.";
        }
        return "";
    }

    std::string getUnit() const override { return "\xCE\xA9"; } // UTF-8 Omega

private:
    void applyModelConstants()
    {
        switch (modelType)
        {
            case ResistorModel::CarbonComposition: noiseIndex=0.0f;  voltageCoeff=0.0005f;   parasiticL=0.0f;      parasiticC=0.5e-12f; driftRate=0.1f;   break;
            case ResistorModel::CarbonFilm:        noiseIndex=0.5f;  voltageCoeff=0.0001f;   parasiticL=0.0f;      parasiticC=0.2e-12f; driftRate=0.05f;  break;
            case ResistorModel::MetalFilm:         noiseIndex=0.9f;  voltageCoeff=0.00001f;  parasiticL=10.0e-9f;  parasiticC=0.1e-12f; driftRate=0.01f;  break;
            case ResistorModel::Wirewound:         noiseIndex=0.95f; voltageCoeff=0.000005f; parasiticL=500.0e-9f; parasiticC=0.05e-12f;driftRate=0.005f; break;
            case ResistorModel::ThickFilmSMD:      noiseIndex=0.6f;  voltageCoeff=0.00008f;  parasiticL=0.5e-9f;   parasiticC=0.3e-12f; driftRate=0.02f;  break;
        }
    }

    // Small deterministic PRNG so the DSP core has zero JUCE dependency.
    float pseudoRandom()
    {
        rngState = rngState * 1664525u + 1013904223u;
        return static_cast<float>(rngState >> 8) / static_cast<float>(1u << 24);
    }

    float nominalValue;
    ResistorModel modelType;
    OperatingMode mode;
    double sampleRate;

    float noiseIndex = 0.0f, voltageCoeff = 0.0f, parasiticL = 0.0f, parasiticC = 0.0f, driftRate = 0.0f, driftPhase = 0.0f;
    uint32_t rngState = 0x9E3779B9u;
};

//==============================================================================
class CapacitorModelImpl : public ComponentModel
{
public:
    CapacitorModelImpl(float value, CapacitorModel model, OperatingMode m, double sr)
        : capacitance(value), modelType(model), mode(m), sampleRate(sr) { applyModelConstants(); }

    void prepare(double sr) override { sampleRate = sr; }
    void reset() override { dielectricMemory = 0.0f; x1 = 0.0f; y1 = 0.0f; }
    void setValue(float v) override { capacitance = v; }
    void setMode(OperatingMode m) override { mode = m; }

    float process(float input, float frequencyHint) override
    {
        float totalR = loadResistanceOhms + (mode == OperatingMode::RealWorld ? esr : 0.0f);

        float effectiveC = capacitance;
        if (mode == OperatingMode::RealWorld && voltageCoeff > 0.0f)
        {
            float voltageFactor = 1.0f - voltageCoeff * std::abs(input) * 0.1f;
            effectiveC *= std::clamp(voltageFactor, 0.5f, 1.5f);
        }

        // Series RC high-pass, backward Euler.
        float T = 1.0f / static_cast<float>(sampleRate);
        float rc = totalR * effectiveC;
        float alpha = rc / (rc + T);
        float output = alpha * (y1 + input - x1);
        x1 = input;
        y1 = output;

        if (mode == OperatingMode::RealWorld)
        {
            float daSignal = dielectricMemory * dielectricAbsorption * 0.01f;
            dielectricMemory = input * 0.001f + dielectricMemory * 0.999f;
            output += daSignal;
            output *= (1.0f - leakage * 0.0001f);

            if (esl > 0.0f && frequencyHint > 0.0f)
            {
                float eslFreq = 1.0f / (2.0f * 3.14159265f * std::sqrt(esl * effectiveC));
                if (frequencyHint > eslFreq * 0.5f)
                    output *= std::clamp(eslFreq / frequencyHint, 0.0f, 1.0f);
            }
        }
        return output;
    }

    float getReactance(float frequency) const override
    {
        if (frequency <= 0.0f) return 0.0f;
        return 1.0f / (2.0f * 3.14159265f * frequency * capacitance);
    }

    float getImpedance(float frequency) const override
    {
        if (mode == OperatingMode::Ideal || frequency <= 0.0f) return getReactance(frequency);
        float xc = getReactance(frequency);
        float xl = 2.0f * 3.14159265f * frequency * esl;
        float zImag = xl - xc;
        return std::sqrt(esr * esr + zImag * zImag);
    }

    std::string getName() const override
    {
        switch (modelType)
        {
            case CapacitorModel::CeramicDisc:   return "Ceramic Disc";
            case CapacitorModel::Electrolytic:  return "Electrolytic";
            case CapacitorModel::PolyesterFilm: return "Polyester Film";
            case CapacitorModel::Polypropylene: return "Polypropylene";
            case CapacitorModel::Tantalum:      return "Tantalum";
        }
        return "Capacitor";
    }

    std::string getDescription() const override
    {
        switch (modelType)
        {
            case CapacitorModel::CeramicDisc:   return "Ceramic: piezoelectric microphonics, voltage-dependent capacitance.";
            case CapacitorModel::Electrolytic:  return "Electrolytic: high ESR, leakage, dielectric absorption, aging character.";
            case CapacitorModel::PolyesterFilm: return "Polyester (Mylar): warm, slightly soft, good general purpose.";
            case CapacitorModel::Polypropylene: return "Polypropylene: audiophile grade, ultra-low distortion, precise.";
            case CapacitorModel::Tantalum:      return "Tantalum: stable, compact, solid bass, slightly dry.";
        }
        return "";
    }

    std::string getUnit() const override { return "F"; }

private:
    void applyModelConstants()
    {
        switch (modelType)
        {
            case CapacitorModel::CeramicDisc:   esr=0.01f; esl=5.0e-9f;  leakage=0.0001f;   dielectricAbsorption=0.001f; voltageCoeff=0.1f;    break;
            case CapacitorModel::Electrolytic:  esr=2.0f;  esl=20.0e-9f; leakage=0.01f;     dielectricAbsorption=0.1f;   voltageCoeff=0.001f;  break;
            case CapacitorModel::PolyesterFilm: esr=0.1f;  esl=15.0e-9f; leakage=0.00001f;  dielectricAbsorption=0.005f; voltageCoeff=0.0001f; break;
            case CapacitorModel::Polypropylene: esr=0.005f;esl=10.0e-9f; leakage=0.000001f; dielectricAbsorption=0.0001f;voltageCoeff=0.00001f;break;
            case CapacitorModel::Tantalum:      esr=0.5f;  esl=8.0e-9f;  leakage=0.0005f;   dielectricAbsorption=0.02f;  voltageCoeff=0.0005f; break;
        }
    }

    float capacitance;
    CapacitorModel modelType;
    OperatingMode mode;
    double sampleRate;

    float esr = 0.0f, esl = 0.0f, leakage = 0.0f, dielectricAbsorption = 0.0f, voltageCoeff = 0.0f;
    float dielectricMemory = 0.0f, x1 = 0.0f, y1 = 0.0f;
};

//==============================================================================
class InductorModelImpl : public ComponentModel
{
public:
    InductorModelImpl(float value, InductorModel model, OperatingMode m, double sr)
        : inductance(value), modelType(model), mode(m), sampleRate(sr) { applyModelConstants(); }

    void prepare(double sr) override { sampleRate = sr; }
    void reset() override { y1 = 0.0f; hysteresisState = 0.0f; }
    void setValue(float v) override { inductance = v; }
    void setMode(OperatingMode m) override { mode = m; }

    float process(float input, float frequencyHint) override
    {
        float totalR = loadResistanceOhms + (mode == OperatingMode::RealWorld ? dcr : 0.0f);

        float effectiveL = inductance;
        if (mode == OperatingMode::RealWorld)
        {
            float current = std::abs(input);
            if (current > saturationCurrent && saturationCurrent < 100.0f)
            {
                float saturationFactor = saturationCurrent / (saturationCurrent + (current - saturationCurrent) * 0.5f);
                effectiveL *= std::clamp(saturationFactor, 0.1f, 1.0f);
            }
        }

        // Series RL low-pass, backward Euler.
        float T = 1.0f / static_cast<float>(sampleRate);
        float alpha = effectiveL / (effectiveL + totalR * T);
        float output = alpha * y1 + (1.0f - alpha) * input;
        y1 = output;

        if (mode == OperatingMode::RealWorld)
        {
            float hysteresisDelta = input - hysteresisState;
            hysteresisState += hysteresisDelta * 0.001f;
            output += hysteresisDelta * hysteresis * 0.1f;
            output *= (1.0f - coreLoss * 0.01f);

            if (parasiticC > 0.0f && frequencyHint > 0.0f)
            {
                float selfResFreq = 1.0f / (2.0f * 3.14159265f * std::sqrt(effectiveL * parasiticC));
                if (frequencyHint > selfResFreq * 0.7f)
                    output *= std::clamp(selfResFreq / frequencyHint, 0.0f, 1.0f);
            }
        }
        return output;
    }

    float getReactance(float frequency) const override
    {
        if (frequency <= 0.0f) return 0.0f;
        return 2.0f * 3.14159265f * frequency * inductance;
    }

    float getImpedance(float frequency) const override
    {
        if (mode == OperatingMode::Ideal || frequency <= 0.0f) return getReactance(frequency);
        float xl = getReactance(frequency);
        float xc = 1.0f / (2.0f * 3.14159265f * frequency * parasiticC);
        float zImag = xl - xc;
        return std::sqrt(dcr * dcr + zImag * zImag);
    }

    std::string getName() const override
    {
        switch (modelType)
        {
            case InductorModel::AirCore:      return "Air Core";
            case InductorModel::FerriteRod:   return "Ferrite Rod";
            case InductorModel::IronCore:     return "Iron Core";
            case InductorModel::Toroidal:     return "Toroidal";
            case InductorModel::PowderedIron: return "Powdered Iron";
        }
        return "Inductor";
    }

    std::string getDescription() const override
    {
        switch (modelType)
        {
            case InductorModel::AirCore:      return "Air core: perfectly linear, no saturation, high DCR, transparent.";
            case InductorModel::FerriteRod:   return "Ferrite rod: compact, slight saturation at high levels, warm.";
            case InductorModel::IronCore:     return "Iron core: high inductance, audible saturation/compression, vintage.";
            case InductorModel::Toroidal:     return "Toroidal: efficient, low stray field, tight bass, hi-fi.";
            case InductorModel::PowderedIron: return "Powdered iron: gradual saturation, smooth compression, RF friendly.";
        }
        return "";
    }

    std::string getUnit() const override { return "H"; }

private:
    void applyModelConstants()
    {
        switch (modelType)
        {
            case InductorModel::AirCore:      dcr=2.0f;  parasiticC=5.0e-12f;  saturationCurrent=999.0f; hysteresis=0.0f;  coreLoss=0.0f;   break;
            case InductorModel::FerriteRod:   dcr=0.5f;  parasiticC=10.0e-12f; saturationCurrent=0.3f;   hysteresis=0.02f; coreLoss=0.01f;  break;
            case InductorModel::IronCore:     dcr=0.2f;  parasiticC=20.0e-12f; saturationCurrent=0.1f;   hysteresis=0.08f; coreLoss=0.05f;  break;
            case InductorModel::Toroidal:     dcr=0.1f;  parasiticC=8.0e-12f;  saturationCurrent=0.5f;   hysteresis=0.01f; coreLoss=0.005f; break;
            case InductorModel::PowderedIron: dcr=0.3f;  parasiticC=15.0e-12f; saturationCurrent=0.2f;   hysteresis=0.04f; coreLoss=0.02f;  break;
        }
    }

    float inductance;
    InductorModel modelType;
    OperatingMode mode;
    double sampleRate;

    float dcr = 0.0f, parasiticC = 0.0f, saturationCurrent = 999.0f, hysteresis = 0.0f, coreLoss = 0.0f;
    float y1 = 0.0f, hysteresisState = 0.0f;
};

//==============================================================================
/** Factory used by the processor (and, later, the circuit builder's per-slot
    component instantiation) so nothing outside this file needs to know the
    concrete model classes. */
inline std::unique_ptr<ComponentModel> createComponentModel(ComponentCategory category, int modelIndex,
                                                              float value, OperatingMode mode, double sampleRate)
{
    modelIndex = std::clamp(modelIndex, 0, 4);
    switch (category)
    {
        case ComponentCategory::Resistor:
            return std::make_unique<ResistorModelImpl>(value, static_cast<ResistorModel>(modelIndex), mode, sampleRate);
        case ComponentCategory::Capacitor:
            return std::make_unique<CapacitorModelImpl>(value, static_cast<CapacitorModel>(modelIndex), mode, sampleRate);
        case ComponentCategory::Inductor:
            return std::make_unique<InductorModelImpl>(value, static_cast<InductorModel>(modelIndex), mode, sampleRate);
    }
    return nullptr;
}

//==============================================================================
// Processor
//==============================================================================
class ComponentPluginAudioProcessor : public juce::AudioProcessor,
                                       private juce::AsyncUpdater
{
public:
    ComponentPluginAudioProcessor();
    ~ComponentPluginAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    ComponentCategory getComponentCategory() const;
    int   getComponentModelIndex() const;
    OperatingMode getOperatingMode() const;
    float getComponentValue() const;
    float getDryWet() const;
    float getLastDetectedFrequency() const { return lastDetectedFrequency.load(); }

    juce::String getFormattedValue() const;
    juce::String getModelName() const;
    juce::String getModelDescription() const;
    juce::String getRealWorldStatusText() const;

    float getReactanceAtFrequency(float frequency) const;
    float getImpedanceAtFrequency(float frequency) const;

    static juce::String formatValueWithPrefix(float value, const juce::String& unit);
    static float parseEngineeringValue(const juce::String& text, const juce::String& unit);

private:
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Rebuilds currentModel (allocates) only when category or model index
        changes; otherwise pushes the new value/mode into the existing model so
        there is no state reset (no click) while turning the value knob or
        A/B'ing Ideal vs Real-World. Only ever called from the message thread:
        directly from prepareToPlay()/setStateInformation(), or via
        triggerAsyncUpdate() -> handleAsyncUpdate() when processBlock() detects
        a category/model change while running. */
    void updateModel();

    /** juce::AsyncUpdater callback -- runs on the message thread, where heap
        allocation is safe. This is what actually calls updateModel() when a
        category/model change is detected while the audio thread is running,
        so processBlock() itself never allocates. */
    void handleAsyncUpdate() override { updateModel(); }

    // currentModel is only ever allocated/replaced on the message thread (see
    // handleAsyncUpdate() above); processBlock() only ever reads it and calls
    // process() on it, never allocates. modelSwapLock guards the pointer swap
    // itself and the editor's brief display reads against that swap -- never
    // the per-sample process() path.
    std::unique_ptr<ComponentModel> currentModel;
    juce::SpinLock modelSwapLock;

    double currentSampleRate = 44100.0;
    std::atomic<float> lastDetectedFrequency { 1000.0f };

    ComponentCategory lastCategory = ComponentCategory::Resistor;
    int   lastModelIndex = -1;
    OperatingMode lastMode = OperatingMode::Ideal;

    juce::LinearSmoothedValue<float> dryWetSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentPluginAudioProcessor)
};
