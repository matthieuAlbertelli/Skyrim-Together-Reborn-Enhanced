#pragma once

#include <atomic>
#include <cstdint>

namespace STRE::Campaign
{
enum class CampaignRuntimeGateState : std::uint8_t
{
    Open,
    ArmedDuringLoad,
    LockedAfterLoad,
    Released
};

class CampaignRuntimeGate
{
public:
    bool ArmNextLoad() noexcept;
    bool OnNativeLoadEnter() noexcept;
    void OnNativeLoadReturn(bool aSucceeded) noexcept;
    bool OnPostLoad() noexcept;
    bool Release() noexcept;

    void ObserveGuardMenu(bool aActive) noexcept;
    void ObserveCefPresentation(bool aActive) noexcept;

    [[nodiscard]] CampaignRuntimeGateState GetState() const noexcept;
    [[nodiscard]] bool IsNextLoadArmed() const noexcept;
    [[nodiscard]] bool IsLocked() const noexcept;
    [[nodiscard]] bool IsGuardMenuObserved() const noexcept;
    [[nodiscard]] bool IsCefPresentationObserved() const noexcept;

private:
    std::atomic<CampaignRuntimeGateState> m_state{
        CampaignRuntimeGateState::Open};
    std::atomic_bool m_nextLoadManaged{false};
    std::atomic_bool m_guardMenuObserved{false};
    std::atomic_bool m_cefPresentationObserved{false};
};
}
