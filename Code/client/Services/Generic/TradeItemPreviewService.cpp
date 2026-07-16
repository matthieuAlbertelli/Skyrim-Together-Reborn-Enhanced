#include <TiltedOnlinePCH.h>

#include <Services/TradeItemPreviewService.h>

#include <Forms/TESBoundObject.h>
#include <Forms/TESForm.h>
#include <Interface/Inventory3DManager.h>
#include <Interface/Menus/TradePreviewHostMenu.h>
#include <World.h>

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
    m_active.store(false, std::memory_order_release);

    if (m_hostOpen.load(std::memory_order_acquire) ||
        m_hostMessagePending.load(std::memory_order_acquire))
    {
        QueueHostHide();
    }
}

void TradeItemPreviewService::OnHostMenuShown(
    bool aApplySelection) noexcept
{
    std::scoped_lock lock(m_managerMutex);

    m_hostOpen.store(true, std::memory_order_release);
    m_hostMessagePending.store(false, std::memory_order_release);
    m_hostAllows3D = aApplySelection;

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

    spdlog::info(
        "Trade preview selection Clear3D calling updateItemProbe=true manager={:p} item={:016X} object={:p}",
        static_cast<void*>(pManager),
        selectedItem,
        static_cast<void*>(m_entry.pObject));
    pManager->Clear3D();
    spdlog::info(
        "Trade preview selection Clear3D returned updateItemProbe=true");
    LogManagerState("after-clear", pManager, selectedItem);

    spdlog::info(
        "Trade preview selection UpdateItem3D calling updateItemProbe=true manager={:p} entry={:p} object={:p}",
        static_cast<void*>(pManager),
        static_cast<void*>(&m_entry),
        static_cast<void*>(m_entry.pObject));
    pManager->UpdateItem3D(&m_entry);
    spdlog::info(
        "Trade preview selection UpdateItem3D returned updateItemProbe=true");
    LogManagerState("after-update", pManager, selectedItem);
}
