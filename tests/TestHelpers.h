#pragma once
#include "Engine.h"

#include <cmath>
#include <vector>

namespace xt::test
{
/// Owns sample data + slice bounds and exposes a SourceView.
struct OwnedSource
{
    std::vector<float> data;
    std::vector<int64_t> bounds;
    double rate { 48000.0 };

    OwnedSource (std::vector<float> d, double sampleRate, std::vector<int64_t> markers = {})
        : data (std::move (d)), rate (sampleRate)
    {
        bounds.push_back (0);
        bounds.insert (bounds.end(), markers.begin(), markers.end());
        bounds.push_back ((int64_t) data.size());
    }

    SourceView view() const
    {
        SourceView v;
        v.sample.ch[0] = v.sample.ch[1] = v.sample.mono = data.data();
        v.sample.frames = (int64_t) data.size();
        v.sample.numCh = 1;
        v.sample.sampleRate = rate;
        v.bounds = bounds.data();
        v.numSlices = (int) bounds.size() - 1;
        return v;
    }
};

inline std::vector<float> makeSine (double f, double fs, double seconds, double amp = 0.5)
{
    std::vector<float> v ((size_t) (seconds * fs));
    for (size_t i = 0; i < v.size(); ++i)
        v[i] = (float) (amp * std::sin (2.0 * 3.14159265358979323846 * f * (double) i / fs));
    return v;
}

/// Neutral params: no velocity scaling, no drive/clip, unity output.
inline EngineParams cleanParams()
{
    EngineParams p;
    p.velSens = 0.0;
    p.clip = 0;
    p.driveDb = 0.0;
    p.outGainDb = 0.0;
    p.envA = 0.0;
    p.envS = 1.0;
    return p;
}

struct Stereo
{
    std::vector<float> l, r;
};

/// Renders `total` samples in blocks of `block`, delivering events at absolute positions.
inline Stereo render (Engine& e, int total, int block, std::vector<MidiEvent> absEvents)
{
    Stereo out;
    out.l.resize ((size_t) total);
    out.r.resize ((size_t) total);
    std::vector<MidiEvent> local;
    size_t ei = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        local.clear();
        while (ei < absEvents.size() && absEvents[ei].offset < pos + n)
        {
            auto ev = absEvents[ei++];
            ev.offset -= pos;
            local.push_back (ev);
        }
        e.process (out.l.data() + pos, out.r.data() + pos, n, local.data(), (int) local.size());
    }
    return out;
}

inline MidiEvent noteOn (int offset, int note, float vel = 1.0f) { return { offset, MidiEvent::NoteOn, note, vel }; }
inline MidiEvent noteOff (int offset, int note) { return { offset, MidiEvent::NoteOff, note, 0.0f }; }
inline MidiEvent bend (int offset, float v) { return { offset, MidiEvent::PitchBend, 0, v }; }
} // namespace xt::test
