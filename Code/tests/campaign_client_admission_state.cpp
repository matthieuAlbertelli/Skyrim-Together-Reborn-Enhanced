#include <CampaignClientAdmissionState.h>
#include <CampaignBootstrapState.h>
#include <CampaignLoadPolicy.h>

#include <catch2/catch.hpp>

#include <utility>

using namespace STRE::Campaign;

namespace
{
CampaignClientAdmission Admission(std::string aCampaign = "campaign-a")
{
    return {std::move(aCampaign), "slot-01", "binding-01"};
}
}

TEST_CASE(
    "Create Start ACTIVE retains client admission for Helgen readiness",
    "[campaign.client][campaign.admission][helgen]")
{
    CampaignBootstrapState bootstrap;
    CampaignClientAdmissionState state;

    bootstrap.BeginFreshGame();
    bootstrap.ShowCreateForm();
    bootstrap.BeginCreate(true);
    state.Accept(Admission());
    bootstrap.OnCampaignAdmitted();
    REQUIRE(bootstrap.GetPhase() == CampaignBootstrapPhase::Lobby);

    state.ObserveSnapshot("campaign-a", true, true, 1, 1);
    REQUIRE(bootstrap.ObserveCanonicalState(true, true, true));

    REQUIRE(state.GetAdmission() == Admission());
    const auto readiness = state.GetHelgenReadinessView();
    REQUIRE(readiness.Admission == Admission());
    REQUIRE(readiness.CanSignal);
}

TEST_CASE(
    "Transport without campaign admission stays fail closed",
    "[campaign.client][campaign.admission][security]")
{
    CampaignClientAdmissionState state;
    state.ObserveSnapshot("campaign-a", true, true, 1, 1);

    REQUIRE_FALSE(state.GetAdmission());
    REQUIRE_FALSE(state.BeginResume());
    const auto readiness = state.GetHelgenReadinessView();
    REQUIRE_FALSE(readiness.Admission);
    REQUIRE_FALSE(readiness.CanSignal);
}

TEST_CASE(
    "Disconnect clears volatile admission and exact resume restores it",
    "[campaign.client][campaign.admission][reconnect]")
{
    CampaignClientAdmissionState state;
    state.Accept(Admission());
    state.ObserveSnapshot("campaign-a", true, true, 1, 1);

    REQUIRE(state.Disconnect() == "campaign-a");
    REQUIRE_FALSE(state.GetAdmission());
    REQUIRE_FALSE(state.GetHelgenReadinessView().CanSignal);

    REQUIRE(state.BeginResume() == "campaign-a");
    REQUIRE_FALSE(state.BeginResume());
    REQUIRE_FALSE(state.GetAdmission());

    state.Accept(Admission());
    state.ObserveSnapshot("campaign-a", true, true, 1, 1);
    REQUIRE(state.GetAdmission() == Admission());
    REQUIRE(state.GetHelgenReadinessView().CanSignal);
}

TEST_CASE(
    "Main Menu runtime departure clears admission without becoming a reconnect",
    "[campaign.client][campaign.admission][main-menu][lifecycle]")
{
    CampaignClientAdmissionState state;
    state.Accept(Admission());
    state.ObserveSnapshot("campaign-a", true, true, 1, 1);

    REQUIRE(state.EndRuntimeSession() == "campaign-a");
    REQUIRE_FALSE(state.GetAdmission());
    REQUIRE_FALSE(state.GetHelgenReadinessView().CanSignal);
    REQUIRE_FALSE(state.BeginResume());
    REQUIRE_FALSE(state.EndRuntimeSession());
    REQUIRE_FALSE(state.Disconnect());

    CampaignLoadPolicyContext markedMainMenuLoad;
    markedMainMenuLoad.Target = CampaignLoadTarget::Campaign;
    markedMainMenuLoad.CampaignRuntimeSensitive =
        state.GetAdmission().has_value();
    REQUIRE(EvaluateCampaignLoadPolicy(markedMainMenuLoad) ==
        CampaignLoadDecision::BeginResumeRequired);
}

TEST_CASE(
    "Rejected resume never fabricates a local campaign admission",
    "[campaign.client][campaign.admission][reconnect][security]")
{
    CampaignClientAdmissionState state;
    state.Accept(Admission());
    REQUIRE(state.Disconnect() == "campaign-a");
    REQUIRE(state.BeginResume() == "campaign-a");

    state.ResumeRejected();
    REQUIRE_FALSE(state.GetAdmission());
    REQUIRE(state.BeginResume() == "campaign-a");
    REQUIRE_FALSE(state.GetHelgenReadinessView().CanSignal);
}

TEST_CASE(
    "Cold-session resume remains unadmitted until a server response is accepted",
    "[campaign.client][campaign.admission][reconnect][security]")
{
    CampaignClientAdmissionState state;

    REQUIRE_FALSE(state.GetAdmission());
    REQUIRE_FALSE(state.BeginResume());
    state.ResumeRejected();
    REQUIRE_FALSE(state.GetAdmission());

    state.Accept(Admission());
    REQUIRE(state.GetAdmission() == Admission());
}

TEST_CASE(
    "Helgen readiness requires the complete ACTIVE admitted roster",
    "[campaign.client][campaign.admission][helgen]")
{
    CampaignClientAdmissionState state;
    state.Accept(Admission());

    state.ObserveSnapshot("campaign-a", false, true, 2, 2);
    REQUIRE_FALSE(state.GetHelgenReadinessView().CanSignal);
    state.ObserveSnapshot("campaign-a", true, false, 2, 2);
    REQUIRE_FALSE(state.GetHelgenReadinessView().CanSignal);
    state.ObserveSnapshot("campaign-a", true, true, 2, 1);
    REQUIRE_FALSE(state.GetHelgenReadinessView().CanSignal);
    state.ObserveSnapshot("campaign-b", true, true, 2, 2);
    REQUIRE_FALSE(state.GetHelgenReadinessView().CanSignal);
    state.ObserveSnapshot("campaign-a", true, true, 2, 2);
    REQUIRE(state.GetHelgenReadinessView().CanSignal);
}

TEST_CASE(
    "Leaving clears both admission and reconnect candidate",
    "[campaign.client][campaign.admission]")
{
    CampaignClientAdmissionState state;
    state.Accept(Admission());
    state.Leave("campaign-a");

    REQUIRE_FALSE(state.GetAdmission());
    REQUIRE_FALSE(state.BeginResume());
}

TEST_CASE(
    "Explicit Leave remains distinct from a Main Menu runtime departure",
    "[campaign.client][campaign.admission][main-menu][lifecycle]")
{
    CampaignClientAdmissionState departure;
    departure.Accept(Admission());
    REQUIRE(departure.EndRuntimeSession() == "campaign-a");
    REQUIRE_FALSE(departure.BeginResume());

    CampaignClientAdmissionState leave;
    leave.Accept(Admission());
    leave.Leave("campaign-a");
    REQUIRE_FALSE(leave.GetAdmission());
    REQUIRE_FALSE(leave.BeginResume());
}
