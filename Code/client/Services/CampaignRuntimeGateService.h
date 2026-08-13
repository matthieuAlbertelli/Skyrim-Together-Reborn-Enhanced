#pragma once

#include <Campaign/CampaignRuntimeGate.h>
#include <Games/Events.h>

#include <chrono>
#include <cstdint>

struct TESLoadGameEvent;
template <class T> struct EventDispatcher;
struct UiSurfaceService;
struct World;

struct CampaignRuntimeGateService final : BSTEventSink<TESLoadGameEvent>
{
    CampaignRuntimeGateService(
        World& aWorld,
        UiSurfaceService& aUiSurface) noexcept;
    ~CampaignRuntimeGateService() noexcept override;

    TP_NOCOPYMOVE(CampaignRuntimeGateService);

    static CampaignRuntimeGateService* TryGet() noexcept;

    void ArmNextLoadForDevelopment() noexcept;
    void ReleaseForDevelopment() noexcept;

    [[nodiscard]] bool OnNativeLoadEnter(const char* apSaveName) noexcept;
    void OnNativeLoadReturn(bool aManaged, bool aSucceeded) noexcept;
    void BeforeWorldUpdate() noexcept;
    void OnTransportUpdate(bool aConnected) noexcept;
    void OnGuardMenuPostDisplay() noexcept;
    void OnGuardMenuDestroyed() noexcept;

    [[nodiscard]] bool IsLocked() const noexcept;

    BSTEventResult OnEvent(
        const TESLoadGameEvent*,
        const EventDispatcher<TESLoadGameEvent>*) override;

private:
    void ApplyInputLock() noexcept;
    void RestoreInputState() noexcept;
    void RequestGuardMenu() noexcept;
    void Log(const char* apEvent, const char* apDetail = "") const noexcept;

    World& m_world;
    UiSurfaceService& m_uiSurface;
    STRE::Campaign::CampaignRuntimeGate m_gate;
    std::chrono::steady_clock::time_point m_startedAt;
    std::uint64_t m_frame{};
    std::uint64_t m_lastTransportLogFrame{};
    bool m_firstWorldUpdatePending{};
    bool m_menuToggleSnapshot{true};
    bool m_inputSnapshotValid{};
};
