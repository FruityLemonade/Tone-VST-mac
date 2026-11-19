#include "Presets.h"

namespace xtp
{
const std::vector<FactoryPreset>& factoryPresets()
{
    // More programs follow with the milestones that add their features (see HANDOFF 11).
    static const std::vector<FactoryPreset> presets {
        { "Init", {} },
        { "Speed Grid 500",
          { { "rate_src", 0.0f }, { "bpm", 500.0f }, { "division", 4.0f }, { "midi_mode", 0.0f }, { "key_mode", 0.0f },
            { "trig_mode", 0.0f }, { "gate", 100.0f }, { "drive", 24.0f }, { "clip", 2.0f }, { "mono", 1.0f }, { "os", 1.0f } } },
        { "Melodic A440",
          { { "midi_mode", 0.0f }, { "key_mode", 1.0f }, { "glide", 30.0f }, { "glide_legato", 1.0f }, { "drive", 12.0f },
            { "clip", 1.0f } } },
    };
    return presets;
}

void applyPreset (juce::AudioProcessorValueTreeState& state, const FactoryPreset& preset)
{
    for (auto* p : state.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : preset.values)
        if (auto* rp = state.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}
} // namespace xtp
