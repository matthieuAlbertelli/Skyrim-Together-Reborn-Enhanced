#pragma once

#include <Forms/TESForm.h>
#include <Components/TESFullName.h>
#include <Components/TESModel.h>

struct TESRace : TESForm, TESFullName
{
    // Read-only prefix through skeletonModels, CommonLibSSE-NG TESRace layout.
    uint8_t remainingComponents[0x68];
    TESModel skeletonModels[2];
    // Read-only RACE_DATA prefix (CommonLibSSE-NG): data at E8, flags at +20.
    uint8_t raceDataPrefix[0x20];
    uint32_t appearanceRaceFlags;
};
static_assert(sizeof(TESModel) == 0x28);
static_assert(offsetof(TESRace, skeletonModels) == 0x98);
static_assert(offsetof(TESRace, appearanceRaceFlags) == 0x108);
