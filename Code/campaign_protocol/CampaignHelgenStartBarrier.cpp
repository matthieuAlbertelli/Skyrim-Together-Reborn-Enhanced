#include <CampaignHelgenStartBarrier.h>

#include <algorithm>

namespace STRE::Campaign
{
HelgenStartResult CampaignHelgenStartBarrier::Observe(
    CampaignAdmissionService& aAdmission, CampaignConnectionHandle aConnection,
    const BuildReader& acReadBuild, bool aReadiness, bool aHasCommittedCheckpoint)
{
    HelgenStartResult result;
    const auto& records = static_cast<const CampaignAdmissionService&>(aAdmission);
    const auto* admission = records.FindConnection(aConnection);
    if (!admission || !admission->AdmittedIdentity)
        return result;
    result.Campaign = admission->AdmittedIdentity->Campaign;
    const auto snapshot = aAdmission.BuildSnapshot(result.Campaign);
    // BuildSnapshot derives ACTIVE through the canonical exact identity/binding
    // predicate, not just a count of connected sockets.
    result.Reason = "runtime-gate";
    if (!snapshot || !snapshot->RosterSealed || snapshot->Roster.empty() ||
        snapshot->RuntimeState != kCampaignWireRuntimeActive)
        return result;

    const auto connections = aAdmission.GetAdmittedConnections(result.Campaign);
    result.Required = snapshot->Roster.size();
    result.Reason = "incomplete-roster";
    if (connections.size() != result.Required ||
        std::any_of(snapshot->Roster.begin(), snapshot->Roster.end(),
            [](const auto& slot) { return !slot.Present; }))
        return result;

    if (HasStarted(result.Campaign))
    {
        result.Outcome = HelgenStartOutcome::AlreadyStarted;
        result.Reason = "already-started";
        return result;
    }

    bool allMissing = true;
    for (const auto connection : connections)
    {
        const auto* member = records.FindConnection(connection);
        result.Reason = "identity-mismatch";
        if (!member || !member->AdmittedIdentity ||
            member->Player != member->AdmittedIdentity->Player ||
            member->AdmittedIdentity->Campaign != result.Campaign)
            return result;
        const auto evidence = acReadBuild(*member);
        if (evidence == HelgenBuildEvidence::Invalid)
            return result;
        allMissing = allMissing && evidence == HelgenBuildEvidence::Missing;
        if (evidence == HelgenBuildEvidence::Applied)
            ++result.Applied;
    }

    bool mayStart = result.Applied == result.Required;
    if (aReadiness && !mayStart && allMissing && aHasCommittedCheckpoint)
    {
        auto& ready = m_helgenReadyConnections[result.Campaign.Value];
        ready.insert(aConnection);
        mayStart = std::all_of(connections.begin(), connections.end(),
            [&](auto connection) { return ready.contains(connection); });
        result.Reason = "checkpoint-readiness";
    }
    else
        result.Reason = "all-builds-applied";

    result.Outcome = HelgenStartOutcome::Waiting;
    if (mayStart && m_helgenStartedCampaigns.insert(result.Campaign.Value).second)
        result.Outcome = HelgenStartOutcome::Started;
    return result;
}

bool CampaignHelgenStartBarrier::HasStarted(const CampaignId& acCampaign) const
{
    return m_helgenStartedCampaigns.contains(acCampaign.Value);
}

void CampaignHelgenStartBarrier::Disconnect(const CampaignId& acCampaign)
{
    // Readiness must be rebuilt by the whole current session after recovery.
    // The start latch is retained; native .ess state owns projection idempotence.
    m_helgenReadyConnections.erase(acCampaign.Value);
}
}
