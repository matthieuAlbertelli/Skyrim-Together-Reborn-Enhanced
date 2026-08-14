#include <campaign_runtime_test_helpers.h>

#include <catch2/catch.hpp>

#include <algorithm>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

TEST_CASE(
    "Mutable Lobby roster enforces exact durable identities and v1 limits",
    "[campaign.runtime][domain][roster]")
{
    CampaignAggregate campaign;
    campaign.Id = CampaignId{"campaign-runtime"};
    campaign.Version = 1;

    REQUIRE(CampaignStateMachine::AddSlot(
        campaign, MakeRuntimeSlotState(2)).Succeeded());
    REQUIRE(CampaignStateMachine::AddSlot(
        campaign, MakeRuntimeSlotState(1)).Succeeded());
    REQUIRE(campaign.Version == 3);
    REQUIRE(campaign.Roster[0].Slot == CampaignSlotId{"slot-1"});
    REQUIRE(campaign.Roster[1].Slot == CampaignSlotId{"slot-2"});

    CampaignSlotState duplicateSlot = MakeRuntimeSlotState(3);
    duplicateSlot.Slot = CampaignSlotId{"slot-1"};
    REQUIRE(
        CampaignStateMachine::AddSlot(campaign, duplicateSlot).Error ==
        CampaignError::DuplicateSlot);

    CampaignSlotState duplicatePlayer = MakeRuntimeSlotState(3);
    duplicatePlayer.Player = PlayerId{"player-1"};
    REQUIRE(
        CampaignStateMachine::AddSlot(campaign, duplicatePlayer).Error ==
        CampaignError::DuplicatePlayer);

    CampaignSlotState duplicateBinding = MakeRuntimeSlotState(3);
    duplicateBinding.CharacterBinding = CharacterBindingId{"binding-1"};
    REQUIRE(
        CampaignStateMachine::AddSlot(campaign, duplicateBinding).Error ==
        CampaignError::DuplicateCharacterBinding);

    CampaignSlotState invalid = MakeRuntimeSlotState(3);
    invalid.Player = PlayerId{};
    REQUIRE(
        CampaignStateMachine::AddSlot(campaign, invalid).Error ==
        CampaignError::InvalidIdentity);

    CampaignSlotState replacement = MakeRuntimeSlotState(9);
    replacement.Slot = CampaignSlotId{"slot-2"};
    REQUIRE(CampaignStateMachine::ReplaceSlot(
        campaign, replacement).Succeeded());
    REQUIRE(campaign.Roster[1].Player == PlayerId{"player-9"});
    REQUIRE(campaign.Roster[1].CharacterBinding ==
            CharacterBindingId{"binding-9"});

    REQUIRE(CampaignStateMachine::RemoveSlot(
        campaign, CampaignSlotId{"slot-1"}).Succeeded());
    REQUIRE(CampaignStateMachine::RemoveSlot(
        campaign, CampaignSlotId{"slot-9"}).Error ==
        CampaignError::SlotNotFound);
    REQUIRE(CampaignStateMachine::RemoveSlot(
        campaign, CampaignSlotId{"slot-2"}).Succeeded());
    REQUIRE(campaign.Roster.empty());
    REQUIRE(CampaignStateMachine::CommitCampaignStart(
        campaign,
        CampaignActor::Server(),
        PlayerId{"player-9"}).Error ==
        CampaignError::InvalidRoster);

    for (std::size_t index = 1; index <= kMaximumCampaignRosterSize; ++index)
    {
        REQUIRE(CampaignStateMachine::AddSlot(
            campaign, MakeRuntimeSlotState(index)).Succeeded());
    }
    REQUIRE(CampaignStateMachine::AddSlot(
        campaign, MakeRuntimeSlotState(11)).Error ==
        CampaignError::RosterLimitExceeded);
}

TEST_CASE(
    "Server-authorized campaign start seals the roster and establishes its manager",
    "[campaign.runtime][domain][seal]")
{
    CampaignAggregate campaign;
    campaign.Id = CampaignId{"campaign-runtime"};
    campaign.Version = 1;
    campaign.Roster = {
        MakeRuntimeSlotState(2),
        MakeRuntimeSlotState(1)};
    CampaignStateMachine::SortRoster(campaign.Roster);

    REQUIRE(CampaignStateMachine::GetTransitionPolicy(
        CampaignTransition::CommitCampaignStart).Authority ==
        CampaignTransitionAuthority::Server);
    REQUIRE(CampaignStateMachine::EvaluateTransition(
        campaign,
        CampaignTransition::CommitCampaignStart,
        CampaignActor::ForPlayer(PlayerId{"player-1"}),
        {}).Error == CampaignError::UnauthorizedActor);
    REQUIRE(CampaignStateMachine::CommitCampaignStart(
        campaign,
        CampaignActor::ForPlayer(PlayerId{"player-1"}),
        PlayerId{"player-1"}).Error ==
        CampaignError::UnauthorizedActor);
    REQUIRE_FALSE(campaign.RosterSealed);
    REQUIRE_FALSE(campaign.SessionManager);
    REQUIRE(campaign.Version == 1);

    REQUIRE(CampaignStateMachine::CommitCampaignStart(
        campaign,
        CampaignActor::Server(),
        PlayerId{"not-a-member"}).Error ==
        CampaignError::InvalidSessionManager);
    REQUIRE_FALSE(campaign.RosterSealed);
    REQUIRE(campaign.Phase == CampaignPhase::Lobby);
    REQUIRE(campaign.Version == 1);

    REQUIRE(CampaignStateMachine::CommitCampaignStart(
        campaign,
        CampaignActor::Server(),
        PlayerId{"player-1"}).Succeeded());
    REQUIRE(campaign.Version == 2);
    REQUIRE(campaign.RosterSealed);
    REQUIRE(campaign.Phase == CampaignPhase::CharacterCreation);
    REQUIRE(campaign.SessionManager == PlayerId{"player-1"});
    const auto sealedRoster = campaign.Roster;

    REQUIRE(CampaignStateMachine::AddSlot(
        campaign, MakeRuntimeSlotState(3)).Error ==
        CampaignError::RosterSealed);
    REQUIRE(CampaignStateMachine::RemoveSlot(
        campaign, CampaignSlotId{"slot-1"}).Error ==
        CampaignError::RosterSealed);
    CampaignSlotState replacement = MakeRuntimeSlotState(9);
    replacement.Slot = CampaignSlotId{"slot-1"};
    REQUIRE(CampaignStateMachine::ReplaceSlot(
        campaign, replacement).Error ==
        CampaignError::RosterSealed);

    REQUIRE(CampaignStateMachine::TransferSessionManager(
        campaign,
        PlayerId{"player-1"},
        PlayerId{"player-2"}).Succeeded());
    REQUIRE(campaign.SessionManager == PlayerId{"player-2"});
    REQUIRE(campaign.Roster == sealedRoster);
    REQUIRE(CampaignStateMachine::EvaluateTransition(
        campaign,
        CampaignTransition::CompleteCharacterCreation,
        CampaignActor::ForPlayer(PlayerId{"player-2"}),
        MakeFullPresence(campaign)).Error ==
        CampaignError::UnauthorizedActor);
}

TEST_CASE(
    "Full roster predicate distinguishes transport admission and exact identity",
    "[campaign.runtime][domain][eligibility]")
{
    CampaignAggregate campaign = MakeSealedRuntimeCampaign(2);
    const auto full = MakeFullPresence(campaign);

    REQUIRE(
        CampaignStateMachine::EvaluateFullRoster(campaign, {}).Failure ==
        FullRosterFailure::MissingMember);
    REQUIRE(
        CampaignStateMachine::DetermineRuntimeState(campaign, {}) ==
        CampaignRuntimeState::WAITING_FOR_ROSTER);
    REQUIRE(CampaignStateMachine::EvaluateFullRoster(
        campaign, full).Eligible());
    REQUIRE(
        CampaignStateMachine::DetermineRuntimeState(campaign, full) ==
        CampaignRuntimeState::ACTIVE);

    SECTION("transport-only outsider is not an admitted campaign member")
    {
        auto presence = full;
        CampaignMemberPresence outsider = MakePresence(
            campaign.Id, MakeRuntimeSlot(9));
        outsider.CampaignAdmitted = false;
        presence.push_back(outsider);
        REQUIRE(CampaignStateMachine::EvaluateFullRoster(
            campaign, presence).Eligible());
    }
    SECTION("extra admitted member is rejected")
    {
        auto presence = full;
        presence.push_back(MakePresence(
            campaign.Id, MakeRuntimeSlot(9)));
        REQUIRE(
            CampaignStateMachine::EvaluateFullRoster(
                campaign, presence).Failure ==
            FullRosterFailure::ExtraMember);
    }
    SECTION("wrong PlayerId is rejected")
    {
        auto presence = full;
        presence[0].Identity.Player = PlayerId{"replacement-player"};
        REQUIRE(
            CampaignStateMachine::EvaluateFullRoster(
                campaign, presence).Failure ==
            FullRosterFailure::WrongPlayer);
    }
    SECTION("wrong slot is rejected")
    {
        auto presence = full;
        presence[0].Identity.Slot = CampaignSlotId{"wrong-slot"};
        REQUIRE(
            CampaignStateMachine::EvaluateFullRoster(
                campaign, presence).Failure ==
            FullRosterFailure::WrongSlot);
    }
    SECTION("wrong campaign is rejected")
    {
        auto presence = full;
        presence[0].Identity.Campaign = CampaignId{"wrong-campaign"};
        REQUIRE(
            CampaignStateMachine::EvaluateFullRoster(
                campaign, presence).Failure ==
            FullRosterFailure::WrongCampaign);
    }
    SECTION("wrong binding is rejected")
    {
        auto presence = full;
        presence[0].Identity.CharacterBinding =
            CharacterBindingId{"wrong-binding"};
        REQUIRE(
            CampaignStateMachine::EvaluateFullRoster(
                campaign, presence).Failure ==
            FullRosterFailure::WrongCharacterBinding);
    }
    SECTION("duplicate active identity is rejected")
    {
        auto presence = full;
        presence.push_back(full.front());
        REQUIRE(
            CampaignStateMachine::EvaluateFullRoster(
                campaign, presence).Failure ==
            FullRosterFailure::DuplicateActiveIdentity);
    }
    SECTION("admission cannot outlive transport")
    {
        auto presence = full;
        presence[0].TransportConnected = false;
        REQUIRE(
            CampaignStateMachine::EvaluateFullRoster(
                campaign, presence).Failure ==
            FullRosterFailure::AdmittedWithoutTransport);
    }
}

TEST_CASE(
    "Readiness is per sealed slot and future progression is full-roster gated",
    "[campaign.runtime][domain][ready][phase]")
{
    CampaignAggregate campaign = MakeSealedRuntimeCampaign(2);
    const CampaignMemberIdentity member = MakeRuntimeIdentity(1);

    REQUIRE(CampaignStateMachine::SetReady(
        campaign, member, true).Succeeded());
    REQUIRE(campaign.Roster.front().Ready);
    const StateVersion readyVersion = campaign.Version;
    CampaignDomainResult duplicate = CampaignStateMachine::SetReady(
        campaign, member, true);
    REQUIRE(duplicate.Succeeded());
    REQUIRE_FALSE(duplicate.Changed);
    REQUIRE(campaign.Version == readyVersion);
    REQUIRE(CampaignStateMachine::SetReady(
        campaign, member, false).Succeeded());
    REQUIRE_FALSE(campaign.Roster.front().Ready);

    CampaignMemberIdentity outsider = MakeRuntimeIdentity(9);
    REQUIRE(CampaignStateMachine::SetReady(
        campaign, outsider, true).Error ==
        CampaignError::NotCampaignMember);

    const CampaignDomainResult partial =
        CampaignStateMachine::EvaluateTransition(
            campaign,
            CampaignTransition::CompleteCharacterCreation,
            CampaignActor::Server(),
            {});
    REQUIRE(partial.Error == CampaignError::RosterIncomplete);
    REQUIRE(campaign.Phase == CampaignPhase::CharacterCreation);

    const CampaignDomainResult full =
        CampaignStateMachine::EvaluateTransition(
            campaign,
            CampaignTransition::CompleteCharacterCreation,
            CampaignActor::Server(),
            MakeFullPresence(campaign));
    REQUIRE(full.Error == CampaignError::TransitionNotImplemented);
    REQUIRE(campaign.Phase == CampaignPhase::CharacterCreation);
}

TEST_CASE(
    "Exact roster predicate scales across canonical v1 party sizes",
    "[campaign.runtime][domain][scale]")
{
    for (const std::size_t count : {2u, 4u, 10u})
    {
        INFO("roster size=" << count);
        CampaignAggregate campaign;
        campaign.Id = CampaignId{"campaign-runtime"};
        campaign.Version = 1;
        for (std::size_t index = 1; index <= count; ++index)
            campaign.Roster.push_back(MakeRuntimeSlotState(index));
        CampaignStateMachine::SortRoster(campaign.Roster);
        REQUIRE(CampaignStateMachine::CommitCampaignStart(
            campaign,
            CampaignActor::Server(),
            PlayerId{"player-1"}).Succeeded());
        const auto presence = MakeFullPresence(campaign);
        REQUIRE(CampaignStateMachine::EvaluateFullRoster(
            campaign, presence).Eligible());
        REQUIRE(
            CampaignStateMachine::DetermineRuntimeState(
                campaign, presence) == CampaignRuntimeState::ACTIVE);
        for (std::size_t index = 1; index <= count; ++index)
        {
            REQUIRE(CampaignStateMachine::SetReady(
                campaign, MakeRuntimeIdentity(index), true).Succeeded());
        }
        REQUIRE(std::all_of(
            campaign.Roster.begin(),
            campaign.Roster.end(),
            [](const CampaignSlotState& acSlot) { return acSlot.Ready; }));
    }
}
