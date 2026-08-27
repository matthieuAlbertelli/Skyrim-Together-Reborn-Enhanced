#include <CampaignResumeState.h>

#include <CampaignIdentityStore.h>
#include <Structs/Campaign.h>

#include <algorithm>
#include <unordered_set>

namespace STRE::Campaign
{
namespace
{
bool IsOpaqueToken(std::string_view acToken) noexcept
{
    return acToken.size() == 32 &&
        std::all_of(
            acToken.begin(), acToken.end(),
            [](char aValue)
            {
                return (aValue >= '0' && aValue <= '9') ||
                    (aValue >= 'a' && aValue <= 'f');
            });
}
}

bool CampaignResumeState::ReplaceCandidates(
    std::vector<CampaignResumeCandidate> aCandidates) noexcept
{
    try
    {
        if (aCandidates.size() > kMaximumCampaignResumeCandidates)
            return false;
        std::unordered_set<std::string> tokens;
        std::unordered_set<std::string> campaigns;
        for (const auto& candidate : aCandidates)
        {
            if (!IsOpaqueToken(candidate.Token) ||
                !CampaignIdentityStore::IsValidCacheId(candidate.CampaignId) ||
                !tokens.emplace(candidate.Token).second ||
                !campaigns.emplace(candidate.CampaignId).second)
            {
                return false;
            }
        }

        m_candidates = std::move(aCandidates);
        m_views.clear();
        m_views.reserve(m_candidates.size());
        for (std::size_t index = 0; index < m_candidates.size(); ++index)
        {
            m_views.push_back({
                m_candidates[index].Token,
                index + 1});
        }
        m_selectedToken.clear();
        m_errorCode.clear();
        m_phase = CampaignResumePhase::Ready;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void CampaignResumeState::FailCache() noexcept
{
    m_candidates.clear();
    m_views.clear();
    m_selectedToken.clear();
    m_errorCode = "cache_invalid";
    m_phase = CampaignResumePhase::Error;
}

void CampaignResumeState::Fail(std::string aErrorCode) noexcept
{
    m_errorCode = std::move(aErrorCode);
    m_phase = CampaignResumePhase::Error;
}

const CampaignResumeCandidate* CampaignResumeState::FindByToken(
    std::string_view acToken) const noexcept
{
    const auto it = std::find_if(
        m_candidates.begin(), m_candidates.end(),
        [acToken](const CampaignResumeCandidate& acCandidate)
        {
            return acCandidate.Token == acToken;
        });
    return it == m_candidates.end() ? nullptr : &*it;
}

bool CampaignResumeState::IsSelectedCampaign(
    std::string_view acCampaignId) const noexcept
{
    const auto* const pCandidate = FindByToken(m_selectedToken);
    return pCandidate && pCandidate->CampaignId == acCampaignId;
}

std::optional<std::string> CampaignResumeState::Select(
    std::string_view acToken) noexcept
{
    if (m_phase != CampaignResumePhase::Ready &&
        m_phase != CampaignResumePhase::Error)
    {
        return std::nullopt;
    }
    const auto* const pCandidate = FindByToken(acToken);
    if (!pCandidate)
    {
        m_errorCode = "candidate_unavailable";
        m_phase = CampaignResumePhase::Error;
        return std::nullopt;
    }
    m_selectedToken = pCandidate->Token;
    m_errorCode.clear();
    m_phase = CampaignResumePhase::Submitting;
    return pCandidate->CampaignId;
}

bool CampaignResumeState::RetrySelected(
    std::string_view acCampaignId) noexcept
{
    if (m_phase != CampaignResumePhase::Error ||
        !IsSelectedCampaign(acCampaignId))
    {
        return false;
    }
    m_errorCode.clear();
    m_phase = CampaignResumePhase::Submitting;
    return true;
}

bool CampaignResumeState::Accept(std::string_view acCampaignId) noexcept
{
    if (!IsSelectedCampaign(acCampaignId))
        return false;
    if (m_phase == CampaignResumePhase::Submitting ||
        (m_phase == CampaignResumePhase::Error &&
         (m_errorCode == "connection_lost" ||
          m_errorCode == "recovery_failed")))
    {
        m_phase = CampaignResumePhase::Admitted;
        m_errorCode.clear();
        return true;
    }
    return m_phase == CampaignResumePhase::Admitted ||
        m_phase == CampaignResumePhase::WaitingForRoster ||
        m_phase == CampaignResumePhase::Recovery ||
        m_phase == CampaignResumePhase::Synchronizing ||
        m_phase == CampaignResumePhase::Active;
}

bool CampaignResumeState::Reject(
    std::string_view acCampaignId,
    std::string aErrorCode) noexcept
{
    if (m_phase != CampaignResumePhase::Submitting ||
        !IsSelectedCampaign(acCampaignId))
    {
        return false;
    }
    m_errorCode = std::move(aErrorCode);
    m_phase = CampaignResumePhase::Error;
    return true;
}

bool CampaignResumeState::ObserveRuntime(
    std::string_view acCampaignId,
    std::uint8_t aRuntimeState,
    bool aRosterSealed,
    std::size_t aRosterSize,
    std::size_t aPresentCount) noexcept
{
    if (!IsSelectedCampaign(acCampaignId) ||
        (m_phase != CampaignResumePhase::Admitted &&
         m_phase != CampaignResumePhase::WaitingForRoster &&
         m_phase != CampaignResumePhase::Recovery &&
         m_phase != CampaignResumePhase::Synchronizing &&
         m_phase != CampaignResumePhase::Active) ||
        aRosterSize == 0 || aPresentCount > aRosterSize)
    {
        return false;
    }

    if (!aRosterSealed)
    {
        m_phase = CampaignResumePhase::Admitted;
    }
    else if (aRuntimeState == kCampaignWireRuntimeActive &&
        aPresentCount == aRosterSize)
    {
        m_phase = CampaignResumePhase::Active;
    }
    else if (aRuntimeState == kCampaignWireRuntimeRecoveryLock)
    {
        m_phase = CampaignResumePhase::Recovery;
    }
    else if (aRuntimeState ==
             kCampaignWireRuntimeRestoringCheckpoint)
    {
        m_phase = CampaignResumePhase::Synchronizing;
    }
    else
    {
        m_phase = CampaignResumePhase::WaitingForRoster;
    }
    return true;
}

void CampaignResumeState::Complete() noexcept
{
    m_phase = CampaignResumePhase::Unavailable;
    m_candidates.clear();
    m_views.clear();
    m_selectedToken.clear();
    m_errorCode.clear();
}
}
