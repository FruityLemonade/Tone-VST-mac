#include "PluginProcessor.h"

#include "OnsetDetector.h"
#include "Presets.h"
#include "gui/PluginEditor.h"

using namespace juce;

namespace
{
constexpr const char* kStateMagic = "XTN1";
constexpr int kStateVersion = 1;
} // namespace

XtoneProcessor::XtoneProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", xtp::createParameterLayout()),
      snapshot (apvts)
{
    events.reserve (4096);
    zeroBuffer.setSize (2, kChunk);
    zeroBuffer.clear();
    directBuffer.setSize (2, kChunk);

    auto def = xtp::loader::makeDefault();
    {
        const ScopedLock sl (guiLock);
        guiAsset = def;
    }
    // hand the default to the audio side directly (no audio thread yet)
    def->incReferenceCount();
    audioAsset = def.get();
    engine.setSource (audioAsset->view());

    startTimerHz (20);
}

XtoneProcessor::~XtoneProcessor()
{
    stopTimer();
    lifetimeToken.reset(); // pending callbacks holding a weak_ptr now see expiry
    loaderPool.removeAllJobs (true, 10000);

    if (auto* p = pending.exchange (nullptr)) p->decReferenceCount();
    timerCallback(); // drain retired assets
    if (audioAsset != nullptr) audioAsset->decReferenceCount();
}

bool XtoneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet().isDisabled()
        && layouts.getMainOutputChannelSet() == AudioChannelSet::stereo();
}

void XtoneProcessor::prepareToPlay (double sampleRate, int)
{
    hostRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    for (int i = 0; i < 3; ++i)
    {
        oversamplers[i] = std::make_unique<dsp::Oversampling<float>> (
            2, (size_t) (i + 1), dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
        oversamplers[i]->initProcessing ((size_t) kChunk);
    }
    osIndex = -1;
    setOversampling (snapshot.osIndex());
    engine.reset();
}

void XtoneProcessor::setOversampling (int index) noexcept
{
    if (index == osIndex) return;
    osIndex = index;
    const int factor = 1 << index;
    if (index > 0 && oversamplers[index - 1] != nullptr) oversamplers[index - 1]->reset();
    engine.setEngineRate (hostRate * factor);
    const float lat = index > 0 && oversamplers[index - 1] != nullptr ? oversamplers[index - 1]->getLatencyInSamples() : 0.0f;
    setLatencySamples ((int) std::lround (lat));
}

void XtoneProcessor::swapPendingAsset() noexcept
{
    if (pending.load (std::memory_order_acquire) == nullptr) return;
    if (retireFifo.getFreeSpace() < 1) return; // try again next chunk
    auto* next = pending.exchange (nullptr, std::memory_order_acq_rel);
    if (next == nullptr) return;
    engine.setSource (next->view());
    if (audioAsset != nullptr)
    {
        const auto scope = retireFifo.write (1);
        if (scope.blockSize1 > 0) retired[scope.startIndex1] = audioAsset;
        else if (scope.blockSize2 > 0) retired[scope.startIndex2] = audioAsset;
    }
    audioAsset = next;
}

void XtoneProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || numSamples <= 0)
    {
        buffer.clear();
        return;
    }

    // host transport
    xt::HostInfo hostInfo;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm()) { hostInfo.bpm = *bpm; hostInfo.valid = true; }
            if (auto ppq = pos->getPpqPosition()) hostInfo.ppq = *ppq;
            else hostInfo.valid = false;
            hostInfo.playing = pos->getIsPlaying();
        }
    }

    auto midiIt = midi.cbegin();
    const auto midiEnd = midi.cend();

    for (int pos = 0; pos < numSamples;)
    {
        const int n = std::min (kChunk, numSamples - pos);

        swapPendingAsset();
        setOversampling (snapshot.osIndex());
        const int factor = 1 << osIndex;

        snapshot.read (engineParams);
        engine.setParams (engineParams);
        xt::HostInfo hi = hostInfo;
        if (hi.valid) hi.ppq += (double) pos / hostRate * hi.bpm / 60.0;
        engine.setHostInfo (hi);

        // MIDI of this chunk -> engine events (positions in engine samples)
        events.clear();
        for (; midiIt != midiEnd; ++midiIt)
        {
            const auto meta = *midiIt;
            if (meta.samplePosition >= pos + n) break;
            const auto m = meta.getMessage();
            xt::MidiEvent e;
            e.offset = std::max (0, meta.samplePosition - pos) * factor;
            if (m.isNoteOn()) { e.type = xt::MidiEvent::NoteOn; e.note = m.getNoteNumber(); e.value = m.getFloatVelocity(); }
            else if (m.isNoteOff()) { e.type = xt::MidiEvent::NoteOff; e.note = m.getNoteNumber(); }
            else if (m.isPitchWheel()) { e.type = xt::MidiEvent::PitchBend; e.value = (float) (m.getPitchWheelValue() - 8192) / 8192.0f; }
            else if (m.isSustainPedalOn()) { e.type = xt::MidiEvent::Sustain; e.value = 1.0f; }
            else if (m.isSustainPedalOff()) { e.type = xt::MidiEvent::Sustain; e.value = 0.0f; }
            else if (m.isAllNotesOff() || m.isAllSoundOff()) { e.type = xt::MidiEvent::AllNotesOff; }
            else continue;
            if (events.size() < events.capacity()) events.push_back (e);
        }

        float* outL = buffer.getWritePointer (0, pos);
        float* outR = buffer.getWritePointer (1, pos);

        if (osIndex == 0)
        {
            engine.process (outL, outR, n, events.data(), (int) events.size());
        }
        else
        {
            auto& os = *oversamplers[osIndex - 1];
            dsp::AudioBlock<const float> in (zeroBuffer.getArrayOfReadPointers(), 2, 0, (size_t) n);
            auto up = os.processSamplesUp (in);
            engine.process (up.getChannelPointer (0), up.getChannelPointer (1), (int) up.getNumSamples(),
                            events.data(), (int) events.size());
            float* chans[2] = { outL, outR };
            dsp::AudioBlock<float> out (chans, 2, (size_t) n);
            os.processSamplesDown (out);
        }
        pos += n;
    }

    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, numSamples);
    for (int c = 0; c < 2; ++c)
    {
        float* d = buffer.getWritePointer (c);
        for (int i = 0; i < numSamples; ++i)
            if (! std::isfinite (d[i])) d[i] = 0.0f;
    }
}

// ------------------------------------------------------------------ assets

bool XtoneProcessor::publish (xtp::SampleAsset::Ptr asset, const xtp::SampleAsset* expected)
{
    if (asset == nullptr) return false;
    xtp::SampleAsset* displaced = nullptr;
    {
        const ScopedLock sl (guiLock);
        if (expected != nullptr && guiAsset.get() != expected) return false;
        guiAsset = asset;
        asset->incReferenceCount();
        displaced = pending.exchange (asset.get(), std::memory_order_acq_rel);
        ++assetVersion;
    }
    if (displaced != nullptr) displaced->decReferenceCount(); // never reached the audio thread
    return true;
}

void XtoneProcessor::timerCallback()
{
    const auto scope = retireFifo.read (retireFifo.getNumReady());
    for (int i = 0; i < scope.blockSize1; ++i) retired[scope.startIndex1 + i]->decReferenceCount();
    for (int i = 0; i < scope.blockSize2; ++i) retired[scope.startIndex2 + i]->decReferenceCount();
}

void XtoneProcessor::loadSampleFile (const File& f)
{
    {
        const ScopedLock sl (guiLock);
        statusMessage = "Lade " + f.getFileName() + " ...";
    }
    ++assetVersion;
    std::weak_ptr<int> token = lifetimeToken;
    loaderPool.addJob ([this, f, token]
    {
        String error;
        auto asset = xtp::loader::fromFile (f, error);
        MessageManager::callAsync ([this, token, asset, error]
        {
            if (token.expired()) return;
            {
                const ScopedLock sl (guiLock);
                statusMessage = asset != nullptr ? String() : "Fehler: " + error;
                if (asset != nullptr && asset->fileBytes == nullptr)
                    statusMessage = "Hinweis: Datei > 32 MB, wird nur per Pfad gespeichert";
            }
            if (asset != nullptr) publish (asset);
            else ++assetVersion;
        });
    });
}

void XtoneProcessor::normalizeSample()
{
    auto a = getCurrentAsset();
    if (a == nullptr || a->normalized) return;
    publish (xtp::loader::normalizedCopy (*a), a.get());
}

bool XtoneProcessor::setMarkers (const xtp::SampleAsset& asset, std::vector<int64_t> markers)
{
    return publish (asset.withMarkers (std::move (markers)), &asset);
}

void XtoneProcessor::detectTransients (xtp::SampleAsset::Ptr asset, double sensitivity,
                                       std::function<void (std::optional<int>)> finished)
{
    std::weak_ptr<int> token = lifetimeToken;
    loaderPool.addJob ([this, asset, sensitivity, token, finished]
    {
        const auto sample = asset->view().sample;
        auto markers = xt::detectOnsets ({ sample.mono, (size_t) sample.frames }, sample.sampleRate, sensitivity);
        MessageManager::callAsync ([this, asset, token, onsets = std::move (markers), finished]() mutable
        {
            if (token.expired()) return;
            const auto count = (int) onsets.size();
            const bool current = count > 0 ? setMarkers (*asset, std::move (onsets)) : getCurrentAsset() == asset;
            finished (current ? std::optional<int> { count } : std::nullopt);
        });
    });
}

xtp::SampleAsset::Ptr XtoneProcessor::getCurrentAsset() const
{
    const ScopedLock sl (guiLock);
    return guiAsset;
}

String XtoneProcessor::getStatusMessage() const
{
    const ScopedLock sl (guiLock);
    return statusMessage;
}

// ------------------------------------------------------------------ programs

int XtoneProcessor::getNumPrograms() { return (int) xtp::factoryPresets().size(); }

void XtoneProcessor::setCurrentProgram (int index)
{
    const auto& p = xtp::factoryPresets();
    // Hosts re-send the current program after restoring a project; that must not
    // overwrite the restored parameters.
    if (index < 0 || index >= (int) p.size() || index == currentProgram) return;
    currentProgram = index;
    xtp::applyPreset (apvts, p[(size_t) index]);
}

const String XtoneProcessor::getProgramName (int index)
{
    const auto& p = xtp::factoryPresets();
    return index >= 0 && index < (int) p.size() ? String (p[(size_t) index].name) : String();
}

// ------------------------------------------------------------------ state

ValueTree XtoneProcessor::buildState()
{
    ValueTree root ("XTONE_STATE");
    root.setProperty ("stateVersion", kStateVersion, nullptr);
    root.appendChild (apvts.copyState(), nullptr); // "PARAMS"
    if (auto a = getCurrentAsset())
    {
        root.appendChild (a->getSampleTree(), nullptr);
        root.appendChild (a->getSlicesTree(), nullptr);
    }
    // SEQ is added with the step sequencer (M5)
    return root;
}

void XtoneProcessor::getStateInformation (MemoryBlock& dest)
{
    MemoryOutputStream out (dest, false);
    out.write (kStateMagic, 4);
    buildState().writeToStream (out);
}

void XtoneProcessor::restoreState (const ValueTree& root)
{
    if (! root.hasType ("XTONE_STATE")) return;
    // stateVersion 1: no migrations yet

    if (auto params = root.getChildWithName (apvts.state.getType()); params.isValid())
        apvts.replaceState (params);

    String error;
    auto asset = xtp::loader::fromStateTree (root.getChildWithName ("SAMPLE"), error);
    if (asset == nullptr) asset = xtp::loader::makeDefault();

    if (auto slices = root.getChildWithName ("SLICES"); slices.isValid())
        if (auto* mb = slices.getProperty ("markers").getBinaryData())
        {
            std::vector<int64_t> m (mb->getSize() / sizeof (int64_t));
            if (! m.empty()) std::memcpy (m.data(), mb->getData(), m.size() * sizeof (int64_t));
            asset = asset->withMarkers (std::move (m));
        }
    {
        const ScopedLock sl (guiLock);
        statusMessage = error;
    }
    publish (asset);
}

void XtoneProcessor::setStateInformation (const void* data, int size)
{
    if (data == nullptr || size < 4 || std::memcmp (data, kStateMagic, 4) != 0) return;
    restoreState (ValueTree::readFromData (static_cast<const char*> (data) + 4, (size_t) size - 4));
}

bool XtoneProcessor::saveStateToFile (const File& f)
{
    MemoryBlock mb;
    getStateInformation (mb);
    return f.replaceWithData (mb.getData(), mb.getSize());
}

bool XtoneProcessor::loadStateFromFile (const File& f)
{
    MemoryBlock mb;
    if (! f.loadFileAsData (mb) || mb.getSize() < 4) return false;
    setStateInformation (mb.getData(), (int) mb.getSize());
    return true;
}

AudioProcessorEditor* XtoneProcessor::createEditor() { return new XtoneEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new XtoneProcessor(); }
