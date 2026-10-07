#pragma once

#ifndef TP_INTERNAL_COMPONENTS_GUARD
#error Include Components.h instead
#endif

#include <Structs/CharacterBuild.h>
#include <Messages/CharacterAppearanceUpdate.h>
#include <CampaignState.h>
#include <optional>

struct CharacterBuildComponent
{
    std::uint64_t Revision{};
    CharacterBuildSnapshotData Build{};
    bool Applied{};
    // Volatile provenance only; not Character Build persistence or restoration.
    std::optional<STRE::Campaign::CampaignMemberIdentity> CampaignIdentity;
    std::optional<CharacterAppearanceUpdate> FinalAppearance;
};
