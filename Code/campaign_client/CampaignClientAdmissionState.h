#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace STRE::Campaign
{
struct CampaignClientAdmission
{
    std::string CampaignId;
    std::string CampaignSlotId;
    std::string CharacterBindingId;

    bool operator==(const CampaignClientAdmission&) const noexcept = default;
};

struct CampaignHelgenReadinessView
{
    std::optional<CampaignClientAdmission> Admission;
    bool CanSignal{};
};

class CampaignClientAdmissionState final
{
public:
    void Accept(CampaignClientAdmission aAdmission) noexcept;
    [[nodiscard]] std::optional<CampaignClientAdmission>
    GetAdmission() const noexcept;

    void ObserveSnapshot(
        std::string_view acCampaignId,
        bool aRosterSealed,
        bool aRuntimeActive,
        std::size_t aRosterSize,
        std::size_t aPresentCount) noexcept;
    [[nodiscard]] CampaignHelgenReadinessView
    GetHelgenReadinessView() const noexcept;

    [[nodiscard]] std::optional<std::string> Disconnect() noexcept;
    [[nodiscard]] std::optional<std::string> BeginResume() noexcept;
    void ResumeRejected() noexcept;
    void Leave(std::string_view acCampaignId) noexcept;

private:
    struct RuntimeProjection
    {
        std::string CampaignId;
        bool RosterSealed{};
        bool RuntimeActive{};
        std::size_t RosterSize{};
        std::size_t PresentCount{};
    };

    mutable std::mutex m_mutex;
    std::optional<CampaignClientAdmission> m_admission;
    // Reconnect metadata only. It never becomes admission until Accept receives
    // the server's successful Resume response.
    std::optional<std::string> m_resumeCandidate;
    std::optional<RuntimeProjection> m_runtimeProjection;
    bool m_resumeInFlight{};
};
}
