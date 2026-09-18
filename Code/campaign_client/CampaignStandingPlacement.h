#pragma once

#include <CharacterCreation/StandingCreation.h>
#include <Structs/Campaign.h>
#include <vector>

namespace STRE::Campaign
{
// Placement-only projection. SlotId validation/admission elsewhere is unchanged.
// Use the durable local identity used for authentication, not Transport's numeric ID.
inline CharacterCreation::StandingCreationPositionResult ResolveCampaignStandingPlacement(const CampaignSnapshotData* aSnapshot, std::string_view aLocalPlayerId) noexcept
{
    if (aLocalPlayerId.empty())
        return {{}, "missing-local-player-id"};
    if (!aSnapshot || !aSnapshot->RosterSealed)
        return {{}, "sealed-roster-unavailable"};
    if (aSnapshot->CampaignId.empty())
        return {{}, "missing-campaign-id"};
    if (aSnapshot->Phase != kCampaignWirePhaseCharacterCreation)
        return {{}, "campaign-phase-not-character-creation"};
    if (aSnapshot->RuntimeState != kCampaignWireRuntimeActive)
        return {{}, "campaign-runtime-not-active"};

    std::vector<std::string_view> players;
    for (const auto& slot : aSnapshot->Roster)
        players.emplace_back(slot.PlayerId.data(), slot.PlayerId.size());
    auto result = CharacterCreation::ResolveStandingCreationPositionIndex(players, aLocalPlayerId);
    if (!result.Index)
        return result;
    for (const auto& slot : aSnapshot->Roster)
        if (!slot.Present)
            return {{}, "sealed-roster-incomplete"};
    return result;
}
} // namespace STRE::Campaign
