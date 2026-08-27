#include <TiltedOnlinePCH.h>

#include <CampaignNativeSave.h>
#include <CampaignNativeSaveBoundary.h>
#include <CampaignNativeSaveCompletion.h>
#include <CampaignSaveTrace.h>
#include <CampaignSavePolicy.h>

#include <Services/CampaignCheckpointService.h>

#include <Structs/Campaign.h>

#include <array>
#include <optional>
#include <type_traits>

struct BGSSaveLoadManager;

using namespace STRE::Campaign;

using TCampaignNativeSave =
    CampaignNativeSaveFunction<BGSSaveLoadManager>;
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
// Diagnostic call depth only. It never stores or transmits a save intent.
thread_local std::uint32_t s_processBoundaryDepth{};

constexpr std::size_t kMaximumObservedNativeSaveNameLength = 260;

struct ObservedNativeSaveName
{
    std::array<char, kMaximumObservedNativeSaveNameLength + 1> Buffer{};
    std::size_t Length{};
    bool PointerPresent{};
    bool Readable{};

    [[nodiscard]] std::string_view View() const noexcept
    {
        return Readable
            ? std::string_view(Buffer.data(), Length)
            : std::string_view{};
    }
};

// Save_Impl is an engine boundary. A non-null register value is not sufficient
// evidence that dereferencing it is safe, especially while diagnosing an ABI
// mismatch. Copy a bounded, terminated value under SEH before classification or
// logging; unreadable and unterminated inputs remain Unknown.
ObservedNativeSaveName ObserveNativeSaveName(const char* apSaveName) noexcept
{
    ObservedNativeSaveName observed;
    observed.PointerPresent = apSaveName != nullptr;
    if (!apSaveName)
        return observed;

    __try
    {
        for (std::size_t i = 0;
             i <= kMaximumObservedNativeSaveNameLength;
             ++i)
        {
            const char value = apSaveName[i];
            if (value == '\0')
            {
                observed.Length = i;
                observed.Readable = true;
                return observed;
            }
            if (i == kMaximumObservedNativeSaveNameLength)
                return observed;
            observed.Buffer[i] = value;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        observed.Length = 0;
        observed.Readable = false;
    }
    return observed;
}

const char* GetCampaignSaveOriginName(CampaignSaveOrigin aOrigin) noexcept
{
    switch (aOrigin)
    {
    case CampaignSaveOrigin::Manual:
        return "Manual";
    case CampaignSaveOrigin::Quick:
        return "Quick";
    case CampaignSaveOrigin::Auto:
        return "Auto";
    case CampaignSaveOrigin::Unknown:
        return "Unknown";
    }
    return "Invalid";
}

void LogNativeSaveAttempt(
    const CampaignNativeSaveArguments& acArguments,
    const ObservedNativeSaveName& acName,
    CampaignSaveOrigin aOrigin,
    CampaignSaveDecision aDecision,
    bool aInternal) noexcept
{
    const bool allowed =
        aDecision == CampaignSaveDecision::AllowVanilla ||
        aDecision == CampaignSaveDecision::AllowInternalCheckpoint;
    if (acName.Readable)
    {
        spdlog::info(
            "[STRE][CampaignSavePolicy] NATIVE_SAVE_{} deviceId={} "
            "outputStats=0x{:08X} fileNamePointer=present "
            "fileName=\"{}\" classification={} decision={} internal={}",
            allowed ? "ALLOWED" : "BLOCKED",
            acArguments.DeviceId,
            acArguments.OutputStats,
            acName.View(),
            GetCampaignSaveOriginName(aOrigin),
            static_cast<unsigned>(aDecision),
            aInternal);
        return;
    }

    spdlog::info(
        "[STRE][CampaignSavePolicy] NATIVE_SAVE_{} deviceId={} "
        "outputStats=0x{:08X} fileNamePointer={} fileNameReadable=false "
        "classification={} decision={} internal={}",
        allowed ? "ALLOWED" : "BLOCKED",
        acArguments.DeviceId,
        acArguments.OutputStats,
        acName.PointerPresent ? "present" : "null",
        GetCampaignSaveOriginName(aOrigin),
        static_cast<unsigned>(aDecision),
        aInternal);
}

void TraceNativeSaveAttempt(
    const CampaignNativeSaveArguments& acArguments,
    const ObservedNativeSaveName& acName,
    CampaignSaveOrigin aOrigin,
    bool aInternal) noexcept
{
    const CampaignSaveTrace::Context context = CampaignSaveTrace::Capture();
    if (acName.Readable)
    {
        spdlog::info(
            "[STRE][CampaignSaveTrace] sequence={} "
            "source=BGSSaveLoadManager.Save_Impl event=Enter frame={} "
            "thread={} deviceId={} outputStats=0x{:08X} "
            "fileNamePointer=present fileName=\"{}\" classification={} "
            "internal={} processBoundaryDepth={}",
            context.Sequence,
            context.Frame,
            context.Thread,
            acArguments.DeviceId,
            acArguments.OutputStats,
            acName.View(),
            GetCampaignSaveOriginName(aOrigin),
            aInternal,
            s_processBoundaryDepth);
        return;
    }

    spdlog::info(
        "[STRE][CampaignSaveTrace] sequence={} "
        "source=BGSSaveLoadManager.Save_Impl event=Enter frame={} thread={} "
        "deviceId={} outputStats=0x{:08X} fileNamePointer={} "
        "fileNameReadable=false classification={} internal={} "
        "processBoundaryDepth={}",
        context.Sequence,
        context.Frame,
        context.Thread,
        acArguments.DeviceId,
        acArguments.OutputStats,
        acName.PointerPresent ? "present" : "null",
        GetCampaignSaveOriginName(aOrigin),
        aInternal,
        s_processBoundaryDepth);
}

class ScopedProcessBoundaryTrace final
{
public:
    ScopedProcessBoundaryTrace() noexcept
    {
        ++s_processBoundaryDepth;
    }

    ~ScopedProcessBoundaryTrace() noexcept
    {
        --s_processBoundaryDepth;
    }

    TP_NOCOPYMOVE(ScopedProcessBoundaryTrace);
};

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

void InvokeOriginalProcessBoundary(BGSSaveLoadManager* apManager)
{
    const ScopedCampaignQuickSaveProcessBoundary quickSaveBoundary;
    TiltedPhoques::ThisCall(s_realProcessBoundary, apManager);
}
}

bool __fastcall HookCampaignNativeSave(
    BGSSaveLoadManager* apThis,
    std::int32_t aDeviceId,
    std::uint32_t aOutputStats,
    const char* apSaveName)
{
    const CampaignNativeSaveArguments arguments{
        aDeviceId, aOutputStats, apSaveName};
    const ObservedNativeSaveName observedName =
        ObserveNativeSaveName(arguments.FileName);
    const bool internal =
        ScopedCampaignCheckpointNativeSave::IsActive();
    CampaignSaveDecision decision = internal
        ? CampaignSaveDecision::AllowInternalCheckpoint
        : CampaignSaveDecision::AllowVanilla;
    CampaignSaveOrigin origin =
        ClassifyCampaignNativeSaveName(observedName.View());
    if (!internal)
    {
        if (const auto explicitOrigin = CampaignSaveProvenance::Consume())
            origin = *explicitOrigin;
        if (CampaignCheckpointService* const pCheckpoint =
                CampaignCheckpointService::TryGet())
        {
            decision = pCheckpoint->HandleNativeSaveAttempt(origin);
        }
    }

    TraceNativeSaveAttempt(arguments, observedName, origin, internal);
    LogNativeSaveAttempt(
        arguments, observedName, origin, decision, internal);

    if (decision == CampaignSaveDecision::AllowVanilla ||
        decision == CampaignSaveDecision::AllowInternalCheckpoint)
    {
        return InvokeCampaignNativeSave(
            s_nativeSave, apThis, arguments);
    }

    return false;
}

static_assert(std::is_same_v<
    decltype(&HookCampaignNativeSave),
    TCampaignNativeSave*>);

void TP_MAKE_THISCALL(
    HookCampaignNativeSaveProcessBoundary,
    BGSSaveLoadManager)
{
    const ScopedProcessBoundaryTrace processBoundaryTrace;
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
        InvokeOriginalProcessBoundary(apThis);
        return;
    }

    spdlog::info(
        "[STRE][CampaignNativeSave] PROCESS_BOUNDARY_ENTER "
        "nativeSaveIdentity={} thread_id={}",
        *requestedIdentity,
        GetCurrentThreadId());

    InvokeOriginalProcessBoundary(apThis);

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
    bool nativeResult{};
    {
        const ScopedCampaignCheckpointNativeSave internalCheckpoint;
        nativeResult = HookCampaignNativeSave(
            apThis,
            std::int32_t{2},
            std::uint32_t{0},
            processingIdentity->c_str());
    }
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

CampaignNativeSaveRequestResult
CampaignNativeSave::ValidateExistingOnGameThread(
    std::string_view acNativeSaveIdentity,
    const STRE::Campaign::NativeSaveBundleArtifact& acExpectedArtifact) noexcept
{
    try
    {
        constexpr std::size_t kMaximumNativeSaveIdentityLength =
            kCampaignNativeSaveIdentityPrefix.size() +
            kCampaignWireMaximumIdLength;
        if (acNativeSaveIdentity.empty() ||
            acNativeSaveIdentity.size() > kMaximumNativeSaveIdentityLength)
        {
            return {CampaignNativeSaveRequestState::InvalidIdentity};
        }
        TiltedPhoques::String nativeSaveIdentity;
        nativeSaveIdentity.assign(
            acNativeSaveIdentity.data(), acNativeSaveIdentity.size());
        const auto parsed = STRE::Campaign::ParseNativeSaveBundleArtifact(
            acNativeSaveIdentity,
            acExpectedArtifact.Fingerprint,
            acExpectedArtifact.Metadata);
        if (!IsValidCampaignNativeSaveIdentity(nativeSaveIdentity) ||
            acExpectedArtifact.Bundle.LogicalIdentity !=
                acNativeSaveIdentity ||
            !parsed || parsed.Value != acExpectedArtifact)
        {
            return {CampaignNativeSaveRequestState::InvalidIdentity};
        }
        if (!CampaignNativeSaveCompletion::IsAvailable())
            return {CampaignNativeSaveRequestState::BoundaryUnavailable};

        std::string requestedIdentity(
            nativeSaveIdentity.data(), nativeSaveIdentity.size());
        if (!s_requestSlot.TryRequest(requestedIdentity))
            return {CampaignNativeSaveRequestState::RequestAlreadyActive};
        const auto processingIdentity = s_requestSlot.BeginProcessing();
        if (!processingIdentity)
        {
            MarkFailed(requestedIdentity, "replay-processing-state-mismatch");
            return {CampaignNativeSaveRequestState::InternalFailure};
        }
        CampaignNativeSaveCompletionPaths paths;
        std::string failureReason;
        if (!CampaignNativeSaveCompletion::PrepareExisting(
                *processingIdentity, paths, failureReason))
        {
            MarkFailed(*processingIdentity, failureReason);
            return {CampaignNativeSaveRequestState::InternalFailure};
        }
        if (!s_requestSlot.BeginAwaitingCompletion())
        {
            MarkFailed(
                *processingIdentity,
                "replay-awaiting-completion-state-mismatch");
            return {CampaignNativeSaveRequestState::InternalFailure};
        }
        if (!CampaignNativeSaveCompletion::Start(
                *processingIdentity,
                std::move(paths),
                s_requestSlot,
                acExpectedArtifact))
        {
            MarkFailed(*processingIdentity, "completion-worker-busy");
            return {CampaignNativeSaveRequestState::InternalFailure};
        }
        spdlog::info(
            "[STRE][CampaignNativeSave] REPLAY_VALIDATION_ACCEPTED "
            "nativeSaveIdentity={} thread_id={}",
            nativeSaveIdentity.c_str(),
            GetCurrentThreadId());
        return {CampaignNativeSaveRequestState::Accepted};
    }
    catch (...)
    {
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

        TP_HOOK(&s_nativeSave, HookCampaignNativeSave);
        TP_HOOK(&s_realProcessBoundary, HookCampaignNativeSaveProcessBoundary);
        s_boundaryAvailable = true;
        spdlog::info(
            "[STRE][CampaignNativeSave] BOUNDARY_CONFIGURED "
            "process_relocation=35772 save_relocation=35727");
    });
