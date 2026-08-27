#include <CampaignSavePolicy.h>
#include <CampaignNativeSaveBoundary.h>

#include <catch2/catch.hpp>

#include <thread>
#include <type_traits>

using namespace STRE::Campaign;

namespace
{
struct NativeSaveManagerProbe
{
};

struct NativeSaveCallProbe
{
    NativeSaveManagerProbe* Self{};
    std::int32_t DeviceId{};
    std::uint32_t OutputStats{};
    const char* FileName{};
    bool Called{};
};

NativeSaveCallProbe s_nativeSaveCallProbe;

bool CaptureNativeSaveCall(
    NativeSaveManagerProbe* apSelf,
    std::int32_t aDeviceId,
    std::uint32_t aOutputStats,
    const char* apFileName)
{
    s_nativeSaveCallProbe = {
        apSelf, aDeviceId, aOutputStats, apFileName, true};
    return true;
}

static_assert(std::is_same_v<
    decltype(&CaptureNativeSaveCall),
    CampaignNativeSaveFunction<NativeSaveManagerProbe>*>);
}

TEST_CASE(
    "Native Save Impl boundary preserves device stats and filename parameter order",
    "[campaign.save-policy][native-boundary]")
{
    NativeSaveManagerProbe manager;
    constexpr std::int32_t deviceId = -37;
    constexpr std::uint32_t outputStats = 0xA1B2C3D4;
    const char* const pFileName = "QuickSave-order-regression";
    const CampaignNativeSaveArguments arguments{
        deviceId, outputStats, pFileName};
    s_nativeSaveCallProbe = {};

    REQUIRE(InvokeCampaignNativeSave(
        &CaptureNativeSaveCall, &manager, arguments));
    REQUIRE(s_nativeSaveCallProbe.Called);
    REQUIRE(s_nativeSaveCallProbe.Self == &manager);
    REQUIRE(s_nativeSaveCallProbe.DeviceId == deviceId);
    REQUIRE(s_nativeSaveCallProbe.OutputStats == outputStats);
    REQUIRE(s_nativeSaveCallProbe.FileName == pFileName);
}

TEST_CASE(
    "Native save names classify the Skyrim manual quick auto and unknown families",
    "[campaign.save-policy][classification]")
{
    REQUIRE(ClassifyCampaignNativeSaveName("Save 12 - Dragonborn") ==
        CampaignSaveOrigin::Manual);
    REQUIRE(ClassifyCampaignNativeSaveName("sAvE42") ==
        CampaignSaveOrigin::Manual);
    REQUIRE(ClassifyCampaignNativeSaveName("QuickSave0") ==
        CampaignSaveOrigin::Quick);
    REQUIRE(ClassifyCampaignNativeSaveName("QUICKSAVE1") ==
        CampaignSaveOrigin::Quick);
    REQUIRE(ClassifyCampaignNativeSaveName("AutoSave3") ==
        CampaignSaveOrigin::Auto);
    REQUIRE(ClassifyCampaignNativeSaveName("AUTOSAVE - Travel") ==
        CampaignSaveOrigin::Auto);
    REQUIRE(ClassifyCampaignNativeSaveName("") ==
        CampaignSaveOrigin::Unknown);
    REQUIRE(ClassifyCampaignNativeSaveName("stre-checkpoint-user-choice") ==
        CampaignSaveOrigin::Unknown);
    REQUIRE(ClassifyCampaignNativeSaveName("modded-save-family") ==
        CampaignSaveOrigin::Unknown);
}

TEST_CASE(
    "Vanilla saves remain available outside campaigns",
    "[campaign.save-policy][outside]")
{
    CampaignSavePolicyContext context;
    for (const CampaignSaveOrigin origin : {
             CampaignSaveOrigin::Manual,
             CampaignSaveOrigin::Quick,
             CampaignSaveOrigin::Auto,
             CampaignSaveOrigin::Unknown})
    {
        REQUIRE(EvaluateCampaignSavePolicy(origin, context) ==
            CampaignSaveDecision::AllowVanilla);
    }
}

TEST_CASE(
    "Campaign manual and quick saves request one collective checkpoint only while active",
    "[campaign.save-policy][checkpoint]")
{
    CampaignSavePolicyContext context;
    context.InCampaign = true;
    context.RuntimeState = CampaignSaveRuntimeState::Active;
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Manual, context) ==
        CampaignSaveDecision::RequestCollectiveCheckpoint);
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Quick, context) ==
        CampaignSaveDecision::RequestCollectiveCheckpoint);

    context.RuntimeState = CampaignSaveRuntimeState::Checkpointing;
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Manual, context) ==
        CampaignSaveDecision::CoalesceWithCheckpoint);
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Quick, context) ==
        CampaignSaveDecision::CoalesceWithCheckpoint);
}

TEST_CASE(
    "Campaign autosaves unknown saves and fenced runtime states fail closed",
    "[campaign.save-policy][fail-closed]")
{
    CampaignSavePolicyContext context;
    context.InCampaign = true;
    context.RuntimeState = CampaignSaveRuntimeState::Active;
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Auto, context) ==
        CampaignSaveDecision::BlockAutosave);
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Unknown, context) ==
        CampaignSaveDecision::BlockUnknown);

    for (const CampaignSaveRuntimeState state : {
             CampaignSaveRuntimeState::Unavailable,
             CampaignSaveRuntimeState::WaitingForRoster,
             CampaignSaveRuntimeState::RecoveryLock,
             CampaignSaveRuntimeState::RestoringCheckpoint})
    {
        context.RuntimeState = state;
        REQUIRE(EvaluateCampaignSavePolicy(
                    CampaignSaveOrigin::Manual, context) ==
            CampaignSaveDecision::BlockUnavailable);
    }

    context.RuntimeState = CampaignSaveRuntimeState::Active;
    context.RuntimeFenced = true;
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Quick, context) ==
        CampaignSaveDecision::BlockUnavailable);

    // A cold-session managed save has no admitted snapshot yet; its existing
    // resume-required gate is the authoritative local fence.
    context.RuntimeState = CampaignSaveRuntimeState::Unavailable;
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Manual, context) ==
        CampaignSaveDecision::BlockUnavailable);
}

TEST_CASE(
    "Only scoped internal checkpoint provenance bypasses campaign interception",
    "[campaign.save-policy][recursion]")
{
    CampaignSavePolicyContext context;
    context.InCampaign = true;
    context.RuntimeState = CampaignSaveRuntimeState::Checkpointing;

    REQUIRE_FALSE(ScopedCampaignCheckpointNativeSave::IsActive());
    REQUIRE(ClassifyCampaignNativeSaveName("stre-checkpoint-1") ==
        CampaignSaveOrigin::Unknown);
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Unknown, context) ==
        CampaignSaveDecision::BlockUnknown);

    {
        const ScopedCampaignCheckpointNativeSave outer;
        REQUIRE(ScopedCampaignCheckpointNativeSave::IsActive());
        context.InternalCheckpoint =
            ScopedCampaignCheckpointNativeSave::IsActive();
        REQUIRE(EvaluateCampaignSavePolicy(
                    CampaignSaveOrigin::Unknown, context) ==
            CampaignSaveDecision::AllowInternalCheckpoint);
        {
            const ScopedCampaignCheckpointNativeSave inner;
            REQUIRE(ScopedCampaignCheckpointNativeSave::IsActive());
        }
        REQUIRE(ScopedCampaignCheckpointNativeSave::IsActive());
    }
    REQUIRE_FALSE(ScopedCampaignCheckpointNativeSave::IsActive());
}

TEST_CASE(
    "Quick save provenance follows only the exact actionable native request",
    "[campaign.save-policy][provenance][quick]")
{
    int correlatedRequest{};
    int unrelatedRequest{};
    {
        const ScopedCampaignQuickSaveAction actionablePress(true);
        CampaignSaveProvenance::ObserveQuickRequestPush(
            &correlatedRequest,
            CampaignSaveProvenance::QuickRequestOperationCode,
            true);
    }

    CampaignSaveProvenance::ObserveQuickRequestPop(
        &unrelatedRequest,
        CampaignSaveProvenance::QuickRequestOperationCode);
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());

    CampaignSaveProvenance::ObserveQuickRequestPop(
        &correlatedRequest,
        CampaignSaveProvenance::QuickRequestOperationCode);
    REQUIRE(CampaignSaveProvenance::Consume() == CampaignSaveOrigin::Quick);
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());
}

TEST_CASE(
    "Quick request tag crosses threads only through its exact pointer",
    "[campaign.save-policy][provenance][quick]")
{
    int request{};
    std::thread producer(
        [&request]()
        {
            const ScopedCampaignQuickSaveAction actionablePress(true);
            CampaignSaveProvenance::ObserveQuickRequestPush(
                &request,
                CampaignSaveProvenance::QuickRequestOperationCode,
                true);
        });
    producer.join();

    CampaignSaveProvenance::ObserveQuickRequestPop(
        &request,
        CampaignSaveProvenance::QuickRequestOperationCode);
    REQUIRE(CampaignSaveProvenance::Consume() == CampaignSaveOrigin::Quick);
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());
}

TEST_CASE(
    "Non-actionable duplicate Quick input never tags a native request",
    "[campaign.save-policy][provenance][quick]")
{
    int nonActionableRequest{};
    {
        const ScopedCampaignQuickSaveAction nonActionable(false);
        CampaignSaveProvenance::ObserveQuickRequestPush(
            &nonActionableRequest,
            CampaignSaveProvenance::QuickRequestOperationCode,
            true);
    }
    CampaignSaveProvenance::ObserveQuickRequestPop(
        &nonActionableRequest,
        CampaignSaveProvenance::QuickRequestOperationCode);
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());

    int duplicateRequest{};
    {
        const ScopedCampaignQuickSaveAction actionablePress(true);
        CampaignSaveProvenance::ObserveQuickRequestPush(
            &duplicateRequest,
            CampaignSaveProvenance::QuickRequestOperationCode,
            true);
    }
    {
        const ScopedCampaignQuickSaveAction repeatedDispatch(false);
    }
    CampaignSaveProvenance::ObserveQuickRequestPop(
        &duplicateRequest,
        CampaignSaveProvenance::QuickRequestOperationCode);
    REQUIRE(CampaignSaveProvenance::Consume() == CampaignSaveOrigin::Quick);

    int failedRequest{};
    {
        const ScopedCampaignQuickSaveAction actionablePress(true);
        CampaignSaveProvenance::ObserveQuickRequestPush(
            &failedRequest,
            CampaignSaveProvenance::QuickRequestOperationCode,
            false);
    }
    CampaignSaveProvenance::ObserveQuickRequestPop(
        &failedRequest,
        CampaignSaveProvenance::QuickRequestOperationCode);
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());
}

TEST_CASE(
    "Quick request correlation survives requeue and cleans dropped operations",
    "[campaign.save-policy][provenance][quick]")
{
    int requeuedRequest{};
    {
        const ScopedCampaignQuickSaveAction actionablePress(true);
        CampaignSaveProvenance::ObserveQuickRequestPush(
            &requeuedRequest,
            CampaignSaveProvenance::QuickRequestOperationCode,
            true);
    }
    {
        const ScopedCampaignQuickSaveProcessBoundary processBoundary;
        CampaignSaveProvenance::ObserveQuickRequestPop(
            &requeuedRequest,
            CampaignSaveProvenance::QuickRequestOperationCode);
        CampaignSaveProvenance::ObserveQuickRequestPush(
            &requeuedRequest,
            CampaignSaveProvenance::QuickRequestOperationCode,
            true);
    }
    CampaignSaveProvenance::ObserveQuickRequestPop(
        &requeuedRequest,
        CampaignSaveProvenance::QuickRequestOperationCode);
    REQUIRE(CampaignSaveProvenance::Consume() == CampaignSaveOrigin::Quick);

    int droppedRequest{};
    {
        const ScopedCampaignQuickSaveAction actionablePress(true);
        CampaignSaveProvenance::ObserveQuickRequestPush(
            &droppedRequest,
            CampaignSaveProvenance::QuickRequestOperationCode,
            true);
    }
    {
        const ScopedCampaignQuickSaveProcessBoundary processBoundary;
        CampaignSaveProvenance::ObserveQuickRequestPop(
            &droppedRequest,
            CampaignSaveProvenance::QuickRequestOperationCode);
    }
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());

    // Address reuse after cleanup must not inherit the previous tag.
    CampaignSaveProvenance::ObserveQuickRequestPop(
        &droppedRequest,
        CampaignSaveProvenance::QuickRequestOperationCode);
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());
}

TEST_CASE(
    "Manual NewSlot provenance is scoped one-shot and never leaks",
    "[campaign.save-policy][provenance][manual]")
{
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());
    {
        const ScopedCampaignManualNewSlotSave manualNewSlotSave;
        REQUIRE(CampaignSaveProvenance::Consume() ==
            CampaignSaveOrigin::Manual);
        REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());
    }
    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());
}

TEST_CASE(
    "Unproven and stre-prefixed saves remain Unknown and blocked",
    "[campaign.save-policy][provenance][fail-closed]")
{
    CampaignSavePolicyContext context;
    context.InCampaign = true;
    context.RuntimeState = CampaignSaveRuntimeState::Active;

    REQUIRE_FALSE(CampaignSaveProvenance::Consume().has_value());
    REQUIRE(EvaluateCampaignSavePolicy(
                CampaignSaveOrigin::Unknown, context) ==
        CampaignSaveDecision::BlockUnknown);
    REQUIRE(ClassifyCampaignNativeSaveName("stre-user-controlled") ==
        CampaignSaveOrigin::Unknown);
    REQUIRE(EvaluateCampaignSavePolicy(
                ClassifyCampaignNativeSaveName("stre-user-controlled"),
                context) == CampaignSaveDecision::BlockUnknown);
}
