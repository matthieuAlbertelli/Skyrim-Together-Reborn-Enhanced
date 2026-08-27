#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace STRE::Campaign
{
enum class CampaignRecoveryUiMode : std::uint8_t
{
    Hidden,
    DisconnectIncident,
    ExistingRecovery
};

enum class CampaignRecoveryIncidentKind : std::uint8_t
{
    RemotePlayerMissing,
    MultiplePlayersMissing,
    LocalTransportLost,
    RosterRestored
};

struct CampaignRecoveryUiRosterMember
{
    bool Present{};
    bool Local{};

    bool operator==(const CampaignRecoveryUiRosterMember&) const noexcept =
        default;
};

// Presentation-only state. It neither authorizes recovery nor owns a
// checkpoint/attempt correlation; those remain exclusively in the existing
// CampaignRecoveryService state machine.
class CampaignRecoveryUiState final
{
public:
    [[nodiscard]] bool ObserveSnapshot(
        std::string_view acCampaignId,
        std::uint8_t aRuntimeState,
        bool aRosterSealed,
        std::vector<CampaignRecoveryUiRosterMember> aRoster) noexcept;
    [[nodiscard]] bool OpenLocalTransportLoss() noexcept;
    [[nodiscard]] bool StayAndRecover() noexcept;
    [[nodiscard]] bool RequestMainMenu() noexcept;
    [[nodiscard]] bool BeginMainMenuDispatch() noexcept;
    void CancelMainMenuRequest() noexcept
    {
        m_mainMenuRequested = false;
        m_mainMenuDispatchStarted = false;
    }
    void Complete() noexcept;

    [[nodiscard]] CampaignRecoveryUiMode GetMode() const noexcept
    {
        return m_mode;
    }
    [[nodiscard]] CampaignRecoveryIncidentKind GetIncidentKind()
        const noexcept;
    [[nodiscard]] const std::string& GetCampaignId() const noexcept
    {
        return m_campaignId;
    }
    [[nodiscard]] const std::vector<CampaignRecoveryUiRosterMember>&
    GetRoster() const noexcept
    {
        return m_roster;
    }
    [[nodiscard]] std::size_t GetMissingRemoteCount() const noexcept;
    [[nodiscard]] bool IsMainMenuRequested() const noexcept
    {
        return m_mainMenuRequested;
    }

private:
    [[nodiscard]] bool HasUsableCampaign() const noexcept
    {
        return !m_campaignId.empty();
    }

    CampaignRecoveryUiMode m_mode{CampaignRecoveryUiMode::Hidden};
    std::string m_campaignId;
    std::vector<CampaignRecoveryUiRosterMember> m_roster;
    bool m_observedActive{};
    bool m_localTransportLost{};
    bool m_mainMenuRequested{};
    bool m_mainMenuDispatchStarted{};
};
}
