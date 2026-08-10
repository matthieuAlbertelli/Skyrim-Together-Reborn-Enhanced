#include "InventoryService.h"

#include <Components.h>
#include <World.h>
#include <GameServer.h>
#include <Game/Player.h>
#include <Events/PlayerEnterWorldEvent.h>
#include <Events/PlayerLeaveEvent.h>
#include <Events/UpdateEvent.h>

#include <Messages/NotifyObjectInventoryChanges.h>
#include <Messages/RequestInventoryChanges.h>
#include <Messages/NotifyInventoryChanges.h>
#include <Messages/RequestEquipmentChanges.h>
#include <Messages/NotifyEquipmentChanges.h>
#include <Messages/DrawWeaponRequest.h>
#include <Messages/RequestWorldEntityManipulation.h>
#include <Messages/NotifyWorldEntityManipulation.h>

#include <Setting.h>
namespace
{
Console::Setting bEnableItemDrops{"Gameplay:bEnableItemDrops", "(Experimental) Syncs dropped items by players", false};
}

InventoryService::InventoryService(World& aWorld, entt::dispatcher& aDispatcher)
    : m_world(aWorld)
{
    m_inventoryChangeConnection = aDispatcher.sink<PacketEvent<RequestInventoryChanges>>().connect<&InventoryService::OnInventoryChanges>(this);
    m_equipmentChangeConnection = aDispatcher.sink<PacketEvent<RequestEquipmentChanges>>().connect<&InventoryService::OnEquipmentChanges>(this);
    m_drawWeaponConnection = aDispatcher.sink<PacketEvent<DrawWeaponRequest>>().connect<&InventoryService::OnWeaponDrawnRequest>(this);
    m_playerEnterWorldConnection = aDispatcher.sink<PlayerEnterWorldEvent>().connect<&InventoryService::OnPlayerEnterWorld>(this);
    m_worldEntityManipulationConnection = aDispatcher.sink<PacketEvent<RequestWorldEntityManipulation>>().connect<&InventoryService::OnWorldEntityManipulation>(this);
    m_updateConnection = aDispatcher.sink<UpdateEvent>().connect<&InventoryService::OnUpdate>(this);
    m_playerLeaveConnection = aDispatcher.sink<PlayerLeaveEvent>().connect<&InventoryService::OnPlayerLeave>(this);
}

uint64_t InventoryService::ResolveOrAdoptPlacedReference(
    const GameId& acReferenceId, uint32_t aSourceServerId, const Inventory::Entry* apItem,
    const WorldEntityTransform* apTransform) noexcept
{
    if (!acReferenceId)
        return 0;

    if (const auto existing = m_placedReferenceEntities.find(acReferenceId);
        existing != m_placedReferenceEntities.end())
    {
        if (auto worldIt = m_worldEntities.find(existing->second); worldIt != m_worldEntities.end())
        {
            if (apItem)
                worldIt->second.Item = *apItem;
            if (apTransform)
                SetTransform(worldIt->second, *apTransform);
            return existing->second;
        }
        m_placedReferenceEntities.erase(existing);
    }

    const uint64_t worldEntityId = m_nextWorldEntityId.fetch_add(1, std::memory_order_relaxed);
    SessionWorldEntity entity;
    entity.SourceServerId = aSourceServerId;
    entity.AuthorityServerId = aSourceServerId;
    entity.PlacedReferenceId = acReferenceId;
    entity.State = SessionWorldEntityState::Free;
    entity.LastAuthorityUpdate = std::chrono::steady_clock::now();
    if (apItem)
        entity.Item = *apItem;
    if (apTransform)
        SetTransform(entity, *apTransform);

    m_worldEntities.emplace(worldEntityId, std::move(entity));
    m_placedReferenceEntities.emplace(acReferenceId, worldEntityId);

    spdlog::info("[STRE][WorldSync] placed_adopted entity={} reference={:08X}:{:08X} sourceServerId={}",
                 worldEntityId, acReferenceId.ModId, acReferenceId.BaseId, aSourceServerId);
    return worldEntityId;
}

void InventoryService::EraseWorldEntity(uint64_t aWorldEntityId) noexcept
{
    const auto worldIt = m_worldEntities.find(aWorldEntityId);
    if (worldIt == m_worldEntities.end())
        return;

    if (worldIt->second.PlacedReferenceId)
        m_placedReferenceEntities.erase(worldIt->second.PlacedReferenceId);
    m_worldEntities.erase(worldIt);
}

void InventoryService::OnInventoryChanges(const PacketEvent<RequestInventoryChanges>& acMessage) noexcept
{
    const auto& message = acMessage.Packet;

    if (message.TransformUpdate && message.WorldEntityId != 0)
    {
        const auto worldIt = m_worldEntities.find(message.WorldEntityId);
        if (worldIt == m_worldEntities.end())
        {
            spdlog::warn("[STRE][WorldSync] transform_rejected entity={} reason=missing", message.WorldEntityId);
            return;
        }

        auto& entity = worldIt->second;
        if (!acMessage.pPlayer || entity.AuthorityPlayerId != acMessage.pPlayer->GetId())
        {
            spdlog::warn("[STRE][WorldSync] transform_rejected entity={} reason=not-authority sender={} authority={}",
                         message.WorldEntityId, acMessage.pPlayer ? acMessage.pPlayer->GetId() : 0, entity.AuthorityPlayerId);
            return;
        }
        if (entity.State != SessionWorldEntityState::Settling)
        {
            spdlog::warn("[STRE][WorldSync] transform_rejected entity={} reason=invalid-state state={}",
                         message.WorldEntityId, static_cast<uint32_t>(entity.State));
            return;
        }

        entity.HasTransform = true;
        entity.PositionX = message.PositionX;
        entity.PositionY = message.PositionY;
        entity.PositionZ = message.PositionZ;
        entity.RotationX = message.RotationX;
        entity.RotationY = message.RotationY;
        entity.RotationZ = message.RotationZ;

        NotifyInventoryChanges transform;
        transform.ServerId = entity.SourceServerId;
        transform.WorldEntityId = message.WorldEntityId;
        transform.PlacedReferenceId = entity.PlacedReferenceId;
        transform.TransformUpdate = true;
        transform.HasTransform = true;
        transform.PositionX = entity.PositionX;
        transform.PositionY = entity.PositionY;
        transform.PositionZ = entity.PositionZ;
        transform.RotationX = entity.RotationX;
        transform.RotationY = entity.RotationY;
        transform.RotationZ = entity.RotationZ;

        const entt::entity origin = static_cast<entt::entity>(entity.AuthorityServerId != 0 ? entity.AuthorityServerId : entity.SourceServerId);
        if (!GameServer::Get()->SendToPlayersInRange(transform, origin, acMessage.GetSender()))
            spdlog::error("{}: transform SendToPlayersInRange failed", __FUNCTION__);

        entity.State = SessionWorldEntityState::Free;
        entity.AuthorityPlayerId = 0;

        spdlog::info("[STRE][WorldSync] transform_committed entity={} position=({:.2f},{:.2f},{:.2f}) state=free",
                     message.WorldEntityId, message.PositionX, message.PositionY, message.PositionZ);
        return;
    }

    auto view = m_world.view<InventoryComponent>();
    const auto it = view.find(static_cast<entt::entity>(message.ServerId));

    uint64_t consumedWorldEntityId = message.WorldEntityId;
    GameId consumedPlacedReferenceId = message.PlacedReferenceId;

    if (!message.Drop && consumedWorldEntityId == 0 && consumedPlacedReferenceId)
    {
        consumedWorldEntityId = ResolveOrAdoptPlacedReference(
            consumedPlacedReferenceId, message.ServerId, &message.Item, nullptr);
    }

    if (!message.Drop && consumedWorldEntityId != 0)
    {
        const auto worldIt = m_worldEntities.find(consumedWorldEntityId);
        if (worldIt == m_worldEntities.end())
        {
            spdlog::warn("[STRE][WorldSync] pickup_rejected entity={} reason=missing-or-already-consumed", consumedWorldEntityId);
            return;
        }

        const auto previousState = worldIt->second.State;
        const uint32_t previousAuthority = worldIt->second.AuthorityPlayerId;
        if (worldIt->second.PlacedReferenceId)
            consumedPlacedReferenceId = worldIt->second.PlacedReferenceId;

        EraseWorldEntity(consumedWorldEntityId);
        spdlog::info("[STRE][WorldSync] pickup_committed entity={} actorServerId={} previousState={} previousAuthority={} placed={}",
                     consumedWorldEntityId, message.ServerId, static_cast<uint32_t>(previousState), previousAuthority,
                     consumedPlacedReferenceId ? true : false);
    }

    if (it != view.end())
        view.get<InventoryComponent>(*it).Content.AddOrRemoveEntry(message.Item);

    const entt::entity cOrigin = static_cast<entt::entity>(message.ServerId);

    // Vanilla activation sync owns the inventory side of non-temporary pickups.
    // Still broadcast a lifecycle-only retirement so every client drops the lazy
    // WorldEntity binding and disables the same placed reference deterministically.
    if (!message.UpdateClients)
    {
        if (consumedWorldEntityId != 0)
        {
            NotifyInventoryChanges lifecycle;
            lifecycle.ServerId = message.ServerId;
            lifecycle.WorldEntityId = consumedWorldEntityId;
            lifecycle.PlacedReferenceId = consumedPlacedReferenceId;
            lifecycle.LifecycleOnly = true;
            if (!GameServer::Get()->SendToPlayersInRange(lifecycle, cOrigin, acMessage.GetSender()))
                spdlog::error("{}: lifecycle SendToPlayersInRange failed", __FUNCTION__);
        }
        return;
    }

    NotifyInventoryChanges notify;
    notify.ServerId = message.ServerId;
    notify.Item = message.Item;
    notify.Drop = bEnableItemDrops && message.Drop && message.DroppedFormId != 0;
    notify.WorldEntityId = consumedWorldEntityId;
    notify.PlacedReferenceId = consumedPlacedReferenceId;

    if (bEnableItemDrops && message.Drop && message.DroppedFormId == 0)
    {
        spdlog::warn("[STRE][WorldSync] drop_sync_skipped actorServerId={} reason=missing-origin-reference",
                     message.ServerId);
    }

    notify.HasTransform = notify.Drop;
    notify.PositionX = message.PositionX;
    notify.PositionY = message.PositionY;
    notify.PositionZ = message.PositionZ;
    notify.RotationX = message.RotationX;
    notify.RotationY = message.RotationY;
    notify.RotationZ = message.RotationZ;

    if (notify.Drop)
    {
        notify.WorldEntityId = m_nextWorldEntityId.fetch_add(1, std::memory_order_relaxed);
        notify.PlacedReferenceId = {};

        SessionWorldEntity entity;
        entity.SourceServerId = message.ServerId;
        entity.AuthorityPlayerId = acMessage.pPlayer ? acMessage.pPlayer->GetId() : 0;
        entity.AuthorityServerId = message.ServerId;
        entity.Item = message.Item;
        entity.State = SessionWorldEntityState::Settling;
        entity.LastAuthorityUpdate = std::chrono::steady_clock::now();
        entity.HasTransform = notify.HasTransform;
        entity.PositionX = message.PositionX;
        entity.PositionY = message.PositionY;
        entity.PositionZ = message.PositionZ;
        entity.RotationX = message.RotationX;
        entity.RotationY = message.RotationY;
        entity.RotationZ = message.RotationZ;
        m_worldEntities.emplace(notify.WorldEntityId, std::move(entity));

        NotifyInventoryChanges assignment = notify;
        assignment.BindOnly = true;
        assignment.OriginFormId = message.DroppedFormId;
        acMessage.pPlayer->Send(assignment);

        spdlog::info("[STRE][WorldSync] drop_committed entity={} actorServerId={} originForm={:08X} initialTransform={} position=({:.2f},{:.2f},{:.2f})",
                     notify.WorldEntityId, message.ServerId, message.DroppedFormId, notify.HasTransform,
                     notify.PositionX, notify.PositionY, notify.PositionZ);
    }

    if (!GameServer::Get()->SendToPlayersInRange(notify, cOrigin, acMessage.GetSender()))
        spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);
}

void InventoryService::OnEquipmentChanges(const PacketEvent<RequestEquipmentChanges>& acMessage) noexcept
{
    auto& message = acMessage.Packet;

    auto view = m_world.view<InventoryComponent>();

    const auto it = view.find(static_cast<entt::entity>(message.ServerId));

    if (it != view.end())
    {
        auto& inventoryComponent = view.get<InventoryComponent>(*it);
        inventoryComponent.Content.UpdateEquipment(message.CurrentInventory);
    }

    NotifyEquipmentChanges notify;
    notify.ServerId = message.ServerId;
    notify.ItemId = message.ItemId;
    notify.EquipSlotId = message.EquipSlotId;
    notify.Count = message.Count;
    notify.Unequip = message.Unequip;
    notify.IsSpell = message.IsSpell;
    notify.IsShout = message.IsShout;

    const entt::entity cOrigin = static_cast<entt::entity>(message.ServerId);
    if (!GameServer::Get()->SendToPlayersInRange(notify, cOrigin, acMessage.GetSender()))
        spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);
}

void InventoryService::OnWeaponDrawnRequest(const PacketEvent<DrawWeaponRequest>& acMessage) noexcept
{
    auto& message = acMessage.Packet;

    auto characterView = m_world.view<CharacterComponent, OwnerComponent>();
    const auto it = characterView.find(static_cast<entt::entity>(message.Id));

    if (it != std::end(characterView) && characterView.get<OwnerComponent>(*it).GetOwner() == acMessage.pPlayer)
    {
        auto& characterComponent = characterView.get<CharacterComponent>(*it);
        characterComponent.SetWeaponDrawn(message.IsWeaponDrawn);
        spdlog::debug("Updating weapon drawn state {:x}:{}", message.Id, message.IsWeaponDrawn);
    }
}

void InventoryService::OnPlayerEnterWorld(const PlayerEnterWorldEvent& acEvent) noexcept
{
    if (!acEvent.pPlayer)
        return;

    size_t sent = 0;
    for (const auto& [worldEntityId, entity] : m_worldEntities)
    {
        NotifyInventoryChanges snapshot;
        snapshot.ServerId = entity.SourceServerId;
        snapshot.Item = entity.Item;
        snapshot.Drop = !entity.PlacedReferenceId;
        snapshot.WorldEntityId = worldEntityId;
        snapshot.PlacedReferenceId = entity.PlacedReferenceId;
        snapshot.Snapshot = true;
        snapshot.HasTransform = entity.HasTransform;
        snapshot.PositionX = entity.PositionX;
        snapshot.PositionY = entity.PositionY;
        snapshot.PositionZ = entity.PositionZ;
        snapshot.RotationX = entity.RotationX;
        snapshot.RotationY = entity.RotationY;
        snapshot.RotationZ = entity.RotationZ;
        acEvent.pPlayer->Send(snapshot);

        if (entity.State == SessionWorldEntityState::Manipulated)
        {
            NotifyWorldEntityManipulation manipulation;
            manipulation.WorldEntityId = worldEntityId;
            manipulation.PlacedReferenceId = entity.PlacedReferenceId;
            manipulation.Action = WorldEntityManipulationAction::Start;
            manipulation.AuthorityPlayerId = entity.AuthorityPlayerId;
            manipulation.Transform = GetTransform(entity);
            acEvent.pPlayer->Send(manipulation);
        }

        ++sent;
    }

    spdlog::info("[STRE][WorldSync] session_snapshot_sent player={} entities={}", acEvent.pPlayer->GetId(), sent);
}

WorldEntityTransform InventoryService::GetTransform(const SessionWorldEntity& acEntity) noexcept
{
    WorldEntityTransform transform;
    transform.PositionX = acEntity.PositionX;
    transform.PositionY = acEntity.PositionY;
    transform.PositionZ = acEntity.PositionZ;
    transform.RotationX = acEntity.RotationX;
    transform.RotationY = acEntity.RotationY;
    transform.RotationZ = acEntity.RotationZ;
    return transform;
}

void InventoryService::SetTransform(
    SessionWorldEntity& arEntity, const WorldEntityTransform& acTransform) noexcept
{
    arEntity.HasTransform = true;
    arEntity.PositionX = acTransform.PositionX;
    arEntity.PositionY = acTransform.PositionY;
    arEntity.PositionZ = acTransform.PositionZ;
    arEntity.RotationX = acTransform.RotationX;
    arEntity.RotationY = acTransform.RotationY;
    arEntity.RotationZ = acTransform.RotationZ;
}

void InventoryService::BroadcastManipulation(
    const NotifyWorldEntityManipulation& acNotify, uint32_t aOriginServerId, const Player* apExcludedPlayer) noexcept
{
    const entt::entity origin = static_cast<entt::entity>(aOriginServerId);
    if (!GameServer::Get()->SendToPlayersInRange(acNotify, origin, apExcludedPlayer))
        GameServer::Get()->SendToPlayers(acNotify, apExcludedPlayer);
}

void InventoryService::OnWorldEntityManipulation(
    const PacketEvent<RequestWorldEntityManipulation>& acMessage) noexcept
{
    if (!acMessage.pPlayer)
        return;

    const uint32_t playerId = acMessage.pPlayer->GetId();
    const auto character = acMessage.pPlayer->GetCharacter();
    uint64_t worldEntityId = acMessage.Packet.WorldEntityId;

    if (acMessage.Packet.Action == WorldEntityManipulationAction::Start && worldEntityId == 0)
    {
        if (!character || !acMessage.Packet.PlacedReferenceId)
        {
            spdlog::warn("[STRE][WorldSync] manipulation_adoption_rejected player={} reason={}",
                         playerId, !character ? "no-character" : "missing-reference-id");
            return;
        }

        worldEntityId = ResolveOrAdoptPlacedReference(
            acMessage.Packet.PlacedReferenceId, World::ToInteger(*character), nullptr, &acMessage.Packet.Transform);
        if (worldEntityId == 0)
            return;
    }

    if (worldEntityId == 0)
        return;

    const auto worldIt = m_worldEntities.find(worldEntityId);
    if (worldIt == m_worldEntities.end())
    {
        NotifyWorldEntityManipulation rejected;
        rejected.WorldEntityId = worldEntityId;
        rejected.PlacedReferenceId = acMessage.Packet.PlacedReferenceId;
        rejected.Action = WorldEntityManipulationAction::Rejected;
        acMessage.pPlayer->Send(rejected);
        spdlog::warn("[STRE][WorldSync] manipulation_rejected entity={} player={} reason=missing",
                     worldEntityId, playerId);
        return;
    }

    auto& entity = worldIt->second;

    switch (acMessage.Packet.Action)
    {
    case WorldEntityManipulationAction::Start:
    {
        if (!character)
        {
            NotifyWorldEntityManipulation rejected;
            rejected.WorldEntityId = worldEntityId;
            rejected.PlacedReferenceId = entity.PlacedReferenceId;
            rejected.Action = WorldEntityManipulationAction::Rejected;
            rejected.AuthorityPlayerId = entity.AuthorityPlayerId;
            rejected.Transform = GetTransform(entity);
            acMessage.pPlayer->Send(rejected);
            spdlog::warn("[STRE][WorldSync] manipulation_rejected entity={} player={} reason=no-character",
                         worldEntityId, playerId);
            return;
        }

        if (entity.State == SessionWorldEntityState::Manipulated && entity.AuthorityPlayerId != playerId)
        {
            NotifyWorldEntityManipulation rejected;
            rejected.WorldEntityId = worldEntityId;
            rejected.PlacedReferenceId = entity.PlacedReferenceId;
            rejected.Action = WorldEntityManipulationAction::Rejected;
            rejected.AuthorityPlayerId = entity.AuthorityPlayerId;
            rejected.Transform = GetTransform(entity);
            acMessage.pPlayer->Send(rejected);
            spdlog::info("[STRE][WorldSync] manipulation_rejected entity={} player={} reason=busy authority={}",
                         worldEntityId, playerId, entity.AuthorityPlayerId);
            return;
        }

        entity.State = SessionWorldEntityState::Manipulated;
        entity.AuthorityPlayerId = playerId;
        entity.AuthorityServerId = World::ToInteger(*character);
        entity.LastAuthorityUpdate = std::chrono::steady_clock::now();
        SetTransform(entity, acMessage.Packet.Transform);

        NotifyWorldEntityManipulation granted;
        granted.WorldEntityId = worldEntityId;
        granted.PlacedReferenceId = entity.PlacedReferenceId;
        granted.Action = WorldEntityManipulationAction::Start;
        granted.AuthorityPlayerId = playerId;
        granted.Transform = GetTransform(entity);
        acMessage.pPlayer->Send(granted);
        BroadcastManipulation(granted, entity.AuthorityServerId, acMessage.pPlayer);

        spdlog::info("[STRE][WorldSync] manipulation_granted entity={} player={} authorityServerId={} placed={}",
                     worldEntityId, playerId, entity.AuthorityServerId, entity.PlacedReferenceId ? true : false);
        return;
    }
    case WorldEntityManipulationAction::Update:
    {
        if (entity.State != SessionWorldEntityState::Manipulated || entity.AuthorityPlayerId != playerId)
        {
            spdlog::debug("[STRE][WorldSync] manipulation_update_rejected entity={} player={} authority={} state={}",
                          worldEntityId, playerId, entity.AuthorityPlayerId, static_cast<uint32_t>(entity.State));
            return;
        }

        entity.LastAuthorityUpdate = std::chrono::steady_clock::now();
        SetTransform(entity, acMessage.Packet.Transform);
        return;
    }
    case WorldEntityManipulationAction::Release:
    {
        if (entity.State != SessionWorldEntityState::Manipulated || entity.AuthorityPlayerId != playerId)
        {
            spdlog::debug("[STRE][WorldSync] manipulation_release_rejected entity={} player={} authority={} state={}",
                          worldEntityId, playerId, entity.AuthorityPlayerId, static_cast<uint32_t>(entity.State));
            return;
        }

        SetTransform(entity, acMessage.Packet.Transform);
        entity.State = SessionWorldEntityState::Settling;
        entity.LastAuthorityUpdate = std::chrono::steady_clock::now();

        NotifyWorldEntityManipulation release;
        release.WorldEntityId = worldEntityId;
        release.PlacedReferenceId = entity.PlacedReferenceId;
        release.Action = WorldEntityManipulationAction::Release;
        release.AuthorityPlayerId = playerId;
        release.Transform = GetTransform(entity);
        BroadcastManipulation(release, entity.AuthorityServerId, acMessage.pPlayer);

        spdlog::info("[STRE][WorldSync] manipulation_released entity={} player={} state=settling placed={}",
                     worldEntityId, playerId, entity.PlacedReferenceId ? true : false);
        return;
    }
    case WorldEntityManipulationAction::Rejected:
        spdlog::warn("[STRE][WorldSync] manipulation_request_rejected entity={} player={} reason=invalid-client-action",
                     worldEntityId, playerId);
        return;
    }
}

void InventoryService::OnUpdate(const UpdateEvent&) noexcept
{
    using namespace std::chrono;
    constexpr auto kManipulationAuthorityTimeout = 2s;
    constexpr auto kSettlementAuthorityTimeout = 8s;
    const auto now = steady_clock::now();

    for (auto& [worldEntityId, entity] : m_worldEntities)
    {
        const bool manipulationTimedOut =
            entity.State == SessionWorldEntityState::Manipulated &&
            now - entity.LastAuthorityUpdate >= kManipulationAuthorityTimeout;
        const bool settlementTimedOut =
            entity.State == SessionWorldEntityState::Settling && entity.AuthorityPlayerId != 0 &&
            now - entity.LastAuthorityUpdate >= kSettlementAuthorityTimeout;

        if (!manipulationTimedOut && !settlementTimedOut)
            continue;

        const uint32_t oldAuthority = entity.AuthorityPlayerId;

        NotifyWorldEntityManipulation release;
        release.WorldEntityId = worldEntityId;
        release.PlacedReferenceId = entity.PlacedReferenceId;
        release.Action = WorldEntityManipulationAction::Release;
        release.AuthorityPlayerId = 0; // No client owns the following settlement.
        release.Transform = GetTransform(entity);

        entity.State = SessionWorldEntityState::Free;
        entity.AuthorityPlayerId = 0;
        BroadcastManipulation(release, entity.AuthorityServerId);

        spdlog::warn("[STRE][WorldSync] {} entity={} previousAuthority={} state=free",
                     settlementTimedOut ? "settlement_timeout" : "authority_timeout", worldEntityId, oldAuthority);
    }
}

void InventoryService::OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept
{
    if (!acEvent.pPlayer)
        return;

    const uint32_t leavingPlayerId = acEvent.pPlayer->GetId();
    for (auto& [worldEntityId, entity] : m_worldEntities)
    {
        if (entity.AuthorityPlayerId != leavingPlayerId)
            continue;

        if (entity.State == SessionWorldEntityState::Manipulated)
        {
            NotifyWorldEntityManipulation release;
            release.WorldEntityId = worldEntityId;
            release.PlacedReferenceId = entity.PlacedReferenceId;
            release.Action = WorldEntityManipulationAction::Release;
            release.AuthorityPlayerId = 0;
            release.Transform = GetTransform(entity);

            entity.State = SessionWorldEntityState::Free;
            entity.AuthorityPlayerId = 0;
            BroadcastManipulation(release, entity.AuthorityServerId, acEvent.pPlayer);

            spdlog::warn("[STRE][WorldSync] authority_lost entity={} player={} reason=disconnect state=free",
                         worldEntityId, leavingPlayerId);
        }
        else if (entity.State == SessionWorldEntityState::Settling)
        {
            // Observers keep a Better-Grabbing-driven copy non-collidable until
            // settlement. If the authority disappears, release them immediately at
            // the last server transform instead of leaving a ghost reference behind.
            NotifyWorldEntityManipulation release;
            release.WorldEntityId = worldEntityId;
            release.PlacedReferenceId = entity.PlacedReferenceId;
            release.Action = WorldEntityManipulationAction::Release;
            release.AuthorityPlayerId = 0;
            release.Transform = GetTransform(entity);

            entity.State = SessionWorldEntityState::Free;
            entity.AuthorityPlayerId = 0;
            BroadcastManipulation(release, entity.AuthorityServerId, acEvent.pPlayer);
            spdlog::warn("[STRE][WorldSync] authority_lost entity={} player={} reason=disconnect-during-settlement state=free",
                         worldEntityId, leavingPlayerId);
        }
    }
}
