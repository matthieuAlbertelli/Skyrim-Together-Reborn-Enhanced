#pragma once

#include <Services/ItemPreview/ItemPreviewRasterTypes.h>

#include <cstdint>
#include <mutex>

class ItemPreviewHostClient
{
public:
    virtual ~ItemPreviewHostClient() = default;

    virtual void UpdatePreviewPlacement() noexcept = 0;
    [[nodiscard]] virtual ItemPreviewRasterCaptureRequest CaptureRasterRequest() noexcept = 0;
    virtual void SubmitRasterMeasurement(const ItemPreviewRasterMeasurement& aMeasurement) noexcept = 0;
    virtual void ProcessPendingReloadOnUiThread() noexcept = 0;

    [[nodiscard]] virtual bool BeginNativeSession(std::uint32_t aLightScheme) noexcept = 0;
    virtual void EndNativeSession() noexcept = 0;
    virtual void OnHostShown(bool aApplySelection) noexcept = 0;
    virtual void OnHostHidden() noexcept = 0;
};

class ItemPreviewHostBridge;

class ItemPreviewHostBinding final
{
public:
    ItemPreviewHostBinding(
        ItemPreviewHostBridge& aBridge,
        ItemPreviewHostClient& aClient) noexcept;
    ~ItemPreviewHostBinding() noexcept;

    ItemPreviewHostBinding(const ItemPreviewHostBinding&) = delete;
    ItemPreviewHostBinding& operator=(const ItemPreviewHostBinding&) = delete;
    ItemPreviewHostBinding(ItemPreviewHostBinding&&) = delete;
    ItemPreviewHostBinding& operator=(ItemPreviewHostBinding&&) = delete;

    [[nodiscard]] bool IsBound() const noexcept
    {
        return m_bound;
    }

private:
    ItemPreviewHostBridge& m_bridge;
    ItemPreviewHostClient& m_client;
    bool m_bound{};
};

class ItemPreviewHostBridge final
{
public:
    [[nodiscard]] static ItemPreviewHostBridge& Get() noexcept;

    void UpdatePreviewPlacement() noexcept;
    [[nodiscard]] ItemPreviewRasterCaptureRequest CaptureRasterRequest() noexcept;
    void SubmitRasterMeasurement(const ItemPreviewRasterMeasurement& aMeasurement) noexcept;
    void ProcessPendingReloadOnUiThread() noexcept;

    [[nodiscard]] bool BeginNativeSession(std::uint32_t aLightScheme) noexcept;
    void EndNativeSession() noexcept;
    void OnHostShown(bool aApplySelection) noexcept;
    void OnHostHidden() noexcept;

private:
    friend class ItemPreviewHostBinding;

    [[nodiscard]] bool Bind(ItemPreviewHostClient& aClient) noexcept;
    void Unbind(ItemPreviewHostClient& aClient) noexcept;

    std::mutex m_mutex;
    ItemPreviewHostClient* m_client{};
};
