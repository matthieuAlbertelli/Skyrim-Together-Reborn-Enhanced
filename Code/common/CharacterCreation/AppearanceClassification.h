#pragma once
#include <Messages/CharacterAppearanceUpdate.h>

namespace STRE::CharacterCreation
{
enum class AppearanceClassification
{
    SameRaceSameSex,
    RaceChangeSameSex,
    SameRaceSexChange,
    RaceAndSexChange,
    Invalid
};

// Pure diff classification; domain eligibility is a separate adapter concern.
inline AppearanceClassification ClassifyAppearanceTransition(const GameId& currentRace, uint8_t currentSex, const GameId& targetRace, uint8_t targetSex) noexcept
{
    if (!currentRace.BaseId || !targetRace.BaseId || currentSex > 1 || targetSex > 1)
        return AppearanceClassification::Invalid;
    if (currentSex != targetSex)
        return currentRace == targetRace ? AppearanceClassification::SameRaceSexChange : AppearanceClassification::RaceAndSexChange;
    return currentRace == targetRace ? AppearanceClassification::SameRaceSameSex : AppearanceClassification::RaceChangeSameSex;
}

// Classification is followed by runtime, private provenance and experiment-specific model guards.
inline AppearanceClassification ClassifyAppearanceUpdate(
    const CharacterAppearanceDescriptor& incoming, const GameId& runtimeRace, const GameId& baseRace, uint8_t sex, bool targetResolved, bool privateBase) noexcept
{
    if (!incoming.IsValid() || !runtimeRace.BaseId || runtimeRace != baseRace || sex > 1 || !targetResolved || !privateBase)
        return AppearanceClassification::Invalid;
    return ClassifyAppearanceTransition(runtimeRace, sex, incoming.Race, incoming.Sex);
}
} // namespace STRE::CharacterCreation
