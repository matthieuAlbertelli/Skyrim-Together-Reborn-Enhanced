#include <TiltedOnlinePCH.h>

#include <CampaignNativeSave.h>
#include <CampaignNativeSaveCompletion.h>

#include <Structs/Campaign.h>

#include <optional>

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

void LogProcessBoundaryExit(const std::string& acIdentity) noexcept
{
    spdlog::info(
        "[STRE][CampaignNativeSave] PROCESS_BOUNDARY_EXIT "
        "nativeSaveIdentity={} thread_id={}",
        acIdentity,
        GetCurrentThreadId());
}

void MarkFailed(
    const std::string& acIdentity,
    const std::string& acReason) noexcept
{
    try
    {
        (void)s_requestSlot.Fail(acReason);
    }
    catch (...)
    {
    }
    spdlog::error(
        "[STRE][CampaignNativeSave] REQUEST_FAILED "
        "nativeSaveIdentity={} reason={} thread_id={}",
        acIdentity,
        acReason,
        GetCurrentThreadId());
}
}

void TP_MAKE_THISCALL(
    HookCampaignNativeSaveProcessBoundary,
    BGSSaveLoadManager)
{
    std::optional<std::string> requestedIdentity;
    try
    {
        requestedIdentity = s_requestSlot.RequestedIdentity();
    }
    catch (...)
    {
        spdlog::error(
            "[STRE][CampaignNativeSave] PROCESS_BOUNDARY_STATE_READ_FAILED "
            "thread_id={}",
            GetCurrentThreadId());
    }
    if (!requestedIdentity)
    {
        TiltedPhoques::ThisCall(s_realProcessBoundary, apThis);
        return;
    }

    spdlog::info(
        "[STRE][CampaignNativeSave] PROCESS_BOUNDARY_ENTER "
        "nativeSaveIdentity={} thread_id={}",
        *requestedIdentity,
        GetCurrentThreadId());

    TiltedPhoques::ThisCall(s_realProcessBoundary, apThis);

    std::optional<std::string> processingIdentity;
    try
    {
        processingIdentity = s_requestSlot.BeginProcessing();
    }
    catch (...)
    {
    }
    if (!processingIdentity)
    {
        spdlog::error(
            "[STRE][CampaignNativeSave] PROCESS_BOUNDARY_EXIT "
            "reason=request-state-changed thread_id={}",
            GetCurrentThreadId());
        return;
    }

    CampaignNativeSaveCompletionPaths paths;
    std::string failureReason;
    try
    {
        if (!CampaignNativeSaveCompletion::PrepareFresh(
                *processingIdentity, paths, failureReason))
        {
            LogProcessBoundaryExit(*processingIdentity);
            MarkFailed(*processingIdentity, failureReason);
            return;
        }
    }
    catch (...)
    {
        LogProcessBoundaryExit(*processingIdentity);
        MarkFailed(*processingIdentity, "save-path-resolution-failed");
        return;
    }

    spdlog::info(
        "[STRE][CampaignNativeSave] SAVE_CALL_ENTER "
        "nativeSaveIdentity={} thread_id={}",
        *processingIdentity,
        GetCurrentThreadId());
    const bool nativeResult = TiltedPhoques::ThisCall(
        s_nativeSave,
        apThis,
        std::int32_t{2},
        std::uint32_t{0},
        processingIdentity->c_str());
    spdlog::info(
        "[STRE][CampaignNativeSave] SAVE_CALL_RETURN "
        "nativeSaveIdentity={} native_return={} completion=unproven "
        "thread_id={}",
        *processingIdentity,
        nativeResult,
        GetCurrentThreadId());

    if (!nativeResult)
    {
        LogProcessBoundaryExit(*processingIdentity);
        MarkFailed(*processingIdentity, "native-save-returned-false");
        return;
    }
    if (!s_requestSlot.BeginAwaitingCompletion())
    {
        LogProcessBoundaryExit(*processingIdentity);
        MarkFailed(*processingIdentity, "awaiting-completion-state-mismatch");
        return;
    }

    spdlog::info(
        "[STRE][CampaignNativeSave] SAVE_COMPLETION_AWAITING "
        "nativeSaveIdentity={} deadline_ms={} thread_id={}",
        *processingIdentity,
        CampaignNativeSaveCompletion::kDeadlineMilliseconds,
        GetCurrentThreadId());
    LogProcessBoundaryExit(*processingIdentity);

    try
    {
        if (!CampaignNativeSaveCompletion::Start(
                *processingIdentity, std::move(paths), s_requestSlot))
        {
            MarkFailed(*processingIdentity, "completion-worker-busy");
        }
    }
    catch (...)
    {
        MarkFailed(*processingIdentity, "completion-worker-start-failed");
    }
}

CampaignNativeSaveRequestResult CampaignNativeSave::RequestOnGameThread(
    std::string_view acNativeSaveIdentity) noexcept
{
    try
    {
        constexpr std::size_t kMaximumNativeSaveIdentityLength =
            kCampaignNativeSaveIdentityPrefix.size() +
            kCampaignWireMaximumIdLength;
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

        if (!s_boundaryAvailable ||
            !CampaignNativeSaveCompletion::IsAvailable())
        {
            spdlog::error(
                "[STRE][CampaignNativeSave] REQUEST_REJECTED "
                "reason=boundary-unavailable nativeSaveIdentity={} "
                "thread_id={}",
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
    catch (...)
    {
        spdlog::error(
            "[STRE][CampaignNativeSave] REQUEST_REJECTED "
            "reason=internal-failure thread_id={}",
            GetCurrentThreadId());
        return {CampaignNativeSaveRequestState::InternalFailure};
    }
}

CampaignNativeSaveLifecycleSnapshot CampaignNativeSave::GetStatus()
{
    return s_requestSlot.Snapshot();
}

static TiltedPhoques::Initializer s_campaignNativeSaveHook(
    []()
    {
        // AE 1.6.1170 Address Library IDs for the process boundary and
        // BGSSaveLoadManager::Save_Impl.
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
