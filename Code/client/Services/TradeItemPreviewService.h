#pragma once

#include <Trade/TradeTypes.h>
#include <Services/ItemPreview/ItemPreviewRasterTypes.h>
#include <Misc/InventoryEntry.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>

struct World;

using TradePreviewProjectionTelemetryState = ItemPreviewRasterCaptureRequest;
using TradePreviewRasterMeasurement = ItemPreviewRasterMeasurement;

class TradeItemPreviewService
{
public:
    TradeItemPreviewService(
        World& aWorld,
        entt::dispatcher& aDispatcher) noexcept;
    ~TradeItemPreviewService() noexcept;

    TP_NOCOPYMOVE(TradeItemPreviewService);

    void SelectItem(Trade::ItemId aItemId) noexcept;
    void Clear() noexcept;
    void SetPreviewRegion(
        float aLeft,
        float aTop,
        float aWidth,
        float aHeight) noexcept;
    void UpdatePreviewPlacement() noexcept;
    [[nodiscard]] ItemPreviewRasterCaptureRequest
    CaptureProjectionTelemetryState() noexcept;
    void SubmitProjectionMeasurement(
        const ItemPreviewRasterMeasurement& aMeasurement) noexcept;
    void ProcessPendingFitReloadOnUiThread() noexcept;

    // Called by TradePreviewHostMenu on Skyrim's UI thread.
    void OnHostMenuShown(bool aApplySelection = true) noexcept;
    void OnHostMenuHidden() noexcept;

    [[nodiscard]] bool IsActive() const noexcept
    {
        return m_active.load(std::memory_order_acquire);
    }

private:
    enum class PreviewFitStage : std::uint8_t
    {
        kApplyBase,
        kMeasureBase,
        kMeasureFit,
        kAwaitUiReload,
        kAwaitReloadedModel,
        kDone,
        kFailed
    };

    void QueueHostShow() noexcept;
    void QueueHostHide() noexcept;
    void ApplySelectionLocked() noexcept;
    void ResetFitLocked() noexcept;
    void RequestMeasurementLocked(PreviewFitStage aStage) noexcept;

    World& m_world;
    InventoryEntry m_entry{};
    std::optional<Trade::ItemId> m_selectedItem;

    std::mutex m_managerMutex;
    std::atomic_bool m_active{};
    std::atomic_bool m_hostOpen{};
    std::atomic_bool m_hostMessagePending{};
    std::atomic_bool m_wantsHost{};
    bool m_hostAllows3D{};

    bool m_previewRegionValid{};
    float m_previewRegionLeft{};
    float m_previewRegionTop{};
    float m_previewRegionWidth{};
    float m_previewRegionHeight{};
    std::uint64_t m_previewRegionRevision{};
    std::uint64_t m_selectionRevision{};
    std::uint64_t m_appliedRegionRevision{};
    std::uint64_t m_appliedSelectionRevision{};

    bool m_baseTransformValid{};
    float m_baseTransformX{};
    float m_baseTransformY{};
    float m_baseTransformZ{};
    float m_baseTransformScale{1.0F};

    PreviewFitStage m_fitStage{PreviewFitStage::kApplyBase};
    std::uintptr_t m_fitSceneObject{};
    std::uint64_t m_fitSolverRevision{};
    float m_fitWorkingX{};
    float m_fitWorkingY{};
    float m_fitWorkingZ{};
    float m_fitWorkingScale{1.0F};
    std::uint32_t m_fitMeasurementFailures{};

    bool m_fitReloadPending{};
    std::uint64_t m_fitReloadSelectionRevision{};
    std::uint64_t m_fitReloadRegionRevision{};
    std::uintptr_t m_fitReloadPreviousSceneObject{};
    float m_fitReloadX{};
    float m_fitReloadY{};
    float m_fitReloadZ{};
    float m_fitReloadScale{1.0F};
};
