#pragma once
#ifndef TP_INTERNAL_COMPONENTS_GUARD
#error Include Components.h instead
#endif
#include <CharacterCreation/AppearanceProbe.h>
#include <CharacterCreation/AppearanceApply.h>

struct RemotePlayerAppearanceBaseComponent : STRE::CharacterCreation::AppearanceBaseIdentity
{
    RemotePlayerAppearanceBaseComponent(uint32_t aActorId, uint32_t aBaseId)
        : AppearanceBaseIdentity{aActorId, aBaseId}
    {
    }
};

struct RemoteAppearanceProbeComponent : STRE::CharacterCreation::AppearanceApply
{
};
