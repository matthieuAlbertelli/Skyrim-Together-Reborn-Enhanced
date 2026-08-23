#include <CampaignBootstrapState.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;

TEST_CASE("Fresh bootstrap Solo releases exactly once", "[campaign.bootstrap]")
{
    CampaignBootstrapState state;
    REQUIRE_FALSE(state.IsActive());
    state.BeginFreshGame();
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::Entry);
    REQUIRE(state.IsActive());
    REQUIRE(state.ChooseSolo());
    REQUIRE_FALSE(state.ChooseSolo());
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::Authorized);
    REQUIRE_FALSE(state.IsActive());
}

TEST_CASE("Campaign intent cannot release CharacterCreation", "[campaign.bootstrap]")
{
    CampaignBootstrapState state;
    state.BeginFreshGame();
    state.ShowCreateForm();
    state.BeginCreate(false);
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::ConnectingCreate);
    REQUIRE_FALSE(state.ObserveCanonicalState(true, true, true));
    state.OnConnected();
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::CreatingCampaign);
    REQUIRE_FALSE(state.ObserveCanonicalState(true, true, true));
    state.OnCampaignAdmitted();
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::Lobby);
}

TEST_CASE("Only the exact canonical multiplayer state releases once", "[campaign.bootstrap]")
{
    CampaignBootstrapState state;
    state.BeginFreshGame();
    state.ShowJoinForm();
    state.BeginJoin(true);
    state.OnCampaignAdmitted();

    REQUIRE_FALSE(state.ObserveCanonicalState(false, true, true));
    REQUIRE_FALSE(state.ObserveCanonicalState(true, false, true));
    REQUIRE_FALSE(state.ObserveCanonicalState(true, true, false));
    REQUIRE(state.ObserveCanonicalState(true, true, true));
    REQUIRE_FALSE(state.ObserveCanonicalState(true, true, true));
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::Authorized);
}

TEST_CASE("Disconnect fails closed and cannot turn a campaign into Solo", "[campaign.bootstrap]")
{
    CampaignBootstrapState state;
    state.BeginFreshGame();
    state.ShowJoinForm();
    state.BeginJoin(true);
    state.OnCampaignAdmitted();
    REQUIRE(state.IsMultiplayerCommitted());

    state.OnDisconnect();
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::Error);
    REQUIRE_FALSE(state.ChooseSolo());
    state.Back();
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::JoinForm);
    REQUIRE_FALSE(state.ChooseSolo());
}

TEST_CASE("A second New Game rearms a clean bootstrap", "[campaign.bootstrap]")
{
    CampaignBootstrapState state;
    state.BeginFreshGame();
    REQUIRE(state.ChooseSolo());
    state.BeginFreshGame();
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::Entry);
    REQUIRE_FALSE(state.IsMultiplayerCommitted());
    REQUIRE(state.ChooseSolo());
}

TEST_CASE("Inactive state models ordinary saves without opening bootstrap", "[campaign.bootstrap]")
{
    CampaignBootstrapState state;
    state.ShowCreateForm();
    state.ShowJoinForm();
    state.OnDisconnect();
    REQUIRE(state.GetPhase() == CampaignBootstrapPhase::Inactive);
    REQUIRE_FALSE(state.IsActive());
}
