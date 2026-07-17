#pragma once

#include <Trade/TradeTypes.h>
#include <Misc/InventoryEntry.h>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>

struct World;

struct TradePreviewProjectionTelemetryState
{
    bool valid{};
    bool captureRequested{};
    float left{};
    float top{};
    float width{};
    float height{};
    std::uint64_t selectionRevision{};
    std::uint64_t regionRevision{};
    std::uint64_t solverRevision{};
    std::uint64_t selectedItem{};
};

struct TradePreviewRasterMeasurement
{
    bool valid{};
    std::uint64_t selectionRevision{};
    std::uint64_t regionRevision{};
    std::uint64_t solverRevision{};
    std::uint64_t selectedItem{};

    std::uint32_t targetWidth{};
    std::uint32_t targetHeight{};
    std::uint32_t modelLeft{};
    std::uint32_t modelTop{};
    std::uint32_t modelRight{};
    std::uint32_t modelBottom{};
    std::uint32_t safeLeft{};
    std::uint32_t safeTop{};
    std::uint32_t safeRight{};
    std::uint32_t safeBottom{};
    std::uint32_t safeOverflowLeft{};
    std::uint32_t safeOverflowTop{};
    std::uint32_t safeOverflowRight{};
    std::uint32_t safeOverflowBottom{};

    float modelCenterX{};
    float modelCenterY{};
    float safeCenterX{};
    float safeCenterY{};
    float containScale{};
    bool insideSafeTarget{};
    bool touchesScreenEdge{};
};

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
    [[nodiscard]] TradePreviewProjectionTelemetryState
    CaptureProjectionTelemetryState() noexcept;
    void SubmitProjectionMeasurement(
        const TradePreviewRasterMeasurement& aMeasurement) noexcept;
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
        kMeasureScaled,
        kMeasureXProbe,
        kMeasureZProbe,
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
    float m_fitBaseCenterX{};
    float m_fitBaseCenterY{};
    float m_fitResponseXX{};
    float m_fitResponseXY{};
    float m_fitResponseZX{};
    float m_fitResponseZY{};
    std::uint32_t m_fitScaleIteration{};
    std::uint32_t m_fitRefinementCount{};
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
