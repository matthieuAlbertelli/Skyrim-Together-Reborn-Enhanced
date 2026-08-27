#include <TiltedOnlinePCH.h>

#include <CampaignLoadPolicy.h>
#include <CampaignNativeLoad.h>
#include <CampaignSaveTrace.h>
#include <Misc/BSFixedString.h>
#include <Services/CampaignNativeLoadService.h>
#include <Services/CampaignResumeService.h>
#include <Services/CampaignRuntimeGateService.h>

#include <cstring>
#include <optional>

struct BGSSaveLoadManager;

TP_THIS_FUNCTION(
    TCampaignGateLoadGame,
    bool,
    BGSSaveLoadManager,
    const char*,
    std::int32_t,
    std::uint32_t,
    bool);

static TCampaignGateLoadGame* s_realLoadGame{};
static BGSSaveLoadManager** s_ppSaveLoadManager{};

namespace
{
constexpr std::size_t kMaximumObservedSaveName = 260;
constexpr std::ptrdiff_t kAeSaveGameListOffset = 0x100;

TP_THIS_FUNCTION(
    TLoadMostRecentSaveGame,
    bool,
    BGSSaveLoadManager);
TLoadMostRecentSaveGame* s_realLoadMostRecentSaveGame{};

TP_THIS_FUNCTION(
    TFxDelegateCallback,
    void,
    void,
    void*,
    const char*,
    const void*,
    std::uint32_t);
TFxDelegateCallback* s_realFxDelegateCallback{};

struct NativeArrayView
{
    void* pData;
    std::uint32_t Capacity;
    std::uint32_t Pad0C;
    std::uint32_t Size;
    std::uint32_t Pad14;
};
static_assert(sizeof(NativeArrayView) == 0x18);

struct NativeSaveLoadFileEntryView
{
    BSFixedString FileName;
};

struct NativeTargetObservation
{
    const char* pTarget{};
    std::string Target;
    bool PointerPresent{};
    bool Readable{};
};

struct NativeStringProbe
{
    std::size_t Length{};
    bool Readable{};
};

struct MostRecentPointerProbe
{
    const char* pTarget{};
};

// Public CommonLibSSE-NG GFxValue ABI. FxDelegate::Callback receives the
// GameDelegate response ID in args[0]; user callback payload starts at args[1].
struct NativeGfxValue
{
    void* pObjectInterface;
    std::uint32_t Type;
    std::uint32_t Pad0C;
    union
    {
        double Number;
        bool Boolean;
        const char* pString;
        const char** ppManagedString;
        const wchar_t* pWideString;
        void* pObject;
    } Value;
};
static_assert(sizeof(NativeGfxValue) == 0x18);

struct GfxValueObservation
{
    std::uint32_t RawType{};
    const char* pTypeName{"Unavailable"};
    std::string Value{"unavailable"};
    bool Readable{};
};

struct GfxValueProbe
{
    std::uint32_t RawType{};
    std::uint32_t Type{};
    double Number{};
    bool Boolean{};
    const char* pString{};
    const wchar_t* pWideString{};
    void* pObject{};
    bool Readable{};
};

thread_local std::optional<std::string> s_semanticLoadTarget;

class ScopedSemanticLoadTarget final
{
public:
    explicit ScopedSemanticLoadTarget(
        const NativeTargetObservation& acTarget)
        : m_previous(std::move(s_semanticLoadTarget))
    {
        if (acTarget.Readable)
            s_semanticLoadTarget = acTarget.Target;
        else
            s_semanticLoadTarget.reset();
    }

    ~ScopedSemanticLoadTarget() noexcept
    {
        s_semanticLoadTarget = std::move(m_previous);
    }

private:
    std::optional<std::string> m_previous;
};

NativeStringProbe ProbeNativeString(const char* apTarget) noexcept
{
    if (!apTarget)
        return {};

    __try
    {
        const std::size_t length = strnlen_s(
            apTarget, kMaximumObservedSaveName);
        if (length < kMaximumObservedSaveName)
            return {length, true};
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return {};
    }
    return {};
}

NativeTargetObservation ObserveTarget(const char* apTarget) noexcept
{
    NativeTargetObservation observation;
    observation.pTarget = apTarget;
    observation.PointerPresent = apTarget != nullptr;
    const NativeStringProbe probe = ProbeNativeString(apTarget);
    if (probe.Readable)
    {
        observation.Target.assign(apTarget, probe.Length);
        observation.Readable = true;
    }
    return observation;
}

GfxValueProbe ProbeGfxValue(
    const void* apArguments,
    std::uint32_t aArgumentCount) noexcept
{
    GfxValueProbe probe;
    if (!apArguments || aArgumentCount == 0)
        return probe;

    __try
    {
        const auto* const pValue =
            static_cast<const NativeGfxValue*>(apArguments);
        probe.RawType = pValue->Type;
        probe.Type = pValue->Type & 0x0F;
        probe.Number = pValue->Value.Number;
        probe.Boolean = pValue->Value.Boolean;
        probe.pString = (pValue->Type & 0x40) != 0
            ? pValue->Value.ppManagedString
                ? *pValue->Value.ppManagedString
                : nullptr
            : pValue->Value.pString;
        probe.pWideString = pValue->Value.pWideString;
        probe.pObject = pValue->Value.pObject;
        probe.Readable = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        probe.Readable = false;
    }
    return probe;
}

GfxValueObservation ObserveGfxValue(
    const void* apArguments,
    std::uint32_t aArgumentCount) noexcept
{
    GfxValueObservation observation;
    const GfxValueProbe probe = ProbeGfxValue(
        apArguments, aArgumentCount);
    observation.RawType = probe.RawType;
    observation.Readable = probe.Readable;
    if (!probe.Readable)
        return observation;

    switch (probe.Type)
    {
        case 0:
            observation.pTypeName = "Undefined";
            observation.Value = "undefined";
            break;
        case 1:
            observation.pTypeName = "Null";
            observation.Value = "null";
            break;
        case 2:
            observation.pTypeName = "Boolean";
            observation.Value = probe.Boolean ? "true" : "false";
            break;
        case 3:
            observation.pTypeName = "Number";
            observation.Value = fmt::format("{}", probe.Number);
            break;
        case 4:
        {
            observation.pTypeName = "String";
            const NativeTargetObservation string =
                ObserveTarget(probe.pString);
            observation.Value = string.Readable
                ? string.Target
                : "unavailable";
            break;
        }
        case 5:
            observation.pTypeName = "StringW";
            observation.Value = probe.pWideString
                ? "present"
                : "null";
            break;
        case 6:
            observation.pTypeName = "Object";
            observation.Value = probe.pObject
                ? "present"
                : "null";
            break;
        case 7:
            observation.pTypeName = "Array";
            observation.Value = probe.pObject
                ? "present"
                : "null";
            break;
        case 8:
            observation.pTypeName = "DisplayObject";
            observation.Value = probe.pObject
                ? "present"
                : "null";
            break;
        default:
            observation.pTypeName = "Unknown";
            break;
    }
    return observation;
}

class ScopedContinueCallbackTrace final
{
public:
    ScopedContinueCallbackTrace() noexcept
    {
        CampaignSaveTrace::EnterContinueCallback();
    }

    ~ScopedContinueCallbackTrace() noexcept
    {
        CampaignSaveTrace::ExitContinueCallback();
    }

    TP_NOCOPYMOVE(ScopedContinueCallbackTrace);
};

MostRecentPointerProbe ProbeMostRecentTargetPointer(
    const BGSSaveLoadManager* apManager) noexcept
{
    if (!apManager)
        return {};

    __try
    {
        const auto* const pBytes =
            reinterpret_cast<const std::byte*>(apManager);
        const auto* const pList = reinterpret_cast<const NativeArrayView*>(
            pBytes + kAeSaveGameListOffset);
        if (!pList->pData || pList->Size == 0 ||
            pList->Size > pList->Capacity || pList->Capacity > 10000)
        {
            return {};
        }
        auto* const* const ppEntries =
            static_cast<NativeSaveLoadFileEntryView* const*>(pList->pData);
        const NativeSaveLoadFileEntryView* const pEntry = ppEntries[0];
        return {pEntry ? pEntry->FileName.AsAscii() : nullptr};
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return {};
    }
}

NativeTargetObservation ObserveMostRecentTarget(
    const BGSSaveLoadManager* apManager) noexcept
{
    return ObserveTarget(
        ProbeMostRecentTargetPointer(apManager).pTarget);
}

void TP_MAKE_THISCALL(
    HookFxDelegateCallback,
    void,
    void* apMovieView,
    const char* apMethodName,
    const void* apArguments,
    std::uint32_t aArgumentCount)
{
    const NativeTargetObservation method = ObserveTarget(apMethodName);
    const bool isContinue = method.Readable &&
        method.Target == "ContinueLastSavedGame";
    if (!isContinue)
    {
        TiltedPhoques::ThisCall(
            s_realFxDelegateCallback,
            apThis,
            apMovieView,
            apMethodName,
            apArguments,
            aArgumentCount);
        return;
    }

    const GfxValueObservation response = ObserveGfxValue(
        apArguments, aArgumentCount);
    const std::uint32_t payloadCount = aArgumentCount > 0
        ? aArgumentCount - 1
        : 0;
    const CampaignSaveTrace::Context enter = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=MainMenu.Continue event=Enter frame={} thread={} "
        "method={} argumentCount={} payloadCount={} responseReadable={} "
        "responseRawType=0x{:08X} responseType={} responseValue={} "
        "canonicalTarget=unavailable-at-callback policy=NotEvaluated",
        enter.Sequence,
        enter.Frame,
        enter.Thread,
        method.Target,
        aArgumentCount,
        payloadCount,
        response.Readable,
        response.RawType,
        response.pTypeName,
        response.Value);

    {
        const ScopedContinueCallbackTrace callbackTrace;
        TiltedPhoques::ThisCall(
            s_realFxDelegateCallback,
            apThis,
            apMovieView,
            apMethodName,
            apArguments,
            aArgumentCount);
    }

    const CampaignSaveTrace::Context exit = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=MainMenu.Continue event=Exit frame={} thread={}",
        exit.Sequence,
        exit.Frame,
        exit.Thread);
}
static_assert(std::is_same_v<
    decltype(&HookFxDelegateCallback),
    TFxDelegateCallback*>);

MostRecentPointerProbe ProbeSaveListTargetPointer(
    const BGSSaveLoadManager* apManager,
    std::uint32_t aSelectionIndex) noexcept
{
    if (!apManager)
        return {};

    __try
    {
        const auto* const pBytes =
            reinterpret_cast<const std::byte*>(apManager);
        const auto* const pList = reinterpret_cast<const NativeArrayView*>(
            pBytes + kAeSaveGameListOffset);
        if (!pList->pData || pList->Size == 0 ||
            pList->Size > pList->Capacity || pList->Capacity > 10000 ||
            aSelectionIndex >= pList->Size)
        {
            return {};
        }

        auto* const* const ppEntries =
            static_cast<NativeSaveLoadFileEntryView* const*>(pList->pData);
        const std::uint32_t nativeIndex =
            pList->Size - aSelectionIndex - 1;
        const NativeSaveLoadFileEntryView* const pEntry =
            ppEntries[nativeIndex];
        return {pEntry ? pEntry->FileName.AsAscii() : nullptr};
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return {};
    }
}

NativeTargetObservation ObserveSaveListTarget(
    const BGSSaveLoadManager* apManager,
    std::uint32_t aSelectionIndex) noexcept
{
    return ObserveTarget(
        ProbeSaveListTargetPointer(apManager, aSelectionIndex).pTarget);
}

const char* TargetName(STRE::Campaign::CampaignLoadTarget aTarget) noexcept
{
    using STRE::Campaign::CampaignLoadTarget;
    switch (aTarget)
    {
    case CampaignLoadTarget::Ordinary: return "Ordinary";
    case CampaignLoadTarget::Campaign: return "Campaign";
    case CampaignLoadTarget::Unknown: return "Unknown";
    }
    return "Unknown";
}

const char* DecisionName(
    STRE::Campaign::CampaignLoadDecision aDecision) noexcept
{
    using STRE::Campaign::CampaignLoadDecision;
    switch (aDecision)
    {
    case CampaignLoadDecision::AllowVanilla: return "AllowVanilla";
    case CampaignLoadDecision::AllowInternalRecovery:
        return "AllowInternalRecovery";
    case CampaignLoadDecision::BeginResumeRequired:
        return "BeginResumeRequired";
    case CampaignLoadDecision::BlockPlayerLoad: return "BlockPlayerLoad";
    case CampaignLoadDecision::BlockUnprovenCampaignTarget:
        return "BlockUnprovenCampaignTarget";
    }
    return "Unknown";
}

bool TP_MAKE_THISCALL(
    HookLoadMostRecentSaveGame,
    BGSSaveLoadManager)
{
    const NativeTargetObservation target = ObserveMostRecentTarget(apThis);
    const CampaignSaveTrace::Context enter = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=BGSSaveLoadManager.LoadMostRecentSaveGame event=Enter "
        "frame={} thread={} targetPointer={} targetReadable={} target={}",
        enter.Sequence,
        enter.Frame,
        enter.Thread,
        target.pTarget,
        target.Readable,
        target.Readable ? target.Target : "unavailable");

    if (!target.Readable)
    {
        STRE::Campaign::CampaignLoadPolicyContext policyContext;
        policyContext.Target = STRE::Campaign::CampaignLoadTarget::Unknown;
        policyContext.SemanticTargetProofRequired = true;
        const auto decision =
            STRE::Campaign::EvaluateCampaignLoadPolicy(policyContext);
        spdlog::warn(
            "[STRE][CampaignLoadPolicy] LoadMostRecent blocked because its target proof is unavailable decision={}",
            DecisionName(decision));
        return false;
    }

    const ScopedSemanticLoadTarget semanticTarget(target);
    const bool result = TiltedPhoques::ThisCall(
        s_realLoadMostRecentSaveGame, apThis);

    const CampaignSaveTrace::Context exit = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=BGSSaveLoadManager.LoadMostRecentSaveGame event=Exit "
        "frame={} thread={} result={}",
        exit.Sequence,
        exit.Frame,
        exit.Thread,
        result);
    return result;
}
static_assert(std::is_same_v<
    decltype(&HookLoadMostRecentSaveGame),
    TLoadMostRecentSaveGame*>);
}

std::optional<std::string> CampaignNativeLoad::InspectSaveListTarget(
    std::uint32_t aSelectionIndex) noexcept
{
    try
    {
        if (!s_ppSaveLoadManager || !*s_ppSaveLoadManager)
            return std::nullopt;

        NativeTargetObservation target = ObserveSaveListTarget(
            *s_ppSaveLoadManager, aSelectionIndex);
        if (!target.Readable)
            return std::nullopt;
        return std::move(target.Target);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

bool TP_MAKE_THISCALL(
    HookCampaignGateLoadGame,
    BGSSaveLoadManager,
    const char* apSaveName,
    std::int32_t aDeviceId,
    std::uint32_t aOutputStats,
    bool aCheckForMods)
{
    using namespace STRE::Campaign;

    const NativeTargetObservation nativeTarget = ObserveTarget(apSaveName);
    const std::string* const pSemanticTarget = nativeTarget.Readable
        ? &nativeTarget.Target
        : s_semanticLoadTarget
            ? &*s_semanticLoadTarget
            : nullptr;
    const char* const pEffectiveTarget = pSemanticTarget
        ? pSemanticTarget->c_str()
        : nullptr;

    CampaignNativeLoadService* const pLoad =
        CampaignNativeLoadService::TryGet();
    const bool correlated =
        pLoad && pLoad->OnNativeLoadEnter(pEffectiveTarget);
    CampaignResumeService* const pResume =
        CampaignResumeService::TryGet();
    const CampaignLoadTarget target = pResume && pSemanticTarget
        ? pResume->InspectNativeLoadTarget(*pSemanticTarget)
        : CampaignLoadTarget::Unknown;
    const bool reservedCampaignNamespace = pSemanticTarget &&
        pSemanticTarget->starts_with("stre-");
    CampaignRuntimeGateService* const pGate =
        CampaignRuntimeGateService::TryGet();
    const bool admitted =
        pLoad && pLoad->HasAuthoritativeAdmission();
    const bool gateLocked = pGate && pGate->IsLocked();
    CampaignLoadPolicyContext policyContext;
    policyContext.Target = target;
    policyContext.Authority = correlated
        ? CampaignLoadAuthority::InternalRecovery
        : CampaignLoadAuthority::Player;
    policyContext.ExactInternalRecoveryCorrelation = correlated;
    policyContext.CampaignRuntimeSensitive = admitted || gateLocked;
    policyContext.ReservedCampaignNamespaceClaim =
        reservedCampaignNamespace;
    const CampaignLoadDecision decision =
        EvaluateCampaignLoadPolicy(policyContext);

    const CampaignSaveTrace::Context trace = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=BGSSaveLoadManager.Load_Impl event=Enter frame={} "
        "thread={} targetPointer={} targetReadable={} target={} "
        "targetSource={} deviceId={} outputStats={} checkForMods={} "
        "admission={} gateLocked={} internalRecoveryCorrelation={} "
        "classifiedTarget={} decision={}",
        trace.Sequence,
        trace.Frame,
        trace.Thread,
        nativeTarget.pTarget,
        nativeTarget.Readable,
        pSemanticTarget ? *pSemanticTarget : "unavailable",
        nativeTarget.Readable
            ? "Load_Impl"
            : s_semanticLoadTarget
                ? "LoadMostRecentSaveGame"
                : "Unavailable",
        aDeviceId,
        aOutputStats,
        aCheckForMods,
        admitted,
        gateLocked,
        correlated,
        TargetName(target),
        DecisionName(decision));

    if (decision == CampaignLoadDecision::BlockPlayerLoad ||
        decision == CampaignLoadDecision::BlockUnprovenCampaignTarget)
    {
        spdlog::warn(
            "[STRE][CampaignLoadPolicy] PLAYER_LOAD_BLOCKED decision={} target={}",
            DecisionName(decision),
            pSemanticTarget ? *pSemanticTarget : "unavailable");
        return false;
    }

    const bool resumeRequired =
        decision == CampaignLoadDecision::BeginResumeRequired &&
        pResume && pEffectiveTarget &&
        pResume->OnNativeLoadEnter(pEffectiveTarget);
    if (decision == CampaignLoadDecision::BeginResumeRequired &&
        !resumeRequired)
    {
        spdlog::error(
            "[STRE][CampaignLoadPolicy] campaign target blocked because resume-required fencing could not be armed");
        return false;
    }
    const bool managed =
        (correlated || resumeRequired) && pGate &&
        pGate->OnNativeLoadEnter(pEffectiveTarget);
    if ((correlated || resumeRequired) && !managed)
    {
        if (correlated)
            pLoad->OnGateArmFailure();
        return false;
    }

    const bool result = TiltedPhoques::ThisCall(
        s_realLoadGame,
        apThis,
        apSaveName,
        aDeviceId,
        aOutputStats,
        aCheckForMods);

    const CampaignSaveTrace::Context returnTrace =
        CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=BGSSaveLoadManager.Load_Impl event=Exit frame={} "
        "thread={} result={} managed={} owner={}",
        returnTrace.Sequence,
        returnTrace.Frame,
        returnTrace.Thread,
        result,
        managed,
        correlated ? "InternalRecovery"
                   : resumeRequired ? "ResumeRequired" : "Vanilla");

    if (pGate)
        pGate->OnNativeLoadReturn(managed, result);
    if (pLoad)
        pLoad->OnNativeLoadReturn(correlated && managed, result);
    if (pResume)
        pResume->OnNativeLoadReturn(resumeRequired, result);

    return result;
}

CampaignNativeLoadInvokeResult CampaignNativeLoad::InvokeValidated(
    std::string_view acNativeSaveIdentity) noexcept
{
    try
    {
        if (acNativeSaveIdentity.empty() || !s_realLoadGame ||
            !s_ppSaveLoadManager || !*s_ppSaveLoadManager)
        {
            return CampaignNativeLoadInvokeResult::BoundaryUnavailable;
        }

        const std::string identity{
            acNativeSaveIdentity.data(), acNativeSaveIdentity.size()};
        const bool accepted = HookCampaignGateLoadGame(
            *s_ppSaveLoadManager,
            identity.c_str(),
            -1,
            0,
            true);
        return accepted
            ? CampaignNativeLoadInvokeResult::NativeAccepted
            : CampaignNativeLoadInvokeResult::NativeRejected;
    }
    catch (...)
    {
        return CampaignNativeLoadInvokeResult::BoundaryUnavailable;
    }
}

static TiltedPhoques::Initializer s_campaignSaveLoadGateHook(
    []()
    {
        // CommonLibSSE-NG BGSSaveLoadManager::Load_Impl, AE Address Library ID.
        POINTER_SKYRIMSE(
            TCampaignGateLoadGame,
            loadGame,
            35728);
        POINTER_SKYRIMSE(
            BGSSaveLoadManager*,
            saveLoadManager,
            403340);
        s_realLoadGame = loadGame.Get();
        s_ppSaveLoadManager = saveLoadManager.Get();
        POINTER_SKYRIMSE(
            TLoadMostRecentSaveGame,
            loadMostRecentSaveGame,
            35766);
        s_realLoadMostRecentSaveGame = loadMostRecentSaveGame.Get();
        if (s_realLoadMostRecentSaveGame)
        {
            TP_HOOK(
                &s_realLoadMostRecentSaveGame,
                HookLoadMostRecentSaveGame);
        }

        // startmenu.swf calls GameDelegate.call("ContinueLastSavedGame")
        // after its vanilla FadeOutAndCall boundary. Hooking the public
        // FxDelegate::Callback virtual ABI observes that exact semantic call
        // without guessing the opaque callback adapter relocation. This is
        // trace-only until the native request chain proves the exact canonical
        // target selected downstream. FxDelegate args[0] is the response ID,
        // not a save-list index or a load-authority input.
        POINTER_SKYRIMSE(void, fxDelegateVtable, 242193);
        auto** const ppFxDelegateVtable = static_cast<void**>(
            fxDelegateVtable.GetPtr());
        if (ppFxDelegateVtable && ppFxDelegateVtable[1])
        {
            s_realFxDelegateCallback =
                reinterpret_cast<TFxDelegateCallback*>(
                    ppFxDelegateVtable[1]);
            TP_HOOK(
                &s_realFxDelegateCallback,
                HookFxDelegateCallback);
        }
        TP_HOOK(&s_realLoadGame, HookCampaignGateLoadGame);
    });
