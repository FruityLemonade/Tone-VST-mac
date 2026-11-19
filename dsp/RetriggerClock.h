#pragma once
#include "Random.h"

namespace xt
{
/// Phase accumulator that emits sub-sample-accurate retrigger events.
///
/// Jitter is applied to the *period* (uniform factor with mean 1), so the mean
/// period - and therefore the mean rate - is unchanged by jitter.
class RetriggerClock
{
public:
    void reset (double startPhase = 0.0) noexcept
    {
        phase = startPhase;
        periodFactor = 1.0;
    }

    void setJitter (double amount01) noexcept { jitter = amount01 < 0.0 ? 0.0 : (amount01 > 1.0 ? 1.0 : amount01); }
    void setSeed (uint64_t seed) noexcept { rng.setSeed (seed); }

    /// Advance by one engine sample. `baseInc` = rateHz / fsE (must be <= ~0.5).
    /// Returns true if a hit occurred during this sample; `subOffset` then receives
    /// the time (in samples, [0,1)) elapsed since the exact crossing.
    bool tick (double baseInc, double& subOffset) noexcept
    {
        const double inc = baseInc / periodFactor;
        lastInc = inc;
        phase += inc;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            if (phase >= 1.0) phase = 0.0; // safety, cannot happen with capped rates
            subOffset = phase / inc;
            if (subOffset >= 1.0) subOffset = 0.999999999;
            // new jitter factor for the next period
            periodFactor = jitter > 0.0 ? 1.0 + jitter * 0.5 * rng.nextBipolar() : 1.0;
            return true;
        }
        return false;
    }

    double getPhase() const noexcept { return phase; }
    void setPhase (double p) noexcept { phase = p - (double) (long long) p; if (phase < 0.0) phase += 1.0; }

    /// Effective increment of the most recent tick (includes jitter).
    double getLastInc() const noexcept { return lastInc; }
    double getPeriodFactor() const noexcept { return periodFactor; }

private:
    double phase { 0.0 };
    double periodFactor { 1.0 };
    double lastInc { 0.0 };
    double jitter { 0.0 };
    Random rng;
};
} // namespace xt
