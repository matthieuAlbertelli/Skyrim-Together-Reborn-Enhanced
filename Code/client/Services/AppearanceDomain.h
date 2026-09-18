#pragma once
#include <CharacterCreation/AppearanceDomain.h>
#include <Forms/TESRace.h>
#include <Games/TES.h>
#include <VersionDb.h>
#include <cstring>

inline std::string_view AppearanceRaceModel(const TESRace* race, uint8_t sex)
{
    if (!race || sex > 1)
        return {};
    const char* text = race->skeletonModels[sex].name.AsAscii();
    const auto size = text ? strnlen(text, 513) : 0;
    return text && size <= 512 ? std::string_view(text, size) : std::string_view{};
}

inline bool IsSupportedVanillaHumanoidAppearanceRace(const TESRace* race)
{
    // Guard before accessing the audited 1.6.1170 prefix.
    if (VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0" || !race || race->IsTemporary())
        return false;
    auto* manager = ModManager::Get();
    auto* master = manager ? manager->GetByName("Skyrim.esm") : nullptr;
    if (!master)
        return false;
    for (auto id : STRE::CharacterCreation::VanillaHumanoidAppearanceBaseIds)
        if (Cast<TESRace>(TESForm::GetById(master->GetFormId(id))) == race)
            return STRE::CharacterCreation::SupportedAppearanceRace(id, true, (race->appearanceRaceFlags & 1u) != 0, AppearanceRaceModel(race, 0), AppearanceRaceModel(race, 1));
    return false;
}
