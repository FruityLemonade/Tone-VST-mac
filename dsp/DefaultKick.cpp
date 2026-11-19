#include "DefaultKick.h"

#include "Random.h"

#include <algorithm>
#include <cmath>

namespace xt
{
std::vector<float> makeDefaultKick (double fs)
{
    constexpr double kPi = 3.14159265358979323846;
    const auto n = (size_t) std::lround (0.45 * fs);
    std::vector<float> out (n);

    // analytic phase: integral of f(t)
    const double tauF = 0.025, tauA = 0.18;
    Random rng (0x5EEDull);
    const double clickLen = 0.002 * fs;
    const double clickGain = std::pow (10.0, -12.0 / 20.0);
    double lp = 0.0;
    const double lpCoef = 1.0 - std::exp (-2.0 * kPi * 6000.0 / fs); // gentle low-pass on the noise

    for (size_t i = 0; i < n; ++i)
    {
        const double t = (double) i / fs;
        const double ph = 2.0 * kPi * (45.0 * t + 135.0 * tauF * (1.0 - std::exp (-t / tauF)));
        double v = std::sin (ph) * std::exp (-t / tauA);
        lp += (rng.nextBipolar() - lp) * lpCoef;
        if ((double) i < clickLen)
        {
            const double w = 0.5 + 0.5 * std::cos (kPi * (double) i / clickLen);
            v += lp * clickGain * w;
        }
        out[i] = (float) v;
    }

    // short fade-out at the very end
    const size_t fade = std::min (n, (size_t) (0.005 * fs));
    for (size_t i = 0; i < fade; ++i)
        out[n - 1 - i] *= (float) i / (float) fade;

    float peak = 0.0f;
    for (float v : out) peak = std::max (peak, std::abs (v));
    if (peak > 0.0f)
    {
        const float g = (float) std::pow (10.0, -1.0 / 20.0) / peak;
        for (float& v : out) v *= g;
    }
    return out;
}
} // namespace xt
