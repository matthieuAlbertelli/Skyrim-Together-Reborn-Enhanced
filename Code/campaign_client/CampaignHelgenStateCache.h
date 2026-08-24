#pragma once

#include <atomic>

class CampaignHelgenStateCache final
{
public:
    void Apply(bool aInvestigationStartAuthorized, bool aSpatialKnown, bool aAllRequiredPlayersOutside) noexcept;
    void Reset() noexcept;

    [[nodiscard]] bool IsInvestigationStartAuthorized() const noexcept
    {
        return m_investigationStartAuthorized.load(std::memory_order_acquire);
    }
    [[nodiscard]] bool AreAllRequiredPlayersOutside() const noexcept
    {
        return m_spatialKnown.load(std::memory_order_acquire) &&
            m_allRequiredPlayersOutside.load(std::memory_order_acquire);
    }

private:
    std::atomic_bool m_investigationStartAuthorized{};
    std::atomic_bool m_spatialKnown{};
    std::atomic_bool m_allRequiredPlayersOutside{};
};
