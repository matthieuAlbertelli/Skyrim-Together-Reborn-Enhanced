#include <CampaignIdentityStore.h>
#include <CampaignNativeLoadBridge.h>
#include <CampaignNativeLoadState.h>

#include <catch2/catch.hpp>

#include <algorithm>

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

TEST_CASE("Campaign native load CEF functions are registered by their shared manifest", "[campaign.native-load][cef]")
{
    REQUIRE(kCampaignNativeLoadFunction ==
        std::string_view{"campaignNativeLoad"});
    REQUIRE(kCampaignNativeLoadReleaseFunction ==
        std::string_view{"campaignNativeLoadRelease"});
    REQUIRE(kCampaignNativeLoadResumeFunction ==
        std::string_view{"campaignNativeLoadResume"});
    REQUIRE(kCampaignNativeLoadCefFunctions.size() == 3);
    REQUIRE(std::find(
                kCampaignNativeLoadCefFunctions.begin(),
                kCampaignNativeLoadCefFunctions.end(),
                kCampaignNativeLoadFunction) !=
        kCampaignNativeLoadCefFunctions.end());
    REQUIRE(std::find(
                kCampaignNativeLoadCefFunctions.begin(),
                kCampaignNativeLoadCefFunctions.end(),
                kCampaignNativeLoadReleaseFunction) !=
        kCampaignNativeLoadCefFunctions.end());
    REQUIRE(std::find(
                kCampaignNativeLoadCefFunctions.begin(),
                kCampaignNativeLoadCefFunctions.end(),
                kCampaignNativeLoadResumeFunction) !=
        kCampaignNativeLoadCefFunctions.end());
}

TEST_CASE("Campaign native load rejects invalid identities", "[campaign.native-load]")
{
    CampaignNativeLoadRequest request;

    REQUIRE_FALSE(request.Request("checkpoint-without-stre-prefix"));
    REQUIRE(request.Snapshot().State == CampaignNativeLoadState::Idle);
    REQUIRE_FALSE(request.OnNativeLoadEnter(kIdentity));
}

TEST_CASE("Cold-session resume rejects malformed campaign identifiers", "[campaign.native-load][cef]")
{
    REQUIRE(CampaignIdentityStore::IsValidCacheId(
        "campaign-367760f49cba23fd72a5ad5013a75e1b"));
    REQUIRE_FALSE(CampaignIdentityStore::IsValidCacheId(""));
    REQUIRE_FALSE(CampaignIdentityStore::IsValidCacheId("../campaign"));
    REQUIRE_FALSE(CampaignIdentityStore::IsValidCacheId("campaign with spaces"));
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
