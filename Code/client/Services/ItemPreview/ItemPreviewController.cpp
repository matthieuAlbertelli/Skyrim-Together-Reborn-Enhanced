#include <TiltedOnlinePCH.h>

#include <Services/ItemPreview/ItemPreviewController.h>
#include <Services/ItemPreview/ItemPreviewFitSolver.h>


#include <algorithm>
#include <cmath>

namespace
{
void LogManagerState(
    const char* apPhase,
    const ItemPreviewNativeSession& aSession,
    std::uint64_t aSelectedItem) noexcept
{
    const Inventory3DManagerDebugState state =
        aSession.CaptureDebugState();
    void* const pManagerAddress = aSession.GetManagerAddress();
    if (!pManagerAddress)
        return;

    spdlog::info(
        "Trade preview Inventory3D state phase={} loadStateProbe=true layout=AE1170 item={:016X} manager={:p} posCopy=({:.3f},{:.3f},{:.3f}) pos=({:.3f},{:.3f},{:.3f}) scaleCopy={:.3f} scale={:.3f} light={} tempRef={:p} loadedCapacityFlags={:08X} loadedCapacity={} loadedLocal={} loadedSize={} loadedData={:p} firstItemBase={:p} firstModelObject={:p} firstSceneObject={:p} zoom={:.3f} loadTask={:p} stateFlags={:08X} tailFlags={:08X}",
        apPhase,
        aSelectedItem,
        pManagerAddress,
        state.itemPosCopyX,
        state.itemPosCopyY,
        state.itemPosCopyZ,
        state.itemPosX,
        state.itemPosY,
        state.itemPosZ,
        state.itemScaleCopy,
        state.itemScale,
        state.lightScheme,
        reinterpret_cast<void*>(state.tempRef),
        state.loadedModelsCapacityFlags,
        state.loadedModelsCapacity,
        state.loadedModelsLocal,
        state.loadedModelsSize,
        reinterpret_cast<void*>(state.loadedModelsData),
        reinterpret_cast<void*>(state.firstItemBase),
        reinterpret_cast<void*>(state.firstModelObject),
        reinterpret_cast<void*>(state.firstSceneObject),
        state.zoomProgress,
        reinterpret_cast<void*>(state.loadTask),
        state.stateFlags,
        state.tailFlags);
}
}

ItemPreviewController::~ItemPreviewController() noexcept
{
    ClearItem();
}

void ItemPreviewController::ResetFitLocked() noexcept
{
    m_appliedSelectionRevision = 0;
    m_appliedRegionRevision = 0;
    m_fitStage = FitStage::kApplyBase;
    m_fitSceneObject = 0;
    ++m_fitSolverRevision;
    m_fitWorkingX = 0.0F;
    m_fitWorkingY = 0.0F;
    m_fitWorkingZ = 0.0F;
    m_fitWorkingScale = 1.0F;
    m_fitMeasurementFailures = 0;
    m_fitRefinementCount = 0;
    m_fitReloadPending = false;
    m_fitReloadSelectionRevision = 0;
    m_fitReloadRegionRevision = 0;
    m_fitReloadPreviousSceneObject = 0;
    m_fitReloadX = 0.0F;
    m_fitReloadY = 0.0F;
    m_fitReloadZ = 0.0F;
    m_fitReloadScale = 1.0F;
}
void ItemPreviewController::RequestMeasurementLocked(
    FitStage aStage) noexcept
{
    m_fitStage = aStage;
    ++m_fitSolverRevision;
}

void ItemPreviewController::SetItem(
    const InventoryEntry& aEntry,
    std::uint64_t aContextId) noexcept
{
    std::scoped_lock lock(m_mutex);

    if (!aEntry.pObject)
    {
        spdlog::warn("Item preview rejected empty entry context={:016X}", aContextId);
        return;
    }

    if (m_contextId == aContextId &&
        m_entry.pObject == aEntry.pObject &&
        m_active.load(std::memory_order_relaxed))
    {
        return;
    }

    m_entry = aEntry;
    m_contextId = aContextId;
    ++m_selectionRevision;
    ResetFitLocked();
    m_active.store(true, std::memory_order_release);

    if (m_hostAllows3D)
        ApplySelectionLocked();
}

void ItemPreviewController::ClearItem() noexcept
{
    std::scoped_lock lock(m_mutex);

    if (m_hostAllows3D)
        (void)m_nativeSession.Clear();

    m_entry = {};
    m_contextId = 0;
    ++m_selectionRevision;
    ResetFitLocked();
    m_active.store(false, std::memory_order_release);
}

void ItemPreviewController::SetPreviewRegion(
    float aLeft,
    float aTop,
    float aWidth,
    float aHeight) noexcept
{
    std::scoped_lock lock(m_mutex);

    const bool finite =
        std::isfinite(aLeft) &&
        std::isfinite(aTop) &&
        std::isfinite(aWidth) &&
        std::isfinite(aHeight);

    if (!finite || aWidth <= 0.001F || aHeight <= 0.001F)
    {
        if (m_previewRegionValid)
        {
            m_previewRegionValid = false;
            ++m_previewRegionRevision;
            ResetFitLocked();
            spdlog::info(
                "Trade preview UI region cleared uiDrivenFit=true revision={}",
                m_previewRegionRevision);
        }
        return;
    }

    const float left = std::clamp(aLeft, 0.0F, 1.0F);
    const float top = std::clamp(aTop, 0.0F, 1.0F);
    const float width = std::clamp(aWidth, 0.0F, 1.0F - left);
    const float height = std::clamp(aHeight, 0.0F, 1.0F - top);

    constexpr float kRegionEpsilon = 0.0001F;
    const bool changed =
        !m_previewRegionValid ||
        std::abs(m_previewRegionLeft - left) > kRegionEpsilon ||
        std::abs(m_previewRegionTop - top) > kRegionEpsilon ||
        std::abs(m_previewRegionWidth - width) > kRegionEpsilon ||
        std::abs(m_previewRegionHeight - height) > kRegionEpsilon;

    if (!changed)
        return;

    m_previewRegionValid = true;
    m_previewRegionLeft = left;
    m_previewRegionTop = top;
    m_previewRegionWidth = width;
    m_previewRegionHeight = height;
    ++m_previewRegionRevision;
    ResetFitLocked();

    spdlog::info(
        "Trade preview UI region received uiDrivenFit=true revision={} rect=({:.6f},{:.6f},{:.6f},{:.6f}) center=({:.6f},{:.6f})",
        m_previewRegionRevision,
        left,
        top,
        width,
        height,
        left + width * 0.5F,
        top + height * 0.5F);
}

ItemPreviewRasterCaptureRequest
ItemPreviewController::CaptureRasterRequest() noexcept
{
    std::scoped_lock lock(m_mutex);

    const bool measurementStage =
        m_fitStage == FitStage::kMeasureBase ||
        m_fitStage == FitStage::kMeasureFit;

    ItemPreviewRasterCaptureRequest state{};
    state.valid =
        m_previewRegionValid &&
        m_active.load(std::memory_order_relaxed) &&
        m_contextId != 0;
    state.captureRequested = state.valid && measurementStage;
    state.left = m_previewRegionLeft;
    state.top = m_previewRegionTop;
    state.width = m_previewRegionWidth;
    state.height = m_previewRegionHeight;
    state.selectionRevision = m_selectionRevision;
    state.regionRevision = m_previewRegionRevision;
    state.solverRevision = m_fitSolverRevision;
    state.selectedItem = m_contextId;
    return state;
}

void ItemPreviewController::UpdatePreviewPlacement() noexcept
{
    std::scoped_lock lock(m_mutex);

    if (!m_previewRegionValid ||
        !m_baseTransformValid ||
        !m_hostAllows3D ||
        !m_active.load(std::memory_order_relaxed) ||
        m_contextId == 0 ||
        !m_entry.pObject ||
        m_fitStage == FitStage::kDone ||
        m_fitStage == FitStage::kFailed ||
        m_fitStage == FitStage::kAwaitUiReload)
    {
        return;
    }

    if (!m_nativeSession.IsOpen())
        return;

    const Inventory3DManagerDebugState state =
        m_nativeSession.CaptureDebugState();
    if (state.loadTask != 0 || state.loadedModelsSize == 0)
        return;

    const std::uintptr_t expectedModel =
        reinterpret_cast<std::uintptr_t>(m_entry.pObject);
    const Inventory3DManagerPreviewModelBounds bounds =
        m_nativeSession.CaptureModelBounds(expectedModel);
    if (!bounds.valid || !bounds.matchedExpectedModel)
        return;

    if (m_fitStage == FitStage::kAwaitReloadedModel)
    {
        const bool scenePointerReused =
            bounds.sceneObject == m_fitReloadPreviousSceneObject;

        m_fitSceneObject = bounds.sceneObject;
        m_fitWorkingX = state.itemPosX;
        m_fitWorkingY = state.itemPosY;
        m_fitWorkingZ = state.itemPosZ;
        m_fitWorkingScale = state.itemScale;
        m_fitMeasurementFailures = 0;
        RequestMeasurementLocked(FitStage::kMeasureFit);

        spdlog::info(
            "Trade preview UI reload observed uiThreadReloadProbe=true managerRestartProbe=true deterministicFit=true item={:016X} selectionRevision={} regionRevision={} previousScene={:p} currentScene={:p} scenePointerReused={} requestedTransform=({:.3f},{:.3f},{:.3f},{:.3f}) managerTransform=({:.3f},{:.3f},{:.3f},{:.3f}) loadTask={:p} loadedSize={} solverRevision={}",
            m_contextId,
            m_selectionRevision,
            m_previewRegionRevision,
            reinterpret_cast<void*>(m_fitReloadPreviousSceneObject),
            reinterpret_cast<void*>(bounds.sceneObject),
            scenePointerReused,
            m_fitReloadX,
            m_fitReloadY,
            m_fitReloadZ,
            m_fitReloadScale,
            state.itemPosX,
            state.itemPosY,
            state.itemPosZ,
            state.itemScale,
            reinterpret_cast<void*>(state.loadTask),
            state.loadedModelsSize,
            m_fitSolverRevision);
        return;
    }

    if (m_fitStage != FitStage::kApplyBase)
        return;

    m_fitSceneObject = bounds.sceneObject;
    m_fitWorkingX = state.itemPosX;
    m_fitWorkingY = state.itemPosY;
    m_fitWorkingZ = state.itemPosZ;
    m_fitWorkingScale = state.itemScale;
    m_fitMeasurementFailures = 0;
    RequestMeasurementLocked(FitStage::kMeasureBase);

    spdlog::info(
        "Trade preview deterministic fit measurement armed uiThreadReloadProbe=true deterministicFit=true phase=baseline item={:016X} selectionRevision={} regionRevision={} solverRevision={} transform=({:.3f},{:.3f},{:.3f},{:.3f}) scene={:p}",
        m_contextId,
        m_selectionRevision,
        m_previewRegionRevision,
        m_fitSolverRevision,
        m_fitWorkingX,
        m_fitWorkingY,
        m_fitWorkingZ,
        m_fitWorkingScale,
        reinterpret_cast<void*>(m_fitSceneObject));
}
void ItemPreviewController::SubmitRasterMeasurement(
    const ItemPreviewRasterMeasurement& aMeasurement) noexcept
{
    std::scoped_lock lock(m_mutex);

    if (m_contextId == 0 ||
        aMeasurement.selectedItem != m_contextId ||
        aMeasurement.selectionRevision != m_selectionRevision ||
        aMeasurement.regionRevision != m_previewRegionRevision ||
        aMeasurement.solverRevision != m_fitSolverRevision)
    {
        return;
    }

    if (!aMeasurement.valid)
    {
        ++m_fitMeasurementFailures;
        spdlog::warn(
            "Trade preview deterministic fit measurement rejected uiThreadReloadProbe=true deterministicFit=true item={:016X} stage={} failures={} solverRevision={}",
            m_contextId,
            static_cast<std::uint32_t>(m_fitStage),
            m_fitMeasurementFailures,
            m_fitSolverRevision);
        if (m_fitMeasurementFailures >= 20)
        {
            m_fitStage = FitStage::kFailed;
            m_appliedSelectionRevision = m_selectionRevision;
            m_appliedRegionRevision = m_previewRegionRevision;
        }
        else
        {
            ++m_fitSolverRevision;
        }
        return;
    }

    m_fitMeasurementFailures = 0;

    if (m_fitStage == FitStage::kMeasureBase)
    {
        const ItemPreviewFitResult fit = SolveItemPreviewFit(
            ItemPreviewTransform{
                m_baseTransformX,
                m_baseTransformY,
                m_baseTransformZ,
                m_baseTransformScale},
            ItemPreviewRasterBounds{
                aMeasurement.targetHeight,
                aMeasurement.modelCenterX,
                aMeasurement.modelCenterY,
                aMeasurement.safeCenterX,
                aMeasurement.safeCenterY,
                aMeasurement.containScale,
                aMeasurement.touchesScreenEdge});
        if (!fit.valid)
        {
            m_fitStage = FitStage::kFailed;
            spdlog::warn(
                "Trade preview deterministic fit solve rejected deterministicFit=true item={:016X}",
                m_contextId);
            return;
        }

        m_fitWorkingX = fit.transform.x;
        m_fitWorkingY = fit.transform.y;
        m_fitWorkingZ = fit.transform.z;
        m_fitWorkingScale = fit.transform.scale;

        m_fitReloadPending = true;
        m_fitReloadSelectionRevision = m_selectionRevision;
        m_fitReloadRegionRevision = m_previewRegionRevision;
        m_fitReloadPreviousSceneObject = m_fitSceneObject;
        m_fitReloadX = m_fitWorkingX;
        m_fitReloadY = m_fitWorkingY;
        m_fitReloadZ = m_fitWorkingZ;
        m_fitReloadScale = m_fitWorkingScale;
        m_fitStage = FitStage::kAwaitUiReload;
        ++m_fitSolverRevision;

        spdlog::info(
            "Trade preview UI reload queued uiThreadReloadProbe=true deterministicFit=true item={:016X} selectionRevision={} regionRevision={} previousScene={:p} modelCenter=({:.3f},{:.3f}) safeCenter=({:.3f},{:.3f}) desiredPixels=({:.3f},{:.3f}) containScale={:.6f} touchesScreenEdge={} queuedTransform=({:.3f},{:.3f},{:.3f},{:.3f}) solverRevision={}",
            m_contextId,
            m_selectionRevision,
            m_previewRegionRevision,
            reinterpret_cast<void*>(m_fitReloadPreviousSceneObject),
            aMeasurement.modelCenterX,
            aMeasurement.modelCenterY,
            aMeasurement.safeCenterX,
            aMeasurement.safeCenterY,
            fit.desiredPixelsX,
            fit.desiredPixelsY,
            aMeasurement.containScale,
            aMeasurement.touchesScreenEdge,
            m_fitReloadX,
            m_fitReloadY,
            m_fitReloadZ,
            m_fitReloadScale,
            m_fitSolverRevision);
        return;
    }

    if (m_fitStage != FitStage::kMeasureFit)
        return;

    constexpr std::uint8_t kMaximumRefinementCount = 1;
    if (!aMeasurement.insideSafeTarget &&
        !aMeasurement.touchesScreenEdge &&
        m_fitRefinementCount < kMaximumRefinementCount)
    {
        const ItemPreviewFitResult refinement = SolveItemPreviewFit(
            ItemPreviewTransform{
                m_fitWorkingX,
                m_fitWorkingY,
                m_fitWorkingZ,
                m_fitWorkingScale},
            ItemPreviewRasterBounds{
                aMeasurement.targetHeight,
                aMeasurement.modelCenterX,
                aMeasurement.modelCenterY,
                aMeasurement.safeCenterX,
                aMeasurement.safeCenterY,
                aMeasurement.containScale,
                aMeasurement.touchesScreenEdge});
        if (refinement.valid)
        {
            m_fitWorkingX = refinement.transform.x;
            m_fitWorkingY = refinement.transform.y;
            m_fitWorkingZ = refinement.transform.z;
            m_fitWorkingScale = refinement.transform.scale;

            ++m_fitRefinementCount;
            m_fitReloadPending = true;
            m_fitReloadSelectionRevision = m_selectionRevision;
            m_fitReloadRegionRevision = m_previewRegionRevision;
            m_fitReloadPreviousSceneObject = m_fitSceneObject;
            m_fitReloadX = m_fitWorkingX;
            m_fitReloadY = m_fitWorkingY;
            m_fitReloadZ = m_fitWorkingZ;
            m_fitReloadScale = m_fitWorkingScale;
            m_fitStage = FitStage::kAwaitUiReload;
            ++m_fitSolverRevision;

            spdlog::info(
                "Trade preview deterministic fit refinement queued deterministicFit=true item={:016X} refinement={}/{} centerError=({:.3f},{:.3f}) safeOverflow=({},{},{},{}) containScale={:.6f} queuedTransform=({:.3f},{:.3f},{:.3f},{:.3f}) solverRevision={}",
                m_contextId,
                m_fitRefinementCount,
                kMaximumRefinementCount,
                aMeasurement.modelCenterX - aMeasurement.safeCenterX,
                aMeasurement.modelCenterY - aMeasurement.safeCenterY,
                aMeasurement.safeOverflowLeft,
                aMeasurement.safeOverflowTop,
                aMeasurement.safeOverflowRight,
                aMeasurement.safeOverflowBottom,
                aMeasurement.containScale,
                m_fitReloadX,
                m_fitReloadY,
                m_fitReloadZ,
                m_fitReloadScale,
                m_fitSolverRevision);
            return;
        }
    }

    m_fitStage = FitStage::kDone;
    m_appliedSelectionRevision = m_selectionRevision;
    m_appliedRegionRevision = m_previewRegionRevision;

    spdlog::info(
        "Trade preview UI reload validation complete uiThreadReloadProbe=true deterministicFit=true item={:016X} previousScene={:p} currentScene={:p} modelCenter=({:.3f},{:.3f}) safeCenter=({:.3f},{:.3f}) centerError=({:.3f},{:.3f}) modelSize={}x{} safeOverflow=({},{},{},{}) containScale={:.6f} insideSafe={} touchesScreenEdge={} fitted=({:.3f},{:.3f},{:.3f},{:.3f})",
        m_contextId,
        reinterpret_cast<void*>(m_fitReloadPreviousSceneObject),
        reinterpret_cast<void*>(m_fitSceneObject),
        aMeasurement.modelCenterX,
        aMeasurement.modelCenterY,
        aMeasurement.safeCenterX,
        aMeasurement.safeCenterY,
        aMeasurement.modelCenterX - aMeasurement.safeCenterX,
        aMeasurement.modelCenterY - aMeasurement.safeCenterY,
        aMeasurement.modelRight - aMeasurement.modelLeft,
        aMeasurement.modelBottom - aMeasurement.modelTop,
        aMeasurement.safeOverflowLeft,
        aMeasurement.safeOverflowTop,
        aMeasurement.safeOverflowRight,
        aMeasurement.safeOverflowBottom,
        aMeasurement.containScale,
        aMeasurement.insideSafeTarget,
        aMeasurement.touchesScreenEdge,
        m_fitWorkingX,
        m_fitWorkingY,
        m_fitWorkingZ,
        m_fitWorkingScale);
}

void ItemPreviewController::ProcessPendingReloadOnUiThread() noexcept
{
    std::scoped_lock lock(m_mutex);

    if (!m_fitReloadPending)
        return;

    const std::uint64_t selectedItem =
        m_contextId;
    if (!m_hostAllows3D ||
        !m_active.load(std::memory_order_relaxed) ||
        m_contextId == 0 ||
        !m_entry.pObject ||
        m_fitReloadSelectionRevision != m_selectionRevision ||
        m_fitReloadRegionRevision != m_previewRegionRevision ||
        m_fitStage != FitStage::kAwaitUiReload)
    {
        spdlog::warn(
            "Trade preview UI reload discarded uiThreadReloadProbe=true deterministicFit=true reason=staleOrInactive item={:016X} pendingSelectionRevision={} currentSelectionRevision={} pendingRegionRevision={} currentRegionRevision={} stage={}",
            selectedItem,
            m_fitReloadSelectionRevision,
            m_selectionRevision,
            m_fitReloadRegionRevision,
            m_previewRegionRevision,
            static_cast<std::uint32_t>(m_fitStage));
        m_fitReloadPending = false;
        m_fitStage = FitStage::kFailed;
        return;
    }

    if (!m_nativeSession.IsOpen() ||
        !m_nativeSession.GetManagerAddress())
    {
        spdlog::warn(
            "Trade preview UI reload deferred uiThreadReloadProbe=true deterministicFit=true reason=managerUnavailable item={:016X}",
            selectedItem);
        return;
    }

    const std::uintptr_t expectedModel =
        reinterpret_cast<std::uintptr_t>(m_entry.pObject);
    const Inventory3DManagerDebugState beforeState =
        m_nativeSession.CaptureDebugState();
    const Inventory3DManagerPreviewModelBounds beforeBounds =
        m_nativeSession.CaptureModelBounds(expectedModel);
    const std::uintptr_t previousScene =
        beforeBounds.valid && beforeBounds.matchedExpectedModel
            ? beforeBounds.sceneObject
            : m_fitReloadPreviousSceneObject;
    m_fitReloadPreviousSceneObject = previousScene;

    spdlog::info(
        "Trade preview UI reload executing uiThreadReloadProbe=true deterministicFit=true phase=before item={:016X} selectionRevision={} regionRevision={} previousScene={:p} managerTransform=({:.3f},{:.3f},{:.3f},{:.3f}) requestedTransform=({:.3f},{:.3f},{:.3f},{:.3f}) loadTask={:p} loadedSize={}",
        selectedItem,
        m_selectionRevision,
        m_previewRegionRevision,
        reinterpret_cast<void*>(previousScene),
        beforeState.itemPosX,
        beforeState.itemPosY,
        beforeState.itemPosZ,
        beforeState.itemScale,
        m_fitReloadX,
        m_fitReloadY,
        m_fitReloadZ,
        m_fitReloadScale,
        reinterpret_cast<void*>(beforeState.loadTask),
        beforeState.loadedModelsSize);

    const std::uint32_t lightScheme =
        beforeState.lightScheme != 0
            ? beforeState.lightScheme
            : 1;

    m_nativeSession.End();
    const Inventory3DManagerDebugState afterEndState =
        m_nativeSession.CaptureDebugState();
    spdlog::info(
        "Trade preview UI reload executing uiThreadReloadProbe=true managerRestartProbe=true deterministicFit=true phase=after-end item={:016X} lightScheme={} loadedSize={} loadTask={:p} managerTransform=({:.3f},{:.3f},{:.3f},{:.3f})",
        selectedItem,
        lightScheme,
        afterEndState.loadedModelsSize,
        reinterpret_cast<void*>(afterEndState.loadTask),
        afterEndState.itemPosX,
        afterEndState.itemPosY,
        afterEndState.itemPosZ,
        afterEndState.itemScale);

    const bool restarted = m_nativeSession.Begin(lightScheme);
    const Inventory3DManagerDebugState afterBeginState =
        m_nativeSession.CaptureDebugState();
    spdlog::info(
        "Trade preview UI reload executing uiThreadReloadProbe=true managerRestartProbe=true deterministicFit=true phase=after-begin item={:016X} lightScheme={} loadedSize={} loadTask={:p} managerTransform=({:.3f},{:.3f},{:.3f},{:.3f})",
        selectedItem,
        lightScheme,
        afterBeginState.loadedModelsSize,
        reinterpret_cast<void*>(afterBeginState.loadTask),
        afterBeginState.itemPosX,
        afterBeginState.itemPosY,
        afterBeginState.itemPosZ,
        afterBeginState.itemScale);

    const ItemPreviewTransform reloadTransform{
        m_fitReloadX,
        m_fitReloadY,
        m_fitReloadZ,
        m_fitReloadScale};
    LogManagerState(
        "before-update-manager-restart-reload",
        m_nativeSession,
        selectedItem);
    const bool loaded = restarted &&
        m_nativeSession.Load(m_entry, &reloadTransform);
    LogManagerState(
        "after-update-manager-restart-reload",
        m_nativeSession,
        selectedItem);

    const Inventory3DManagerDebugState afterState =
        m_nativeSession.CaptureDebugState();
    const Inventory3DManagerPreviewModelBounds afterBounds =
        m_nativeSession.CaptureModelBounds(expectedModel);

    m_fitReloadPending = false;
    m_fitStage = FitStage::kAwaitReloadedModel;
    ++m_fitSolverRevision;

    spdlog::info(
        "Trade preview UI reload executing uiThreadReloadProbe=true managerRestartProbe=true deterministicFit=true phase=after item={:016X} previousScene={:p} immediateScene={:p} immediateSceneChanged={} managerTransform=({:.3f},{:.3f},{:.3f},{:.3f}) loadTask={:p} loadedSize={} solverRevision={}",
        selectedItem,
        reinterpret_cast<void*>(previousScene),
        reinterpret_cast<void*>(afterBounds.sceneObject),
        loaded && afterBounds.valid && afterBounds.sceneObject != previousScene,
        afterState.itemPosX,
        afterState.itemPosY,
        afterState.itemPosZ,
        afterState.itemScale,
        reinterpret_cast<void*>(afterState.loadTask),
        afterState.loadedModelsSize,
        m_fitSolverRevision);
}
bool ItemPreviewController::BeginSession(
    std::uint32_t aLightScheme) noexcept
{
    std::scoped_lock lock(m_mutex);
    return m_nativeSession.Begin(aLightScheme);
}

void ItemPreviewController::EndSession() noexcept
{
    std::scoped_lock lock(m_mutex);
    m_nativeSession.End();
}

void ItemPreviewController::OnHostShown(
    bool aApplySelection) noexcept
{
    std::scoped_lock lock(m_mutex);

    m_hostAllows3D = aApplySelection;
    m_baseTransformValid = false;
    ResetFitLocked();

    if (m_nativeSession.IsOpen())
    {
        const Inventory3DManagerDebugState state =
            m_nativeSession.CaptureDebugState();
        if (std::isfinite(state.itemPosX) &&
            std::isfinite(state.itemPosY) &&
            std::isfinite(state.itemPosZ) &&
            std::isfinite(state.itemScale) &&
            state.itemScale > 0.0F)
        {
            m_baseTransformX = state.itemPosX;
            m_baseTransformY = state.itemPosY;
            m_baseTransformZ = state.itemPosZ;
            m_baseTransformScale = state.itemScale;
            m_baseTransformValid = true;
            spdlog::info(
                "Trade preview native base transform captured rasterFitProbe=true base=({:.3f},{:.3f},{:.3f},{:.3f})",
                m_baseTransformX,
                m_baseTransformY,
                m_baseTransformZ,
                m_baseTransformScale);
        }
    }

    if (aApplySelection)
        ApplySelectionLocked();
    else
        spdlog::info(
            "Trade preview host menu selection skipped lifecycleProbe=true");
}

void ItemPreviewController::OnHostHidden() noexcept
{
    std::scoped_lock lock(m_mutex);
    m_hostAllows3D = false;
    m_baseTransformValid = false;
    ResetFitLocked();
}

void ItemPreviewController::ApplySelectionLocked() noexcept
{
    if (!m_active.load(std::memory_order_relaxed) ||
        !m_entry.pObject)
    {
        return;
    }

    if (!m_nativeSession.IsOpen() ||
        !m_nativeSession.GetManagerAddress())
    {
        spdlog::warn(
            "Trade preview could not apply selection: Inventory3DManager unavailable");
        return;
    }

    const std::uint64_t selectedItem =
        m_contextId;

    LogManagerState("before-clear", m_nativeSession, selectedItem);
    (void)m_nativeSession.Clear();
    LogManagerState("after-clear", m_nativeSession, selectedItem);

    ItemPreviewTransform baseTransform{};
    const ItemPreviewTransform* pBaseTransform = nullptr;
    if (m_baseTransformValid)
    {
        baseTransform = ItemPreviewTransform{
            m_baseTransformX,
            m_baseTransformY,
            m_baseTransformZ,
            m_baseTransformScale};
        pBaseTransform = &baseTransform;
        spdlog::info(
            "Trade preview deterministic fit base applied deterministicFit=true item={:016X} transform=({:.3f},{:.3f},{:.3f},{:.3f})",
            selectedItem,
            m_baseTransformX,
            m_baseTransformY,
            m_baseTransformZ,
            m_baseTransformScale);
    }
    else
    {
        spdlog::warn(
            "Trade preview deterministic fit base unavailable deterministicFit=true item={:016X}",
            selectedItem);
    }

    LogManagerState(
        "before-update-deterministic-base",
        m_nativeSession,
        selectedItem);
    (void)m_nativeSession.Load(m_entry, pBaseTransform);
    LogManagerState(
        "after-update-deterministic-base",
        m_nativeSession,
        selectedItem);

    ResetFitLocked();
    spdlog::info(
        "Trade preview deterministic fit pending deterministicFit=true item={:016X} selectionRevision={} regionRevision={} regionValid={} baseValid={}",
        selectedItem,
        m_selectionRevision,
        m_previewRegionRevision,
        m_previewRegionValid,
        m_baseTransformValid);
}
