#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace STRE::Campaign
{
inline constexpr std::size_t kMaximumCampaignResumeCandidates = 256;

enum class CampaignResumePhase : std::uint8_t
{
    Unavailable,
    Ready,
    Submitting,
    Admitted,
    WaitingForRoster,
    Recovery,
    Synchronizing,
    Active,
    Error
};

struct CampaignResumeCandidate
{
    std::string Token;
    std::string CampaignId;
};

struct CampaignResumeCandidateView
{
    std::string Token;
    std::size_t Ordinal{};

    bool operator==(const CampaignResumeCandidateView&) const noexcept = default;
};

class CampaignResumeState final
{
public:
    [[nodiscard]] bool ReplaceCandidates(
        std::vector<CampaignResumeCandidate> aCandidates) noexcept;
    void FailCache() noexcept;
    void Fail(std::string aErrorCode) noexcept;

    [[nodiscard]] std::optional<std::string> Select(
        std::string_view acToken) noexcept;
    [[nodiscard]] bool RetrySelected(
        std::string_view acCampaignId) noexcept;
    [[nodiscard]] bool Accept(std::string_view acCampaignId) noexcept;
    [[nodiscard]] bool Reject(
        std::string_view acCampaignId,
        std::string aErrorCode) noexcept;
    [[nodiscard]] bool ObserveRuntime(
        std::string_view acCampaignId,
        std::uint8_t aRuntimeState,
        bool aRosterSealed,
        std::size_t aRosterSize,
        std::size_t aPresentCount) noexcept;
    void Complete() noexcept;

    [[nodiscard]] CampaignResumePhase GetPhase() const noexcept
    {
        return m_phase;
    }
    [[nodiscard]] const std::vector<CampaignResumeCandidateView>&
    GetCandidates() const noexcept
    {
        return m_views;
    }
    [[nodiscard]] const std::string& GetSelectedToken() const noexcept
    {
        return m_selectedToken;
    }
    [[nodiscard]] const std::string& GetErrorCode() const noexcept
    {
        return m_errorCode;
    }

private:
    [[nodiscard]] const CampaignResumeCandidate* FindByToken(
        std::string_view acToken) const noexcept;
    [[nodiscard]] bool IsSelectedCampaign(
        std::string_view acCampaignId) const noexcept;

    CampaignResumePhase m_phase{CampaignResumePhase::Unavailable};
    std::vector<CampaignResumeCandidate> m_candidates;
    std::vector<CampaignResumeCandidateView> m_views;
    std::string m_selectedToken;
    std::string m_errorCode;
};
}
