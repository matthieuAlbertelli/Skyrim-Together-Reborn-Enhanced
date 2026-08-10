#include "InventoryService.h"

#include <Components.h>
#include <World.h>
#include <GameServer.h>
#include <Game/Player.h>
#include <Events/PlayerEnterWorldEvent.h>

#include <Messages/NotifyObjectInventoryChanges.h>
#include <Messages/RequestInventoryChanges.h>
#include <Messages/NotifyInventoryChanges.h>
#include <Messages/RequestEquipmentChanges.h>
#include <Messages/NotifyEquipmentChanges.h>
#include <Messages/DrawWeaponRequest.h>

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
}

void InventoryService::OnInventoryChanges(const PacketEvent<RequestInventoryChanges>& acMessage) noexcept
{
    auto& message = acMessage.Packet;

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
        transform.TransformUpdate = true;
        transform.HasTransform = true;
        transform.PositionX = entity.PositionX;
        transform.PositionY = entity.PositionY;
        transform.PositionZ = entity.PositionZ;
        transform.RotationX = entity.RotationX;
        transform.RotationY = entity.RotationY;
        transform.RotationZ = entity.RotationZ;

        const entt::entity origin = static_cast<entt::entity>(entity.SourceServerId);
        if (!GameServer::Get()->SendToPlayersInRange(transform, origin, acMessage.GetSender()))
            spdlog::error("{}: transform SendToPlayersInRange failed", __FUNCTION__);

        spdlog::info("[STRE][WorldSync] transform_committed entity={} position=({:.2f},{:.2f},{:.2f})",
                     message.WorldEntityId, message.PositionX, message.PositionY, message.PositionZ);
        return;
    }

    auto view = m_world.view<InventoryComponent>();
    const auto it = view.find(static_cast<entt::entity>(message.ServerId));

    if (!message.Drop && message.WorldEntityId != 0)
    {
        const auto worldIt = m_worldEntities.find(message.WorldEntityId);
        if (worldIt == m_worldEntities.end())
        {
            spdlog::warn("[STRE][WorldSync] pickup_rejected entity={} reason=missing-or-already-consumed", message.WorldEntityId);
            return;
        }
        m_worldEntities.erase(worldIt);
        spdlog::info("[STRE][WorldSync] pickup_committed entity={} actorServerId={}", message.WorldEntityId, message.ServerId);
    }

    if (it != view.end())
        view.get<InventoryComponent>(*it).Content.AddOrRemoveEntry(message.Item);

    if (!message.UpdateClients)
        return;

    NotifyInventoryChanges notify;
    notify.ServerId = message.ServerId;
    notify.Item = message.Item;
    notify.Drop = bEnableItemDrops && message.Drop && message.DroppedFormId != 0;
    notify.WorldEntityId = message.WorldEntityId;

    if (bEnableItemDrops && message.Drop && message.DroppedFormId == 0)
    {
        spdlog::warn("[STRE][WorldSync] drop_sync_skipped actorServerId={} reason=missing-origin-reference",
                     message.ServerId);
    }
    // RequestInventoryChanges already carried transform floats before World Sync.
    // Infer their validity for an initial drop instead of extending the client->server
    // wire format with another boolean. This keeps mixed/restarted binaries from
    // silently mis-decoding the packet.
    notify.HasTransform = notify.Drop;
    notify.PositionX = message.PositionX;
    notify.PositionY = message.PositionY;
    notify.PositionZ = message.PositionZ;
    notify.RotationX = message.RotationX;
    notify.RotationY = message.RotationY;
    notify.RotationZ = message.RotationZ;

    const entt::entity cOrigin = static_cast<entt::entity>(message.ServerId);
    if (notify.Drop)
    {
        notify.WorldEntityId = m_nextWorldEntityId.fetch_add(1, std::memory_order_relaxed);

        SessionWorldEntity entity;
        entity.SourceServerId = message.ServerId;
        entity.AuthorityPlayerId = acMessage.pPlayer ? acMessage.pPlayer->GetId() : 0;
        entity.Item = message.Item;
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
        snapshot.Drop = true;
        snapshot.WorldEntityId = worldEntityId;
        snapshot.Snapshot = true;
        snapshot.HasTransform = entity.HasTransform;
        snapshot.PositionX = entity.PositionX;
        snapshot.PositionY = entity.PositionY;
        snapshot.PositionZ = entity.PositionZ;
        snapshot.RotationX = entity.RotationX;
        snapshot.RotationY = entity.RotationY;
        snapshot.RotationZ = entity.RotationZ;
        acEvent.pPlayer->Send(snapshot);
        ++sent;
    }

    spdlog::info("[STRE][WorldSync] session_snapshot_sent player={} entities={}", acEvent.pPlayer->GetId(), sent);
}
