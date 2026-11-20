#pragma once
#include <cmath>

namespace xt
{
/// One-pole smoother (exponential). timeMs = 0 -> jumps immediately.
class OnePole
{
public:
    void setTime (double timeMs, double fs) noexcept
    {
        const double n = timeMs * 0.001 * fs;
        coef = n <= 0.0 ? 1.0 : 1.0 - std::exp (-1.0 / n);
    }
    void reset (double v) noexcept { value = v; }
    double process (double target) noexcept
    {
        value += (target - value) * coef;
        return value;
    }
    double get() const noexcept { return value; }

private:
    double value { 0.0 };
    double coef { 1.0 };
};

/// Linear-attack / exponential decay & release ADSR on the lane sum.
class Adsr
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate (double newFs) noexcept { fs = newFs; }

    void setParams (double aMs, double dMs, double sustain01, double rMs) noexcept
    {
        attackInc = aMs <= 0.0 ? 2.0 : 1.0 / (aMs * 0.001 * fs);
        decayCoef = timeToCoef (dMs);
        sustain = sustain01;
        releaseCoef = timeToCoef (rMs);
        releaseInstant = rMs <= 0.0;
    }

    void noteOn() noexcept { stage = Stage::Attack; }
    void noteOff() noexcept
    {
        if (stage == Stage::Idle) return;
        stage = releaseInstant ? Stage::Idle : Stage::Release;
        if (releaseInstant) level = 0.0;
    }
    void reset() noexcept { stage = Stage::Idle; level = 0.0; }

    bool isIdle() const noexcept { return stage == Stage::Idle; }
    bool isReleasing() const noexcept { return stage == Stage::Release; }
    double getLevel() const noexcept { return level; }
    Stage getStage() const noexcept { return stage; }

    double process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle: level = 0.0; break;
            case Stage::Attack:
                level += attackInc;
                if (level >= 1.0) { level = 1.0; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                level = sustain + (level - sustain) * decayCoef;
                if (std::abs (level - sustain) < 1.0e-5) { level = sustain; stage = Stage::Sustain; }
                break;
            case Stage::Sustain: level = sustain; break;
            case Stage::Release:
                level *= releaseCoef;
                if (level < 1.0e-5) { level = 0.0; stage = Stage::Idle; }
                break;
        }
        return level;
    }

private:
    // time to fall to -80 dB (factor 1e-4)
    double timeToCoef (double ms) const noexcept
    {
        const double n = ms * 0.001 * fs;
        return n <= 0.0 ? 0.0 : std::exp (std::log (1.0e-4) / n);
    }

    double fs { 48000.0 };
    Stage stage { Stage::Idle };
    double level { 0.0 };
    double attackInc { 1.0 }, decayCoef { 0.0 }, sustain { 1.0 }, releaseCoef { 0.0 };
    bool releaseInstant { false };
};
} // namespace xt
