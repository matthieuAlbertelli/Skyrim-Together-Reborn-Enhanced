#include <CampaignNativeLoadState.h>
#include <Campaign/CampaignRuntimeGate.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;

namespace
{
constexpr auto kIdentity =
    "stre-checkpoint-4a33f050b434778db8b09094658831d5";

CampaignNativeLoadRequest ReadyRequest()
{
    CampaignNativeLoadRequest request;
    REQUIRE(request.Request(kIdentity));
    REQUIRE(request.MarkArtifactValidated());
    REQUIRE(request.BeginInvocation());
    return request;
}
}

TEST_CASE("Campaign native load rejects invalid identities", "[campaign.native-load]")
{
    CampaignNativeLoadRequest request;

    REQUIRE_FALSE(request.Request("checkpoint-without-stre-prefix"));
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Idle);
    REQUIRE_FALSE(request.OnNativeLoadEnter(kIdentity));
}

TEST_CASE("Ordinary native loads are never correlated while idle", "[campaign.native-load]")
{
    CampaignNativeLoadRequest request;

    REQUIRE_FALSE(request.OnNativeLoadEnter("Manual Save 1"));
    request.OnNativeLoadReturn(false, true);
    REQUIRE_FALSE(request.OnPostLoad());
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Idle);
}

TEST_CASE("Matching native load requires every safety milestone", "[campaign.native-load]")
{
    auto request = ReadyRequest();

    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    request.OnNativeLoadReturn(true, true);
    REQUIRE(request.Snapshot().State ==
        CampaignNativeLoadState::AwaitingPostLoad);
    REQUIRE(request.OnPostLoad());
    request.ObserveGateLocked();
    request.ObserveGuardMenu(true);
    REQUIRE(request.Snapshot().State ==
        CampaignNativeLoadState::AwaitingSafetyProof);
    request.ObserveTransportAlive();

    const auto& proof = request.Snapshot();
    REQUIRE(proof.State == CampaignNativeLoadState::Completed);
    REQUIRE(proof.ArtifactValidated);
    REQUIRE(proof.NativeEntered);
    REQUIRE(proof.NativeReturned);
    REQUIRE(proof.NativeAccepted);
    REQUIRE(proof.PostLoadObserved);
    REQUIRE(proof.GateLocked);
    REQUIRE(proof.GuardMenuObserved);
    REQUIRE(proof.GamePaused);
    REQUIRE(proof.TransportAlive);
}

TEST_CASE("Unrelated native load fails the outstanding correlation", "[campaign.native-load]")
{
    auto request = ReadyRequest();

    REQUIRE_FALSE(request.OnNativeLoadEnter("Manual Save 1"));
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Failed);
    REQUIRE(request.Snapshot().Failure ==
        CampaignNativeLoadFailure::UnexpectedNativeLoad);
}

TEST_CASE("Native load rejection fails without post-load proof", "[campaign.native-load]")
{
    auto request = ReadyRequest();

    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    request.OnNativeLoadReturn(true, false);
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Failed);
    REQUIRE(request.Snapshot().Failure ==
        CampaignNativeLoadFailure::NativeLoadRejected);
    REQUIRE_FALSE(request.Snapshot().PostLoadObserved);
}

TEST_CASE("Post-load can arrive synchronously before native return", "[campaign.native-load]")
{
    auto request = ReadyRequest();

    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    REQUIRE(request.OnPostLoad());
    request.ObserveGateLocked();
    request.ObserveGuardMenu(true);
    request.ObserveTransportAlive();
    REQUIRE(request.Snapshot().State ==
        CampaignNativeLoadState::AwaitingSafetyProof);

    request.OnNativeLoadReturn(true, true);
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Completed);
}

TEST_CASE("Guard pause and transport are both mandatory", "[campaign.native-load]")
{
    auto request = ReadyRequest();
    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    request.OnNativeLoadReturn(true, true);
    REQUIRE(request.OnPostLoad());
    request.ObserveGateLocked();
    request.ObserveGuardMenu(false);
    request.ObserveTransportAlive();

    REQUIRE(request.Snapshot().State ==
        CampaignNativeLoadState::AwaitingSafetyProof);
    REQUIRE(request.Snapshot().GuardMenuObserved);
    REQUIRE_FALSE(request.Snapshot().GamePaused);
    REQUIRE(request.Snapshot().TransportAlive);
}

TEST_CASE("Duplicate request remains single-flight until explicit reset", "[campaign.native-load]")
{
    auto request = ReadyRequest();
    REQUIRE_FALSE(request.Request(kIdentity));

    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    request.OnNativeLoadReturn(true, true);
    REQUIRE(request.OnPostLoad());
    request.ObserveGateLocked();
    request.ObserveGuardMenu(true);
    request.ObserveTransportAlive();
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Completed);
    REQUIRE_FALSE(request.Request(kIdentity));

    REQUIRE(request.Reset());
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Idle);
    REQUIRE(request.Request(kIdentity));
}

TEST_CASE(
    "Internal recovery correlation is exact one-shot state and never leaks",
    "[campaign.native-load][campaign.load-policy][provenance]")
{
    auto request = ReadyRequest();
    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    request.OnNativeLoadReturn(true, false);
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Failed);

    REQUIRE_FALSE(request.OnNativeLoadEnter(kIdentity));
    REQUIRE_FALSE(request.OnNativeLoadEnter("stre-player-chosen-name"));
    REQUIRE_FALSE(request.OnNativeLoadEnter("Manual Save 1"));
}

TEST_CASE(
    "Pre-opened guard menu surviving native load completes from observable post-load state",
    "[campaign.native-load][campaign.gate][post-load-proof]")
{
    CampaignRuntimeGate gate;
    REQUIRE(gate.LockForRecovery());
    gate.ObserveGuardMenu(true);
    REQUIRE(gate.IsGuardMenuObserved());

    auto request = ReadyRequest();
    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    REQUIRE_FALSE(gate.IsGuardMenuObserved());
    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    gate.OnNativeLoadReturn(true);
    request.OnNativeLoadReturn(true, true);
    REQUIRE(gate.OnPostLoad());
    REQUIRE(request.OnPostLoad());
    request.ObserveGateLocked();

    gate.ObserveGuardMenu(true);
    REQUIRE(request.ObservePostLoadGuardState(true, true) ==
        CampaignNativeLoadFailure::None);
    request.ObserveTransportAlive();
    REQUIRE(request.Snapshot().State ==
        CampaignNativeLoadState::Completed);
    REQUIRE(request.Snapshot().GuardMenuObserved);
    REQUIRE(request.Snapshot().GamePaused);
}

TEST_CASE(
    "Absent guard menu at first post-load world update fails closed",
    "[campaign.native-load][post-load-proof]")
{
    auto request = ReadyRequest();
    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    request.OnNativeLoadReturn(true, true);
    REQUIRE(request.OnPostLoad());
    request.ObserveGateLocked();

    const CampaignNativeLoadFailure failure =
        request.ObservePostLoadGuardState(false, true);
    REQUIRE(failure ==
        CampaignNativeLoadFailure::GuardMenuUnavailable);
    REQUIRE(request.Fail(failure));
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Failed);
    REQUIRE(request.Snapshot().Failure ==
        CampaignNativeLoadFailure::GuardMenuUnavailable);
    REQUIRE_FALSE(request.Snapshot().GuardMenuObserved);
}

TEST_CASE(
    "Open guard menu without a paused game fails closed",
    "[campaign.native-load][post-load-proof]")
{
    auto request = ReadyRequest();
    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    request.OnNativeLoadReturn(true, true);
    REQUIRE(request.OnPostLoad());
    request.ObserveGateLocked();

    const CampaignNativeLoadFailure failure =
        request.ObservePostLoadGuardState(true, false);
    REQUIRE(failure == CampaignNativeLoadFailure::GameNotPaused);
    REQUIRE(request.Fail(failure));
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Failed);
    REQUIRE(request.Snapshot().Failure ==
        CampaignNativeLoadFailure::GameNotPaused);
    REQUIRE(request.Snapshot().GuardMenuObserved);
    REQUIRE_FALSE(request.Snapshot().GamePaused);
}

TEST_CASE(
    "Guard menu PostDisplay remains a valid post-load proof path",
    "[campaign.native-load][post-load-proof][post-display]")
{
    auto request = ReadyRequest();
    REQUIRE(request.OnNativeLoadEnter(kIdentity));
    request.OnNativeLoadReturn(true, true);
    REQUIRE(request.OnPostLoad());
    request.ObserveGateLocked();
    request.ObserveGuardMenu(true);
    request.ObserveTransportAlive();

    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Completed);
    REQUIRE(request.Snapshot().GuardMenuObserved);
    REQUIRE(request.Snapshot().GamePaused);
}
