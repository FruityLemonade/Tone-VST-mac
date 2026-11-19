#pragma once
#include "HitVoice.h"
#include "RetriggerClock.h"

namespace xt
{
/// One unison lane: its own clock and voice pool.
class Lane
{
public:
    static constexpr int kMaxVoices = 33; // 32 overlap voices + 1 slot for a fading, stolen voice

    RetriggerClock clock;
    double detuneRatio { 1.0 };
    float gainL { 1.0f }, gainR { 1.0f };

    void killAll() noexcept
    {
        for (auto& v : voices) v.kill();
    }

    /// Start a new hit. At most `limit` voices are non-releasing afterwards; displaced
    /// voices fade out over `stealFade` engine samples (0 = hard cut).
    void trigger (const HitStart& h, int limit, double stealFade) noexcept;

    /// Fade out all voices (sequencer rest, etc.).
    void releaseAll (double fade) noexcept
    {
        for (auto& v : voices) v.release (fade);
    }

    void render (const SampleView& s, double pitchMul, float& l, float& r) noexcept
    {
        float vl = 0.0f, vr = 0.0f;
        for (int i = 0; i < kMaxVoices; ++i)
            if (voices[i].isActive()) voices[i].render (s, pitchMul, vl, vr);
        l += vl * gainL;
        r += vr * gainR;
    }

    int activeCount() const noexcept;
    int nonReleasingCount() const noexcept;
    /// Position of the newest active voice, or -1.
    double newestPosition() const noexcept;

private:
    HitVoice voices[kMaxVoices];
    uint64_t stampCounter { 0 };
};
} // namespace xt
