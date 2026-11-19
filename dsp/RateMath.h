#pragma once
// Pure C++20 rate / pitch helpers. No JUCE dependency.
#include <string>

namespace xt
{
// Index order matches the `division` parameter choices.
enum Division : int { Div1_1 = 0, Div1_2, Div1_4, Div1_8, Div1_16, Div1_32, Div1_64, NumDivisions };
// Index order matches the `div_mod` parameter choices.
enum DivMod : int { ModStraight = 0, ModTriplet, ModDotted, NumDivMods };

enum class Scale : int
{
    Chromatic = 0,
    Major,
    Minor,
    HarmonicMinor,
    Phrygian,
    MinorPentatonic,
    RootOnly,
    Count
};

/// Note length in quarter notes for a division (+ modifier). 1/4 -> 1, 1/16 -> 0.25.
double lenQ (int division, int divMod) noexcept;

/// Retrigger rate in Hz for a tempo and note length: bpm / 60 / lenQ.
double bpmToHz (double bpm, int division, int divMod) noexcept;

/// Equivalent tempo of quarter-note kicks at the given rate.
inline double hzToBpmEquivalent (double hz) noexcept { return hz * 60.0; }

/// MIDI note (fractional allowed) -> Hz.
double noteToHz (double note, double a4 = 440.0) noexcept;

/// Hz -> fractional MIDI note.
double hzToNote (double hz, double a4 = 440.0) noexcept;

/// True if the pitch class (0 = C) belongs to `scale` rooted at `root` (0..11).
bool isInScale (int pitchClass, Scale scale, int root) noexcept;

/// Nearest integer MIDI note of `scale`/`root` to a fractional note.
/// Ties resolve towards the lower note.
int snapNote (double note, Scale scale, int root) noexcept;

/// Snap a frequency to the nearest allowed note (tuning reference a4).
double snapHz (double hz, Scale scale, int root, double a4 = 440.0) noexcept;

/// e.g. "C1 +33 ct" (scientific: MIDI 60 = C4) or with fl=true "C2 +33 ct" (FL: MIDI 60 = C5).
/// Cents are omitted when |cents| < 0.5.
std::string noteName (double note, bool flNotation);

/// Name of an integer note without cents, e.g. "C4" or (fl) "C5".
std::string noteNameInt (int note, bool flNotation);

/// "C4 / FL C5"
std::string noteNameBoth (int note);
} // namespace xt
