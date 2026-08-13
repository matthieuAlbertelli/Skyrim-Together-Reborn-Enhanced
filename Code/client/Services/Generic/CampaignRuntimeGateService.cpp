#include <TiltedOnlinePCH.h>

#include <Services/CampaignRuntimeGateService.h>

#include <DInputHook.hpp>
#include <Games/Skyrim/Events/EventDispatcher.h>
#include <Games/Skyrim/Interface/MenuControls.h>
#include <Games/Skyrim/Interface/Menus/CampaignGateMenu.h>
#include <Games/Skyrim/Interface/UI.h>
#include <Services/TransportService.h>
#include <Services/UiSurfaceService.h>
#include <World.h>

namespace
{
CampaignRuntimeGateService* s_pCampaignRuntimeGateService{};
}

CampaignRuntimeGateService::CampaignRuntimeGateService(
    World& aWorld,
    UiSurfaceService& aUiSurface) noexcept
    : m_world(aWorld)
    , m_uiSurface(aUiSurface)
    , m_startedAt(std::chrono::steady_clock::now())
{
    s_pCampaignRuntimeGateService = this;

    if (EventDispatcherManager* const pEvents = EventDispatcherManager::Get())
        pEvents->loadGameEvent.RegisterSink(this);

    Log("Initialized", "devKeys=F9-arm,F10-release");
}

CampaignRuntimeGateService::~CampaignRuntimeGateService() noexcept
{
    if (EventDispatcherManager* const pEvents = EventDispatcherManager::Get())
        pEvents->loadGameEvent.UnRegisterSink(this);

    if (s_pCampaignRuntimeGateService == this)
        s_pCampaignRuntimeGateService = nullptr;
}

CampaignRuntimeGateService* CampaignRuntimeGateService::TryGet() noexcept
{
    return s_pCampaignRuntimeGateService;
}

void CampaignRuntimeGateService::ArmNextLoadForDevelopment() noexcept
{
#if (!IS_MASTER)
    const bool armed = m_gate.ArmNextLoad();
    Log(armed ? "DevArmRequested" : "DevArmIgnored",
        "nextLoad=CampaignManaged");
#endif
}

void CampaignRuntimeGateService::ReleaseForDevelopment() noexcept
{
#if (!IS_MASTER)
    if (!m_gate.Release())
    {
        Log("DevReleaseIgnored", "gateNotLocked=true");
        return;
    }

    Log("Released", "authority=explicitDevRelease");
    CampaignGateMenu::Hide();
    RestoreInputState();
#endif
}

bool CampaignRuntimeGateService::OnNativeLoadEnter(
    const char* apSaveName) noexcept
{
    if (!m_gate.OnNativeLoadEnter())
        return false;

    Log("PreLoad", apSaveName ? apSaveName : "saveNameUnavailable");
    Log("GateArmed", "state=ArmedDuringLoad");
    Log("NativeLoadEnter",
        "boundary=BGSSaveLoadManager::Load_Impl");
    return true;
}

void CampaignRuntimeGateService::OnNativeLoadReturn(
    bool aManaged,
    bool aSucceeded) noexcept
{
    if (!aManaged)
        return;

    Log("NativeLoadReturn",
        aSucceeded
            ? "boundary=BGSSaveLoadManager::Load_Impl result=true"
            : "boundary=BGSSaveLoadManager::Load_Impl result=false");
    m_gate.OnNativeLoadReturn(aSucceeded);
}

BSTEventResult CampaignRuntimeGateService::OnEvent(
    const TESLoadGameEvent*,
    const EventDispatcher<TESLoadGameEvent>*)
{
    if (!m_gate.OnPostLoad())
        return BSTEventResult::kOk;

    m_firstWorldUpdatePending = true;
    Log("PostLoad", "boundary=TESLoadGameEvent");
    ApplyInputLock();
    RequestGuardMenu();
    return BSTEventResult::kOk;
}

void CampaignRuntimeGateService::BeforeWorldUpdate() noexcept
{
    ++m_frame;
    m_gate.ObserveCefPresentation(m_uiSurface.IsInteractive());

    if (!m_gate.IsLocked())
        return;

    ApplyInputLock();

    UI* const pUI = UI::Get();
    const bool menuOpen =
        pUI && pUI->GetMenuOpen(CampaignGateMenu::GetName());
    if (!menuOpen)
        RequestGuardMenu();

    if (m_firstWorldUpdatePending)
    {
        m_firstWorldUpdatePending = false;
        const bool paused = pUI && pUI->GameIsPaused();
        Log("FirstWorldUpdateAfterLoad",
            paused ? "GameIsPaused=true" : "GameIsPaused=false");
    }
}

void CampaignRuntimeGateService::OnTransportUpdate(bool aConnected) noexcept
{
    if (!m_gate.IsLocked())
        return;

    if (m_lastTransportLogFrame == 0 ||
        m_frame - m_lastTransportLogFrame >= 300)
    {
        m_lastTransportLogFrame = m_frame;
        Log("TransportUpdateWhileLocked",
            aConnected ? "connected=true" : "connected=false");
    }
}

void CampaignRuntimeGateService::OnGuardMenuPostDisplay() noexcept
{
    m_gate.ObserveGuardMenu(true);
    Log("PauseMenuPostDisplay", "menu=STRECampaignGateMenu");

    UI* const pUI = UI::Get();
    Log("GameIsPaused",
        pUI && pUI->GameIsPaused()
            ? "value=true"
            : "value=false");
}

void CampaignRuntimeGateService::OnGuardMenuDestroyed() noexcept
{
    m_gate.ObserveGuardMenu(false);
    Log("PauseMenuDestroyed",
        m_gate.IsLocked()
            ? "gateRemainsLocked=true"
            : "gateRemainsLocked=false");
}

bool CampaignRuntimeGateService::IsLocked() const noexcept
{
    return m_gate.IsLocked();
}

void CampaignRuntimeGateService::ApplyInputLock() noexcept
{
    if (MenuControls* const pControls = MenuControls::GetInstance())
    {
        if (!m_inputSnapshotValid)
        {
            m_menuToggleSnapshot = pControls->GetToggle();
            m_inputSnapshotValid = true;
        }
        pControls->SetToggle(false);
    }

    TiltedPhoques::DInputHook::Get().SetEnabled(true);
}

void CampaignRuntimeGateService::RestoreInputState() noexcept
{
    if (m_inputSnapshotValid)
    {
        if (MenuControls* const pControls = MenuControls::GetInstance())
            pControls->SetToggle(m_menuToggleSnapshot);
    }
    m_inputSnapshotValid = false;

    m_uiSurface.RefreshInputCapture();
}

void CampaignRuntimeGateService::RequestGuardMenu() noexcept
{
    Log("PauseMenuRequested", "menu=STRECampaignGateMenu");
    CampaignGateMenu::Show();
}

void CampaignRuntimeGateService::Log(
    const char* apEvent,
    const char* apDetail) const noexcept
{
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - m_startedAt);
    spdlog::info(
        "[STRE][CampaignGate] {} frame={} tick={} monotonic_us={} {}",
        apEvent,
        m_frame,
        m_world.GetTick(),
        elapsed.count(),
        apDetail);
}
