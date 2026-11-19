#include "RateMath.h"

#include <array>
#include <cmath>
#include <cstdio>

namespace xt
{
double lenQ (int division, int divMod) noexcept
{
    static constexpr std::array<double, NumDivisions> base { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625 };
    if (division < 0) division = 0;
    if (division >= NumDivisions) division = NumDivisions - 1;
    double q = base[(size_t) division];
    if (divMod == ModTriplet) q *= 2.0 / 3.0;
    else if (divMod == ModDotted) q *= 1.5;
    return q;
}

double bpmToHz (double bpm, int division, int divMod) noexcept
{
    return bpm / 60.0 / lenQ (division, divMod);
}

double noteToHz (double note, double a4) noexcept
{
    return a4 * std::exp2 ((note - 69.0) / 12.0);
}

double hzToNote (double hz, double a4) noexcept
{
    if (hz <= 0.0) return -1000.0;
    return 69.0 + 12.0 * std::log2 (hz / a4);
}

bool isInScale (int pitchClass, Scale scale, int root) noexcept
{
    // Interval masks relative to the root (bit i = semitone i).
    auto mask = [] (std::initializer_list<int> iv)
    {
        unsigned m = 0;
        for (int i : iv) m |= 1u << i;
        return m;
    };
    unsigned m = 0;
    switch (scale)
    {
        case Scale::Chromatic:       return true;
        case Scale::Major:           m = mask ({ 0, 2, 4, 5, 7, 9, 11 }); break;
        case Scale::Minor:           m = mask ({ 0, 2, 3, 5, 7, 8, 10 }); break;
        case Scale::HarmonicMinor:   m = mask ({ 0, 2, 3, 5, 7, 8, 11 }); break;
        case Scale::Phrygian:        m = mask ({ 0, 1, 3, 5, 7, 8, 10 }); break;
        case Scale::MinorPentatonic: m = mask ({ 0, 3, 5, 7, 10 }); break;
        case Scale::RootOnly:        m = mask ({ 0 }); break;
        case Scale::Count:           return true;
    }
    const int rel = ((pitchClass - root) % 12 + 12) % 12;
    return (m >> rel) & 1u;
}

int snapNote (double note, Scale scale, int root) noexcept
{
    const int centre = (int) std::floor (note);
    int best = centre;
    double bestDist = 1.0e9;
    for (int d = -12; d <= 13; ++d)
    {
        const int n = centre + d;
        const int pc = ((n % 12) + 12) % 12;
        if (! isInScale (pc, scale, root)) continue;
        const double dist = std::abs ((double) n - note);
        if (dist < bestDist - 1.0e-12)
        {
            bestDist = dist;
            best = n;
        }
    }
    return best;
}

double snapHz (double hz, Scale scale, int root, double a4) noexcept
{
    if (hz <= 0.0) return hz;
    return noteToHz ((double) snapNote (hzToNote (hz, a4), scale, root), a4);
}

static const char* const kNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

std::string noteNameInt (int note, bool flNotation)
{
    const int pc = ((note % 12) + 12) % 12;
    const int octave = (int) std::floor ((double) note / 12.0) - 1 + (flNotation ? 1 : 0);
    return std::string (kNames[pc]) + std::to_string (octave);
}

std::string noteName (double note, bool flNotation)
{
    const int n = (int) std::lround (note);
    const int cents = (int) std::lround ((note - (double) n) * 100.0);
    std::string s = noteNameInt (n, flNotation);
    if (cents != 0)
    {
        char buf[16];
        std::snprintf (buf, sizeof (buf), " %+d ct", cents);
        s += buf;
    }
    return s;
}

std::string noteNameBoth (int note)
{
    return noteNameInt (note, false) + " / FL " + noteNameInt (note, true);
}
} // namespace xt
