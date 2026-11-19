#pragma once
#include <cstdint>

namespace xt
{
/// Non-owning view of decoded sample audio. Owner guarantees lifetime.
struct SampleView
{
    const float* ch[2] { nullptr, nullptr }; // ch[1] == ch[0] for mono files
    const float* mono { nullptr };           // (L+R)/2 mix (== ch[0] for mono files)
    int64_t frames { 0 };
    int numCh { 0 };
    double sampleRate { 48000.0 };

    bool valid() const noexcept { return frames > 0 && ch[0] != nullptr && mono != nullptr; }
};

/// Sample plus slice table, as seen by the audio thread.
struct SourceView
{
    SampleView sample;
    const int64_t* bounds { nullptr }; // numSlices + 1 entries, ascending, bounds[0] = 0, bounds[n] = frames
    int numSlices { 0 };

    bool valid() const noexcept { return sample.valid() && bounds != nullptr && numSlices > 0; }
};
} // namespace xt
