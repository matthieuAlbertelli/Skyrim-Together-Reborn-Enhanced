#pragma once

#include <atomic>
#include <unordered_map>
#include <unordered_set>

#include <Events/PacketEvent.h>
#include <Structs/Inventory.h>

struct World;
struct UpdateEvent;
struct RequestObjectInventoryChanges;
struct RequestInventoryChanges;
struct RequestEquipmentChanges;
struct DrawWeaponRequest;
struct PlayerLeaveCellEvent;
struct PlayerEnterWorldEvent;

/**
 * @brief Relays inventory/equipment changes and updates the server side state.
 */
class InventoryService
{
public:
    InventoryService(World& aWorld, entt::dispatcher& aDispatcher);

    /**
     * @brief Relays inventory changes to other clients and updates server side inventories.
     */
    void OnInventoryChanges(const PacketEvent<RequestInventoryChanges>& acMessage) noexcept;
    /**
     * @brief Relays equipment changes to other clients and updates server side equipment.
     */
    void OnEquipmentChanges(const PacketEvent<RequestEquipmentChanges>& acMessage) noexcept;
    /**
     * @brief Relays weapon draw changes to other clients and updates server side weapon draw state.
     */
    void OnWeaponDrawnRequest(const PacketEvent<DrawWeaponRequest>& acMessage) noexcept;
    void OnPlayerEnterWorld(const PlayerEnterWorldEvent& acEvent) noexcept;

private:
    World& m_world;

    entt::scoped_connection m_inventoryChangeConnection;
    entt::scoped_connection m_equipmentChangeConnection;
    entt::scoped_connection m_drawWeaponConnection;
    entt::scoped_connection m_playerEnterWorldConnection;

    struct SessionWorldEntity
    {
        uint32_t SourceServerId{};
        Inventory::Entry Item{};
        bool HasTransform = false;
        float PositionX{};
        float PositionY{};
        float PositionZ{};
        float RotationX{};
        float RotationY{};
        float RotationZ{};
    };

    std::atomic_uint64_t m_nextWorldEntityId{1};
    std::unordered_map<uint64_t, SessionWorldEntity> m_worldEntities;
};
