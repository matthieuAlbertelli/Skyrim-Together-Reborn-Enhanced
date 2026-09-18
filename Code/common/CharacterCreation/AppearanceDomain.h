#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace STRE::CharacterCreation
{
// Skyrim.esm-relative records, audited against the base master and UI catalogue.
// This is phase eligibility only; concrete races never select a native strategy.
inline constexpr std::array<uint32_t, 8> VanillaHumanoidAppearanceBaseIds{0x13741, 0x13742, 0x13743, 0x13744, 0x13746, 0x13747, 0x13748, 0x13749};

inline bool AppearanceModelsValid(std::string_view male, std::string_view female)
{
    return !male.empty() && male.size() <= 512 && !female.empty() && female.size() <= 512;
}

inline bool SupportedAppearanceRace(uint32_t baseId, bool resolvedFromSkyrim, bool playable, std::string_view male, std::string_view female)
{
    if (!resolvedFromSkyrim || !playable || !AppearanceModelsValid(male, female))
        return false;
    for (auto id : VanillaHumanoidAppearanceBaseIds)
        if (baseId == id)
            return true;
    return false;
}
} // namespace STRE::CharacterCreation
