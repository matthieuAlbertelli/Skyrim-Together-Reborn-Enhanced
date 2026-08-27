#include <CampaignRecoveryUiState.h>

#include <Structs/Campaign.h>

#include <algorithm>

namespace STRE::Campaign
{
bool CampaignRecoveryUiState::ObserveSnapshot(
    std::string_view acCampaignId,
    std::uint8_t aRuntimeState,
    bool aRosterSealed,
    std::vector<CampaignRecoveryUiRosterMember> aRoster) noexcept
{
    if (acCampaignId.empty() || !aRosterSealed || aRoster.empty() ||
        aRoster.size() > kCampaignWireMaximumRosterSize ||
        std::count_if(
            aRoster.begin(), aRoster.end(),
            [](const CampaignRecoveryUiRosterMember& acMember)
            {
                return acMember.Local;
            }) != 1)
    {
        return false;
    }

    const bool active = aRuntimeState == kCampaignWireRuntimeActive;
    const bool recovering =
        aRuntimeState == kCampaignWireRuntimeRecoveryLock ||
        aRuntimeState == kCampaignWireRuntimeRestoringCheckpoint;
    if (!active && !recovering)
        return false;

    if (m_mode != CampaignRecoveryUiMode::Hidden &&
        m_campaignId != acCampaignId)
    {
        return false;
    }

    if (m_mode == CampaignRecoveryUiMode::Hidden)
    {
        if (active)
        {
            m_campaignId.assign(acCampaignId);
            m_roster = std::move(aRoster);
            m_observedActive = true;
            return true;
        }
        if (!m_observedActive || m_campaignId != acCampaignId)
            return false;

        m_mode = CampaignRecoveryUiMode::DisconnectIncident;
        m_localTransportLost = false;
        m_mainMenuRequested = false;
        m_mainMenuDispatchStarted = false;
    }

    m_roster = std::move(aRoster);
    return true;
}

bool CampaignRecoveryUiState::OpenLocalTransportLoss() noexcept
{
    if (m_mode != CampaignRecoveryUiMode::Hidden || !m_observedActive ||
        !HasUsableCampaign())
    {
        return false;
    }
    m_mode = CampaignRecoveryUiMode::DisconnectIncident;
    m_localTransportLost = true;
    m_mainMenuRequested = false;
    m_mainMenuDispatchStarted = false;
    return true;
}

bool CampaignRecoveryUiState::StayAndRecover() noexcept
{
    if (m_mode != CampaignRecoveryUiMode::DisconnectIncident ||
        m_mainMenuRequested)
    {
        return false;
    }
    m_mode = CampaignRecoveryUiMode::ExistingRecovery;
    return true;
}

bool CampaignRecoveryUiState::RequestMainMenu() noexcept
{
    if (m_mode != CampaignRecoveryUiMode::DisconnectIncident ||
        m_mainMenuRequested)
    {
        return false;
    }
    m_mainMenuRequested = true;
    return true;
}

bool CampaignRecoveryUiState::BeginMainMenuDispatch() noexcept
{
    if (m_mode != CampaignRecoveryUiMode::DisconnectIncident ||
        !m_mainMenuRequested || m_mainMenuDispatchStarted)
    {
        return false;
    }
    m_mainMenuDispatchStarted = true;
    return true;
}

void CampaignRecoveryUiState::Complete() noexcept
{
    m_mode = CampaignRecoveryUiMode::Hidden;
    m_campaignId.clear();
    m_roster.clear();
    m_observedActive = false;
    m_localTransportLost = false;
    m_mainMenuRequested = false;
    m_mainMenuDispatchStarted = false;
}

std::size_t CampaignRecoveryUiState::GetMissingRemoteCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(
        m_roster.begin(), m_roster.end(),
        [](const CampaignRecoveryUiRosterMember& acMember)
        {
            return !acMember.Local && !acMember.Present;
        }));
}

CampaignRecoveryIncidentKind CampaignRecoveryUiState::GetIncidentKind()
    const noexcept
{
    if (m_localTransportLost)
        return CampaignRecoveryIncidentKind::LocalTransportLost;
    const std::size_t missing = GetMissingRemoteCount();
    if (missing > 1)
        return CampaignRecoveryIncidentKind::MultiplePlayersMissing;
    if (missing == 1)
        return CampaignRecoveryIncidentKind::RemotePlayerMissing;
    return CampaignRecoveryIncidentKind::RosterRestored;
}
}
