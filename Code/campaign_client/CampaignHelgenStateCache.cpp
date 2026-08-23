#include <CampaignHelgenStateCache.h>

void CampaignHelgenStateCache::Apply(bool aInvestigationStartAuthorized, bool aSpatialKnown, bool aAllRequiredPlayersOutside) noexcept
{
    m_investigationStartAuthorized = m_investigationStartAuthorized || aInvestigationStartAuthorized;
    m_spatialKnown = aSpatialKnown;
    m_allRequiredPlayersOutside = aSpatialKnown && aAllRequiredPlayersOutside;
}

void CampaignHelgenStateCache::Reset() noexcept
{
    m_investigationStartAuthorized = false;
    m_spatialKnown = false;
    m_allRequiredPlayersOutside = false;
}
