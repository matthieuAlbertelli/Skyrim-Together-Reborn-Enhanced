#pragma once

#include <Services/ItemPreview/ItemPreviewNativeSession.h>
#include <Services/ItemPreview/ItemPreviewRasterTypes.h>
#include <Misc/InventoryEntry.h>

#include <atomic>
#include <cstdint>
#include <mutex>

class ItemPreviewController
{
public:
    ItemPreviewController() noexcept = default;
    ~ItemPreviewController() noexcept;

    TP_NOCOPYMOVE(ItemPreviewController);

    void SetItem(const InventoryEntry& aEntry, std::uint64_t aContextId) noexcept;
    void ClearItem() noexcept;
    void SetPreviewRegion(float aLeft, float aTop, float aWidth, float aHeight) noexcept;
    void UpdatePreviewPlacement() noexcept;
    [[nodiscard]] ItemPreviewRasterCaptureRequest CaptureRasterRequest() noexcept;
    void SubmitRasterMeasurement(const ItemPreviewRasterMeasurement& aMeasurement) noexcept;
    void ProcessPendingReloadOnUiThread() noexcept;

    [[nodiscard]] bool BeginSession(std::uint32_t aLightScheme = 1) noexcept;
    void EndSession() noexcept;
    void OnHostShown(bool aApplySelection = true) noexcept;
    void OnHostHidden() noexcept;

    [[nodiscard]] bool IsActive() const noexcept
    {
        return m_active.load(std::memory_order_acquire);
    }

private:
    enum class FitStage : std::uint8_t
    {
        kApplyBase,
        kMeasureBase,
        kMeasureFit,
        kAwaitUiReload,
        kAwaitReloadedModel,
        kDone,
        kFailed
    };

    void ApplySelectionLocked() noexcept;
    void ResetFitLocked() noexcept;
    void RequestMeasurementLocked(FitStage aStage) noexcept;

    ItemPreviewNativeSession m_nativeSession;
    InventoryEntry m_entry{};
    std::uint64_t m_contextId{};

    std::mutex m_mutex;
    std::atomic_bool m_active{};
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

    FitStage m_fitStage{FitStage::kApplyBase};
    std::uintptr_t m_fitSceneObject{};
    std::uint64_t m_fitSolverRevision{};
    float m_fitWorkingX{};
    float m_fitWorkingY{};
    float m_fitWorkingZ{};
    float m_fitWorkingScale{1.0F};
    std::uint32_t m_fitMeasurementFailures{};
    std::uint8_t m_fitRefinementCount{};

    bool m_fitReloadPending{};
    std::uint64_t m_fitReloadSelectionRevision{};
    std::uint64_t m_fitReloadRegionRevision{};
    std::uintptr_t m_fitReloadPreviousSceneObject{};
    float m_fitReloadX{};
    float m_fitReloadY{};
    float m_fitReloadZ{};
    float m_fitReloadScale{1.0F};
};
