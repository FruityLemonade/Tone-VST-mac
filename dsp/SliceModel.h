#pragma once
#include <cstdint>
#include <vector>

namespace xt
{
/// Sorted slice markers in source frames. 0 and `frames` are implicit.
/// Not real-time safe (allocates) - used on the message / loader thread only.
class SliceModel
{
public:
    static constexpr int kMaxMarkers = 127;

    void setLength (int64_t frames);
    int64_t getLength() const noexcept { return frames; }

    void clear() { markers.clear(); }
    void setEqual (int numSlices);
    /// Inserts a marker; returns false if invalid / duplicate / full.
    bool add (int64_t frame);
    /// Moves marker `index` to `frame` (clamped between its neighbours).
    void move (int index, int64_t frame);
    void remove (int index);
    void setMarkers (std::vector<int64_t> m);

    const std::vector<int64_t>& getMarkers() const noexcept { return markers; }
    int numSlices() const noexcept { return (int) markers.size() + 1; }

    /// numSlices()+1 boundaries: 0, markers..., frames
    std::vector<int64_t> bounds() const;

    /// Index of the nearest marker within maxDist frames, or -1.
    int findMarker (int64_t frame, int64_t maxDist) const noexcept;

private:
    void sanitize();
    int64_t frames { 0 };
    std::vector<int64_t> markers;
};
} // namespace xt
