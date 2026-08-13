#include <Campaign/CampaignRuntimeGate.h>

namespace STRE::Campaign
{
bool CampaignRuntimeGate::ArmNextLoad() noexcept
{
    if (IsLocked() || GetState() == CampaignRuntimeGateState::ArmedDuringLoad)
        return false;

    bool expected = false;
    return m_nextLoadManaged.compare_exchange_strong(expected, true);
}

bool CampaignRuntimeGate::OnNativeLoadEnter() noexcept
{
    if (!m_nextLoadManaged.exchange(false))
        return false;

    m_guardMenuObserved = false;
    m_cefPresentationObserved = false;
    m_state = CampaignRuntimeGateState::ArmedDuringLoad;
    return true;
}

void CampaignRuntimeGate::OnNativeLoadReturn(bool aSucceeded) noexcept
{
    if (aSucceeded)
        return;

    CampaignRuntimeGateState expected =
        CampaignRuntimeGateState::ArmedDuringLoad;
    m_state.compare_exchange_strong(
        expected,
        CampaignRuntimeGateState::Open);
}

bool CampaignRuntimeGate::OnPostLoad() noexcept
{
    CampaignRuntimeGateState expected =
        CampaignRuntimeGateState::ArmedDuringLoad;
    return m_state.compare_exchange_strong(
        expected,
        CampaignRuntimeGateState::LockedAfterLoad);
}

bool CampaignRuntimeGate::Release() noexcept
{
    CampaignRuntimeGateState expected =
        CampaignRuntimeGateState::LockedAfterLoad;
    return m_state.compare_exchange_strong(
        expected,
        CampaignRuntimeGateState::Released);
}

void CampaignRuntimeGate::ObserveGuardMenu(bool aActive) noexcept
{
    m_guardMenuObserved = aActive;
}

void CampaignRuntimeGate::ObserveCefPresentation(bool aActive) noexcept
{
    m_cefPresentationObserved = aActive;
}

CampaignRuntimeGateState CampaignRuntimeGate::GetState() const noexcept
{
    return m_state.load();
}

bool CampaignRuntimeGate::IsNextLoadArmed() const noexcept
{
    return m_nextLoadManaged.load();
}

bool CampaignRuntimeGate::IsLocked() const noexcept
{
    return GetState() == CampaignRuntimeGateState::LockedAfterLoad;
}

bool CampaignRuntimeGate::IsGuardMenuObserved() const noexcept
{
    return m_guardMenuObserved.load();
}

bool CampaignRuntimeGate::IsCefPresentationObserved() const noexcept
{
    return m_cefPresentationObserved.load();
}
}
