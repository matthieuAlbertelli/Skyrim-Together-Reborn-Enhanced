#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace STRE::Valen
{
// Local presentation only, on STRE_QUEST_ValenDialogue (excluded from quest sync).
// Native completed-stage bits live in this character's .ess; there is no process
// cache to replay into a loaded save or a subsequent character. No FormID here.
inline constexpr std::array<std::string_view, 21> kClasses{
    "class.archer", "class.barbarian", "class.crusader", "class.knight",
    "class.rogue", "class.scout", "class.warrior", "class.acrobat",
    "class.agent", "class.assassin", "class.bard", "class.monk",
    "class.pilgrim", "class.thief", "class.battlemage", "class.healer",
    "class.mage", "class.nightblade", "class.sorcerer", "class.spellsword",
    "class.witchhunter"};

constexpr std::uint16_t ClassStage(std::string_view aValidatedClass) noexcept
{
    for (std::uint16_t i = 0; i < kClasses.size(); ++i)
        if (kClasses[i] == aValidatedClass)
            return 101 + i;
    return 100; // Explicit unknown-class fallback; never infer from equipment.
}
inline constexpr std::uint16_t kGroupStage = 50;
}
