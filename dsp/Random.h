#pragma once
#include <cstdint>

namespace xt
{
/// Small deterministic PRNG (xorshift64*), real-time safe.
class Random
{
public:
    explicit Random (uint64_t seed = 0x9E3779B97F4A7C15ull) noexcept { setSeed (seed); }

    void setSeed (uint64_t seed) noexcept { state = seed ? seed : 0x9E3779B97F4A7C15ull; }

    uint64_t nextU64() noexcept
    {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545F4914F6CDD1Dull;
    }

    /// Uniform in [0, 1).
    double next01() noexcept { return (double) (nextU64() >> 11) * (1.0 / 9007199254740992.0); }

    /// Uniform in [-1, 1).
    double nextBipolar() noexcept { return next01() * 2.0 - 1.0; }

private:
    uint64_t state {};
};
} // namespace xt
