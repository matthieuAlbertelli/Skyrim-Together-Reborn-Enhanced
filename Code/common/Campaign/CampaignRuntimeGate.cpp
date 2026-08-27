#include <Campaign/CampaignRuntimeGate.h>

namespace STRE::Campaign
{
bool CampaignRuntimeGate::LockForRecovery() noexcept
{
    m_recoveryOwned = true;
    m_guardMenuObserved = false;
    m_cefPresentationObserved = false;

    CampaignRuntimeGateState state = m_state.load();
    while (state == CampaignRuntimeGateState::Open ||
           state == CampaignRuntimeGateState::Released)
    {
        if (m_state.compare_exchange_weak(
                state, CampaignRuntimeGateState::RecoveryLocked))
        {
            return true;
        }
    }
    return state == CampaignRuntimeGateState::RecoveryLocked ||
        state == CampaignRuntimeGateState::ArmedDuringLoad ||
        state == CampaignRuntimeGateState::LockedAfterLoad;
}

bool CampaignRuntimeGate::ArmNextLoad() noexcept
{
    const CampaignRuntimeGateState state = GetState();
    if (state == CampaignRuntimeGateState::ArmedDuringLoad ||
        (state == CampaignRuntimeGateState::LockedAfterLoad &&
         !m_recoveryOwned.load()))
        return false;

    bool expected = false;
    return m_nextLoadManaged.compare_exchange_strong(expected, true);
}

bool CampaignRuntimeGate::ArmResumeRequiredLoad() noexcept
{
    if (!ArmNextLoad())
        return false;
    m_resumeRequiredOwned = true;
    return true;
}

bool CampaignRuntimeGate::CommitResumeRequiredTransition() noexcept
{
    if (!m_resumeRequiredOwned.load() ||
        !m_nextLoadManaged.exchange(false))
    {
        return false;
    }

    CampaignRuntimeGateState state = GetState();
    while (state == CampaignRuntimeGateState::Open ||
           state == CampaignRuntimeGateState::Released)
    {
        if (m_state.compare_exchange_weak(
                state, CampaignRuntimeGateState::LockedAfterLoad))
        {
            m_guardMenuObserved = false;
            m_cefPresentationObserved = false;
            return true;
        }
    }

    m_nextLoadManaged = true;
    return false;
}

bool CampaignRuntimeGate::CancelArmedLoad() noexcept
{
    const bool wasPending = m_nextLoadManaged.exchange(false);
    CampaignRuntimeGateState expected =
        CampaignRuntimeGateState::ArmedDuringLoad;
    const bool wasEntered = m_state.compare_exchange_strong(
        expected,
        m_recoveryOwned ? CampaignRuntimeGateState::RecoveryLocked
                        : CampaignRuntimeGateState::Open);
    if (!m_recoveryOwned.load() &&
        m_state.load() == CampaignRuntimeGateState::Open)
    {
        m_resumeRequiredOwned = false;
    }
    return wasPending || wasEntered;
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
        m_recoveryOwned ? CampaignRuntimeGateState::RecoveryLocked
                        : CampaignRuntimeGateState::Open);
    if (!m_recoveryOwned.load())
        m_resumeRequiredOwned = false;
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
    CampaignRuntimeGateState state = m_state.load();
    while (state == CampaignRuntimeGateState::RecoveryLocked ||
           state == CampaignRuntimeGateState::LockedAfterLoad)
    {
        if (m_state.compare_exchange_weak(
                state, CampaignRuntimeGateState::Released))
        {
            m_nextLoadManaged = false;
            m_recoveryOwned = false;
            m_resumeRequiredOwned = false;
            return true;
        }
    }
    return false;
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
    const CampaignRuntimeGateState state = GetState();
    return state == CampaignRuntimeGateState::RecoveryLocked ||
        state == CampaignRuntimeGateState::LockedAfterLoad ||
        (state == CampaignRuntimeGateState::ArmedDuringLoad &&
         (m_recoveryOwned.load() || m_resumeRequiredOwned.load()));
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
