#pragma once

#include <Trade/TradeTypes.h>
#include <Services/ItemPreview/ItemPreviewController.h>
#include <Services/ItemPreview/ItemPreviewHostSession.h>
#include <Services/ItemPreview/ItemPreviewHostBridge.h>

#include <cstdint>

struct World;

using TradePreviewProjectionTelemetryState = ItemPreviewRasterCaptureRequest;
using TradePreviewRasterMeasurement = ItemPreviewRasterMeasurement;

class TradeItemPreviewService final : public ItemPreviewHostClient
{
public:
    TradeItemPreviewService(World& aWorld, entt::dispatcher& aDispatcher) noexcept;
    ~TradeItemPreviewService() noexcept;

    TP_NOCOPYMOVE(TradeItemPreviewService);

    void SelectItem(Trade::ItemId aItemId) noexcept;
    void Clear() noexcept;
    void SetPreviewRegion(float aLeft, float aTop, float aWidth, float aHeight) noexcept;
    void UpdatePreviewPlacement() noexcept override;
    [[nodiscard]] ItemPreviewRasterCaptureRequest CaptureProjectionTelemetryState() noexcept;
    [[nodiscard]] ItemPreviewRasterCaptureRequest CaptureRasterRequest() noexcept override;
    void SubmitProjectionMeasurement(const ItemPreviewRasterMeasurement& aMeasurement) noexcept;
    void SubmitRasterMeasurement(const ItemPreviewRasterMeasurement& aMeasurement) noexcept override;
    void ProcessPendingFitReloadOnUiThread() noexcept;
    void ProcessPendingReloadOnUiThread() noexcept override;

    [[nodiscard]] bool BeginNativePreviewSession(std::uint32_t aLightScheme = 1) noexcept;
    [[nodiscard]] bool BeginNativeSession(std::uint32_t aLightScheme) noexcept override;
    void EndNativePreviewSession() noexcept;
    void EndNativeSession() noexcept override;
    void OnHostMenuShown(bool aApplySelection = true) noexcept;
    void OnHostShown(bool aApplySelection) noexcept override;
    void OnHostMenuHidden() noexcept;
    void OnHostHidden() noexcept override;

    [[nodiscard]] bool IsActive() const noexcept
    {
        return m_controller.IsActive();
    }

private:
    static void ExecuteHostCommand(ItemPreviewHostSession::Command aCommand) noexcept;

    World& m_world;
    ItemPreviewController m_controller;
    ItemPreviewHostSession m_hostSession;
    ItemPreviewHostBinding m_hostBinding;
};
