#pragma once

class CampaignHelgenStateCache final
{
public:
    void Apply(bool aInvestigationStartAuthorized, bool aSpatialKnown, bool aAllRequiredPlayersOutside) noexcept;
    void Reset() noexcept;

    [[nodiscard]] bool IsInvestigationStartAuthorized() const noexcept { return m_investigationStartAuthorized; }
    [[nodiscard]] bool AreAllRequiredPlayersOutside() const noexcept { return m_spatialKnown && m_allRequiredPlayersOutside; }

private:
    bool m_investigationStartAuthorized{};
    bool m_spatialKnown{};
    bool m_allRequiredPlayersOutside{};
};
