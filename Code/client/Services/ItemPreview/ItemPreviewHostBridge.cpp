#include <TiltedOnlinePCH.h>

#include <Services/ItemPreview/ItemPreviewHostBridge.h>

ItemPreviewHostBinding::ItemPreviewHostBinding(
    ItemPreviewHostBridge& aBridge,
    ItemPreviewHostClient& aClient) noexcept
    : m_bridge(aBridge)
    , m_client(aClient)
    , m_bound(m_bridge.Bind(m_client))
{
}

ItemPreviewHostBinding::~ItemPreviewHostBinding() noexcept
{
    if (m_bound)
        m_bridge.Unbind(m_client);
}

ItemPreviewHostBridge& ItemPreviewHostBridge::Get() noexcept
{
    static ItemPreviewHostBridge s_instance;
    return s_instance;
}

bool ItemPreviewHostBridge::Bind(ItemPreviewHostClient& aClient) noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client && m_client != &aClient)
        return false;

    m_client = &aClient;
    return true;
}

void ItemPreviewHostBridge::Unbind(ItemPreviewHostClient& aClient) noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client == &aClient)
        m_client = nullptr;
}

void ItemPreviewHostBridge::UpdatePreviewPlacement() noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client)
        m_client->UpdatePreviewPlacement();
}

ItemPreviewRasterCaptureRequest ItemPreviewHostBridge::CaptureRasterRequest() noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client)
        return m_client->CaptureRasterRequest();

    return {};
}

void ItemPreviewHostBridge::SubmitRasterMeasurement(
    const ItemPreviewRasterMeasurement& aMeasurement) noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client)
        m_client->SubmitRasterMeasurement(aMeasurement);
}

void ItemPreviewHostBridge::ProcessPendingReloadOnUiThread() noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client)
        m_client->ProcessPendingReloadOnUiThread();
}

bool ItemPreviewHostBridge::BeginNativeSession(
    std::uint32_t aLightScheme) noexcept
{
    std::lock_guard lock{m_mutex};
    return m_client && m_client->BeginNativeSession(aLightScheme);
}

void ItemPreviewHostBridge::EndNativeSession() noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client)
        m_client->EndNativeSession();
}

void ItemPreviewHostBridge::OnHostShown(bool aApplySelection) noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client)
        m_client->OnHostShown(aApplySelection);
}

void ItemPreviewHostBridge::OnHostHidden() noexcept
{
    std::lock_guard lock{m_mutex};
    if (m_client)
        m_client->OnHostHidden();
}
