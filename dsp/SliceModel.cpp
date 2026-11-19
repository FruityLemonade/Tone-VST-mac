#include "SliceModel.h"

#include <algorithm>
#include <cstdlib>

namespace xt
{
void SliceModel::setLength (int64_t f)
{
    frames = f < 0 ? 0 : f;
    sanitize();
}

void SliceModel::setEqual (int n)
{
    markers.clear();
    n = std::clamp (n, 1, kMaxMarkers + 1);
    for (int i = 1; i < n; ++i)
        markers.push_back ((int64_t) ((double) frames * i / n));
    sanitize();
}

bool SliceModel::add (int64_t frame)
{
    if (frame <= 0 || frame >= frames || (int) markers.size() >= kMaxMarkers) return false;
    if (std::find (markers.begin(), markers.end(), frame) != markers.end()) return false;
    markers.insert (std::upper_bound (markers.begin(), markers.end(), frame), frame);
    return true;
}

void SliceModel::move (int index, int64_t frame)
{
    if (index < 0 || index >= (int) markers.size()) return;
    const int64_t lo = index > 0 ? markers[(size_t) index - 1] + 1 : 1;
    const int64_t hi = index + 1 < (int) markers.size() ? markers[(size_t) index + 1] - 1 : frames - 1;
    if (lo > hi) return;
    markers[(size_t) index] = std::clamp (frame, lo, hi);
}

void SliceModel::remove (int index)
{
    if (index >= 0 && index < (int) markers.size())
        markers.erase (markers.begin() + index);
}

void SliceModel::setMarkers (std::vector<int64_t> m)
{
    markers = std::move (m);
    sanitize();
}

std::vector<int64_t> SliceModel::bounds() const
{
    std::vector<int64_t> b;
    b.reserve (markers.size() + 2);
    b.push_back (0);
    b.insert (b.end(), markers.begin(), markers.end());
    b.push_back (frames);
    return b;
}

int SliceModel::findMarker (int64_t frame, int64_t maxDist) const noexcept
{
    int best = -1;
    int64_t bestD = maxDist + 1;
    for (size_t i = 0; i < markers.size(); ++i)
    {
        const int64_t d = std::llabs (markers[i] - frame);
        if (d < bestD) { bestD = d; best = (int) i; }
    }
    return best;
}

void SliceModel::sanitize()
{
    std::sort (markers.begin(), markers.end());
    markers.erase (std::unique (markers.begin(), markers.end()), markers.end());
    markers.erase (std::remove_if (markers.begin(), markers.end(),
                                   [this] (int64_t m) { return m <= 0 || m >= frames; }),
                   markers.end());
    if ((int) markers.size() > kMaxMarkers) markers.resize ((size_t) kMaxMarkers);
}
} // namespace xt
