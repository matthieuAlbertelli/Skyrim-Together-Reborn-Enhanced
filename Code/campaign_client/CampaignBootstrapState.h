#pragma once

#include <cstdint>

namespace STRE::Campaign
{
enum class CampaignBootstrapPhase : std::uint8_t
{
    Inactive,
    Entry,
    CreateForm,
    JoinForm,
    ConnectingCreate,
    ConnectingJoin,
    CreatingCampaign,
    JoiningCampaign,
    Lobby,
    Starting,
    Authorized,
    Error
};

class CampaignBootstrapState final
{
public:
    void BeginFreshGame() noexcept;
    [[nodiscard]] bool ChooseSolo() noexcept;
    void ShowCreateForm() noexcept;
    void ShowJoinForm() noexcept;
    void BeginCreate(bool aConnected) noexcept;
    void BeginJoin(bool aConnected) noexcept;
    void OnConnected() noexcept;
    void OnCampaignAdmitted() noexcept;
    void BeginStart() noexcept;
    void RejectStart() noexcept;
    [[nodiscard]] bool ObserveCanonicalState(
        bool aRosterSealed,
        bool aCharacterCreationPhase,
        bool aFullRosterActive) noexcept;
    void Fail() noexcept;
    void OnDisconnect() noexcept;
    void Back() noexcept;
    void RestoreLobby() noexcept;

    [[nodiscard]] CampaignBootstrapPhase GetPhase() const noexcept
    {
        return m_phase;
    }
    [[nodiscard]] bool IsActive() const noexcept
    {
        return m_phase != CampaignBootstrapPhase::Inactive &&
            m_phase != CampaignBootstrapPhase::Authorized;
    }
    [[nodiscard]] bool IsBusy() const noexcept;
    [[nodiscard]] bool IsMultiplayerCommitted() const noexcept
    {
        return m_multiplayerCommitted;
    }

private:
    CampaignBootstrapPhase m_phase{CampaignBootstrapPhase::Inactive};
    bool m_multiplayerCommitted{};
    bool m_authorizationEmitted{};
};
}
