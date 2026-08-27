#include <CampaignRecoveryState.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;

namespace
{
CampaignRecoveryCorrelation Correlation(std::uint64_t aRestoreRevision = 0)
{
    return {
        "campaign-1", "restore-attempt-1", "checkpoint-1", 8,
        aRestoreRevision};
}

TEST_CASE(
    "Recovery disconnect gate projection distinguishes gameplay from Main Menu",
    "[campaign.recovery][client][disconnect][main-menu][gate]")
{
    REQUIRE(ProjectRecoveryDisconnectGate(
                CampaignRecoveryDisconnectContext::GameplayWorld) ==
        CampaignRecoveryLocalGateAction::LockGameplay);
    REQUIRE(ProjectRecoveryDisconnectGate(
                CampaignRecoveryDisconnectContext::MainMenuRuntimeDeparture) ==
        CampaignRecoveryLocalGateAction::SkipNoGameplay);

    CampaignRecoveryState gameplay;
    if (ProjectRecoveryDisconnectGate(
            CampaignRecoveryDisconnectContext::GameplayWorld) ==
        CampaignRecoveryLocalGateAction::LockGameplay)
    {
        REQUIRE(gameplay.LockProvisional("campaign-1"));
    }
    REQUIRE(gameplay.GetStage() ==
        CampaignClientRecoveryStage::RecoveryLocked);

    CampaignRecoveryState mainMenu;
    if (ProjectRecoveryDisconnectGate(
            CampaignRecoveryDisconnectContext::MainMenuRuntimeDeparture) ==
        CampaignRecoveryLocalGateAction::LockGameplay)
    {
        REQUIRE(mainMenu.LockProvisional("campaign-1"));
    }
    REQUIRE(mainMenu.GetStage() == CampaignClientRecoveryStage::Idle);
}

CampaignRecoveryCorrelation ReconnectCorrelation(
    std::uint64_t aRestoreRevision = 0)
{
    return {
        "campaign-1", "restore-attempt-2", "checkpoint-2", 12,
        aRestoreRevision};
}
}

TEST_CASE(
    "Client recovery state enforces both correlated barriers",
    "[campaign.recovery][client]")
{
    CampaignRecoveryState state;
    REQUIRE(state.Lock("campaign-1"));
    REQUIRE(state.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(state.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::Ignore);
    REQUIRE(state.FinishNativeLoad(true));
    REQUIRE(state.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::ResendLoaded);
    REQUIRE(state.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::ApplySnapshot);
    REQUIRE(state.FinishSnapshotApply());
    REQUIRE(state.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::ResendSnapshotApplied);
    REQUIRE(state.ObserveComplete(Correlation(10)) ==
        CampaignRecoveryClientAction::Release);
    REQUIRE(state.GetStage() == CampaignClientRecoveryStage::Completed);
    REQUIRE(state.ObserveComplete(Correlation(10)) ==
        CampaignRecoveryClientAction::Release);
}

TEST_CASE(
    "Client recovery rejects stale correlation and keeps failed loads locked",
    "[campaign.recovery][client]")
{
    CampaignRecoveryState state;
    REQUIRE(state.Lock("campaign-1"));
    REQUIRE(state.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(state.FinishNativeLoad(false));
    REQUIRE(state.GetStage() == CampaignClientRecoveryStage::Failed);
    REQUIRE(state.ObserveComplete(Correlation(10)) ==
        CampaignRecoveryClientAction::Reject);

    auto stale = Correlation();
    stale.RestoreAttemptId = "restore-attempt-stale";
    REQUIRE(state.ObserveLoadRequest(stale) ==
        CampaignRecoveryClientAction::Reject);
    REQUIRE(state.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
}

TEST_CASE(
    "Client crash during snapshot apply replays native load barrier",
    "[campaign.recovery][client][client-crash]")
{
    CampaignRecoveryState survivor;
    REQUIRE(survivor.Lock("campaign-1"));
    REQUIRE(survivor.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(survivor.FinishNativeLoad(true));
    REQUIRE(survivor.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::ApplySnapshot);
    REQUIRE(survivor.FinishSnapshotApply());
    REQUIRE(survivor.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::ResendLoaded);

    CampaignRecoveryState restarted;
    REQUIRE(restarted.Lock("campaign-1"));
    REQUIRE(restarted.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(restarted.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::Reject);
    REQUIRE(restarted.FinishNativeLoad(true));
    REQUIRE(restarted.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::ApplySnapshot);
}

TEST_CASE(
    "Fresh client re-establishes exact correlation before accepting a rehydrated snapshot",
    "[campaign.recovery][client][restart][rehydration]")
{
    CampaignRecoveryState fresh;
    REQUIRE(fresh.Lock("campaign-1"));
    REQUIRE(fresh.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::Reject);
    REQUIRE(fresh.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(fresh.FinishNativeLoad(true));
    REQUIRE(fresh.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::ResendLoaded);
    REQUIRE(fresh.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::ApplySnapshot);
    REQUIRE(fresh.FinishSnapshotApply());
    REQUIRE(fresh.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::ResendSnapshotApplied);
    REQUIRE(fresh.ObserveComplete(Correlation(10)) ==
        CampaignRecoveryClientAction::Release);

    CampaignRecoveryState mismatch;
    REQUIRE(mismatch.Lock("campaign-1"));
    auto wrongAttempt = Correlation();
    wrongAttempt.RestoreAttemptId = "restore-attempt-wrong";
    REQUIRE(mismatch.ObserveLoadRequest(wrongAttempt) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(mismatch.FinishNativeLoad(true));
    REQUIRE(mismatch.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::Reject);
    auto wrongCheckpoint = wrongAttempt;
    wrongCheckpoint.CheckpointId = "checkpoint-wrong";
    wrongCheckpoint.RestoreRevision = 10;
    REQUIRE(mismatch.ObserveSnapshot(wrongCheckpoint) ==
        CampaignRecoveryClientAction::Reject);
    auto wrongCampaign = wrongAttempt;
    wrongCampaign.CampaignId = "campaign-wrong";
    wrongCampaign.RestoreRevision = 10;
    REQUIRE(mismatch.ObserveSnapshot(wrongCampaign) ==
        CampaignRecoveryClientAction::Reject);
}

TEST_CASE(
    "Snapshot applied client resends final acknowledgement after server crash",
    "[campaign.recovery][client][complete-crash]")
{
    CampaignRecoveryState state;
    REQUIRE(state.Lock("campaign-1"));
    REQUIRE(state.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(state.FinishNativeLoad(true));
    REQUIRE(state.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::ApplySnapshot);
    REQUIRE(state.FinishSnapshotApply());
    REQUIRE(state.ObserveAuthoritativeActive("campaign-1") ==
        CampaignRecoveryClientAction::ResendSnapshotApplied);
    REQUIRE(state.ObserveComplete(Correlation(10)) ==
        CampaignRecoveryClientAction::Release);
}

TEST_CASE(
    "Authoritative active releases only an uncorrelated provisional lock",
    "[campaign.recovery][client][server-crash]")
{
    CampaignRecoveryState provisional;
    REQUIRE(provisional.LockProvisional("campaign-1"));
    REQUIRE(provisional.ObserveAuthoritativeActive("campaign-1") ==
        CampaignRecoveryClientAction::Release);
    REQUIRE(provisional.GetStage() == CampaignClientRecoveryStage::Idle);

    CampaignRecoveryState authoritative;
    REQUIRE(authoritative.Lock("campaign-1"));
    REQUIRE(authoritative.ObserveAuthoritativeActive("campaign-1") ==
        CampaignRecoveryClientAction::Ignore);

    CampaignRecoveryState correlated;
    REQUIRE(correlated.LockProvisional("campaign-1"));
    REQUIRE(correlated.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(correlated.ObserveAuthoritativeActive("campaign-1") ==
        CampaignRecoveryClientAction::Ignore);
    REQUIRE(correlated.GetStage() ==
        CampaignClientRecoveryStage::LoadingNativeSave);
}

TEST_CASE(
    "A completed client starts a new correlated recovery after reconnect",
    "[campaign.recovery][client][reconnect][roster-one]")
{
    CampaignRecoveryState state;
    REQUIRE(state.Lock("campaign-1"));
    REQUIRE(state.ObserveLoadRequest(Correlation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);
    REQUIRE(state.FinishNativeLoad(true));
    REQUIRE(state.ObserveSnapshot(Correlation(10)) ==
        CampaignRecoveryClientAction::ApplySnapshot);
    REQUIRE(state.FinishSnapshotApply());
    REQUIRE(state.ObserveComplete(Correlation(10)) ==
        CampaignRecoveryClientAction::Release);

    REQUIRE(state.LockProvisional("campaign-1"));
    REQUIRE(state.GetStage() ==
        CampaignClientRecoveryStage::RecoveryLocked);
    REQUIRE(state.Lock("campaign-1"));
    REQUIRE(state.ObserveLoadRequest(ReconnectCorrelation()) ==
        CampaignRecoveryClientAction::StartNativeLoad);

    auto stale = Correlation();
    REQUIRE(state.ObserveLoadRequest(stale) ==
        CampaignRecoveryClientAction::Reject);
    REQUIRE(state.ObserveAuthoritativeActive("campaign-1") ==
        CampaignRecoveryClientAction::Ignore);
    REQUIRE(state.FinishNativeLoad(true));
    REQUIRE(state.ObserveLoadRequest(ReconnectCorrelation()) ==
        CampaignRecoveryClientAction::ResendLoaded);
    REQUIRE(state.ObserveSnapshot(ReconnectCorrelation(14)) ==
        CampaignRecoveryClientAction::ApplySnapshot);
    REQUIRE(state.FinishSnapshotApply());
    REQUIRE(state.ObserveSnapshot(ReconnectCorrelation(14)) ==
        CampaignRecoveryClientAction::ResendSnapshotApplied);
    REQUIRE(state.GetStage() ==
        CampaignClientRecoveryStage::SnapshotApplied);
    REQUIRE(state.ObserveComplete(ReconnectCorrelation(14)) ==
        CampaignRecoveryClientAction::Release);
    REQUIRE(state.GetStage() == CampaignClientRecoveryStage::Completed);
}
