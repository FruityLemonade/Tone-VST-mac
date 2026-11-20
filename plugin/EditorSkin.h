#pragma once

#include <juce_core/juce_core.h>
#include <array>

namespace xtp
{
enum class EditorSkin { hammerDrill, makita, festool, hilti };

struct EditorSkinInfo
{
    EditorSkin skin;
    const char* id;
    const char* name;
};

inline constexpr std::array editorSkins {
    EditorSkinInfo { EditorSkin::hammerDrill, "hammer-drill", "Hammer Drill" },
    EditorSkinInfo { EditorSkin::makita, "makita", "Makita" },
    EditorSkinInfo { EditorSkin::festool, "festool", "Festool" },
    EditorSkinInfo { EditorSkin::hilti, "hilti", "Hilti" }
};

inline EditorSkin editorSkinFromId (const juce::String& id)
{
    for (const auto& entry : editorSkins)
        if (id == entry.id)
            return entry.skin;

    return EditorSkin::hammerDrill;
}
} // namespace xtp
