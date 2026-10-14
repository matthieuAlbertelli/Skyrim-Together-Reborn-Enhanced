#pragma once

#include <CampaignAdmissionService.h>

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace STRE::Campaign
{
// Session evidence only. The adapter verifies the live Player/character owner
// and the build's admission identity before reporting Applied.
enum class HelgenBuildEvidence { Missing, Pending, Applied, Invalid };
enum class HelgenStartOutcome { Rejected, Waiting, Started, AlreadyStarted };

struct HelgenStartResult
{
    HelgenStartOutcome Outcome{HelgenStartOutcome::Rejected};
    CampaignId Campaign;
    std::size_t Required{};
    std::size_t Applied{};
    const char* Reason{"no-admission"};
};

class CampaignHelgenStartBarrier final
{
public:
    using BuildReader = std::function<HelgenBuildEvidence(const CampaignAdmissionRecord&)>;

    // A Started result is the single broadcast intent. Readiness may reconstruct
    // old native-save state only when the caller proves a committed checkpoint.
    [[nodiscard]] HelgenStartResult Observe(
        CampaignAdmissionService& aAdmission, CampaignConnectionHandle aConnection,
        const BuildReader& acReadBuild, bool aReadiness = false,
        bool aHasCommittedCheckpoint = false);
    [[nodiscard]] bool HasStarted(const CampaignId& acCampaign) const;
    void Disconnect(const CampaignId& acCampaign);

private:
    std::unordered_map<std::string, std::unordered_set<CampaignConnectionHandle>> m_helgenReadyConnections;
    std::unordered_set<std::string> m_helgenStartedCampaigns;
};
}
