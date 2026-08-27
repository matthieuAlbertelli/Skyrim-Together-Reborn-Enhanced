#include <TiltedOnlinePCH.h>

#include <Services/CampaignSaveTraceService.h>

#include <CampaignLoadPolicy.h>
#include <CampaignLoadUiProjection.h>
#include <CampaignContinueLoad.h>
#include <CampaignNativeLoad.h>
#include <CampaignSavePolicy.h>
#include <CampaignSaveTrace.h>
#include <Events/CampaignMainMenuEnteredEvent.h>
#include <Events/PreUpdateEvent.h>
#include <Games/Events.h>
#include <Games/Skyrim/Interface/UI.h>
#include <Misc/BSFixedString.h>
#include <Services/CampaignNativeLoadService.h>
#include <Services/CampaignResumeService.h>
#include <Services/CampaignRuntimeGateService.h>
#include <World.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <type_traits>

using namespace STRE::Campaign;

// These declarations mirror only public CommonLibSSE-NG event boundaries.
// The opaque save/load event payloads deliberately remain uninterpreted.
struct InputEvent
{
    virtual ~InputEvent();
    [[nodiscard]] virtual bool HasIDCode() const;
    [[nodiscard]] virtual const BSFixedString& QUserEvent() const;

    std::uint32_t Device;
    std::uint32_t EventType;
    InputEvent* pNext;
};
static_assert(sizeof(InputEvent) == 0x18);

struct ButtonInputEvent : InputEvent
{
    BSFixedString UserEvent;
    std::uint32_t IdCode;
    std::uint32_t Pad24;
    float Value;
    float HeldDownSeconds;
};
static_assert(sizeof(ButtonInputEvent) == 0x30);

struct BSSaveDataEvent
{
};

struct BGSSaveLoadManagerEvent
{
};

struct MenuOpenCloseEvent
{
    BSFixedString MenuName;
    bool Opening;
    std::uint8_t Pad09;
    std::uint16_t Pad0A;
    std::uint32_t Pad0C;
};
static_assert(sizeof(MenuOpenCloseEvent) == 0x10);

// Minimal mirrors of the public CommonLibSSE-NG save/load request, queue, and
// Scaleform callback ABIs. They are trace-only and never carry save authority.
struct NativeSaveLoadRequest
{
    void* pVtable;
    std::uint32_t RefCount;
    std::uint32_t OperationCode;
    std::uint32_t State;
    std::uint32_t Pad14;
};
static_assert(sizeof(NativeSaveLoadRequest) == 0x18);

struct NativeSaveLoadRequestRef
{
    NativeSaveLoadRequest* pRequest;
};
static_assert(sizeof(NativeSaveLoadRequestRef) == 0x8);

// ID 35772's 0xD0000010 branch reads this exact LoadRequest payload before
// calling AE ID 442580. The vtable and 0x28 allocation size are independently
// exposed by CommonLib RTTI/VTABLE data and the native deleting destructor.
struct NativeLoadRequestView
{
    NativeSaveLoadRequest Base;
    void* pResolvedLoadSource;
    std::uint32_t Value20;
    std::uint8_t Value24;
    std::uint8_t Value25;
    std::uint16_t Pad26;
};
static_assert(sizeof(NativeLoadRequestView) == 0x28);

// The exact ID 442580 consumer dereferences the resolved source carried at
// LoadRequest+0x18 and reads its filename pointer at +0xBB0 before dispatching
// the native load. No other fields are interpreted here.
struct NativeResolvedLoadSourceView
{
    std::byte Pad000[0xBB0];
    const char* pNativeSaveIdentity;
};
static_assert(
    offsetof(NativeResolvedLoadSourceView, pNativeSaveIdentity) == 0xBB0);

struct NativeResolvedLoadSourceRef
{
    NativeResolvedLoadSourceView* pSource;
};
static_assert(sizeof(NativeResolvedLoadSourceRef) == 0x8);

// Fields touched by the proven ID 35772 branches only. Names remain
// descriptive of their observed use; unknown manager storage is not promoted
// to policy or authority.
struct NativeContinueManagerView
{
    std::byte Pad000[0x240];
    const char* pCallbackName;
    std::uint16_t CallbackPending;
    std::uint16_t Pad24A;
    std::uint32_t Pad24C;
    std::int32_t Value250;
    std::uint32_t Pad254;
    NativeResolvedLoadSourceView* pResolvedLoadSource;
    std::uint8_t Value260;
    std::uint8_t ResolvedSourceReady;
};
static_assert(offsetof(NativeContinueManagerView, pCallbackName) == 0x240);
static_assert(offsetof(NativeContinueManagerView, CallbackPending) == 0x248);
static_assert(offsetof(NativeContinueManagerView, Value250) == 0x250);
static_assert(
    offsetof(NativeContinueManagerView, pResolvedLoadSource) == 0x258);
static_assert(
    offsetof(NativeContinueManagerView, ResolvedSourceReady) == 0x261);

struct NativeGfxValue
{
    void* pObjectInterface;
    std::uint32_t Type;
    std::uint32_t Pad0C;
    union
    {
        double Number;
        void* pObject;
    } Value;
};
static_assert(sizeof(NativeGfxValue) == 0x18);

struct NativeFxDelegateArgs
{
    std::byte ResponseId[0x18];
    void* pHandler;
    void* pMovieView;
    const NativeGfxValue* pArguments;
    std::uint32_t ArgumentCount;
    std::uint32_t Pad34;
};
static_assert(sizeof(NativeFxDelegateArgs) == 0x38);

namespace
{
constexpr std::ptrdiff_t kSaveDataEventSourceOffset = 0x8;
constexpr std::ptrdiff_t kSaveLoadManagerEventSourceOffset = 0x18;
constexpr std::ptrdiff_t kUiMenuEventSourceOffset = 0x8;
constexpr std::ptrdiff_t kAeAsyncRequestQueueOffset = 0x358;
constexpr std::ptrdiff_t kAeProcessRequestQueueOffset = 0x3B8;
constexpr std::uint32_t kGfxNumberType = 3;
constexpr std::uint32_t kGfxTypeMask = 0x8F;
constexpr std::uint32_t kContinueCallbackOperation = 0xD0000100;
constexpr std::size_t kMaximumObservedNativeIdentity = 260;
constexpr std::size_t kMaximumContinueCorrelations = 32;

TP_THIS_FUNCTION(
    TMenuControlsProcessEvent,
    BSTEventResult,
    void,
    InputEvent* const*,
    EventDispatcher<InputEvent*>*);
TMenuControlsProcessEvent* s_realMenuControlsProcessEvent{};

TP_THIS_FUNCTION(
    TQuickSaveLoadProcessButton,
    bool,
    void,
    InputEvent*);
TQuickSaveLoadProcessButton* s_realQuickSaveLoadProcessButton{};

TP_THIS_FUNCTION(
    TQueueNativeSaveLoadRequest,
    void,
    void,
    std::uint32_t);
TQueueNativeSaveLoadRequest* s_realQueueNativeSaveLoadRequest{};

TP_THIS_FUNCTION(
    TNativeRequestQueueOperation,
    bool,
    void,
    NativeSaveLoadRequestRef*);
TNativeRequestQueueOperation* s_realNativeRequestQueuePush{};
TNativeRequestQueueOperation* s_realNativeRequestQueuePop{};

TP_THIS_FUNCTION(
    TNativeLoadRequestDispatch,
    bool,
    void,
    NativeResolvedLoadSourceRef*,
    std::uint32_t,
    bool,
    bool,
    std::uint32_t);
TNativeLoadRequestDispatch* s_realNativeLoadRequestDispatch{};

void* s_requestVtable{};
void* s_loadRequestVtable{};
void* s_loadEntryRequestVtable{};
void* s_saveOperationRequestVtable{};
void* s_buildSaveListRequestVtable{};

using TManualSaveGameCallback =
    void __fastcall(const NativeFxDelegateArgs&);
TManualSaveGameCallback* s_realManualSaveGameCallback{};

using TManualLoadGameCallback =
    void __fastcall(const NativeFxDelegateArgs&);
TManualLoadGameCallback* s_realManualLoadGameCallback{};

void** s_saveLoadManagerSingleton{};

constexpr const char* kJournalMenuName = "Journal Menu";
constexpr const char* kMainMenuName = "Main Menu";
constexpr const char* kPlayerLoadBlockedTranslation =
    "COMPONENT.CAMPAIGN_SAVE.LOAD_BLOCKED";

const char* LoadTargetName(CampaignLoadTarget aTarget) noexcept
{
    switch (aTarget)
    {
    case CampaignLoadTarget::Ordinary: return "Ordinary";
    case CampaignLoadTarget::Campaign: return "Campaign";
    case CampaignLoadTarget::Unknown: return "Unknown";
    }
    return "Unknown";
}

const char* LoadDecisionName(CampaignLoadDecision aDecision) noexcept
{
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

const char* LoadUiHostName(CampaignLoadUiHost aHost) noexcept
{
    switch (aHost)
    {
    case CampaignLoadUiHost::None: return "None";
    case CampaignLoadUiHost::Journal: return "Journal";
    case CampaignLoadUiHost::MainMenu: return "MainMenu";
    }
    return "None";
}

std::optional<std::uint32_t> ReadSelectionIndex(
    const NativeFxDelegateArgs& acArguments) noexcept
{
    const NativeGfxValue* const pSelection =
        acArguments.ArgumentCount > 0
        ? acArguments.pArguments
        : nullptr;
    if (!pSelection ||
        (pSelection->Type & kGfxTypeMask) != kGfxNumberType)
    {
        return std::nullopt;
    }

    const double selection = pSelection->Value.Number;
    if (!(selection >= 0.0) ||
        selection > static_cast<double>(
            std::numeric_limits<std::uint32_t>::max()))
    {
        return std::nullopt;
    }

    const auto index = static_cast<std::uint32_t>(selection);
    return static_cast<double>(index) == selection
        ? std::optional<std::uint32_t>{index}
        : std::nullopt;
}

bool RequestJournalClose() noexcept
{
    try
    {
        UI* const pUi = UI::Get();
        if (!pUi)
            return false;

        const BSFixedString journalMenu(kJournalMenuName);
        if (!pUi->GetMenuOpen(journalMenu))
            return false;

        pUi->QueueMessage(journalMenu, UIMessage::kHide);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool RequestMainMenuRebuild() noexcept
{
    try
    {
        UI* const pUi = UI::Get();
        if (!pUi)
            return false;

        const BSFixedString mainMenu(kMainMenuName);
        if (!pUi->GetMenuOpen(mainMenu))
            return false;

        // LoadGame is a void Scaleform callback with no cancel result. Rebuild
        // the entire public menu instance through normal UI messages so any
        // private disabled/busy state dies with the rejected SWF instance.
        pUi->QueueMessage(mainMenu, UIMessage::kHide);
        pUi->QueueMessage(mainMenu, UIMessage::kShow);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void NotifyPlayerLoadBlocked() noexcept
{
    try
    {
        World::Get().GetOverlayService().SendSystemMessage(
            kPlayerLoadBlockedTranslation);
    }
    catch (...)
    {
    }
}

const char* FindSaveAction(InputEvent* const* appEvent) noexcept
{
    for (const InputEvent* pEvent = appEvent ? *appEvent : nullptr;
         pEvent;
         pEvent = pEvent->pNext)
    {
        const char* const pAction = pEvent->QUserEvent().AsAscii();
        if (pAction && std::strcmp(pAction, "Quicksave") == 0)
            return "Quicksave";
        if (pAction && std::strcmp(pAction, "NewSave") == 0)
            return "NewSave";
    }
    return nullptr;
}

void LogMenuControlsAction(
    const char* acAction,
    const char* acPhase) noexcept
{
    const CampaignSaveTrace::Context context = CampaignSaveTrace::Capture();
    spdlog::debug(
        "[STRE][CampaignSaveTrace] sequence={} "
        "source=MenuControls.ProcessEvent event={} frame={} thread={} "
        "phase={}",
        context.Sequence,
        acAction,
        context.Frame,
        context.Thread,
        acPhase);
}

BSTEventResult TP_MAKE_THISCALL(
    HookMenuControlsProcessEvent,
    void,
    InputEvent* const* appEvent,
    EventDispatcher<InputEvent*>* apEventSource)
{
    const char* const pAction = FindSaveAction(appEvent);
    if (pAction)
        LogMenuControlsAction(pAction, "Enter");

    const BSTEventResult result = TiltedPhoques::ThisCall(
        s_realMenuControlsProcessEvent,
        apThis,
        appEvent,
        apEventSource);

    if (pAction)
        LogMenuControlsAction(pAction, "Exit");
    return result;
}

bool IsQuicksave(const InputEvent* apEvent) noexcept
{
    if (!apEvent)
        return false;
    const char* const pAction = apEvent->QUserEvent().AsAscii();
    return pAction && std::strcmp(pAction, "Quicksave") == 0;
}

bool IsQuickload(const InputEvent* apEvent) noexcept
{
    if (!apEvent)
        return false;
    const char* const pAction = apEvent->QUserEvent().AsAscii();
    return pAction && std::strcmp(pAction, "Quickload") == 0;
}

bool TP_MAKE_THISCALL(
    HookQuickSaveLoadProcessButton,
    void,
    InputEvent* apEvent)
{
    const bool quicksave = IsQuicksave(apEvent);
    const bool quickload = IsQuickload(apEvent);
    bool actionablePress{};
    if ((quicksave || quickload) && apEvent->EventType == 0)
    {
        const auto* const pButton =
            static_cast<const ButtonInputEvent*>(apEvent);
        actionablePress =
            pButton->Value != 0.0F && pButton->HeldDownSeconds == 0.0F;
    }

    if (quicksave)
    {
        const CampaignSaveTrace::Context context =
            CampaignSaveTrace::Capture();
        spdlog::debug(
            "[STRE][CampaignSaveTrace] sequence={} "
            "source=QuickSaveLoadHandler.ProcessButton event=Quicksave "
            "frame={} thread={} phase=Enter actionablePress={}",
            context.Sequence,
            context.Frame,
            context.Thread,
            actionablePress);
    }
    if (quickload)
    {
        const CampaignSaveTrace::Context context =
            CampaignSaveTrace::Capture();
        spdlog::debug(
            "[STRE][CampaignLoadTrace] sequence={} "
            "source=QuickSaveLoadHandler.ProcessButton event=Quickload "
            "frame={} thread={} phase=Enter actionablePress={}",
            context.Sequence,
            context.Frame,
            context.Thread,
            actionablePress);
    }

    if (quickload && actionablePress)
    {
        CampaignNativeLoadService* const pLoad =
            CampaignNativeLoadService::TryGet();
        STRE::Campaign::CampaignLoadPolicyContext policyContext;
        policyContext.Target = STRE::Campaign::CampaignLoadTarget::Unknown;
        policyContext.CampaignRuntimeSensitive =
            pLoad && pLoad->IsCampaignRuntimeSensitive();
        const auto decision =
            STRE::Campaign::EvaluateCampaignLoadPolicy(policyContext);
        if (decision ==
            STRE::Campaign::CampaignLoadDecision::BlockPlayerLoad)
        {
            const CampaignSaveTrace::Context context =
                CampaignSaveTrace::Capture();
            spdlog::warn(
                "[STRE][CampaignLoadTrace] sequence={} "
                "source=QuickSaveLoadHandler.ProcessButton "
                "event=QuickloadBlocked frame={} thread={} "
                "decision=BlockPlayerLoad",
                context.Sequence,
                context.Frame,
                context.Thread);
            // The press was handled by STRE. Do not let the native QuickLoad
            // path turn a deliberate policy rejection into Skyrim's corrupt-
            // save error dialog.
            return true;
        }
    }

    const ScopedCampaignQuickSaveAction quickSaveAction(
        quicksave && actionablePress);
    const bool result = TiltedPhoques::ThisCall(
        s_realQuickSaveLoadProcessButton,
        apThis,
        apEvent);

    if (quicksave)
    {
        const CampaignSaveTrace::Context context =
            CampaignSaveTrace::Capture();
        spdlog::debug(
            "[STRE][CampaignSaveTrace] sequence={} "
            "source=QuickSaveLoadHandler.ProcessButton event=Quicksave "
            "frame={} thread={} phase=Exit actionablePress={} "
            "handlerResult={}",
            context.Sequence,
            context.Frame,
            context.Thread,
            actionablePress,
            result);
    }
    if (quickload)
    {
        const CampaignSaveTrace::Context context =
            CampaignSaveTrace::Capture();
        spdlog::debug(
            "[STRE][CampaignLoadTrace] sequence={} "
            "source=QuickSaveLoadHandler.ProcessButton event=Quickload "
            "frame={} thread={} phase=Exit actionablePress={} "
            "handlerResult={}",
            context.Sequence,
            context.Frame,
            context.Thread,
            actionablePress,
            result);
    }
    return result;
}
static_assert(std::is_same_v<
    decltype(&HookQuickSaveLoadProcessButton),
    TQuickSaveLoadProcessButton*>);

void TP_MAKE_THISCALL(
    HookQueueNativeSaveLoadRequest,
    void,
    std::uint32_t aOperationCode)
{
    const CampaignSaveTrace::Context enter = CampaignSaveTrace::Capture();
    spdlog::debug(
        "[STRE][CampaignSaveTrace] sequence={} "
        "source=BGSSaveLoadManager.QueueRequest event=Enter frame={} "
        "thread={} operationCode=0x{:08X}",
        enter.Sequence,
        enter.Frame,
        enter.Thread,
        aOperationCode);

    TiltedPhoques::ThisCall(
        s_realQueueNativeSaveLoadRequest,
        apThis,
        aOperationCode);

    const CampaignSaveTrace::Context exit = CampaignSaveTrace::Capture();
    spdlog::debug(
        "[STRE][CampaignSaveTrace] sequence={} "
        "source=BGSSaveLoadManager.QueueRequest event=Exit frame={} "
        "thread={} operationCode=0x{:08X}",
        exit.Sequence,
        exit.Frame,
        exit.Thread,
        aOperationCode);
}
static_assert(std::is_same_v<
    decltype(&HookQueueNativeSaveLoadRequest),
    TQueueNativeSaveLoadRequest*>);

struct NativeRequestObservation
{
    const void* pRequest{};
    const void* pVtable{};
    const void* pResolvedLoadSource{};
    const char* pNativeSaveIdentity{};
    const char* pTypeName{"Unknown"};
    std::uint32_t OperationCode{};
    std::uint32_t State{};
    bool PointerPresent{};
    bool Readable{};
};

struct NativeStringObservation
{
    const char* pPointer{};
    char Value[kMaximumObservedNativeIdentity + 1]{};
    bool Readable{};
};

NativeStringObservation ObserveNativeString(const char* apString) noexcept
{
    NativeStringObservation observation;
    observation.pPointer = apString;
    if (!apString)
        return observation;

    __try
    {
        for (std::size_t i = 0; i < kMaximumObservedNativeIdentity; ++i)
        {
            observation.Value[i] = apString[i];
            if (apString[i] == '\0')
            {
                observation.Readable = true;
                return observation;
            }
        }
        observation.Value[kMaximumObservedNativeIdentity] = '\0';
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        observation.Readable = false;
    }
    return observation;
}

const char* GetRequestTypeName(const void* apVtable) noexcept
{
    if (apVtable == s_loadRequestVtable)
        return "LoadRequest";
    if (apVtable == s_loadEntryRequestVtable)
        return "LoadEntryRequest";
    if (apVtable == s_saveOperationRequestVtable)
        return "SaveOperationRequest";
    if (apVtable == s_buildSaveListRequestVtable)
        return "BuildSaveListRequest";
    if (apVtable == s_requestVtable)
        return "Request";
    return "Unknown";
}

NativeRequestObservation ObserveRequest(
    const NativeSaveLoadRequestRef* apRequestRef) noexcept
{
    NativeRequestObservation observation;
    __try
    {
        const NativeSaveLoadRequest* const pRequest =
            apRequestRef ? apRequestRef->pRequest : nullptr;
        observation.PointerPresent = pRequest != nullptr;
        if (pRequest)
        {
            observation.pRequest = pRequest;
            observation.pVtable = pRequest->pVtable;
            observation.pTypeName = GetRequestTypeName(pRequest->pVtable);
            observation.OperationCode = pRequest->OperationCode;
            observation.State = pRequest->State;
            if (pRequest->pVtable == s_loadRequestVtable)
            {
                const auto* const pLoadRequest =
                    reinterpret_cast<const NativeLoadRequestView*>(pRequest);
                observation.pResolvedLoadSource =
                    pLoadRequest->pResolvedLoadSource;
                if (pLoadRequest->pResolvedLoadSource)
                {
                    const auto* const pSource =
                        static_cast<const NativeResolvedLoadSourceView*>(
                            pLoadRequest->pResolvedLoadSource);
                    observation.pNativeSaveIdentity =
                        pSource->pNativeSaveIdentity;
                }
            }
            observation.Readable = true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        observation.Readable = false;
    }
    return observation;
}

struct NativeManagerObservation
{
    const void* pManager{};
    const char* pCallbackName{};
    const void* pResolvedLoadSource{};
    const char* pNativeSaveIdentity{};
    std::uint16_t CallbackPending{};
    std::int32_t Value250{};
    std::uint8_t ResolvedSourceReady{};
    bool Readable{};
};

NativeManagerObservation ObserveManager(const void* apManager) noexcept
{
    NativeManagerObservation observation;
    observation.pManager = apManager;
    if (!apManager)
        return observation;

    __try
    {
        const auto* const pManager =
            static_cast<const NativeContinueManagerView*>(apManager);
        observation.pCallbackName = pManager->pCallbackName;
        observation.CallbackPending = pManager->CallbackPending;
        observation.Value250 = pManager->Value250;
        observation.pResolvedLoadSource = pManager->pResolvedLoadSource;
        observation.ResolvedSourceReady = pManager->ResolvedSourceReady;
        if (pManager->pResolvedLoadSource)
        {
            observation.pNativeSaveIdentity =
                pManager->pResolvedLoadSource->pNativeSaveIdentity;
        }
        observation.Readable = true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        observation.Readable = false;
    }
    return observation;
}

struct ContinueCorrelation
{
    const void* pRequest{};
    const void* pParent{};
    const void* pRoot{};
    std::uint32_t OperationCode{};
    std::uint64_t Order{};
};

struct ActiveContinueDispatch
{
    ContinueCorrelation Correlation;
    NativeRequestObservation Request;
    NativeStringObservation RequestTarget;
    const void* pManager{};
};

std::array<ContinueCorrelation, kMaximumContinueCorrelations>
    s_continueCorrelations{};
std::mutex s_continueCorrelationMutex;
std::uint64_t s_continueCorrelationOrder{};
thread_local std::optional<ActiveContinueDispatch> s_activeContinueDispatch;
CampaignContinueLoadAttempt s_continueLoadAttempt;
std::mutex s_continueLoadAttemptMutex;

std::optional<ContinueCorrelation> FindContinueCorrelation(
    const void* apRequest) noexcept
{
    if (!apRequest)
        return std::nullopt;
    std::scoped_lock lock(s_continueCorrelationMutex);
    for (const ContinueCorrelation& correlation : s_continueCorrelations)
    {
        if (correlation.pRequest == apRequest)
            return correlation;
    }
    return std::nullopt;
}

ContinueCorrelation TagContinueCorrelation(
    const void* apRequest,
    std::uint32_t aOperationCode,
    const ContinueCorrelation* apParent) noexcept
{
    std::scoped_lock lock(s_continueCorrelationMutex);
    for (ContinueCorrelation& correlation : s_continueCorrelations)
    {
        if (correlation.pRequest == apRequest)
        {
            if (!apParent)
            {
                correlation = {
                    apRequest,
                    nullptr,
                    apRequest,
                    aOperationCode,
                    ++s_continueCorrelationOrder};
            }
            else if (apParent->pRequest != apRequest)
            {
                correlation = {
                    apRequest,
                    apParent->pRequest,
                    apParent->pRoot,
                    aOperationCode,
                    ++s_continueCorrelationOrder};
            }
            else
            {
                correlation.OperationCode = aOperationCode;
            }
            return correlation;
        }
    }

    ContinueCorrelation* pSlot{};
    for (ContinueCorrelation& correlation : s_continueCorrelations)
    {
        if (!correlation.pRequest)
        {
            pSlot = &correlation;
            break;
        }
    }
    if (!pSlot)
    {
        pSlot = &*std::min_element(
            s_continueCorrelations.begin(),
            s_continueCorrelations.end(),
            [](const ContinueCorrelation& acLeft,
               const ContinueCorrelation& acRight)
            {
                return acLeft.Order < acRight.Order;
            });
    }

    *pSlot = {
        apRequest,
        apParent ? apParent->pRequest : nullptr,
        apParent ? apParent->pRoot : apRequest,
        aOperationCode,
        ++s_continueCorrelationOrder};
    return *pSlot;
}

void RemoveContinueCorrelation(const void* apRequest) noexcept
{
    if (!apRequest)
        return;
    std::scoped_lock lock(s_continueCorrelationMutex);
    for (ContinueCorrelation& correlation : s_continueCorrelations)
    {
        if (correlation.pRequest == apRequest)
        {
            correlation = {};
            return;
        }
    }
}

void ResetContinueCorrelations() noexcept
{
    std::scoped_lock lock(s_continueCorrelationMutex);
    s_continueCorrelations.fill({});
}

void RemoveContinueCorrelationsForRoot(const void* apRoot) noexcept
{
    if (!apRoot)
        return;
    std::scoped_lock lock(s_continueCorrelationMutex);
    for (ContinueCorrelation& correlation : s_continueCorrelations)
    {
        if (correlation.pRoot == apRoot)
            correlation = {};
    }
}

bool HasContinueCorrelationForRoot(const void* apRoot) noexcept
{
    if (!apRoot)
        return false;
    std::scoped_lock lock(s_continueCorrelationMutex);
    return std::any_of(
        s_continueCorrelations.begin(),
        s_continueCorrelations.end(),
        [apRoot](const ContinueCorrelation& acCorrelation)
        {
            return acCorrelation.pRoot == apRoot;
        });
}

void BeginContinueLoadAttempt(const void* apRoot) noexcept
{
    std::scoped_lock lock(s_continueLoadAttemptMutex);
    s_continueLoadAttempt.Begin(
        reinterpret_cast<std::uintptr_t>(apRoot));
}

bool IsContinueLoadAttemptActive(const void* apRoot) noexcept
{
    std::scoped_lock lock(s_continueLoadAttemptMutex);
    return s_continueLoadAttempt.IsActive(
        reinterpret_cast<std::uintptr_t>(apRoot));
}

std::optional<CampaignContinueLoadClaim> ClaimContinueLoadTarget(
    const ActiveContinueDispatch& acDispatch) noexcept
{
    const ContinueCorrelation& correlation = acDispatch.Correlation;
    const NativeRequestObservation& request = acDispatch.Request;
    std::scoped_lock lock(s_continueLoadAttemptMutex);
    return s_continueLoadAttempt.ClaimTarget(
        reinterpret_cast<std::uintptr_t>(correlation.pRoot),
        reinterpret_cast<std::uintptr_t>(correlation.pParent),
        reinterpret_cast<std::uintptr_t>(correlation.pRequest),
        request.pVtable == s_loadRequestVtable,
        acDispatch.RequestTarget.Readable
            ? std::string_view(acDispatch.RequestTarget.Value)
            : std::string_view{},
        acDispatch.RequestTarget.Readable);
}

void CompleteContinueLoadAttempt(const void* apRoot) noexcept
{
    {
        std::scoped_lock lock(s_continueLoadAttemptMutex);
        s_continueLoadAttempt.Complete(
            reinterpret_cast<std::uintptr_t>(apRoot));
    }
    RemoveContinueCorrelationsForRoot(apRoot);
}

bool IsProcessRequestQueue(const void* apQueue) noexcept
{
    void* pManager{};
    __try
    {
        pManager = s_saveLoadManagerSingleton
            ? *s_saveLoadManagerSingleton
            : nullptr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    if (!pManager)
        return false;
    return apQueue == static_cast<const std::byte*>(pManager) +
        kAeProcessRequestQueueOffset;
}

const void* GetProcessQueueManager(const void* apQueue) noexcept
{
    if (!IsProcessRequestQueue(apQueue))
        return nullptr;
    return static_cast<const std::byte*>(apQueue) -
        kAeProcessRequestQueueOffset;
}

void LogContinueCorrelation(
    const char* acEvent,
    const ContinueCorrelation& acCorrelation,
    const NativeRequestObservation& acRequest,
    const void* apManager,
    const NativeStringObservation* apCapturedRequestTarget = nullptr) noexcept
{
    const NativeManagerObservation manager = ObserveManager(apManager);
    const NativeStringObservation observedRequestTarget =
        ObserveNativeString(acRequest.pNativeSaveIdentity);
    const NativeStringObservation& requestTarget = apCapturedRequestTarget
        ? *apCapturedRequestTarget
        : observedRequestTarget;
    const NativeStringObservation managerTarget =
        ObserveNativeString(manager.pNativeSaveIdentity);
    const NativeStringObservation callbackName =
        ObserveNativeString(manager.pCallbackName);
    const CampaignSaveTrace::Context context = CampaignSaveTrace::Capture();
    spdlog::debug(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=MainMenu.ContinueRequest event={} frame={} thread={} "
        "rootRequest={} parentRequest={} requestAddress={} "
        "operationCode=0x{:08X} requestType={} requestVtable={} "
        "requestState={} loadSource={} loadTargetReadable={} "
        "loadTarget={} managerReadable={} callbackPending={} "
        "callbackNameReadable={} callbackName={} managerValue250={} "
        "managerLoadSource={} managerSourceReady={} "
        "managerTargetReadable={} managerTarget={}",
        context.Sequence,
        acEvent,
        context.Frame,
        context.Thread,
        acCorrelation.pRoot,
        acCorrelation.pParent,
        acCorrelation.pRequest,
        acRequest.OperationCode,
        acRequest.pTypeName,
        acRequest.pVtable,
        acRequest.State,
        acRequest.pResolvedLoadSource,
        requestTarget.Readable,
        requestTarget.Readable ? requestTarget.Value : "unavailable",
        manager.Readable,
        manager.CallbackPending,
        callbackName.Readable,
        callbackName.Readable ? callbackName.Value : "unavailable",
        manager.Value250,
        manager.pResolvedLoadSource,
        manager.ResolvedSourceReady,
        managerTarget.Readable,
        managerTarget.Readable ? managerTarget.Value : "unavailable");
}

void FinishActiveContinueDispatch() noexcept
{
    if (!s_activeContinueDispatch)
        return;

    // The native consumer may release the request before it asks the queue for
    // its next item. Reuse the pre-dispatch observation and inspect only the
    // still-owned manager after dispatch; never dereference a retired request.
    LogContinueCorrelation(
        "AfterDispatch",
        s_activeContinueDispatch->Correlation,
        s_activeContinueDispatch->Request,
        s_activeContinueDispatch->pManager,
        &s_activeContinueDispatch->RequestTarget);
    const ContinueCorrelation correlation =
        s_activeContinueDispatch->Correlation;
    RemoveContinueCorrelation(correlation.pRequest);
    if (!HasContinueCorrelationForRoot(correlation.pRoot))
    {
        CompleteContinueLoadAttempt(correlation.pRoot);
    }
    s_activeContinueDispatch.reset();
}

const void* ObserveRequestPointer(
    const NativeSaveLoadRequestRef* apRequestRef) noexcept
{
    __try
    {
        return apRequestRef ? apRequestRef->pRequest : nullptr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

const char* GetRequestQueueName(const void* apQueue) noexcept
{
    void* pManager{};
    __try
    {
        pManager = s_saveLoadManagerSingleton
            ? *s_saveLoadManagerSingleton
            : nullptr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return "Unknown";
    }
    if (!pManager)
        return "Unknown";

    const auto* const pManagerBytes =
        static_cast<const std::byte*>(pManager);
    if (apQueue == pManagerBytes + kAeAsyncRequestQueueOffset)
        return "AsyncThread";
    if (apQueue == pManagerBytes + kAeProcessRequestQueueOffset)
        return "ProcessBoundary";
    return "Other";
}

void LogRequestQueueOperation(
    const char* acEvent,
    const void* apQueue,
    const NativeSaveLoadRequestRef* apRequestRef,
    bool aResult) noexcept
{
    const NativeRequestObservation request = ObserveRequest(apRequestRef);
    const NativeStringObservation target =
        ObserveNativeString(request.pNativeSaveIdentity);
    const CampaignSaveTrace::Context context = CampaignSaveTrace::Capture();
    spdlog::debug(
        "[STRE][CampaignSaveTrace] sequence={} "
        "source=BGSSaveLoadManager.RequestQueue event={} frame={} "
        "thread={} queue={} queueAddress={} requestAddress={} "
        "requestPointer={} requestReadable={} requestType={} "
        "requestVtable={} operationCode=0x{:08X} requestState={} "
        "loadSource={} loadTargetReadable={} loadTarget={} result={}",
        context.Sequence,
        acEvent,
        context.Frame,
        context.Thread,
        GetRequestQueueName(apQueue),
        apQueue,
        ObserveRequestPointer(apRequestRef),
        request.PointerPresent ? "present" : "null",
        request.Readable,
        request.pTypeName,
        request.pVtable,
        request.OperationCode,
        request.State,
        request.pResolvedLoadSource,
        target.Readable,
        target.Readable ? target.Value : "unavailable",
        aResult);
}

bool TP_MAKE_THISCALL(
    HookNativeRequestQueuePush,
    void,
    NativeSaveLoadRequestRef* apRequest)
{
    const bool result = TiltedPhoques::ThisCall(
        s_realNativeRequestQueuePush,
        apThis,
        apRequest);
    const NativeRequestObservation request = ObserveRequest(apRequest);
    CampaignSaveProvenance::ObserveQuickRequestPush(
        ObserveRequestPointer(apRequest),
        request.Readable ? request.OperationCode : 0,
        result);
    if (request.Readable)
    {
        const void* const pRequest = request.pRequest;
        std::optional<ContinueCorrelation> correlation;
        if (result && CampaignSaveTrace::IsContinueCallbackActive() &&
            request.OperationCode == kContinueCallbackOperation)
        {
            if (CampaignResumeService* const pResume =
                    CampaignResumeService::TryGet())
            {
                pResume->CancelContinueLoadPending();
            }
            ResetContinueCorrelations();
            correlation = TagContinueCorrelation(
                pRequest, request.OperationCode, nullptr);
            BeginContinueLoadAttempt(pRequest);
        }
        else if (result && s_activeContinueDispatch &&
            IsContinueLoadAttemptActive(
                s_activeContinueDispatch->Correlation.pRoot))
        {
            correlation = TagContinueCorrelation(
                pRequest,
                request.OperationCode,
                &s_activeContinueDispatch->Correlation);
        }
        else if (result)
        {
            correlation = FindContinueCorrelation(pRequest);
        }
        else if (const std::optional<ContinueCorrelation> failed =
                     FindContinueCorrelation(pRequest);
                 failed)
        {
            RemoveContinueCorrelation(pRequest);
            CompleteContinueLoadAttempt(failed->pRoot);
        }

        if (correlation)
        {
            LogContinueCorrelation(
                "PushCorrelated",
                *correlation,
                request,
                GetProcessQueueManager(apThis));
        }
    }
    LogRequestQueueOperation("Push", apThis, apRequest, result);
    return result;
}
static_assert(std::is_same_v<
    decltype(&HookNativeRequestQueuePush),
    TNativeRequestQueueOperation*>);

bool TP_MAKE_THISCALL(
    HookNativeRequestQueuePop,
    void,
    NativeSaveLoadRequestRef* apRequest)
{
    if (IsProcessRequestQueue(apThis))
        FinishActiveContinueDispatch();

    const bool result = TiltedPhoques::ThisCall(
        s_realNativeRequestQueuePop,
        apThis,
        apRequest);
    if (result)
    {
        const NativeRequestObservation request = ObserveRequest(apRequest);
        CampaignSaveProvenance::ObserveQuickRequestPop(
            ObserveRequestPointer(apRequest),
            request.Readable ? request.OperationCode : 0);
        if (IsProcessRequestQueue(apThis) && request.Readable)
        {
            const std::optional<ContinueCorrelation> correlation =
                FindContinueCorrelation(request.pRequest);
            if (correlation)
            {
                // ID 35772 decrements requests whose state is greater than
                // one and requeues them. Only state 0/1 enters the operation
                // dispatch branch, so keep requeues correlated without
                // claiming that their semantic consumer ran.
                if (request.State <= 1)
                {
                    s_activeContinueDispatch = ActiveContinueDispatch{
                        *correlation,
                        request,
                        ObserveNativeString(request.pNativeSaveIdentity),
                        GetProcessQueueManager(apThis)};
                    LogContinueCorrelation(
                        "BeforeDispatch",
                        *correlation,
                        request,
                        s_activeContinueDispatch->pManager);
                }
                else
                {
                    LogContinueCorrelation(
                        "DeferredRequeue",
                        *correlation,
                        request,
                        GetProcessQueueManager(apThis));
                }
            }
        }
        LogRequestQueueOperation("Pop", apThis, apRequest, result);
    }
    return result;
}

const void* ObserveResolvedLoadSource(
    const NativeResolvedLoadSourceRef* apSource) noexcept
{
    __try
    {
        return apSource ? apSource->pSource : nullptr;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return nullptr;
    }
}

bool ConsumeBlockedContinueLoad(
    CampaignLoadDecision aDecision,
    const char* apReason) noexcept
{
    const CampaignLoadUiAction action = ProjectCampaignLoadUiAction(
        aDecision, CampaignLoadUiHost::MainMenu);
    const bool rebuildRequested =
        action == CampaignLoadUiAction::ConsumeAndRebuildMainMenu &&
        RequestMainMenuRebuild();
    NotifyPlayerLoadBlocked();
    const CampaignSaveTrace::Context context = CampaignSaveTrace::Capture();
    spdlog::warn(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=MainMenu.ContinueRequest event=Consumed frame={} "
        "thread={} reason={} decision={} nativeDispatch=false "
        "mainMenuRebuildRequested={}",
        context.Sequence,
        context.Frame,
        context.Thread,
        apReason,
        LoadDecisionName(aDecision),
        rebuildRequested);
    return true;
}

bool TP_MAKE_THISCALL(
    HookNativeLoadRequestDispatch,
    void,
    NativeResolvedLoadSourceRef* apSource,
    std::uint32_t aValue20,
    bool aValue24,
    bool aValue25,
    std::uint32_t aOperationCode)
{
    if (!s_activeContinueDispatch)
    {
        return TiltedPhoques::ThisCall(
            s_realNativeLoadRequestDispatch,
            apThis,
            apSource,
            aValue20,
            aValue24,
            aValue25,
            aOperationCode);
    }

    const ActiveContinueDispatch dispatch = *s_activeContinueDispatch;
    if (ObserveResolvedLoadSource(apSource) !=
        dispatch.Request.pResolvedLoadSource)
    {
        return TiltedPhoques::ThisCall(
            s_realNativeLoadRequestDispatch,
            apThis,
            apSource,
            aValue20,
            aValue24,
            aValue25,
            aOperationCode);
    }

    std::optional<CampaignContinueLoadClaim> claim =
        ClaimContinueLoadTarget(dispatch);
    if (!claim)
    {
        return TiltedPhoques::ThisCall(
            s_realNativeLoadRequestDispatch,
            apThis,
            apSource,
            aValue20,
            aValue24,
            aValue25,
            aOperationCode);
    }

    CampaignResumeService* const pResume = CampaignResumeService::TryGet();
    CampaignNativeLoadService* const pLoad =
        CampaignNativeLoadService::TryGet();
    CampaignRuntimeGateService* const pGate =
        CampaignRuntimeGateService::TryGet();
    const CampaignLoadTarget target = !claim->TargetReadable
        ? CampaignLoadTarget::Unknown
        : !claim->NativeSaveIdentity.starts_with("stre-")
            ? CampaignLoadTarget::Ordinary
            : pResume
                ? pResume->InspectNativeLoadTarget(
                    claim->NativeSaveIdentity)
                : CampaignLoadTarget::Unknown;
    const bool admitted = pLoad && pLoad->HasAuthoritativeAdmission();
    const bool gateLocked = pGate && pGate->IsLocked();
    CampaignLoadDecision decision = EvaluateCampaignContinueLoad(
        *claim, target, admitted || gateLocked);

    const CampaignSaveTrace::Context context = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=MainMenu.ContinueRequest event=TargetResolved frame={} "
        "thread={} rootRequest={} requestAddress={} target={} "
        "nativeIdentity={} targetReadable={} admission={} gateLocked={} "
        "classifiedTarget={} decision={}",
        context.Sequence,
        context.Frame,
        context.Thread,
        dispatch.Correlation.pRoot,
        dispatch.Correlation.pRequest,
        dispatch.RequestTarget.Readable
            ? dispatch.RequestTarget.Value
            : "unavailable",
        claim->TargetReadable
            ? claim->NativeSaveIdentity
            : "unavailable",
        claim->TargetReadable,
        admitted,
        gateLocked,
        LoadTargetName(target),
        LoadDecisionName(decision));

    // The policy decision belongs to this exact semantic attempt. Stop all
    // propagation before invoking Skyrim so requeues or downstream requests
    // cannot arm ResumeRequired a second time.
    CompleteContinueLoadAttempt(dispatch.Correlation.pRoot);

    if (decision == CampaignLoadDecision::AllowVanilla)
    {
        return TiltedPhoques::ThisCall(
            s_realNativeLoadRequestDispatch,
            apThis,
            apSource,
            aValue20,
            aValue24,
            aValue25,
            aOperationCode);
    }

    if (decision == CampaignLoadDecision::BeginResumeRequired)
    {
        const bool pending = pResume && claim->TargetReadable &&
            pResume->BeginContinueLoadPending(
                claim->NativeSaveIdentity);
        if (!pending)
        {
            decision = CampaignLoadDecision::BlockUnprovenCampaignTarget;
            return ConsumeBlockedContinueLoad(
                decision, "resume-required-pending-unavailable");
        }

        const bool result = TiltedPhoques::ThisCall(
            s_realNativeLoadRequestDispatch,
            apThis,
            apSource,
            aValue20,
            aValue24,
            aValue25,
            aOperationCode);
        (void)pResume->ObserveContinueNativeResult(result);
        return result;
    }

    if (decision == CampaignLoadDecision::AllowInternalRecovery)
    {
        decision = CampaignLoadDecision::BlockUnprovenCampaignTarget;
        return ConsumeBlockedContinueLoad(
            decision, "player-continue-cannot-own-internal-recovery");
    }

    return ConsumeBlockedContinueLoad(decision, "campaign-load-policy");
}
static_assert(std::is_same_v<
    decltype(&HookNativeLoadRequestDispatch),
    TNativeLoadRequestDispatch*>);

void __fastcall HookManualLoadGameCallback(
    const NativeFxDelegateArgs& acArguments)
{
    const std::optional<std::uint32_t> selectionIndex =
        ReadSelectionIndex(acArguments);
    const std::optional<std::string> target = selectionIndex
        ? CampaignNativeLoad::InspectSaveListTarget(*selectionIndex)
        : std::nullopt;

    CampaignNativeLoadService* const pLoad =
        CampaignNativeLoadService::TryGet();
    CampaignRuntimeGateService* const pGate =
        CampaignRuntimeGateService::TryGet();
    CampaignResumeService* const pResume = CampaignResumeService::TryGet();
    const bool admitted = pLoad && pLoad->HasAuthoritativeAdmission();
    const bool gateLocked = pGate && pGate->IsLocked();
    const CampaignLoadTarget classifiedTarget = pResume && target
        ? pResume->InspectNativeLoadTarget(*target)
        : CampaignLoadTarget::Unknown;

    CampaignLoadPolicyContext policyContext;
    policyContext.Target = classifiedTarget;
    policyContext.CampaignRuntimeSensitive = admitted || gateLocked;
    policyContext.ReservedCampaignNamespaceClaim =
        target && target->starts_with("stre-");
    const CampaignLoadDecision decision =
        EvaluateCampaignLoadPolicy(policyContext);
    CampaignLoadUiHost uiHost = CampaignLoadUiHost::None;
    if (UI* const pUi = UI::Get())
    {
        if (pUi->GetMenuOpen(BSFixedString(kMainMenuName)))
            uiHost = CampaignLoadUiHost::MainMenu;
        else if (pUi->GetMenuOpen(BSFixedString(kJournalMenuName)))
            uiHost = CampaignLoadUiHost::Journal;
    }
    const CampaignLoadUiAction uiAction =
        ProjectCampaignLoadUiAction(decision, uiHost);

    const CampaignSaveTrace::Context enter = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=UISaveLoadManager.LoadGameCallback event=Enter frame={} "
        "thread={} argumentCount={} selectionReadable={} selection={} "
        "targetReadable={} target={} admission={} gateLocked={} "
        "classifiedTarget={} decision={} uiHost={}",
        enter.Sequence,
        enter.Frame,
        enter.Thread,
        acArguments.ArgumentCount,
        selectionIndex.has_value(),
        selectionIndex.value_or(0),
        target.has_value(),
        target ? target->c_str() : "unavailable",
        admitted,
        gateLocked,
        LoadTargetName(classifiedTarget),
        LoadDecisionName(decision),
        LoadUiHostName(uiHost));

    if (uiAction != CampaignLoadUiAction::ForwardNative)
    {
        const CampaignSaveTrace::Context consumed =
            CampaignSaveTrace::Capture();
        spdlog::warn(
            "[STRE][CampaignLoadTrace] sequence={} "
            "source=UISaveLoadManager.LoadGameCallback event=Consumed "
            "frame={} thread={} target={} decision={}",
            consumed.Sequence,
            consumed.Frame,
            consumed.Thread,
            target ? target->c_str() : "unavailable",
            LoadDecisionName(decision));

        // The owning SWF has already disabled its save-list selection before
        // invoking this void callback. There is no native response/cancel
        // channel, so a rejected operation must finish through normal public
        // menu messages rather than leaving that private SWF state alive.
        const bool uxRequested =
            uiAction == CampaignLoadUiAction::ConsumeAndRebuildMainMenu
            ? RequestMainMenuRebuild()
            : RequestJournalClose();
        NotifyPlayerLoadBlocked();
        const CampaignSaveTrace::Context ux = CampaignSaveTrace::Capture();
        if (uiAction == CampaignLoadUiAction::ConsumeAndRebuildMainMenu)
        {
            spdlog::info(
                "[STRE][CampaignLoadTrace] sequence={} "
                "source=UISaveLoadManager.LoadGameCallback "
                "event=MainMenuRebuildRequested frame={} thread={} "
                "rebuildRequested={} messageTypes=Hide,Show notification={}",
                ux.Sequence,
                ux.Frame,
                ux.Thread,
                uxRequested,
                kPlayerLoadBlockedTranslation);
        }
        else
        {
            spdlog::info(
                "[STRE][CampaignLoadTrace] sequence={} "
                "source=UISaveLoadManager.LoadGameCallback "
                "event=JournalCloseRequested frame={} thread={} "
                "closeRequested={} messageType=Hide notification={}",
                ux.Sequence,
                ux.Frame,
                ux.Thread,
                uxRequested,
                kPlayerLoadBlockedTranslation);
        }
        return;
    }

    s_realManualLoadGameCallback(acArguments);

    const CampaignSaveTrace::Context exit = CampaignSaveTrace::Capture();
    spdlog::debug(
        "[STRE][CampaignLoadTrace] sequence={} "
        "source=UISaveLoadManager.LoadGameCallback event=Exit frame={} "
        "thread={} target={} decision={}",
        exit.Sequence,
        exit.Frame,
        exit.Thread,
        target ? target->c_str() : "unavailable",
        LoadDecisionName(decision));
}
static_assert(std::is_same_v<
    decltype(&HookManualLoadGameCallback),
    TManualLoadGameCallback*>);

void __fastcall HookManualSaveGameCallback(
    const NativeFxDelegateArgs& acArguments)
{
    const NativeGfxValue* const pSelection =
        acArguments.ArgumentCount > 0
        ? acArguments.pArguments
        : nullptr;
    const bool selectionReadable =
        pSelection &&
        (pSelection->Type & kGfxTypeMask) == kGfxNumberType;
    const double selection = selectionReadable
        ? pSelection->Value.Number
        : 0.0;
    const char* const pSelectionKind = !selectionReadable
        ? "Unavailable"
        : selection == 0.0
            ? "NewSlot"
            : "ExistingSlot";

    const CampaignSaveTrace::Context enter = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignSaveTrace] sequence={} "
        "source=UISaveLoadManager.SaveGameCallback event=Enter frame={} "
        "thread={} argumentCount={} selectionReadable={} selectionKind={} "
        "selection={}",
        enter.Sequence,
        enter.Frame,
        enter.Thread,
        acArguments.ArgumentCount,
        selectionReadable,
        pSelectionKind,
        selection);

    if (selectionReadable && selection == 0.0)
    {
        const ScopedCampaignManualNewSlotSave manualNewSlotSave;
        s_realManualSaveGameCallback(acArguments);
    }
    else
    {
        s_realManualSaveGameCallback(acArguments);
    }

    const CampaignSaveTrace::Context exit = CampaignSaveTrace::Capture();
    spdlog::debug(
        "[STRE][CampaignSaveTrace] sequence={} "
        "source=UISaveLoadManager.SaveGameCallback event=Exit frame={} "
        "thread={} selectionKind={}",
        exit.Sequence,
        exit.Frame,
        exit.Thread,
        pSelectionKind);
}
static_assert(std::is_same_v<
    decltype(&HookManualSaveGameCallback),
    TManualSaveGameCallback*>);

static TiltedPhoques::Initializer s_campaignSaveTraceMenuControlsHook(
    []()
    {
        // CommonLibSSE-NG VTABLE_MenuControls[0], AE Address Library ID.
        // Virtual 01 is the public InputEvent* ProcessEvent override.
        POINTER_SKYRIMSE(void, menuControlsVtable, 215773);
        auto** const ppVtable = static_cast<void**>(
            menuControlsVtable.GetPtr());
        if (!ppVtable || !ppVtable[1])
            return;
        s_realMenuControlsProcessEvent =
            reinterpret_cast<TMenuControlsProcessEvent*>(ppVtable[1]);
        TP_HOOK(
            &s_realMenuControlsProcessEvent,
            HookMenuControlsProcessEvent);
    });

static TiltedPhoques::Initializer s_campaignSaveTraceNativeHooks(
    []()
    {
        // QuickSaveLoadHandler derives from the public MenuEventHandler ABI;
        // ID 52251 is its exact ProcessButton override on AE 1.6.1170.
        POINTER_SKYRIMSE(
            TQuickSaveLoadProcessButton,
            quickSaveLoadProcessButton,
            52251);
        s_realQuickSaveLoadProcessButton =
            quickSaveLoadProcessButton.Get();
        if (s_realQuickSaveLoadProcessButton)
        {
            TP_HOOK(
                &s_realQuickSaveLoadProcessButton,
                HookQuickSaveLoadProcessButton);
        }

        // This is the exact bgs::saveload::Request creation/queue boundary.
        POINTER_SKYRIMSE(
            TQueueNativeSaveLoadRequest,
            queueNativeSaveLoadRequest,
            35769);
        s_realQueueNativeSaveLoadRequest =
            queueNativeSaveLoadRequest.Get();
        if (s_realQueueNativeSaveLoadRequest)
        {
            TP_HOOK(
                &s_realQueueNativeSaveLoadRequest,
                HookQueueNativeSaveLoadRequest);
        }

        // CommonLibSSE-NG exposes this exact static queue specialization and
        // its PushInternal/PopInternal virtual ABI. Logging only successful
        // pops avoids the per-frame noise of hooking process boundary 35772.
        POINTER_SKYRIMSE(void, nativeRequestQueueVtable, 206618);
        auto** const ppRequestQueueVtable = static_cast<void**>(
            nativeRequestQueueVtable.GetPtr());
        if (ppRequestQueueVtable)
        {
            s_realNativeRequestQueuePush =
                reinterpret_cast<TNativeRequestQueueOperation*>(
                    ppRequestQueueVtable[5]);
            s_realNativeRequestQueuePop =
                reinterpret_cast<TNativeRequestQueueOperation*>(
                    ppRequestQueueVtable[6]);
            if (s_realNativeRequestQueuePush)
            {
                TP_HOOK(
                    &s_realNativeRequestQueuePush,
                    HookNativeRequestQueuePush);
            }
            if (s_realNativeRequestQueuePop)
            {
                TP_HOOK(
                    &s_realNativeRequestQueuePop,
                    HookNativeRequestQueuePop);
            }
        }

        // AE ID 442580 is the exact typed LoadRequest consumer called by the
        // 0xD0000010 branch of ID 35772. The hook never derives authority from
        // that opcode: it acts only while the exact request pointer is the
        // first LoadRequest child of a live-proven Continue root.
        POINTER_SKYRIMSE(
            TNativeLoadRequestDispatch,
            nativeLoadRequestDispatch,
            442580);
        s_realNativeLoadRequestDispatch = nativeLoadRequestDispatch.Get();
        if (s_realNativeLoadRequestDispatch)
        {
            TP_HOOK(
                &s_realNativeLoadRequestDispatch,
                HookNativeLoadRequestDispatch);
        }

        // FxDelegateHandler::CallbackFn is public; ID 52915 is the exact
        // SaveGame callback adapter registered by the save/load panel.
        POINTER_SKYRIMSE(
            TManualSaveGameCallback,
            manualSaveGameCallback,
            52915);
        s_realManualSaveGameCallback = manualSaveGameCallback.Get();
        if (s_realManualSaveGameCallback)
        {
            TP_HOOK(
                &s_realManualSaveGameCallback,
                HookManualSaveGameCallback);
        }

        // UISaveLoadManager::Accept registers the literal Scaleform callback
        // "LoadGame" on AE adapter ID 52914. Its original body receives the
        // selected save-list index and creates the native load operation, so
        // consuming here precedes the Journal transition/fade.
        POINTER_SKYRIMSE(
            TManualLoadGameCallback,
            manualLoadGameCallback,
            52914);
        s_realManualLoadGameCallback = manualLoadGameCallback.Get();
        if (s_realManualLoadGameCallback)
        {
            TP_HOOK(
                &s_realManualLoadGameCallback,
                HookManualLoadGameCallback);
        }

        POINTER_SKYRIMSE(
            void*,
            saveLoadManagerSingleton,
            403340);
        s_saveLoadManagerSingleton = saveLoadManagerSingleton.Get();

        // Public CommonLibSSE-NG VTABLE identifiers for the exact request
        // classes observed by the queue hook. Comparisons are diagnostic only.
        POINTER_SKYRIMSE(void, requestVtable, 206668);
        POINTER_SKYRIMSE(void, loadRequestVtable, 206600);
        POINTER_SKYRIMSE(void, loadEntryRequestVtable, 206602);
        POINTER_SKYRIMSE(void, saveOperationRequestVtable, 206598);
        POINTER_SKYRIMSE(void, buildSaveListRequestVtable, 216371);
        s_requestVtable = requestVtable.GetPtr();
        s_loadRequestVtable = loadRequestVtable.GetPtr();
        s_loadEntryRequestVtable = loadEntryRequestVtable.GetPtr();
        s_saveOperationRequestVtable = saveOperationRequestVtable.GetPtr();
        s_buildSaveListRequestVtable = buildSaveListRequestVtable.GetPtr();
    });

template <class T>
EventDispatcher<T>* EventSourceAt(
    void* apObject,
    std::ptrdiff_t aOffset) noexcept
{
    if (!apObject)
        return nullptr;
    return reinterpret_cast<EventDispatcher<T>*>(
        static_cast<std::byte*>(apObject) + aOffset);
}

void LogOpaqueEvent(
    const char* acSource,
    bool aPointerPresent) noexcept
{
    const CampaignSaveTrace::Context context = CampaignSaveTrace::Capture();
    spdlog::debug(
        "[STRE][CampaignSaveTrace] sequence={} source={} event=Dispatch "
        "frame={} thread={} eventPointer={} payload=opaque",
        context.Sequence,
        acSource,
        context.Frame,
        context.Thread,
        aPointerPresent ? "present" : "null");
}

class InputEventSink final : public BSTEventSink<InputEvent*>
{
public:
    BSTEventResult OnEvent(
        InputEvent* const* appEvent,
        const EventDispatcher<InputEvent*>*) override
    {
        for (const InputEvent* pEvent = appEvent ? *appEvent : nullptr;
             pEvent;
             pEvent = pEvent->pNext)
        {
            const char* const pAction = pEvent->QUserEvent().AsAscii();
            const bool quicksave =
                pAction && std::strcmp(pAction, "Quicksave") == 0;
            const bool newSave =
                pAction && std::strcmp(pAction, "NewSave") == 0;
            if (!quicksave && !newSave)
                continue;

            const CampaignSaveTrace::Context context =
                CampaignSaveTrace::Capture();
            if (pEvent->EventType == 0)
            {
                const auto* const pButton =
                    static_cast<const ButtonInputEvent*>(pEvent);
                spdlog::debug(
                    "[STRE][CampaignSaveTrace] sequence={} "
                    "source=BSInputDeviceManager event={} frame={} "
                    "thread={} device={} inputEventType={} hasIdCode={} "
                    "idCode={} value={} heldDownSeconds={}",
                    context.Sequence,
                    quicksave ? "Quicksave" : "NewSave",
                    context.Frame,
                    context.Thread,
                    pEvent->Device,
                    pEvent->EventType,
                    pEvent->HasIDCode(),
                    pButton->IdCode,
                    pButton->Value,
                    pButton->HeldDownSeconds);
                continue;
            }

            spdlog::debug(
                "[STRE][CampaignSaveTrace] sequence={} "
                "source=BSInputDeviceManager event={} frame={} thread={} "
                "device={} inputEventType={} hasIdCode={}",
                context.Sequence,
                quicksave ? "Quicksave" : "NewSave",
                context.Frame,
                context.Thread,
                pEvent->Device,
                pEvent->EventType,
                pEvent->HasIDCode());
        }
        return BSTEventResult::kOk;
    }
};

class SaveDataEventSink final : public BSTEventSink<BSSaveDataEvent>
{
public:
    BSTEventResult OnEvent(
        const BSSaveDataEvent* apEvent,
        const EventDispatcher<BSSaveDataEvent>*) override
    {
        LogOpaqueEvent("BSSaveDataSystemUtility", apEvent != nullptr);
        return BSTEventResult::kOk;
    }
};

class SaveLoadManagerEventSink final
    : public BSTEventSink<BGSSaveLoadManagerEvent>
{
public:
    BSTEventResult OnEvent(
        const BGSSaveLoadManagerEvent* apEvent,
        const EventDispatcher<BGSSaveLoadManagerEvent>*) override
    {
        LogOpaqueEvent("BGSSaveLoadManagerEventSource", apEvent != nullptr);
        return BSTEventResult::kOk;
    }
};

class JournalMenuEventSink final : public BSTEventSink<MenuOpenCloseEvent>
{
public:
    explicit JournalMenuEventSink(entt::dispatcher& aDispatcher) noexcept
        : m_dispatcher(aDispatcher)
    {
    }

    BSTEventResult OnEvent(
        const MenuOpenCloseEvent* apEvent,
        const EventDispatcher<MenuOpenCloseEvent>*) override
    {
        if (!apEvent)
            return BSTEventResult::kOk;

        const char* const pMenuName = apEvent->MenuName.AsAscii();
        if (!pMenuName)
            return BSTEventResult::kOk;

        const bool journal = std::strcmp(pMenuName, "Journal Menu") == 0;
        const bool mainMenu = std::strcmp(pMenuName, "Main Menu") == 0;
        if (!journal && !mainMenu)
            return BSTEventResult::kOk;

        const CampaignSaveTrace::Context context =
            CampaignSaveTrace::Capture();
        if (mainMenu)
        {
            spdlog::info(
                "[STRE][CampaignLoadTrace] sequence={} "
                "source=UI.MenuOpenCloseEvent event={} frame={} thread={} "
                "contextOnly={}",
                context.Sequence,
                apEvent->Opening ? "MainMenuOpened" : "MainMenuClosed",
                context.Frame,
                context.Thread,
                apEvent->Opening);
            if (!apEvent->Opening)
            {
                if (CampaignResumeService* const pResume =
                        CampaignResumeService::TryGet())
                {
                    (void)pResume->CommitContinueMainMenuClosed();
                }
            }
            if (apEvent->Opening)
                m_dispatcher.trigger<CampaignMainMenuEnteredEvent>();
            return BSTEventResult::kOk;
        }

        spdlog::info(
            "[STRE][CampaignLoadTrace] sequence={} "
            "source=UI.MenuOpenCloseEvent event={} frame={} thread={} "
            "contextOnly=true",
            context.Sequence,
            apEvent->Opening ? "JournalOpened" : "JournalClosed",
            context.Frame,
            context.Thread);
        return BSTEventResult::kOk;
    }

private:
    entt::dispatcher& m_dispatcher;
};
}

struct CampaignSaveTraceService::Detail
{
    explicit Detail(entt::dispatcher& aDispatcher) noexcept
        : JournalMenuSink(aDispatcher)
    {
    }

    InputEventSink InputSink;
    SaveDataEventSink SaveDataSink;
    SaveLoadManagerEventSink SaveLoadManagerSink;
    JournalMenuEventSink JournalMenuSink;

    EventDispatcher<InputEvent*>* pInputSource{};
    EventDispatcher<BSSaveDataEvent>* pSaveDataSource{};
    EventDispatcher<BGSSaveLoadManagerEvent>* pSaveLoadManagerSource{};
    EventDispatcher<MenuOpenCloseEvent>* pMenuSource{};
};

CampaignSaveTraceService::CampaignSaveTraceService(
    entt::dispatcher& aDispatcher) noexcept
    : m_detail(std::make_unique<Detail>(aDispatcher))
{
    m_preUpdateConnection = aDispatcher.sink<PreUpdateEvent>()
                                .connect<&CampaignSaveTraceService::OnPreUpdate>(
                                    this);

    // CommonLibSSE-NG BSInputDeviceManager::Singleton (AE Address Library).
    POINTER_SKYRIMSE(void*, inputManagerSingleton, 402776);
    void* const pInputManager = inputManagerSingleton.Get()
        ? *inputManagerSingleton.Get()
        : nullptr;
    m_detail->pInputSource =
        EventSourceAt<InputEvent*>(pInputManager, 0x0);

    // CommonLibSSE-NG BSWin32SaveDataSystemUtility::GetSingleton.
    using TGetSaveDataSystemUtility = void*();
    POINTER_SKYRIMSE(
        TGetSaveDataSystemUtility,
        getSaveDataSystemUtility,
        109278);
    void* const pSaveDataSystemUtility = getSaveDataSystemUtility.Get()
        ? getSaveDataSystemUtility.Get()()
        : nullptr;
    m_detail->pSaveDataSource = EventSourceAt<BSSaveDataEvent>(
        pSaveDataSystemUtility,
        kSaveDataEventSourceOffset);

    // CommonLibSSE-NG BGSSaveLoadManager::Singleton (AE Address Library).
    POINTER_SKYRIMSE(void*, saveLoadManagerSingleton, 403340);
    void* const pSaveLoadManager = saveLoadManagerSingleton.Get()
        ? *saveLoadManagerSingleton.Get()
        : nullptr;
    m_detail->pSaveLoadManagerSource =
        EventSourceAt<BGSSaveLoadManagerEvent>(
            pSaveLoadManager,
            kSaveLoadManagerEventSourceOffset);

    m_detail->pMenuSource = EventSourceAt<MenuOpenCloseEvent>(
        UI::Get(),
        kUiMenuEventSourceOffset);

    if (m_detail->pInputSource)
        m_detail->pInputSource->RegisterSink(&m_detail->InputSink);
    if (m_detail->pSaveDataSource)
        m_detail->pSaveDataSource->RegisterSink(&m_detail->SaveDataSink);
    if (m_detail->pSaveLoadManagerSource)
    {
        m_detail->pSaveLoadManagerSource->RegisterSink(
            &m_detail->SaveLoadManagerSink);
    }
    if (m_detail->pMenuSource)
        m_detail->pMenuSource->RegisterSink(&m_detail->JournalMenuSink);

    const CampaignSaveTrace::Context context = CampaignSaveTrace::Capture();
    spdlog::info(
        "[STRE][CampaignSaveTrace] sequence={} source=TraceService "
        "event=Initialized frame={} thread={} input={} saveData={} "
        "saveLoadManager={} menu={}",
        context.Sequence,
        context.Frame,
        context.Thread,
        m_detail->pInputSource != nullptr,
        m_detail->pSaveDataSource != nullptr,
        m_detail->pSaveLoadManagerSource != nullptr,
        m_detail->pMenuSource != nullptr);
}

CampaignSaveTraceService::~CampaignSaveTraceService() noexcept
{
    if (m_detail->pMenuSource)
        m_detail->pMenuSource->UnRegisterSink(&m_detail->JournalMenuSink);
    if (m_detail->pSaveLoadManagerSource)
    {
        m_detail->pSaveLoadManagerSource->UnRegisterSink(
            &m_detail->SaveLoadManagerSink);
    }
    if (m_detail->pSaveDataSource)
        m_detail->pSaveDataSource->UnRegisterSink(&m_detail->SaveDataSink);
    if (m_detail->pInputSource)
        m_detail->pInputSource->UnRegisterSink(&m_detail->InputSink);
}

void CampaignSaveTraceService::OnPreUpdate(const PreUpdateEvent&) noexcept
{
    CampaignSaveTrace::AdvanceFrame();
}
