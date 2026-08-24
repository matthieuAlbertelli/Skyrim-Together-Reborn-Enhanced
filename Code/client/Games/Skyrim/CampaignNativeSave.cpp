#include <TiltedOnlinePCH.h>

#include <CampaignNativeSave.h>

#include <Structs/Campaign.h>

struct BGSSaveLoadManager;

TP_THIS_FUNCTION(
    TCampaignNativeSave,
    bool,
    BGSSaveLoadManager,
    std::int32_t,
    std::uint32_t,
    const char*);
TP_THIS_FUNCTION(
    TCampaignNativeSaveProcessBoundary,
    void,
    BGSSaveLoadManager);

namespace
{
TCampaignNativeSave* s_nativeSave{};
TCampaignNativeSaveProcessBoundary* s_realProcessBoundary{};
CampaignNativeSaveDetail::RequestSlot s_requestSlot;
bool s_boundaryAvailable{};
}

void TP_MAKE_THISCALL(
    HookCampaignNativeSaveProcessBoundary,
    BGSSaveLoadManager)
{
    const std::string* const pRequestedIdentity =
        s_requestSlot.RequestedIdentity();
    if (!pRequestedIdentity)
    {
        TiltedPhoques::ThisCall(s_realProcessBoundary, apThis);
        return;
    }

    spdlog::info(
        "[STRE][CampaignNativeSave] PROCESS_BOUNDARY_ENTER "
        "nativeSaveIdentity={} thread_id={}",
        *pRequestedIdentity,
        GetCurrentThreadId());

    TiltedPhoques::ThisCall(s_realProcessBoundary, apThis);

    const std::string* const pProcessingIdentity =
        s_requestSlot.BeginProcessing();
    if (!pProcessingIdentity)
    {
        spdlog::error(
            "[STRE][CampaignNativeSave] PROCESS_BOUNDARY_EXIT "
            "reason=request-state-changed thread_id={}",
            GetCurrentThreadId());
        return;
    }

    spdlog::info(
        "[STRE][CampaignNativeSave] SAVE_CALL_ENTER "
        "nativeSaveIdentity={} thread_id={}",
        *pProcessingIdentity,
        GetCurrentThreadId());
    const bool nativeResult = TiltedPhoques::ThisCall(
        s_nativeSave,
        apThis,
        std::int32_t{2},
        std::uint32_t{0},
        pProcessingIdentity->c_str());
    spdlog::info(
        "[STRE][CampaignNativeSave] SAVE_CALL_RETURN "
        "nativeSaveIdentity={} native_return={} completion=unproven "
        "thread_id={}",
        *pProcessingIdentity,
        nativeResult,
        GetCurrentThreadId());
    spdlog::info(
        "[STRE][CampaignNativeSave] PROCESS_BOUNDARY_EXIT "
        "nativeSaveIdentity={} thread_id={}",
        *pProcessingIdentity,
        GetCurrentThreadId());
    s_requestSlot.FinishProcessing();
}

CampaignNativeSaveRequestResult CampaignNativeSave::RequestOnGameThread(
    std::string_view acNativeSaveIdentity) noexcept
{
    constexpr std::size_t kMaximumNativeSaveIdentityLength =
        kCampaignNativeSaveIdentityPrefix.size() + kCampaignWireMaximumIdLength;
    if (acNativeSaveIdentity.empty() ||
        acNativeSaveIdentity.size() > kMaximumNativeSaveIdentityLength)
    {
        spdlog::error(
            "[STRE][CampaignNativeSave] REQUEST_REJECTED "
            "reason=invalid-identity thread_id={}",
            GetCurrentThreadId());
        return {CampaignNativeSaveRequestState::InvalidIdentity};
    }

    TiltedPhoques::String nativeSaveIdentity;
    nativeSaveIdentity.assign(
        acNativeSaveIdentity.data(), acNativeSaveIdentity.size());
    if (!IsValidCampaignNativeSaveIdentity(nativeSaveIdentity))
    {
        spdlog::error(
            "[STRE][CampaignNativeSave] REQUEST_REJECTED "
            "reason=invalid-identity thread_id={}",
            GetCurrentThreadId());
        return {CampaignNativeSaveRequestState::InvalidIdentity};
    }

    if (!s_boundaryAvailable)
    {
        spdlog::error(
            "[STRE][CampaignNativeSave] REQUEST_REJECTED "
            "reason=boundary-unavailable nativeSaveIdentity={} thread_id={}",
            nativeSaveIdentity.c_str(),
            GetCurrentThreadId());
        return {CampaignNativeSaveRequestState::BoundaryUnavailable};
    }

    std::string requestedIdentity(
        nativeSaveIdentity.data(), nativeSaveIdentity.size());
    if (!s_requestSlot.TryRequest(std::move(requestedIdentity)))
    {
        spdlog::warn(
            "[STRE][CampaignNativeSave] REQUEST_REJECTED "
            "reason=request-active nativeSaveIdentity={} thread_id={}",
            nativeSaveIdentity.c_str(),
            GetCurrentThreadId());
        return {CampaignNativeSaveRequestState::RequestAlreadyActive};
    }

    spdlog::info(
        "[STRE][CampaignNativeSave] REQUEST_ACCEPTED "
        "nativeSaveIdentity={} thread_id={}",
        nativeSaveIdentity.c_str(),
        GetCurrentThreadId());
    return {CampaignNativeSaveRequestState::Accepted};
}

static TiltedPhoques::Initializer s_campaignNativeSaveHook(
    []()
    {
        // Address Library AE IDs established for Skyrim 1.6.1170. ID 35772 is
        // the BGSSaveLoadManager process function used by SKSE's RequestSave
        // path; ID 35727 is BGSSaveLoadManager::Save_Impl.
        POINTER_SKYRIMSE(
            TCampaignNativeSaveProcessBoundary,
            processBoundary,
            35772);
        POINTER_SKYRIMSE(TCampaignNativeSave, nativeSave, 35727);

        s_realProcessBoundary = processBoundary.Get();
        s_nativeSave = nativeSave.Get();
        if (!s_realProcessBoundary || !s_nativeSave)
        {
            spdlog::error(
                "[STRE][CampaignNativeSave] BOUNDARY_UNAVAILABLE "
                "process_boundary_resolved={} save_resolved={}",
                s_realProcessBoundary != nullptr,
                s_nativeSave != nullptr);
            return;
        }

        TP_HOOK(
            &s_realProcessBoundary,
            HookCampaignNativeSaveProcessBoundary);
        s_boundaryAvailable = true;
        spdlog::info(
            "[STRE][CampaignNativeSave] BOUNDARY_CONFIGURED "
            "process_relocation=35772 save_relocation=35727");
    });
