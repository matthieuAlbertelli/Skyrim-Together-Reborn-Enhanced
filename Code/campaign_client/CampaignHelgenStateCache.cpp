#include <CampaignHelgenStateCache.h>

void CampaignHelgenStateCache::Apply(bool aInvestigationStartAuthorized, bool aSpatialKnown, bool aAllRequiredPlayersOutside) noexcept
{
    if (aInvestigationStartAuthorized)
    {
        m_investigationStartAuthorized.store(true, std::memory_order_release);
    }
    m_allRequiredPlayersOutside.store(
        aSpatialKnown && aAllRequiredPlayersOutside,
        std::memory_order_release);
    m_spatialKnown.store(aSpatialKnown, std::memory_order_release);
}

void CampaignHelgenStateCache::Reset() noexcept
{
    m_investigationStartAuthorized.store(false, std::memory_order_release);
    m_spatialKnown.store(false, std::memory_order_release);
    m_allRequiredPlayersOutside.store(false, std::memory_order_release);
}
