#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace xtp
{
/// Factory programs (parameters only - the current sample is kept).
struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values; // param id -> real (denormalised) value; others = default
};

const std::vector<FactoryPreset>& factoryPresets();

/// Sets every parameter to its default, then applies the preset values.
void applyPreset (juce::AudioProcessorValueTreeState& state, const FactoryPreset& preset);
} // namespace xtp
