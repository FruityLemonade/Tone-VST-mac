#include "RetriggerClock.h"

#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace xt;

TEST_CASE ("T3 clock accuracy", "[T3]")
{
    const double fs = 48000.0, f = 1234.5678;
    RetriggerClock c;
    c.reset();
    const double inc = f / fs;
    double first = -1.0, last = 0.0;
    long hits = 0;
    bool subOk = true;
    for (long i = 0; i < (long) (10.0 * fs); ++i)
    {
        double sub = 0.0;
        if (c.tick (inc, sub))
        {
            subOk &= sub >= 0.0 && sub < 1.0;
            const double t = (double) i - sub;
            if (first < 0.0) first = t;
            last = t;
            ++hits;
        }
    }
    REQUIRE (subOk);
    REQUIRE (hits > 1000);
    const double measured = (double) (hits - 1) / (last - first) * fs;
    CHECK (std::abs (measured / f - 1.0) < 1.0e-9);
}

TEST_CASE ("T7 jitter", "[T7]")
{
    const double fs = 48000.0, f = 100.0;
    auto run = [&] (double jitter, uint64_t seed)
    {
        RetriggerClock c;
        c.setSeed (seed);
        c.setJitter (jitter);
        c.reset();
        std::vector<double> times;
        for (long i = 0; i < (long) (60.0 * fs); ++i)
        {
            double sub = 0.0;
            if (c.tick (f / fs, sub)) times.push_back ((double) i - sub);
        }
        return times;
    };
    // deterministic without jitter
    CHECK (run (0.0, 1) == run (0.0, 2));
    // mean rate preserved with 100 % jitter
    const auto t = run (1.0, 1234);
    const double mean = (double) (t.size() - 1) / (t.back() - t.front()) * fs;
    CHECK (std::abs (mean / f - 1.0) < 0.01);
    // and the periods really vary
    double minP = 1e9, maxP = 0;
    for (size_t i = 1; i < t.size(); ++i)
    {
        minP = std::min (minP, t[i] - t[i - 1]);
        maxP = std::max (maxP, t[i] - t[i - 1]);
    }
    CHECK (maxP / minP > 2.0);
}
