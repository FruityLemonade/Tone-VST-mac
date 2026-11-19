#pragma once
#include <vector>

namespace xt
{
/// Synthetic default kick (48 kHz, 450 ms, mono, normalised to -1 dBFS).
/// f(t) = 45 + 135 * exp(-t / 25 ms) Hz, amp exp(-t / 180 ms), plus a 2 ms noise click at -12 dB.
std::vector<float> makeDefaultKick (double sampleRate = 48000.0);
} // namespace xt
