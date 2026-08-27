#include <CampaignRecoveryUiState.h>
#include <Structs/Campaign.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;

namespace
{
std::vector<CampaignRecoveryUiRosterMember> Roster(
    std::initializer_list<CampaignRecoveryUiRosterMember> aMembers)
{
    return {aMembers};
}
}

TEST_CASE(
    "ACTIVE remote disconnect opens a derived incident before existing recovery UI",
    "[campaign.recovery][client][ui][disconnect]")
{
    CampaignRecoveryUiState state;
    REQUIRE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeActive, true,
        Roster({{true, true}, {true, false}})));
    REQUIRE(state.GetMode() == CampaignRecoveryUiMode::Hidden);

    REQUIRE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeRecoveryLock, true,
        Roster({{true, true}, {false, false}})));
    REQUIRE(state.GetMode() ==
        CampaignRecoveryUiMode::DisconnectIncident);
    REQUIRE(state.GetCampaignId() == "campaign-1");
    REQUIRE(state.GetMissingRemoteCount() == 1);
    REQUIRE(state.GetIncidentKind() ==
        CampaignRecoveryIncidentKind::RemotePlayerMissing);

    REQUIRE(state.StayAndRecover());
    REQUIRE(state.GetMode() == CampaignRecoveryUiMode::ExistingRecovery);
    REQUIRE_FALSE(state.StayAndRecover());
    REQUIRE_FALSE(state.RequestMainMenu());
}

TEST_CASE(
    "Disconnect incident roster projection follows current authoritative presence",
    "[campaign.recovery][client][ui][roster]")
{
    CampaignRecoveryUiState state;
    REQUIRE(state.ObserveSnapshot(
        "campaign-many", kCampaignWireRuntimeActive, true,
        Roster({
            {true, true}, {true, false}, {true, false}, {true, false}})));
    REQUIRE(state.ObserveSnapshot(
        "campaign-many", kCampaignWireRuntimeRecoveryLock, true,
        Roster({
            {true, true}, {false, false}, {false, false}, {true, false}})));
    REQUIRE(state.GetMissingRemoteCount() == 2);
    REQUIRE(state.GetIncidentKind() ==
        CampaignRecoveryIncidentKind::MultiplePlayersMissing);

    REQUIRE(state.ObserveSnapshot(
        "campaign-many", kCampaignWireRuntimeRecoveryLock, true,
        Roster({
            {true, true}, {true, false}, {false, false}, {true, false}})));
    REQUIRE(state.GetMissingRemoteCount() == 1);
    REQUIRE(state.GetIncidentKind() ==
        CampaignRecoveryIncidentKind::RemotePlayerMissing);

    REQUIRE(state.ObserveSnapshot(
        "campaign-many", kCampaignWireRuntimeRecoveryLock, true,
        Roster({
            {true, true}, {true, false}, {true, false}, {true, false}})));
    REQUIRE(state.GetMissingRemoteCount() == 0);
    REQUIRE(state.GetIncidentKind() ==
        CampaignRecoveryIncidentKind::RosterRestored);
    REQUIRE(state.GetMode() ==
        CampaignRecoveryUiMode::DisconnectIncident);
}

TEST_CASE(
    "Local transport loss uses local semantics for N one and remains fail closed",
    "[campaign.recovery][client][ui][disconnect][roster-one]")
{
    CampaignRecoveryUiState state;
    REQUIRE(state.ObserveSnapshot(
        "campaign-solo", kCampaignWireRuntimeActive, true,
        Roster({{true, true}})));
    REQUIRE(state.OpenLocalTransportLoss());
    REQUIRE(state.GetMode() ==
        CampaignRecoveryUiMode::DisconnectIncident);
    REQUIRE(state.GetIncidentKind() ==
        CampaignRecoveryIncidentKind::LocalTransportLost);
    REQUIRE(state.GetMissingRemoteCount() == 0);
    REQUIRE(state.StayAndRecover());
    REQUIRE(state.GetMode() == CampaignRecoveryUiMode::ExistingRecovery);
}

TEST_CASE(
    "Return to Main Menu is a bounded presentation action",
    "[campaign.recovery][client][ui][main-menu][idempotency]")
{
    CampaignRecoveryUiState state;
    REQUIRE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeActive, true,
        Roster({{true, true}, {true, false}})));
    REQUIRE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeRecoveryLock, true,
        Roster({{true, true}, {false, false}})));
    REQUIRE(state.RequestMainMenu());
    REQUIRE(state.IsMainMenuRequested());
    REQUIRE_FALSE(state.RequestMainMenu());
    REQUIRE_FALSE(state.StayAndRecover());
    REQUIRE(state.GetMode() ==
        CampaignRecoveryUiMode::DisconnectIncident);

    // The CEF action only records intent. Native dispatch begins after that
    // callback has returned, and can be consumed exactly once.
    REQUIRE(state.BeginMainMenuDispatch());
    REQUIRE_FALSE(state.BeginMainMenuDispatch());
    REQUIRE(state.IsMainMenuRequested());
    REQUIRE(state.GetMode() ==
        CampaignRecoveryUiMode::DisconnectIncident);

    state.CancelMainMenuRequest();
    REQUIRE_FALSE(state.IsMainMenuRequested());
    REQUIRE(state.RequestMainMenu());
    REQUIRE(state.BeginMainMenuDispatch());
    state.Complete();
    REQUIRE(state.GetMode() == CampaignRecoveryUiMode::Hidden);
    REQUIRE(state.GetCampaignId().empty());
    REQUIRE(state.GetRoster().empty());
    REQUIRE_FALSE(state.BeginMainMenuDispatch());
}

TEST_CASE(
    "Cold admission into an existing recovery does not fabricate a disconnect incident",
    "[campaign.recovery][client][ui][resume][security]")
{
    CampaignRecoveryUiState state;
    REQUIRE_FALSE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeRecoveryLock, true,
        Roster({{true, true}, {false, false}})));
    REQUIRE(state.GetMode() == CampaignRecoveryUiMode::Hidden);
    REQUIRE_FALSE(state.OpenLocalTransportLoss());

    REQUIRE_FALSE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeActive, false,
        Roster({{true, true}, {true, false}})));
    REQUIRE(state.GetMode() == CampaignRecoveryUiMode::Hidden);
}

TEST_CASE(
    "Roster completion never bypasses an already authoritative recovery",
    "[campaign.recovery][client][ui][authority]")
{
    CampaignRecoveryUiState state;
    REQUIRE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeActive, true,
        Roster({{true, true}, {true, false}})));
    REQUIRE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeRecoveryLock, true,
        Roster({{true, true}, {false, false}})));
    REQUIRE(state.StayAndRecover());
    REQUIRE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeRecoveryLock, true,
        Roster({{true, true}, {true, false}})));
    REQUIRE(state.GetMode() == CampaignRecoveryUiMode::ExistingRecovery);
    REQUIRE(state.GetIncidentKind() ==
        CampaignRecoveryIncidentKind::RosterRestored);

    REQUIRE(state.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeRestoringCheckpoint, true,
        Roster({{true, true}, {true, false}})));
    REQUIRE(state.GetMode() == CampaignRecoveryUiMode::ExistingRecovery);
}
