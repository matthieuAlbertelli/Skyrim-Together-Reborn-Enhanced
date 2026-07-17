#include <TiltedOnlinePCH.h>

#include <Services/TradeItemPreviewService.h>

#include <Forms/TESBoundObject.h>
#include <Forms/TESForm.h>
#include <Interface/Inventory3DManager.h>
#include <Interface/Menus/TradePreviewHostMenu.h>
#include <World.h>

#include <algorithm>
#include <cmath>

namespace
{
void LogManagerState(
    const char* apPhase,
    const Inventory3DManager* apManager,
    std::uint64_t aSelectedItem) noexcept
{
    if (!apManager)
        return;

    const Inventory3DManagerDebugState state =
        apManager->CaptureDebugState();

    spdlog::info(
        "Trade preview Inventory3D state phase={} loadStateProbe=true layout=AE1170 item={:016X} manager={:p} posCopy=({:.3f},{:.3f},{:.3f}) pos=({:.3f},{:.3f},{:.3f}) scaleCopy={:.3f} scale={:.3f} light={} tempRef={:p} loadedCapacityFlags={:08X} loadedCapacity={} loadedLocal={} loadedSize={} loadedData={:p} firstItemBase={:p} firstModelObject={:p} firstSceneObject={:p} zoom={:.3f} loadTask={:p} stateFlags={:08X} tailFlags={:08X}",
        apPhase,
        aSelectedItem,
        static_cast<const void*>(apManager),
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

TradeItemPreviewService::TradeItemPreviewService(
    World& aWorld,
    entt::dispatcher&) noexcept
    : m_world(aWorld)
{
    (void)TradePreviewHostMenu::Register();
}

TradeItemPreviewService::~TradeItemPreviewService() noexcept
{
    Clear();
}

void TradeItemPreviewService::ResetFitLocked() noexcept
{
    m_appliedSelectionRevision = 0;
    m_appliedRegionRevision = 0;
    m_fitStage = PreviewFitStage::kApplyBase;
    m_fitSceneObject = 0;
    ++m_fitSolverRevision;
    m_fitWorkingX = 0.0F;
    m_fitWorkingY = 0.0F;
    m_fitWorkingZ = 0.0F;
    m_fitWorkingScale = 1.0F;
    m_fitBaseCenterX = 0.0F;
    m_fitBaseCenterY = 0.0F;
    m_fitResponseXX = 0.0F;
    m_fitResponseXY = 0.0F;
    m_fitResponseZX = 0.0F;
    m_fitResponseZY = 0.0F;
    m_fitScaleIteration = 0;
    m_fitRefinementCount = 0;
    m_fitMeasurementFailures = 0;
    m_fitReloadPending = false;
    m_fitReloadSelectionRevision = 0;
    m_fitReloadRegionRevision = 0;
    m_fitReloadPreviousSceneObject = 0;
    m_fitReloadX = 0.0F;
    m_fitReloadY = 0.0F;
    m_fitReloadZ = 0.0F;
    m_fitReloadScale = 1.0F;
}
void TradeItemPreviewService::RequestMeasurementLocked(
    PreviewFitStage aStage) noexcept
{
    m_fitStage = aStage;
    ++m_fitSolverRevision;
}

void TradeItemPreviewService::SelectItem(
    Trade::ItemId aItemId) noexcept
{
    std::scoped_lock lock(m_managerMutex);

    if (m_selectedItem &&
        *m_selectedItem == aItemId &&
        m_active.load(std::memory_order_relaxed))
    {
        return;
    }

    const GameId serverId{
        static_cast<std::uint32_t>(aItemId >> 32),
        static_cast<std::uint32_t>(aItemId)};

    const std::uint32_t gameId =
        m_world.GetModSystem().GetGameId(serverId);

    TESForm* const pForm = gameId
        ? TESForm::GetById(gameId)
        : nullptr;
    TESBoundObject* const pObject = pForm
        ? Cast<TESBoundObject>(pForm)
        : nullptr;

    if (!pObject)
    {
        spdlog::warn(
            "Trade preview rejected item {:016X}: game form {:08X}",
            aItemId,
            gameId);
        return;
    }

    m_entry.pObject = pObject;
    m_entry.pExtraLists = nullptr;
    m_entry.count = 1;
    m_selectedItem = aItemId;
    ++m_selectionRevision;
    ResetFitLocked();
    m_active.store(true, std::memory_order_release);
    m_wantsHost.store(true, std::memory_order_release);

    if (m_hostOpen.load(std::memory_order_acquire))
    {
        if (m_hostAllows3D)
            ApplySelectionLocked();
    }
    else
    {
        QueueHostShow();
    }

    spdlog::info(
        "Trade preview selected item {:016X} (game form {:08X})",
        aItemId,
        gameId);
}

void TradeItemPreviewService::Clear() noexcept
{
    std::scoped_lock lock(m_managerMutex);

    m_wantsHost.store(false, std::memory_order_release);

    if (m_hostOpen.load(std::memory_order_acquire) &&
        m_hostAllows3D)
    {
        if (Inventory3DManager* const pManager =
                Inventory3DManager::GetSingleton())
        {
            pManager->Clear3D();
        }
    }

    m_entry = {};
    m_selectedItem.reset();
    ++m_selectionRevision;
    ResetFitLocked();
    m_active.store(false, std::memory_order_release);

    if (m_hostOpen.load(std::memory_order_acquire) ||
        m_hostMessagePending.load(std::memory_order_acquire))
    {
        QueueHostHide();
    }
}

void TradeItemPreviewService::SetPreviewRegion(
    float aLeft,
    float aTop,
    float aWidth,
    float aHeight) noexcept
{
    std::scoped_lock lock(m_managerMutex);

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

TradePreviewProjectionTelemetryState
TradeItemPreviewService::CaptureProjectionTelemetryState() noexcept
{
    std::scoped_lock lock(m_managerMutex);

    const bool measurementStage =
        m_fitStage == PreviewFitStage::kMeasureBase ||
        m_fitStage == PreviewFitStage::kMeasureScaled ||
        m_fitStage == PreviewFitStage::kMeasureXProbe ||
        m_fitStage == PreviewFitStage::kMeasureZProbe ||
        m_fitStage == PreviewFitStage::kMeasureFit;

    TradePreviewProjectionTelemetryState state{};
    state.valid =
        m_previewRegionValid &&
        m_active.load(std::memory_order_relaxed) &&
        m_selectedItem.has_value();
    state.captureRequested = state.valid && measurementStage;
    state.left = m_previewRegionLeft;
    state.top = m_previewRegionTop;
    state.width = m_previewRegionWidth;
    state.height = m_previewRegionHeight;
    state.selectionRevision = m_selectionRevision;
    state.regionRevision = m_previewRegionRevision;
    state.solverRevision = m_fitSolverRevision;
    state.selectedItem = m_selectedItem.value_or(0);
    return state;
}

void TradeItemPreviewService::UpdatePreviewPlacement() noexcept
{
    std::scoped_lock lock(m_managerMutex);

    if (!m_previewRegionValid ||
        !m_baseTransformValid ||
        !m_hostAllows3D ||
        !m_active.load(std::memory_order_relaxed) ||
        !m_selectedItem ||
        !m_entry.pObject ||
        m_fitStage == PreviewFitStage::kDone ||
        m_fitStage == PreviewFitStage::kFailed ||
        m_fitStage == PreviewFitStage::kAwaitUiReload)
    {
        return;
    }

    Inventory3DManager* const pManager =
        Inventory3DManager::GetSingleton();
    if (!pManager)
        return;

    const Inventory3DManagerDebugState state =
        pManager->CaptureDebugState();
    if (state.loadTask != 0 || state.loadedModelsSize == 0)
        return;

    const std::uintptr_t expectedModel =
        reinterpret_cast<std::uintptr_t>(m_entry.pObject);
    const Inventory3DManagerPreviewModelBounds bounds =
        pManager->CaptureModelBounds(expectedModel);
    if (!bounds.valid || !bounds.matchedExpectedModel)
        return;

    if (m_fitStage == PreviewFitStage::kAwaitReloadedModel)
    {
        const bool scenePointerReused =
            bounds.sceneObject == m_fitReloadPreviousSceneObject;

        m_fitSceneObject = bounds.sceneObject;
        m_fitWorkingX = state.itemPosX;
        m_fitWorkingY = state.itemPosY;
        m_fitWorkingZ = state.itemPosZ;
        m_fitWorkingScale = state.itemScale;
        m_fitMeasurementFailures = 0;
        RequestMeasurementLocked(PreviewFitStage::kMeasureFit);

        spdlog::info(
            "Trade preview UI reload observed uiThreadReloadProbe=true managerRestartProbe=true deterministicFit=true item={:016X} selectionRevision={} regionRevision={} previousScene={:p} currentScene={:p} scenePointerReused={} requestedTransform=({:.3f},{:.3f},{:.3f},{:.3f}) managerTransform=({:.3f},{:.3f},{:.3f},{:.3f}) loadTask={:p} loadedSize={} solverRevision={}",
            m_selectedItem.value_or(0),
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

    if (m_fitStage != PreviewFitStage::kApplyBase)
        return;

    m_fitSceneObject = bounds.sceneObject;
    m_fitWorkingX = state.itemPosX;
    m_fitWorkingY = state.itemPosY;
    m_fitWorkingZ = state.itemPosZ;
    m_fitWorkingScale = state.itemScale;
    m_fitMeasurementFailures = 0;
    RequestMeasurementLocked(PreviewFitStage::kMeasureBase);

    spdlog::info(
        "Trade preview deterministic fit measurement armed uiThreadReloadProbe=true deterministicFit=true phase=baseline item={:016X} selectionRevision={} regionRevision={} solverRevision={} transform=({:.3f},{:.3f},{:.3f},{:.3f}) scene={:p}",
        m_selectedItem.value_or(0),
        m_selectionRevision,
        m_previewRegionRevision,
        m_fitSolverRevision,
        m_fitWorkingX,
        m_fitWorkingY,
        m_fitWorkingZ,
        m_fitWorkingScale,
        reinterpret_cast<void*>(m_fitSceneObject));
}
void TradeItemPreviewService::SubmitProjectionMeasurement(
    const TradePreviewRasterMeasurement& aMeasurement) noexcept
{
    std::scoped_lock lock(m_managerMutex);

    if (!m_selectedItem ||
        aMeasurement.selectedItem != m_selectedItem.value_or(0) ||
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
            m_selectedItem.value_or(0),
            static_cast<std::uint32_t>(m_fitStage),
            m_fitMeasurementFailures,
            m_fitSolverRevision);
        if (m_fitMeasurementFailures >= 20)
        {
            m_fitStage = PreviewFitStage::kFailed;
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

    constexpr float kItemXPerScreenHeight = -0.0111472801F;
    constexpr float kItemZPerScreenHeight = -0.0111111111F;
    constexpr float kInitialFill = 0.94F;
    constexpr float kMinimumScale = 0.15F;
    constexpr float kMaximumScale = 8.0F;

    const float renderHeight =
        static_cast<float>(aMeasurement.targetHeight);
    if (!std::isfinite(renderHeight) || renderHeight <= 1.0F)
    {
        m_fitStage = PreviewFitStage::kFailed;
        return;
    }

    if (m_fitStage == PreviewFitStage::kMeasureBase)
    {
        const float pixelsPerItemX =
            kItemXPerScreenHeight * renderHeight;
        const float pixelsPerItemZ =
            kItemZPerScreenHeight * renderHeight;
        const float desiredPixelsX =
            aMeasurement.safeCenterX - aMeasurement.modelCenterX;
        const float desiredPixelsY =
            aMeasurement.safeCenterY - aMeasurement.modelCenterY;

        float scaleFactor =
            aMeasurement.containScale * kInitialFill;
        if (aMeasurement.touchesScreenEdge)
            scaleFactor = std::min(scaleFactor, 0.60F);
        scaleFactor = std::clamp(scaleFactor, 0.20F, 6.0F);

        m_fitWorkingX = std::clamp(
            m_baseTransformX + desiredPixelsX / pixelsPerItemX,
            -256.0F,
            256.0F);
        m_fitWorkingY = m_baseTransformY;
        m_fitWorkingZ = std::clamp(
            m_baseTransformZ + desiredPixelsY / pixelsPerItemZ,
            -256.0F,
            256.0F);
        m_fitWorkingScale = std::clamp(
            m_baseTransformScale * scaleFactor,
            kMinimumScale,
            kMaximumScale);

        m_fitReloadPending = true;
        m_fitReloadSelectionRevision = m_selectionRevision;
        m_fitReloadRegionRevision = m_previewRegionRevision;
        m_fitReloadPreviousSceneObject = m_fitSceneObject;
        m_fitReloadX = m_fitWorkingX;
        m_fitReloadY = m_fitWorkingY;
        m_fitReloadZ = m_fitWorkingZ;
        m_fitReloadScale = m_fitWorkingScale;
        m_fitScaleIteration = 1;
        m_fitRefinementCount = 0;
        m_fitStage = PreviewFitStage::kAwaitUiReload;
        ++m_fitSolverRevision;

        spdlog::info(
            "Trade preview UI reload queued uiThreadReloadProbe=true deterministicFit=true item={:016X} selectionRevision={} regionRevision={} previousScene={:p} modelCenter=({:.3f},{:.3f}) safeCenter=({:.3f},{:.3f}) desiredPixels=({:.3f},{:.3f}) containScale={:.6f} touchesScreenEdge={} queuedTransform=({:.3f},{:.3f},{:.3f},{:.3f}) solverRevision={}",
            m_selectedItem.value_or(0),
            m_selectionRevision,
            m_previewRegionRevision,
            reinterpret_cast<void*>(m_fitReloadPreviousSceneObject),
            aMeasurement.modelCenterX,
            aMeasurement.modelCenterY,
            aMeasurement.safeCenterX,
            aMeasurement.safeCenterY,
            desiredPixelsX,
            desiredPixelsY,
            aMeasurement.containScale,
            aMeasurement.touchesScreenEdge,
            m_fitReloadX,
            m_fitReloadY,
            m_fitReloadZ,
            m_fitReloadScale,
            m_fitSolverRevision);
        return;
    }

    if (m_fitStage != PreviewFitStage::kMeasureFit)
        return;

    m_fitStage = PreviewFitStage::kDone;
    m_appliedSelectionRevision = m_selectionRevision;
    m_appliedRegionRevision = m_previewRegionRevision;

    spdlog::info(
        "Trade preview UI reload validation complete uiThreadReloadProbe=true deterministicFit=true item={:016X} previousScene={:p} currentScene={:p} modelCenter=({:.3f},{:.3f}) safeCenter=({:.3f},{:.3f}) centerError=({:.3f},{:.3f}) modelSize={}x{} safeOverflow=({},{},{},{}) containScale={:.6f} insideSafe={} touchesScreenEdge={} fitted=({:.3f},{:.3f},{:.3f},{:.3f})",
        m_selectedItem.value_or(0),
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

void TradeItemPreviewService::ProcessPendingFitReloadOnUiThread() noexcept
{
    std::scoped_lock lock(m_managerMutex);

    if (!m_fitReloadPending)
        return;

    const std::uint64_t selectedItem =
        m_selectedItem.value_or(0);
    if (!m_hostAllows3D ||
        !m_active.load(std::memory_order_relaxed) ||
        !m_selectedItem ||
        !m_entry.pObject ||
        m_fitReloadSelectionRevision != m_selectionRevision ||
        m_fitReloadRegionRevision != m_previewRegionRevision ||
        m_fitStage != PreviewFitStage::kAwaitUiReload)
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
        m_fitStage = PreviewFitStage::kFailed;
        return;
    }

    Inventory3DManager* const pManager =
        Inventory3DManager::GetSingleton();
    if (!pManager)
    {
        spdlog::warn(
            "Trade preview UI reload deferred uiThreadReloadProbe=true deterministicFit=true reason=managerUnavailable item={:016X}",
            selectedItem);
        return;
    }

    const std::uintptr_t expectedModel =
        reinterpret_cast<std::uintptr_t>(m_entry.pObject);
    const Inventory3DManagerDebugState beforeState =
        pManager->CaptureDebugState();
    const Inventory3DManagerPreviewModelBounds beforeBounds =
        pManager->CaptureModelBounds(expectedModel);
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

    pManager->End3D();
    const Inventory3DManagerDebugState afterEndState =
        pManager->CaptureDebugState();
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

    pManager->Begin3D(lightScheme);
    const Inventory3DManagerDebugState afterBeginState =
        pManager->CaptureDebugState();
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

    pManager->SetPreviewTransform(
        m_fitReloadX,
        m_fitReloadY,
        m_fitReloadZ,
        m_fitReloadScale);
    LogManagerState(
        "before-update-manager-restart-reload",
        pManager,
        selectedItem);
    pManager->UpdateItem3D(&m_entry);
    LogManagerState(
        "after-update-manager-restart-reload",
        pManager,
        selectedItem);

    const Inventory3DManagerDebugState afterState =
        pManager->CaptureDebugState();
    const Inventory3DManagerPreviewModelBounds afterBounds =
        pManager->CaptureModelBounds(expectedModel);

    m_fitReloadPending = false;
    m_fitStage = PreviewFitStage::kAwaitReloadedModel;
    ++m_fitSolverRevision;

    spdlog::info(
        "Trade preview UI reload executing uiThreadReloadProbe=true managerRestartProbe=true deterministicFit=true phase=after item={:016X} previousScene={:p} immediateScene={:p} immediateSceneChanged={} managerTransform=({:.3f},{:.3f},{:.3f},{:.3f}) loadTask={:p} loadedSize={} solverRevision={}",
        selectedItem,
        reinterpret_cast<void*>(previousScene),
        reinterpret_cast<void*>(afterBounds.sceneObject),
        afterBounds.valid && afterBounds.sceneObject != previousScene,
        afterState.itemPosX,
        afterState.itemPosY,
        afterState.itemPosZ,
        afterState.itemScale,
        reinterpret_cast<void*>(afterState.loadTask),
        afterState.loadedModelsSize,
        m_fitSolverRevision);
}
void TradeItemPreviewService::OnHostMenuShown(
    bool aApplySelection) noexcept
{
    std::scoped_lock lock(m_managerMutex);

    m_hostOpen.store(true, std::memory_order_release);
    m_hostMessagePending.store(false, std::memory_order_release);
    m_hostAllows3D = aApplySelection;
    m_baseTransformValid = false;
    ResetFitLocked();

    if (Inventory3DManager* const pManager =
            Inventory3DManager::GetSingleton())
    {
        const Inventory3DManagerDebugState state =
            pManager->CaptureDebugState();
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

    if (!m_wantsHost.load(std::memory_order_acquire))
    {
        QueueHostHide();
        return;
    }

    if (aApplySelection)
        ApplySelectionLocked();
    else
        spdlog::info(
            "Trade preview host menu selection skipped lifecycleProbe=true");
}

void TradeItemPreviewService::OnHostMenuHidden() noexcept
{
    std::scoped_lock lock(m_managerMutex);

    m_hostOpen.store(false, std::memory_order_release);
    m_hostMessagePending.store(false, std::memory_order_release);
    m_hostAllows3D = false;
    m_baseTransformValid = false;
    ResetFitLocked();

    if (m_wantsHost.load(std::memory_order_acquire))
        QueueHostShow();
}

void TradeItemPreviewService::QueueHostShow() noexcept
{
    if (m_hostOpen.load(std::memory_order_acquire))
        return;

    bool expected = false;
    if (!m_hostMessagePending.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel))
    {
        return;
    }

    TradePreviewHostMenu::Show();
}

void TradeItemPreviewService::QueueHostHide() noexcept
{
    bool expected = false;
    if (!m_hostMessagePending.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel))
    {
        // A show may already be queued. Queueing hide after it is intentional.
        TradePreviewHostMenu::Hide();
        return;
    }

    TradePreviewHostMenu::Hide();
}

void TradeItemPreviewService::ApplySelectionLocked() noexcept
{
    if (!m_active.load(std::memory_order_relaxed) ||
        !m_entry.pObject)
    {
        return;
    }

    Inventory3DManager* const pManager =
        Inventory3DManager::GetSingleton();
    if (!pManager)
    {
        spdlog::warn(
            "Trade preview could not apply selection: Inventory3DManager unavailable");
        return;
    }

    const std::uint64_t selectedItem =
        m_selectedItem.value_or(0);

    LogManagerState("before-clear", pManager, selectedItem);
    pManager->Clear3D();
    LogManagerState("after-clear", pManager, selectedItem);

    if (m_baseTransformValid)
    {
        pManager->SetPreviewTransform(
            m_baseTransformX,
            m_baseTransformY,
            m_baseTransformZ,
            m_baseTransformScale);
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
        pManager,
        selectedItem);
    pManager->UpdateItem3D(&m_entry);
    LogManagerState(
        "after-update-deterministic-base",
        pManager,
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
