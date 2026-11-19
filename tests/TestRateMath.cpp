#include "RateMath.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace xt;
using Catch::Approx;

TEST_CASE ("T1 RateMath", "[T1]")
{
    CHECK (bpmToHz (500.0, Div1_16, ModStraight) == Approx (33.3333333333).epsilon (1e-9));
    CHECK (bpmToHz (500.0, Div1_16, ModDotted) == Approx (22.2222222222).epsilon (1e-9));
    CHECK (bpmToHz (500.0, Div1_16, ModTriplet) == Approx (50.0).epsilon (1e-9));
    CHECK (bpmToHz (500.0, Div1_4, ModStraight) == Approx (8.3333333333).epsilon (1e-9));
    CHECK (bpmToHz (500.0, Div1_64, ModStraight) == Approx (133.333333333).epsilon (1e-9));
    CHECK (bpmToHz (26400.0, Div1_4, ModStraight) == Approx (440.0));
    CHECK (noteToHz (69.0) == Approx (440.0));
    CHECK (noteToHz (36.0) == Approx (65.406).epsilon (1e-4));
    CHECK (hzToNote (440.0) == Approx (69.0));
    // relative keytrack: root 60, note 72 doubles the rate
    CHECK (std::exp2 ((72 - 60) / 12.0) == Approx (2.0));
    CHECK (hzToBpmEquivalent (33.3333333333) == Approx (2000.0));

    CHECK (noteName (hzToNote (bpmToHz (500.0, Div1_16, ModStraight)), false) == "C1 +33 ct");
    CHECK (noteName (hzToNote (bpmToHz (500.0, Div1_16, ModStraight)), true) == "C2 +33 ct");
    CHECK (noteName (hzToNote (bpmToHz (500.0, Div1_4, ModStraight)), false) == "C-1 +33 ct");
    CHECK (noteName (hzToNote (bpmToHz (500.0, Div1_16, ModDotted)), false) == "F0 +31 ct");
    CHECK (noteName (hzToNote (bpmToHz (500.0, Div1_16, ModTriplet)), false) == "G1 +35 ct");
    CHECK (noteNameBoth (60) == "C4 / FL C5");
}

TEST_CASE ("T2 Tune-Snap", "[T2]")
{
    CHECK (snapHz (8.3333333, Scale::Chromatic, 0) == Approx (8.1758).epsilon (1e-4));
    CHECK (snapNote (60.4, Scale::Chromatic, 0) == 60);
    CHECK (snapNote (60.6, Scale::Chromatic, 0) == 61);
    // root only (C): every result must be a C
    for (double n = 20.0; n < 100.0; n += 0.37)
        CHECK (snapNote (n, Scale::RootOnly, 0) % 12 == 0);
    CHECK (snapNote (65.0, Scale::RootOnly, 0) == 60);
    CHECK (snapNote (67.0, Scale::RootOnly, 0) == 72);
    // major scale in C: C# -> C (tie resolves downwards), F# -> F
    CHECK (snapNote (61.0, Scale::Major, 0) == 60);
    CHECK (snapNote (66.0, Scale::Major, 0) == 65);
    CHECK (isInScale (9, Scale::Minor, 9));
    CHECK_FALSE (isInScale (1, Scale::MinorPentatonic, 0));
}
