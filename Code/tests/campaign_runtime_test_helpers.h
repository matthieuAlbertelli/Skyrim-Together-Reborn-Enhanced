#pragma once

#include <CampaignRuntimeService.h>
#include <campaign_persistence_test_helpers.h>

#include <string>
#include <vector>

namespace STRE::Campaign::Test
{
inline CampaignSlotRecord MakeRuntimeSlot(std::size_t aIndex)
{
    const std::string suffix = std::to_string(aIndex);
    return {
        CampaignSlotId{"slot-" + suffix},
        PlayerId{"player-" + suffix},
        CharacterBindingId{"binding-" + suffix}};
}

inline CampaignSlotState MakeRuntimeSlotState(std::size_t aIndex)
{
    const CampaignSlotRecord slot = MakeRuntimeSlot(aIndex);
    return {slot.Slot, slot.Player, slot.CharacterBinding, false};
}

inline std::vector<CampaignSlotRecord> MakeRuntimeRoster(std::size_t aCount)
{
    std::vector<CampaignSlotRecord> result;
    result.reserve(aCount);
    for (std::size_t index = 1; index <= aCount; ++index)
        result.push_back(MakeRuntimeSlot(index));
    return result;
}

inline std::vector<CampaignSlotRecord> ToRuntimeRosterRecords(
    const CampaignAggregate& acCampaign)
{
    std::vector<CampaignSlotRecord> result;
    result.reserve(acCampaign.Roster.size());
    for (const CampaignSlotState& slot : acCampaign.Roster)
    {
        result.push_back(
            {slot.Slot, slot.Player, slot.CharacterBinding});
    }
    return result;
}

inline CampaignAggregate MakeSealedRuntimeCampaign(std::size_t aCount)
{
    CampaignAggregate campaign;
    campaign.Id = CampaignId{"campaign-runtime"};
    campaign.Version = 2;
    campaign.Phase = CampaignPhase::CharacterCreation;
    campaign.RosterSealed = true;
    campaign.SessionManager = PlayerId{"player-1"};
    for (std::size_t index = 1; index <= aCount; ++index)
        campaign.Roster.push_back(MakeRuntimeSlotState(index));
    CampaignStateMachine::SortRoster(campaign.Roster);
    return campaign;
}

inline CampaignMemberPresence MakePresence(
    const CampaignId& acCampaign,
    const CampaignSlotRecord& acSlot)
{
    return {
        {acCampaign,
         acSlot.Slot,
         acSlot.Player,
         acSlot.CharacterBinding},
        true,
        true};
}

inline std::vector<CampaignMemberPresence> MakeFullPresence(
    const CampaignAggregate& acCampaign)
{
    std::vector<CampaignMemberPresence> result;
    result.reserve(acCampaign.Roster.size());
    for (const CampaignSlotState& slot : acCampaign.Roster)
    {
        result.push_back(
            {{acCampaign.Id,
              slot.Slot,
              slot.Player,
              slot.CharacterBinding},
             true,
             true});
    }
    return result;
}

inline CreateLobbyCampaignCommand MakeRuntimeCampaignCommand(
    std::size_t aRosterSize = 0)
{
    CreateLobbyCampaignCommand command;
    command.Campaign = CampaignId{"campaign-runtime"};
    command.Mutation = MutationId{"mutation-create-runtime"};
    command.InitialRoster = MakeRuntimeRoster(aRosterSize);
    return command;
}

inline CampaignMemberIdentity MakeRuntimeIdentity(std::size_t aIndex)
{
    const CampaignSlotRecord slot = MakeRuntimeSlot(aIndex);
    return {
        CampaignId{"campaign-runtime"},
        slot.Slot,
        slot.Player,
        slot.CharacterBinding};
}
}
