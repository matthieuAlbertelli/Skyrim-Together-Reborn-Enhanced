#pragma once

#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <chrono>
#include <Messages/NotifyInventoryChanges.h>
#include <Messages/NotifyWorldEntityManipulation.h>
#include <Structs/WorldEntity.h>
#include <Structs/GameId.h>
#include <Events/EventDispatcher.h>
#include <Games/Events.h>
#include <optional>

struct World;
struct TransportService;

struct UpdateEvent;
struct NotifyObjectInventoryChanges;
struct InventoryChangeEvent;
struct EquipmentChangeEvent;
struct NotifyEquipmentChanges;
struct NotifyWorldEntityManipulation;

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
struct InventoryService final : public BSTEventSink<TESGrabReleaseEvent>
{
    InventoryService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransport) noexcept;
    ~InventoryService() noexcept;

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
    void OnNotifyWorldEntityManipulation(const NotifyWorldEntityManipulation& acMessage) noexcept;
    BSTEventResult OnEvent(const TESGrabReleaseEvent* apEvent, const EventDispatcher<TESGrabReleaseEvent>* apSender) override;

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
    void ResetWorldSyncState() noexcept;
    bool TryMaterializeWorldEntity(const NotifyInventoryChanges& acMessage) noexcept;
    void RunPendingWorldEntitySnapshots() noexcept;
    void RunPendingRemoteWorldEntities() noexcept;
    void RunPendingRemoteWorldEntityManipulations() noexcept;
    void RunPendingDropStabilization() noexcept;
    void StoreAuthoritativeTransform(uint64_t aWorldEntityId, const NotifyInventoryChanges& acMessage) noexcept;
    void RunLocalWorldEntityManipulation() noexcept;
    void RunBlockingMenuGrabRelease() noexcept;
    void ProcessNotifyWorldEntityManipulation(const NotifyWorldEntityManipulation& acMessage) noexcept;
    void BeginRemoteWorldEntityManipulation(const NotifyWorldEntityManipulation& acMessage) noexcept;
    void EndRemoteWorldEntityManipulation(const NotifyWorldEntityManipulation& acMessage) noexcept;
    bool BindPlacedWorldEntity(uint64_t aWorldEntityId, const GameId& acReferenceId, uint32_t aSourceServerId = 0) noexcept;
    bool ApplyPlacedWorldEntityTransform(uint64_t aWorldEntityId, const WorldEntityTransform& acTransform, bool aEnableAfter) noexcept;
    void SendLocalManipulationRelease(const WorldEntityTransform& acTransform) noexcept;
    void StartSettlementTracking(uint64_t aWorldEntityId, uint32_t aLocalFormId, bool aIsAuthority) noexcept;
    static WorldEntityTransform ReadWorldEntityTransform(const TESObjectREFR* apReference) noexcept;

    World& m_world;
    entt::dispatcher& m_dispatcher;
    TransportService& m_transport;

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_inventoryConnection;
    entt::scoped_connection m_equipmentConnection;
    entt::scoped_connection m_inventoryChangeConnection;
    entt::scoped_connection m_equipmentChangeConnection;
    entt::scoped_connection m_worldEntityManipulationConnection;

    std::unordered_map<uint64_t, uint32_t> m_worldEntityToFormId;
    std::unordered_map<uint32_t, uint64_t> m_formIdToWorldEntity;
    std::unordered_set<uint32_t> m_retiredWorldReferences;
    std::deque<NotifyInventoryChanges> m_pendingWorldEntitySnapshots;
    std::unordered_map<uint64_t, NotifyInventoryChanges> m_pendingRemoteWorldEntities;

    struct WorldEntityMetadata
    {
        Inventory::Entry Item{};
        uint32_t SourceServerId{};
        GameId PlacedReferenceId{};

        [[nodiscard]] bool IsPlacedReference() const noexcept { return static_cast<bool>(PlacedReferenceId); }
    };

    struct LocalManipulation
    {
        uint64_t WorldEntityId{};
        uint32_t LocalFormId{};
        bool Granted = false;
        bool ReleasePending = false;
        bool ForcedReleaseRequested = false;
        GameId PlacedReferenceId{};
        WorldEntityTransform PendingReleaseTransform{};
        WorldEntityTransform LastSentTransform{};
        std::chrono::steady_clock::time_point LastSentAt{};
        std::chrono::steady_clock::time_point ForcedReleaseRequestedAt{};
    };

    struct RemoteManipulation
    {
        uint32_t LocalFormId{};
    };

    std::unordered_map<uint64_t, WorldEntityMetadata> m_worldEntityMetadata;
    std::optional<LocalManipulation> m_localManipulation;
    std::unordered_map<uint64_t, RemoteManipulation> m_remoteManipulations;
    std::unordered_map<uint64_t, NotifyWorldEntityManipulation> m_pendingRemoteManipulations;

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
        uint32_t SourceServerId{};
        GameId PlacedReferenceId{};
        bool IsAuthority = false;
        bool LocalSettled = false;
        bool HasAuthoritativeTransform = false;
        bool ForceRecreate = false;
        float AuthoritativePositionX{};
        float AuthoritativePositionY{};
        float AuthoritativePositionZ{};
        float AuthoritativeRotationX{};
        float AuthoritativeRotationY{};
        float AuthoritativeRotationZ{};
        uint8_t RecreationAttempts{};
        uint8_t ForcedRecreateAttempts{};
    };

    bool RecreateWorldEntityAtAuthoritativeTransform(uint64_t aWorldEntityId, PendingDropStabilization& arPending) noexcept;
    bool QueueNetworkDrivenRecreate(uint64_t aWorldEntityId, const WorldEntityTransform& acTransform) noexcept;

    std::unordered_map<uint64_t, PendingDropStabilization> m_pendingDropStabilization;
    std::unordered_set<uint64_t> m_remoteAwaitingSettlementRecreate;
};
