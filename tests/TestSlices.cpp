#include "SliceModel.h"

#include <catch2/catch_test_macros.hpp>

using namespace xt;

TEST_CASE ("SliceModel basics", "[slices]")
{
    SliceModel m;
    m.setLength (1000);
    CHECK (m.numSlices() == 1);
    m.setEqual (4);
    CHECK (m.getMarkers() == std::vector<int64_t> { 250, 500, 750 });
    CHECK (m.bounds() == std::vector<int64_t> { 0, 250, 500, 750, 1000 });
    CHECK (m.add (100));
    CHECK_FALSE (m.add (100));
    CHECK_FALSE (m.add (0));
    CHECK_FALSE (m.add (1000));
    CHECK (m.numSlices() == 5);
    m.move (0, 600); // clamped below the next marker (250)
    CHECK (m.getMarkers()[0] == 249);
    m.remove (0);
    CHECK (m.numSlices() == 4);
    CHECK (m.findMarker (505, 10) == 1);
    CHECK (m.findMarker (600, 10) == -1);
    m.setEqual (200);
    CHECK ((int) m.getMarkers().size() == SliceModel::kMaxMarkers);
}
