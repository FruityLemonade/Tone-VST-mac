#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Engine.h"
#include "Parameters.h"
#include "SampleLoader.h"

class XtoneProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    static constexpr int kChunk = 256; // host samples per internal chunk
    static constexpr int kMaxOs = 8;

    XtoneProcessor();
    ~XtoneProcessor() override;

    // ---- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- for the editor (message thread)
    juce::AudioProcessorValueTreeState apvts;
    const xt::Telemetry& telemetry() const noexcept { return engine.telemetry; }

    /// Asynchronous load (decodes on a background thread).
    void loadSampleFile (const juce::File& f);
    void normalizeSample();
    /// Replace the slice markers (creates a new asset).
    void setMarkers (std::vector<int64_t> markers);

    xtp::SampleAsset::Ptr getCurrentAsset() const;
    juce::String getStatusMessage() const;
    /// Incremented whenever the asset shown in the GUI changes.
    int getAssetVersion() const noexcept { return assetVersion.load(); }

    bool saveStateToFile (const juce::File& f);
    bool loadStateFromFile (const juce::File& f);

private:
    void timerCallback() override;
    void publish (xtp::SampleAsset::Ptr asset); // message / loader thread
    void swapPendingAsset() noexcept;            // audio thread
    void setOversampling (int index) noexcept;   // audio thread / prepare
    juce::ValueTree buildState();
    void restoreState (const juce::ValueTree& t);

    xtp::ParamSnapshot snapshot;
    xt::Engine engine;
    xt::EngineParams engineParams;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplers[3]; // 2x, 4x, 8x
    int osIndex { -1 };
    juce::AudioBuffer<float> zeroBuffer, directBuffer;
    double hostRate { 48000.0 };

    std::vector<xt::MidiEvent> events; // preallocated

    // asset hand-over
    std::atomic<xtp::SampleAsset*> pending { nullptr };
    xtp::SampleAsset* audioAsset { nullptr }; // owned (one reference) by the audio thread
    static constexpr int kRetireSize = 32;
    juce::AbstractFifo retireFifo { kRetireSize };
    xtp::SampleAsset* retired[kRetireSize] {};

    mutable juce::CriticalSection guiLock; // guards guiAsset + statusMessage (never taken on the audio thread)
    xtp::SampleAsset::Ptr guiAsset;
    juce::String statusMessage;
    std::atomic<int> assetVersion { 0 };

    std::shared_ptr<int> lifetimeToken = std::make_shared<int> (0);
    juce::ThreadPool loaderPool { 1 };
    int currentProgram { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XtoneProcessor)
};
