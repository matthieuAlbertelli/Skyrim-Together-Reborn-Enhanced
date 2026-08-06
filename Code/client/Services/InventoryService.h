#pragma once

#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <chrono>
#include <Messages/NotifyInventoryChanges.h>

struct World;
struct TransportService;

struct UpdateEvent;
struct NotifyObjectInventoryChanges;
struct InventoryChangeEvent;
struct EquipmentChangeEvent;
struct NotifyEquipmentChanges;

/**
 * @brief Manages inventories of actors and containers.
 *
 * The initial contents of actor inventories are synced through spawn messages through CharacterService
 * @see CharacterService
 * The initial contents of container inventories are synced through the ObjectService
 * @see ObjectService
 * The InventoryService manages any changes to both the current equipment of actors,
 * and the contents of both actor and object inventories.
 */
struct InventoryService
{
    InventoryService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransport) noexcept;
    ~InventoryService() noexcept = default;

    TP_NOCOPYMOVE(InventoryService);

    /**
     * @brief Check weapon state.
     * @see RunWeaponStateUpdates
     */
    void OnUpdate(const UpdateEvent& acUpdateEvent) noexcept;
    /**
     * @brief Sends out inventory changes made by the local client.
     */
    void OnInventoryChangeEvent(const InventoryChangeEvent& acEvent) noexcept;
    /**
     * @brief Sends out equipment changes made by the local client.
     */
    void OnEquipmentChangeEvent(const EquipmentChangeEvent& acEvent) noexcept;

    /**
     * @brief Applies inventory changes sent by the server.
     */
    void OnNotifyInventoryChanges(const NotifyInventoryChanges& acMessage) noexcept;
    /**
     * @brief Applies equipment changes sent by the server.
     */
    void OnNotifyEquipmentChanges(const NotifyEquipmentChanges& acMessage) noexcept;

private:
    /**
     * Checks whether local actors their weapon draw states have changed,
     * and if so, send the new states to the server.
     */
    void RunWeaponStateUpdates() noexcept;
    /**
    * Checks whether an NPC's (local or remote) equipment is bugged (i.e. naked NPCS)
    * and resets their inventory.
    */
    void RunNakedNPCBugChecks() noexcept;
    bool TryMaterializeWorldEntity(const NotifyInventoryChanges& acMessage) noexcept;
    void RunPendingWorldEntitySnapshots() noexcept;
    void RunPendingRemoteWorldEntities() noexcept;
    void RunPendingDropStabilization() noexcept;
    void ApplyAuthoritativeTransform(uint64_t aWorldEntityId, const NotifyInventoryChanges& acMessage) noexcept;

    World& m_world;
    entt::dispatcher& m_dispatcher;
    TransportService& m_transport;

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_inventoryConnection;
    entt::scoped_connection m_equipmentConnection;
    entt::scoped_connection m_inventoryChangeConnection;
    entt::scoped_connection m_equipmentChangeConnection;

    std::unordered_map<uint64_t, uint32_t> m_worldEntityToFormId;
    std::unordered_map<uint32_t, uint64_t> m_formIdToWorldEntity;
    std::unordered_set<uint32_t> m_retiredWorldReferences;
    std::deque<NotifyInventoryChanges> m_pendingWorldEntitySnapshots;
    std::unordered_map<uint64_t, NotifyInventoryChanges> m_pendingRemoteWorldEntities;

    struct PendingDropStabilization
    {
        uint32_t LocalFormId{};
        std::chrono::steady_clock::time_point StartedAt{};
        std::chrono::steady_clock::time_point LastSampleAt{};
        float LastX{};
        float LastY{};
        float LastZ{};
        uint8_t StableSamples{};
        Inventory::Entry Item{};
        uint8_t RecreationAttempts{};
    };

    std::unordered_map<uint64_t, PendingDropStabilization> m_pendingDropStabilization;
};
