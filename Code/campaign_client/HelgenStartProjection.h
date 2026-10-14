#pragma once

#include <string>
#include <string_view>
#include <utility>

// One local request per creation/session. Papyrus/native saves own the durable
// idempotence of the consequence; this latch is not campaign authority.
class HelgenStartProjection final
{
public:
    void Begin(std::string aCampaign) { Reset(); m_campaign = std::move(aCampaign); }
    void Finalize() noexcept { m_finalized = true; }
    void Authorize(std::string_view aCampaign) noexcept
    {
        if (!m_campaign.empty() && m_campaign == aCampaign)
            m_authorized = true;
    }
    bool Consume(bool aConnected, bool aCampaignActive, bool aNativeReady) noexcept
    {
        if (!m_finalized || m_issued || !aNativeReady)
            return false;
        if (m_campaign.empty() ? aConnected : (!aConnected || !aCampaignActive || !m_authorized))
            return false;
        m_issued = true;
        return true;
    }
    void Reset() noexcept { m_campaign.clear(); m_finalized = m_authorized = m_issued = false; }
    const std::string& Campaign() const noexcept { return m_campaign; }
    bool Pending() const noexcept { return m_finalized && !m_issued; }

private:
    std::string m_campaign;
    bool m_finalized{};
    bool m_authorized{};
    bool m_issued{};
};
