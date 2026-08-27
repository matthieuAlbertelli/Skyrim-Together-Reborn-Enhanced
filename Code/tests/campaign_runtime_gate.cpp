#include <Campaign/CampaignRuntimeGate.h>
#include <CampaignLoadPolicy.h>
#include <CampaignRecoveryState.h>
#include <CampaignRecoveryUiState.h>
#include <Structs/Campaign.h>

#include <catch2/catch.hpp>

using STRE::Campaign::CampaignRuntimeGate;
using STRE::Campaign::CampaignRuntimeGateState;
using namespace STRE::Campaign;

TEST_CASE("Campaign runtime gate follows explicit load transitions", "[campaign.gate]")
{
    CampaignRuntimeGate gate;

    REQUIRE(gate.GetState() == CampaignRuntimeGateState::Open);
    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.IsNextLoadArmed());
    REQUIRE(gate.OnNativeLoadEnter());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::ArmedDuringLoad);

    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());
    REQUIRE(gate.IsLocked());
    REQUIRE(gate.Release());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::Released);
}

TEST_CASE("Campaign runtime gate arm and release are idempotent", "[campaign.gate]")
{
    CampaignRuntimeGate gate;

    REQUIRE(gate.ArmNextLoad());
    REQUIRE_FALSE(gate.ArmNextLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    REQUIRE_FALSE(gate.OnNativeLoadEnter());
    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());
    REQUIRE_FALSE(gate.OnPostLoad());
    REQUIRE(gate.Release());
    REQUIRE_FALSE(gate.Release());
}

TEST_CASE("Normal and failed loads do not leave the campaign gate locked", "[campaign.gate]")
{
    CampaignRuntimeGate normal;
    REQUIRE_FALSE(normal.OnNativeLoadEnter());
    normal.OnNativeLoadReturn(true);
    REQUIRE_FALSE(normal.OnPostLoad());
    REQUIRE_FALSE(normal.IsLocked());

    CampaignRuntimeGate failed;
    REQUIRE(failed.ArmNextLoad());
    REQUIRE(failed.OnNativeLoadEnter());
    failed.OnNativeLoadReturn(false);
    REQUIRE(failed.GetState() == CampaignRuntimeGateState::Open);
    REQUIRE_FALSE(failed.OnPostLoad());
}

TEST_CASE("Presentation observations cannot release the campaign gate", "[campaign.gate]")
{
    CampaignRuntimeGate gate;
    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());

    gate.ObserveGuardMenu(true);
    gate.ObserveCefPresentation(true);
    REQUIRE(gate.IsLocked());
    REQUIRE(gate.IsGuardMenuObserved());
    REQUIRE(gate.IsCefPresentationObserved());

    gate.ObserveGuardMenu(false);
    gate.ObserveCefPresentation(false);
    REQUIRE(gate.IsLocked());
    REQUIRE_FALSE(gate.IsGuardMenuObserved());
    REQUIRE_FALSE(gate.IsCefPresentationObserved());
}

TEST_CASE("Campaign runtime gate can cancel only an armed managed load", "[campaign.gate]")
{
    CampaignRuntimeGate gate;

    REQUIRE_FALSE(gate.CancelArmedLoad());
    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.CancelArmedLoad());
    REQUIRE_FALSE(gate.IsNextLoadArmed());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::Open);
    REQUIRE_FALSE(gate.OnNativeLoadEnter());

    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    REQUIRE(gate.CancelArmedLoad());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::Open);
    REQUIRE_FALSE(gate.OnPostLoad());
}

TEST_CASE(
    "Campaign recovery gate locks before load and never fails open",
    "[campaign.gate][campaign.recovery]")
{
    CampaignRuntimeGate gate;

    REQUIRE(gate.LockForRecovery());
    REQUIRE(gate.LockForRecovery());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::RecoveryLocked);
    REQUIRE(gate.IsLocked());

    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    REQUIRE(gate.IsLocked());
    gate.OnNativeLoadReturn(false);
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::RecoveryLocked);
    REQUIRE(gate.IsLocked());
    REQUIRE_FALSE(gate.OnPostLoad());

    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.CancelArmedLoad());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::RecoveryLocked);
    REQUIRE(gate.IsLocked());

    REQUIRE(gate.Release());
    REQUIRE_FALSE(gate.IsLocked());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::Released);
}

TEST_CASE(
    "Resume-required lock hands off to correlated recovery without opening",
    "[campaign.gate][campaign.load][campaign.recovery]")
{
    CampaignRuntimeGate gate;
    REQUIRE(gate.ArmResumeRequiredLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    REQUIRE(gate.IsLocked());
    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());
    REQUIRE(gate.IsLocked());
    REQUIRE_FALSE(gate.ArmNextLoad());

    REQUIRE(gate.LockForRecovery());
    REQUIRE(gate.IsLocked());
    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());
    REQUIRE(gate.IsLocked());
    REQUIRE(gate.Release());
}

TEST_CASE(
    "Continue ResumeRequired gate activates only at committed Main Menu transition",
    "[campaign.gate][campaign.load][main-menu][continue]")
{
    CampaignRuntimeGate gate;
    REQUIRE(gate.ArmResumeRequiredLoad());

    // Logical ownership is pending but Main Menu remains the active context.
    REQUIRE_FALSE(gate.IsLocked());
    REQUIRE(gate.IsNextLoadArmed());

    REQUIRE(gate.CommitResumeRequiredTransition());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::LockedAfterLoad);
    REQUIRE(gate.IsLocked());
    REQUIRE_FALSE(gate.IsNextLoadArmed());
    REQUIRE_FALSE(gate.CommitResumeRequiredTransition());
    REQUIRE(gate.Release());

    CampaignRuntimeGate unrelatedClose;
    REQUIRE_FALSE(unrelatedClose.CommitResumeRequiredTransition());
    REQUIRE_FALSE(unrelatedClose.IsLocked());
}

TEST_CASE(
    "Main Menu departure skips only the provisional gate before marked resume",
    "[campaign.gate][campaign.recovery][campaign.load][main-menu]")
{
    CampaignRuntimeGate gate;
    REQUIRE(ProjectRecoveryDisconnectGate(
                CampaignRecoveryDisconnectContext::MainMenuRuntimeDeparture) ==
        CampaignRecoveryLocalGateAction::SkipNoGameplay);
    REQUIRE_FALSE(gate.IsLocked());

    CampaignLoadPolicyContext markedLoad;
    markedLoad.Target = CampaignLoadTarget::Campaign;
    REQUIRE(EvaluateCampaignLoadPolicy(markedLoad) ==
        CampaignLoadDecision::BeginResumeRequired);
    REQUIRE(gate.ArmResumeRequiredLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());
    REQUIRE(gate.IsLocked());

    REQUIRE(gate.LockForRecovery());
    REQUIRE(gate.IsLocked());
    REQUIRE(gate.Release());
    REQUIRE_FALSE(gate.IsLocked());
}

TEST_CASE(
    "Disconnect Main Menu action cannot release recovery gate before semantic entry",
    "[campaign.gate][campaign.recovery][client][ui][main-menu]")
{
    CampaignRuntimeGate gate;
    REQUIRE(gate.LockForRecovery());

    CampaignRecoveryUiState ui;
    REQUIRE(ui.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeActive, true,
        {{true, true}, {true, false}}));
    REQUIRE(ui.ObserveSnapshot(
        "campaign-1", kCampaignWireRuntimeRecoveryLock, true,
        {{true, true}, {false, false}}));
    REQUIRE(ui.RequestMainMenu());
    REQUIRE(gate.IsLocked());

    REQUIRE(ui.BeginMainMenuDispatch());
    REQUIRE_FALSE(ui.BeginMainMenuDispatch());
    REQUIRE(gate.IsLocked());

    // Models the existing CampaignMainMenuEnteredEvent projection: only the
    // semantic world-departure boundary may clear the local presentation.
    ui.Complete();
    REQUIRE(gate.Release());
    REQUIRE_FALSE(gate.IsLocked());
}
