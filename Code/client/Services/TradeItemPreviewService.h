#pragma once

#include <Trade/TradeTypes.h>
#include <Misc/InventoryEntry.h>

#include <atomic>
#include <mutex>
#include <optional>

struct World;

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

    // Called by TradePreviewHostMenu on Skyrim's UI thread.
    void OnHostMenuShown(bool aApplySelection = true) noexcept;
    void OnHostMenuHidden() noexcept;

    [[nodiscard]] bool IsActive() const noexcept
    {
        return m_active.load(std::memory_order_acquire);
    }

private:
    void QueueHostShow() noexcept;
    void QueueHostHide() noexcept;
    void ApplySelectionLocked() noexcept;

    World& m_world;
    InventoryEntry m_entry{};
    std::optional<Trade::ItemId> m_selectedItem;

    std::mutex m_managerMutex;
    std::atomic_bool m_active{};
    std::atomic_bool m_hostOpen{};
    std::atomic_bool m_hostMessagePending{};
    std::atomic_bool m_wantsHost{};
    bool m_hostAllows3D{};
};
