#include <CampaignBootstrapState.h>

namespace STRE::Campaign
{
void CampaignBootstrapState::BeginFreshGame() noexcept
{
    m_phase = CampaignBootstrapPhase::Entry;
    m_multiplayerCommitted = false;
    m_authorizationEmitted = false;
}

bool CampaignBootstrapState::ChooseSolo() noexcept
{
    if (m_phase != CampaignBootstrapPhase::Entry)
        return false;
    m_phase = CampaignBootstrapPhase::Authorized;
    m_authorizationEmitted = true;
    return true;
}

void CampaignBootstrapState::ShowCreateForm() noexcept
{
    if (m_phase == CampaignBootstrapPhase::Entry ||
        m_phase == CampaignBootstrapPhase::Error)
    {
        m_phase = CampaignBootstrapPhase::CreateForm;
    }
}

void CampaignBootstrapState::ShowJoinForm() noexcept
{
    if (m_phase == CampaignBootstrapPhase::Entry ||
        m_phase == CampaignBootstrapPhase::Error)
    {
        m_phase = CampaignBootstrapPhase::JoinForm;
    }
}

void CampaignBootstrapState::BeginCreate(bool aConnected) noexcept
{
    if (m_phase != CampaignBootstrapPhase::CreateForm)
        return;
    m_phase = aConnected
        ? CampaignBootstrapPhase::CreatingCampaign
        : CampaignBootstrapPhase::ConnectingCreate;
}

void CampaignBootstrapState::BeginJoin(bool aConnected) noexcept
{
    if (m_phase != CampaignBootstrapPhase::JoinForm)
        return;
    m_phase = aConnected
        ? CampaignBootstrapPhase::JoiningCampaign
        : CampaignBootstrapPhase::ConnectingJoin;
}

void CampaignBootstrapState::OnConnected() noexcept
{
    if (m_phase == CampaignBootstrapPhase::ConnectingCreate)
        m_phase = CampaignBootstrapPhase::CreatingCampaign;
    else if (m_phase == CampaignBootstrapPhase::ConnectingJoin)
        m_phase = CampaignBootstrapPhase::JoiningCampaign;
}

void CampaignBootstrapState::OnCampaignAdmitted() noexcept
{
    if (m_phase == CampaignBootstrapPhase::CreatingCampaign ||
        m_phase == CampaignBootstrapPhase::JoiningCampaign)
    {
        m_multiplayerCommitted = true;
        m_phase = CampaignBootstrapPhase::Lobby;
    }
}

void CampaignBootstrapState::BeginStart() noexcept
{
    if (m_phase == CampaignBootstrapPhase::Lobby)
        m_phase = CampaignBootstrapPhase::Starting;
}

void CampaignBootstrapState::RejectStart() noexcept
{
    if (m_phase == CampaignBootstrapPhase::Starting)
        m_phase = CampaignBootstrapPhase::Lobby;
}

bool CampaignBootstrapState::ObserveCanonicalState(
    bool aRosterSealed,
    bool aCharacterCreationPhase,
    bool aFullRosterActive) noexcept
{
    if (!m_multiplayerCommitted || m_authorizationEmitted ||
        !aRosterSealed || !aCharacterCreationPhase || !aFullRosterActive)
    {
        return false;
    }
    m_phase = CampaignBootstrapPhase::Authorized;
    m_authorizationEmitted = true;
    return true;
}

void CampaignBootstrapState::Fail() noexcept
{
    if (m_phase != CampaignBootstrapPhase::Inactive &&
        m_phase != CampaignBootstrapPhase::Authorized)
    {
        m_phase = CampaignBootstrapPhase::Error;
    }
}

void CampaignBootstrapState::OnDisconnect() noexcept
{
    Fail();
}

void CampaignBootstrapState::RestoreLobby() noexcept
{
    if (m_phase == CampaignBootstrapPhase::Entry ||
        m_phase == CampaignBootstrapPhase::JoiningCampaign ||
        m_phase == CampaignBootstrapPhase::CreatingCampaign)
    {
        m_multiplayerCommitted = true;
        m_phase = CampaignBootstrapPhase::Lobby;
    }
}

void CampaignBootstrapState::Back() noexcept
{
    if (m_multiplayerCommitted)
    {
        if (m_phase == CampaignBootstrapPhase::Error)
            m_phase = CampaignBootstrapPhase::JoinForm;
        return;
    }
    if (m_phase == CampaignBootstrapPhase::CreateForm ||
        m_phase == CampaignBootstrapPhase::JoinForm ||
        m_phase == CampaignBootstrapPhase::Error)
    {
        m_phase = CampaignBootstrapPhase::Entry;
    }
}

bool CampaignBootstrapState::IsBusy() const noexcept
{
    return m_phase == CampaignBootstrapPhase::ConnectingCreate ||
        m_phase == CampaignBootstrapPhase::ConnectingJoin ||
        m_phase == CampaignBootstrapPhase::CreatingCampaign ||
        m_phase == CampaignBootstrapPhase::JoiningCampaign ||
        m_phase == CampaignBootstrapPhase::Starting;
}
}
