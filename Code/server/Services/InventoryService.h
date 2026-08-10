#pragma once

#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <Structs/WorldEntity.h>
#include <Structs/GameId.h>

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
struct PlayerLeaveEvent;
struct RequestWorldEntityManipulation;
struct NotifyWorldEntityManipulation;

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
    void OnWorldEntityManipulation(const PacketEvent<RequestWorldEntityManipulation>& acMessage) noexcept;
    void OnUpdate(const UpdateEvent& acEvent) noexcept;
    void OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept;

private:
    World& m_world;

    entt::scoped_connection m_inventoryChangeConnection;
    entt::scoped_connection m_equipmentChangeConnection;
    entt::scoped_connection m_drawWeaponConnection;
    entt::scoped_connection m_playerEnterWorldConnection;
    entt::scoped_connection m_worldEntityManipulationConnection;
    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_playerLeaveConnection;

    enum class SessionWorldEntityState : uint8_t
    {
        Free,
        Manipulated,
        Settling,
    };

    struct SessionWorldEntity
    {
        uint32_t SourceServerId{};
        uint32_t AuthorityPlayerId{};
        uint32_t AuthorityServerId{};
        Inventory::Entry Item{};
        GameId PlacedReferenceId{};
        SessionWorldEntityState State{SessionWorldEntityState::Settling};
        std::chrono::steady_clock::time_point LastAuthorityUpdate{};
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
    std::unordered_map<GameId, uint64_t> m_placedReferenceEntities;

    uint64_t ResolveOrAdoptPlacedReference(const GameId& acReferenceId, uint32_t aSourceServerId, const Inventory::Entry* apItem, const WorldEntityTransform* apTransform) noexcept;
    void EraseWorldEntity(uint64_t aWorldEntityId) noexcept;
    static WorldEntityTransform GetTransform(const SessionWorldEntity& acEntity) noexcept;
    static void SetTransform(SessionWorldEntity& arEntity, const WorldEntityTransform& acTransform) noexcept;
    void BroadcastManipulation(const NotifyWorldEntityManipulation& acNotify, uint32_t aOriginServerId, const Player* apExcludedPlayer = nullptr) noexcept;
};
