#pragma once

#include <memory>

struct PreUpdateEvent;

class CampaignSaveTraceService final
{
public:
    explicit CampaignSaveTraceService(entt::dispatcher& aDispatcher) noexcept;
    ~CampaignSaveTraceService() noexcept;

    TP_NOCOPYMOVE(CampaignSaveTraceService);

private:
    void OnPreUpdate(const PreUpdateEvent&) noexcept;

    struct Detail;
    std::unique_ptr<Detail> m_detail;
    entt::scoped_connection m_preUpdateConnection;
};
