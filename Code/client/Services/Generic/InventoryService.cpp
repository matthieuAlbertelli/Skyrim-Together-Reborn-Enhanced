#include <Services/InventoryService.h>

#include <Messages/RequestObjectInventoryChanges.h>
#include <Messages/NotifyObjectInventoryChanges.h>
#include <Messages/RequestInventoryChanges.h>
#include <Messages/NotifyInventoryChanges.h>
#include <Messages/RequestEquipmentChanges.h>
#include <Messages/NotifyEquipmentChanges.h>
#include <Messages/DrawWeaponRequest.h>
#include <Messages/NotifyDrawWeapon.h>

#include <Events/UpdateEvent.h>
#include <Events/InventoryChangeEvent.h>
#include <Events/EquipmentChangeEvent.h>

#include <World.h>
#include <Games/Skyrim/Interface/UI.h>
#include <PlayerCharacter.h>
#include <Forms/TESObjectCELL.h>
#include <Actor.h>
#include <Structs/ObjectData.h>
#include <Forms/TESWorldSpace.h>
#include <Games/TES.h>
#include <Games/Overrides.h>
#include <EquipManager.h>
#include <Games/ActorExtension.h>
#include <Forms/TESNPC.h>
#include <DefaultObjectManager.h>

#include <cmath>

namespace
{
// Provisional gameplay tolerance. Validate 64/96/128 Skyrim units in-game before
// promoting this to a user-facing/server setting.
constexpr float cWorldEntityMaximumSettledDrift = 96.0f;
constexpr float cWorldEntityMaximumSettledDriftSquared =
    cWorldEntityMaximumSettledDrift * cWorldEntityMaximumSettledDrift;
}

InventoryService::InventoryService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransport) noexcept
    : m_world(aWorld)
    , m_dispatcher(aDispatcher)
    , m_transport(aTransport)
{
    m_updateConnection = m_dispatcher.sink<UpdateEvent>().connect<&InventoryService::OnUpdate>(this);
    m_inventoryConnection = m_dispatcher.sink<InventoryChangeEvent>().connect<&InventoryService::OnInventoryChangeEvent>(this);
    m_equipmentConnection = m_dispatcher.sink<EquipmentChangeEvent>().connect<&InventoryService::OnEquipmentChangeEvent>(this);
    m_inventoryChangeConnection = m_dispatcher.sink<NotifyInventoryChanges>().connect<&InventoryService::OnNotifyInventoryChanges>(this);
    m_equipmentChangeConnection = m_dispatcher.sink<NotifyEquipmentChanges>().connect<&InventoryService::OnNotifyEquipmentChanges>(this);
}

void InventoryService::OnUpdate(const UpdateEvent& acUpdateEvent) noexcept
{
    RunWeaponStateUpdates();
    RunNakedNPCBugChecks();

    if (!m_transport.IsConnected())
    {
        ResetWorldSyncState();
        return;
    }

    RunPendingWorldEntitySnapshots();
    RunPendingRemoteWorldEntities();
    RunPendingDropStabilization();
}


void InventoryService::ResetWorldSyncState() noexcept
{
    if (m_worldEntityToFormId.empty() && m_formIdToWorldEntity.empty() && m_retiredWorldReferences.empty() &&
        m_pendingWorldEntitySnapshots.empty() && m_pendingRemoteWorldEntities.empty() && m_pendingDropStabilization.empty())
    {
        return;
    }

    spdlog::info("[STRE][WorldSync] reset reason=disconnected entities={} pendingRemote={} pendingStabilization={}",
                 m_worldEntityToFormId.size(), m_pendingRemoteWorldEntities.size(), m_pendingDropStabilization.size());

    m_worldEntityToFormId.clear();
    m_formIdToWorldEntity.clear();
    m_retiredWorldReferences.clear();
    m_pendingWorldEntitySnapshots.clear();
    m_pendingRemoteWorldEntities.clear();
    m_pendingDropStabilization.clear();
}

void InventoryService::OnInventoryChangeEvent(const InventoryChangeEvent& acEvent) noexcept
{
    if (!m_transport.IsConnected())
        return;

    auto view = m_world.view<FormIdComponent>();

    const auto iter = std::find_if(std::begin(view), std::end(view), [view, formId = acEvent.FormId](auto entity) { return view.get<FormIdComponent>(entity).Id == formId; });

    if (iter == std::end(view))
        return;

    std::optional<uint32_t> serverIdRes = Utils::GetServerId(*iter);
    if (!serverIdRes.has_value())
    {
        spdlog::error(__FUNCTION__ ": failed to find server id, target form id: {:X}, item id: {:X}, count: {}", acEvent.FormId, acEvent.Item.BaseId.BaseId, acEvent.Item.Count);
        return;
    }

    spdlog::info(
        "[STRE][WorldSync][LegacyInventory] request_send actorForm={:08X} actorServerId={} item={:X}:{:X} count={} drop={} updateClients={} droppedForm={:08X}",
        acEvent.FormId, serverIdRes.value(), acEvent.Item.BaseId.ModId, acEvent.Item.BaseId.BaseId, acEvent.Item.Count,
        acEvent.Drop, acEvent.UpdateClients, acEvent.DroppedFormId);

    RequestInventoryChanges request;
    request.ServerId = serverIdRes.value();
    request.Item = acEvent.Item;
    request.Drop = acEvent.Drop;
    request.UpdateClients = acEvent.UpdateClients;
    request.DroppedFormId = acEvent.DroppedFormId;

    if (acEvent.Drop && acEvent.DroppedFormId != 0)
    {
        if (acEvent.HasDropTransform)
        {
            request.PositionX = acEvent.DropPositionX;
            request.PositionY = acEvent.DropPositionY;
            request.PositionZ = acEvent.DropPositionZ;
            request.RotationX = acEvent.DropRotationX;
            request.RotationY = acEvent.DropRotationY;
            request.RotationZ = acEvent.DropRotationZ;
        }
        else if (auto* pDroppedReference = Cast<TESObjectREFR>(TESForm::GetById(acEvent.DroppedFormId)))
        {
            // Compatibility fallback for any drop producer that does not yet
            // capture the reference transform at the native drop site.
            request.PositionX = pDroppedReference->position.x;
            request.PositionY = pDroppedReference->position.y;
            request.PositionZ = pDroppedReference->position.z;
            request.RotationX = pDroppedReference->rotation.x;
            request.RotationY = pDroppedReference->rotation.y;
            request.RotationZ = pDroppedReference->rotation.z;
        }

        spdlog::info("[STRE][WorldSync] drop_initial_transform localForm={:08X} position=({:.2f},{:.2f},{:.2f}) immediate={}",
                     acEvent.DroppedFormId, request.PositionX, request.PositionY, request.PositionZ,
                     acEvent.HasDropTransform);
    }

    if (!acEvent.Drop && acEvent.DroppedFormId != 0)
    {
        const auto entityIt = m_formIdToWorldEntity.find(acEvent.DroppedFormId);
        if (entityIt != m_formIdToWorldEntity.end())
        {
            request.WorldEntityId = entityIt->second;
            m_pendingDropStabilization.erase(request.WorldEntityId);
            m_worldEntityToFormId.erase(request.WorldEntityId);
            m_formIdToWorldEntity.erase(entityIt);
            spdlog::info("[STRE][WorldSync] pickup_request entity={} localForm={:08X}", request.WorldEntityId, acEvent.DroppedFormId);
        }
    }

    m_transport.Send(request);

    spdlog::info("Sending item request, item: {:X}, count: {}, target object: {:X}", acEvent.Item.BaseId.BaseId, acEvent.Item.Count, acEvent.FormId);
}

void InventoryService::OnEquipmentChangeEvent(const EquipmentChangeEvent& acEvent) noexcept
{
    if (!m_transport.IsConnected())
        return;

    auto view = m_world.view<FormIdComponent>();

    const auto iter = std::find_if(std::begin(view), std::end(view), [view, formId = acEvent.ActorId](auto entity) { return view.get<FormIdComponent>(entity).Id == formId; });

    if (iter == std::end(view))
        return;

    std::optional<uint32_t> serverIdRes = Utils::GetServerId(*iter);
    if (!serverIdRes.has_value())
    {
        spdlog::error(__FUNCTION__ ": failed to find server id, actor id: {:X}, item id: {:X}, isAmmo: {}, unequip: {}, slot: {:X}", acEvent.ActorId, acEvent.ItemId, acEvent.IsAmmo, acEvent.Unequip, acEvent.EquipSlotId);
        return;
    }

    Actor* pActor = Cast<Actor>(TESForm::GetById(acEvent.ActorId));
    if (!pActor)
        return;

    auto& modSystem = World::Get().GetModSystem();

    RequestEquipmentChanges request;
    request.ServerId = serverIdRes.value();

    if (!modSystem.GetServerModId(acEvent.EquipSlotId, request.EquipSlotId))
        return;
    if (!modSystem.GetServerModId(acEvent.ItemId, request.ItemId))
        return;

    request.Count = acEvent.Count;
    request.Unequip = acEvent.Unequip;
    request.IsSpell = acEvent.IsSpell;
    request.IsShout = acEvent.IsShout;
    request.IsAmmo = acEvent.IsAmmo;
    request.CurrentInventory = pActor->GetEquipment();

    m_transport.Send(request);

    spdlog::info("Sending equipment request, item: {:X}, count: {}, target object: {:X}", acEvent.ItemId, acEvent.Count, acEvent.ActorId);
}

void InventoryService::OnNotifyInventoryChanges(const NotifyInventoryChanges& acMessage) noexcept
{
    if (!m_transport.IsConnected())
    {
        ResetWorldSyncState();
        return;
    }

    spdlog::info("[STRE][WorldSync] notify_receive actorServerId={} item={:X}:{:X} count={} drop={} entity={} bindOnly={} originForm={:08X}",
                 acMessage.ServerId, acMessage.Item.BaseId.ModId, acMessage.Item.BaseId.BaseId, acMessage.Item.Count,
                 acMessage.Drop, acMessage.WorldEntityId, acMessage.BindOnly, acMessage.OriginFormId);

    if (acMessage.BindOnly)
    {
        if (acMessage.WorldEntityId == 0 || acMessage.OriginFormId == 0)
            return;
        m_worldEntityToFormId[acMessage.WorldEntityId] = acMessage.OriginFormId;
        m_formIdToWorldEntity[acMessage.OriginFormId] = acMessage.WorldEntityId;
        if (auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(acMessage.OriginFormId)))
        {
            PendingDropStabilization pending;
            pending.LocalFormId = acMessage.OriginFormId;
            pending.StartedAt = std::chrono::steady_clock::now();
            pending.LastSampleAt = pending.StartedAt;
            pending.LastX = pReference->position.x;
            pending.LastY = pReference->position.y;
            pending.LastZ = pReference->position.z;
            pending.Item = acMessage.Item;
            pending.SourceServerId = acMessage.ServerId;
            pending.IsAuthority = true;
            m_pendingDropStabilization[acMessage.WorldEntityId] = pending;
        }
        spdlog::info("[STRE][WorldSync] bound entity={} localForm={:08X} source=origin-ack", acMessage.WorldEntityId, acMessage.OriginFormId);
        return;
    }

    if (acMessage.TransformUpdate && acMessage.WorldEntityId != 0)
    {
        const auto pendingIt = m_pendingRemoteWorldEntities.find(acMessage.WorldEntityId);
        if (pendingIt != m_pendingRemoteWorldEntities.end())
        {
            NotifyInventoryChanges materialization = pendingIt->second;
            materialization.TransformUpdate = true;
            materialization.HasTransform = true;
            materialization.PositionX = acMessage.PositionX;
            materialization.PositionY = acMessage.PositionY;
            materialization.PositionZ = acMessage.PositionZ;
            materialization.RotationX = acMessage.RotationX;
            materialization.RotationY = acMessage.RotationY;
            materialization.RotationZ = acMessage.RotationZ;

            if (TryMaterializeWorldEntity(materialization))
            {
                m_pendingRemoteWorldEntities.erase(pendingIt);
                StoreAuthoritativeTransform(acMessage.WorldEntityId, acMessage);
                spdlog::info("[STRE][WorldSync] deferred_drop_materialized entity={} position=({:.2f},{:.2f},{:.2f})",
                             acMessage.WorldEntityId, acMessage.PositionX, acMessage.PositionY, acMessage.PositionZ);
            }
            else
            {
                pendingIt->second = std::move(materialization);
                spdlog::info("[STRE][WorldSync] deferred_drop_retry entity={} reason=actor-unavailable", acMessage.WorldEntityId);
            }
            return;
        }

        StoreAuthoritativeTransform(acMessage.WorldEntityId, acMessage);
        return;
    }

    if (acMessage.Drop)
    {
        if (!acMessage.Snapshot && !acMessage.HasTransform && acMessage.WorldEntityId != 0)
        {
            m_pendingRemoteWorldEntities[acMessage.WorldEntityId] = acMessage;
            spdlog::info("[STRE][WorldSync] remote_drop_deferred entity={} sourceServerId={} reason=waiting-transform",
                         acMessage.WorldEntityId, acMessage.ServerId);
            return;
        }

        if (!TryMaterializeWorldEntity(acMessage))
        {
            if (acMessage.Snapshot)
            {
                m_pendingWorldEntitySnapshots.push_back(acMessage);
                spdlog::info("[STRE][WorldSync] session_snapshot_deferred entity={} sourceServerId={}",
                             acMessage.WorldEntityId, acMessage.ServerId);
            }
            else if (acMessage.WorldEntityId != 0)
            {
                m_pendingRemoteWorldEntities[acMessage.WorldEntityId] = acMessage;
                spdlog::info("[STRE][WorldSync] remote_drop_deferred entity={} sourceServerId={} reason=actor-unavailable",
                             acMessage.WorldEntityId, acMessage.ServerId);
            }
        }
        return;
    }

    if (acMessage.WorldEntityId != 0)
    {
        m_pendingRemoteWorldEntities.erase(acMessage.WorldEntityId);
        const auto entityIt = m_worldEntityToFormId.find(acMessage.WorldEntityId);
        if (entityIt != m_worldEntityToFormId.end())
        {
            const uint32_t localFormId = entityIt->second;
            if (auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(localFormId)))
            {
                // Do not call Delete() here. Skyrim may recycle the dynamic FormID on
                // the next remote drop while keeping the reference permanently marked
                // as deleted, which leaves a visible but non-activatable object.
                pReference->Disable();
                m_retiredWorldReferences.insert(localFormId);
            }
            m_pendingDropStabilization.erase(acMessage.WorldEntityId);
            m_pendingRemoteWorldEntities.erase(acMessage.WorldEntityId);
            m_formIdToWorldEntity.erase(localFormId);
            m_worldEntityToFormId.erase(entityIt);
            spdlog::info("[STRE][WorldSync] removed entity={} localForm={:08X} mode=disabled", acMessage.WorldEntityId, localFormId);
        }
    }

    TESObjectREFR* pObject = Utils::GetByServerId<TESObjectREFR>(acMessage.ServerId);
    if (!pObject)
        return;
    ScopedInventoryOverride _;
    pObject->AddOrRemoveItem(acMessage.Item);
}

void InventoryService::OnNotifyEquipmentChanges(const NotifyEquipmentChanges& acMessage) noexcept
{
    Actor* pActor = Utils::GetByServerId<Actor>(acMessage.ServerId);
    if (!pActor)
    {
        spdlog::error("{}: could not find actor server id {:X}", __FUNCTION__, acMessage.ServerId);
        return;
    }

    auto& modSystem = World::Get().GetModSystem();

    uint32_t itemId = modSystem.GetGameId(acMessage.ItemId);
    TESForm* pItem = TESForm::GetById(itemId);

    if (!pItem)
    {
        spdlog::error("Could not find inventory item {:X}:{:X}", acMessage.ItemId.ModId, acMessage.ItemId.BaseId);
        return;
    }

    uint32_t equipSlotId = modSystem.GetGameId(acMessage.EquipSlotId);
    TESForm* pEquipSlot = TESForm::GetById(equipSlotId);

    uint32_t slotId = 0;
    if (pEquipSlot == DefaultObjectManager::Get().rightEquipSlot)
        slotId = 1;

    auto* pEquipManager = EquipManager::Get();

    if (acMessage.IsSpell)
    {
        if (acMessage.Unequip)
            pEquipManager->UnEquipSpell(pActor, pItem, slotId);
        else
            pEquipManager->EquipSpell(pActor, pItem, slotId);

        return;
    }
    else if (acMessage.IsShout)
    {
        if (acMessage.Unequip)
            pEquipManager->UnEquipShout(pActor, pItem);
        else
            pEquipManager->EquipShout(pActor, pItem);

        return;
    }

    auto* pObject = Cast<TESBoundObject>(pItem);

    // TODO: ExtraData necessary? probably
    if (acMessage.Unequip)
    {
        pEquipManager->UnEquip(pActor, pItem, nullptr, acMessage.Count, pEquipSlot, false, true, false, false, nullptr);
    }
    else
    {
        // Unequip all armor first, since the game won't auto unequip armor
        Inventory wornArmor{};
        if (pItem->formType == FormType::Armor)
        {
            wornArmor = pActor->GetWornArmor();
            for (const auto& armor : wornArmor.Entries)
            {
                uint32_t armorId = modSystem.GetGameId(armor.BaseId);
                TESForm* pArmor = TESForm::GetById(armorId);
                if (pArmor)
                    pEquipManager->UnEquip(pActor, pArmor, nullptr, 1, pEquipSlot, false, true, false, false, nullptr);
            }
        }

        pEquipManager->Equip(pActor, pItem, nullptr, acMessage.Count, pEquipSlot, false, true, false, false);

        for (const auto& armor : wornArmor.Entries)
        {
            uint32_t armorId = modSystem.GetGameId(armor.BaseId);
            TESForm* pArmor = TESForm::GetById(armorId);
            if (pArmor)
                pEquipManager->Equip(pActor, pArmor, nullptr, 1, pEquipSlot, false, true, false, false);
        }
    }
}

void InventoryService::RunWeaponStateUpdates() noexcept
{
    if (!m_transport.IsConnected())
        return;

    static std::chrono::steady_clock::time_point lastSendTimePoint;
    constexpr auto cDelayBetweenUpdates = 500ms;

    const auto now = std::chrono::steady_clock::now();
    if (now - lastSendTimePoint < cDelayBetweenUpdates)
        return;

    lastSendTimePoint = now;

    auto view = m_world.view<FormIdComponent, LocalComponent>();

    for (auto entity : view)
    {
        const auto& formIdComponent = view.get<FormIdComponent>(entity);
        Actor* const pActor = Cast<Actor>(TESForm::GetById(formIdComponent.Id));
        auto& localComponent = view.get<LocalComponent>(entity);

        bool isWeaponDrawn = pActor->actorState.IsWeaponDrawn();
        if (isWeaponDrawn != localComponent.IsWeaponDrawn)
        {
            localComponent.IsWeaponDrawn = isWeaponDrawn;

            DrawWeaponRequest request;
            request.Id = localComponent.Id;
            request.IsWeaponDrawn = isWeaponDrawn;

            m_transport.Send(request);
        }
    }
}

void InventoryService::RunNakedNPCBugChecks() noexcept
{
    if (!m_transport.IsConnected())
        return;

    static std::chrono::steady_clock::time_point lastSendTimePoint;
    constexpr auto cDelayBetweenUpdates = 1000ms;

    const auto now = std::chrono::steady_clock::now();
    if (now - lastSendTimePoint < cDelayBetweenUpdates)
        return;

    lastSendTimePoint = now;

    auto view = m_world.view<FormIdComponent>();

    for (auto entity : view)
    {
        const auto& formIdComponent = view.get<FormIdComponent>(entity);
        Actor* pActor = Cast<Actor>(TESForm::GetById(formIdComponent.Id));
        if (!pActor)
            continue;

        if (pActor->GetExtension()->IsPlayer())
            continue;

        if (pActor->IsDead())
            continue;

        if (pActor->IsWearingBodyPiece())
            continue;

        if (!pActor->ShouldWearBodyPiece())
            continue;

        // Don't broadcast changes, it'll just make things messier.
        // If all clients have this problem, they'll all fix it individually.
        ScopedEquipOverride seo;
        ScopedInventoryOverride sio;

        pActor->ResetInventory(false);
    }
}

bool InventoryService::TryMaterializeWorldEntity(const NotifyInventoryChanges& acMessage) noexcept
{
    if (acMessage.WorldEntityId == 0)
        return false;

    if (m_worldEntityToFormId.find(acMessage.WorldEntityId) != m_worldEntityToFormId.end())
        return true;

    Actor* pActor = Utils::GetByServerId<Actor>(acMessage.ServerId);
    if (!pActor)
    {
        spdlog::debug("[STRE][WorldSync] materialize_wait entity={} sourceServerId={} reason=actor-unavailable",
                      acMessage.WorldEntityId, acMessage.ServerId);
        return false;
    }

    NiPoint3 position{};
    NiPoint3 rotation{};
    NiPoint3* pPosition = nullptr;
    NiPoint3* pRotation = nullptr;
    if (acMessage.HasTransform)
    {
        position.x = acMessage.PositionX;
        position.y = acMessage.PositionY;
        position.z = acMessage.PositionZ + 4.0f;
        rotation.x = acMessage.RotationX;
        rotation.y = acMessage.RotationY;
        rotation.z = acMessage.RotationZ;
        pPosition = &position;
        pRotation = &rotation;
    }

    ScopedInventoryOverride _;
    TESObjectREFR* pDroppedObject = pActor->DropOrPickUpObject(acMessage.Item, pPosition, pRotation);
    if (!pDroppedObject)
    {
        spdlog::warn("[STRE][WorldSync] materialize_failed entity={} sourceServerId={} item={:X}:{:X} count={} hasTransform={}",
                     acMessage.WorldEntityId, acMessage.ServerId, acMessage.Item.BaseId.ModId,
                     acMessage.Item.BaseId.BaseId, acMessage.Item.Count, acMessage.HasTransform);
        return false;
    }

    const uint32_t localFormId = pDroppedObject->formID;

    // A disabled dynamic reference can legitimately be recycled by Skyrim. Re-enable
    // it before binding it to the new logical entity. Unlike Delete(), Disable() keeps
    // the reference activatable after it is brought back.
    if (m_retiredWorldReferences.erase(localFormId) != 0)
    {
        pDroppedObject->Enable();
        spdlog::info("[STRE][WorldSync] recycled_ref_reactivated entity={} localForm={:08X}", acMessage.WorldEntityId, localFormId);
    }

    // Keep both registry directions one-to-one even if Skyrim recycled a FormID.
    if (const auto oldEntityIt = m_formIdToWorldEntity.find(localFormId);
        oldEntityIt != m_formIdToWorldEntity.end())
    {
        m_worldEntityToFormId.erase(oldEntityIt->second);
        m_formIdToWorldEntity.erase(oldEntityIt);
    }

    m_worldEntityToFormId[acMessage.WorldEntityId] = localFormId;
    m_formIdToWorldEntity[localFormId] = acMessage.WorldEntityId;

    if (!acMessage.Snapshot)
    {
        PendingDropStabilization pending;
        pending.LocalFormId = localFormId;
        pending.StartedAt = std::chrono::steady_clock::now();
        pending.LastSampleAt = pending.StartedAt;
        pending.LastX = pDroppedObject->position.x;
        pending.LastY = pDroppedObject->position.y;
        pending.LastZ = pDroppedObject->position.z;
        pending.Item = acMessage.Item;
        pending.SourceServerId = acMessage.ServerId;
        pending.IsAuthority = false;
        m_pendingDropStabilization[acMessage.WorldEntityId] = pending;
    }

    spdlog::info("[STRE][WorldSync] bound entity={} localForm={:08X} source={} directTransform={}",
                 acMessage.WorldEntityId, localFormId, acMessage.Snapshot ? "session-snapshot" : "remote-drop", acMessage.HasTransform);
    return true;
}

void InventoryService::RunPendingWorldEntitySnapshots() noexcept
{
    constexpr size_t kMaxAttemptsPerUpdate = 8;
    size_t attempts = 0;
    const size_t pendingCount = m_pendingWorldEntitySnapshots.size();

    while (!m_pendingWorldEntitySnapshots.empty() && attempts < kMaxAttemptsPerUpdate && attempts < pendingCount)
    {
        NotifyInventoryChanges snapshot = std::move(m_pendingWorldEntitySnapshots.front());
        m_pendingWorldEntitySnapshots.pop_front();
        if (!TryMaterializeWorldEntity(snapshot))
            m_pendingWorldEntitySnapshots.push_back(std::move(snapshot));
        ++attempts;
    }
}



void InventoryService::RunPendingRemoteWorldEntities() noexcept
{
    constexpr size_t kMaxAttemptsPerUpdate = 8;
    size_t attempts = 0;

    for (auto it = m_pendingRemoteWorldEntities.begin();
         it != m_pendingRemoteWorldEntities.end() && attempts < kMaxAttemptsPerUpdate;)
    {
        if (!it->second.HasTransform)
        {
            ++it;
            continue;
        }

        const uint64_t worldEntityId = it->first;
        const bool carriesAuthoritativeTransform = it->second.TransformUpdate;
        if (TryMaterializeWorldEntity(it->second))
        {
            if (carriesAuthoritativeTransform)
                StoreAuthoritativeTransform(worldEntityId, it->second);

            spdlog::info("[STRE][WorldSync] deferred_drop_materialized_retry entity={} authoritative={}",
                         worldEntityId, carriesAuthoritativeTransform);
            it = m_pendingRemoteWorldEntities.erase(it);
        }
        else
        {
            ++it;
        }
        ++attempts;
    }
}

void InventoryService::RunPendingDropStabilization() noexcept
{
    using namespace std::chrono;

    const auto now = steady_clock::now();
    constexpr auto kSampleInterval = 100ms;
    constexpr auto kMinimumAge = 500ms;
    constexpr auto kMaximumAge = 4000ms;
    constexpr auto kAuthoritativeTransformWait = 8000ms;
    constexpr float kStableDistanceSquared = 1.0f;

    for (auto it = m_pendingDropStabilization.begin(); it != m_pendingDropStabilization.end();)
    {
        auto& pending = it->second;

        auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(pending.LocalFormId));
        if (!pReference)
        {
            it = m_pendingDropStabilization.erase(it);
            continue;
        }

        if (!pending.LocalSettled)
        {
            if (now - pending.LastSampleAt < kSampleInterval)
            {
                ++it;
                continue;
            }

            // Keep the historical fresh-reference recovery only for the authority,
            // and make it deliberately conservative. The former vertical threshold
            // could mistake a legitimate fall (notably into water) for corruption.
            if (pending.IsAuthority && pending.RecreationAttempts == 0 && now - pending.StartedAt < 1s)
            {
                if (auto* pPlayer = PlayerCharacter::Get())
                {
                    const float playerDx = pReference->position.x - pPlayer->position.x;
                    const float playerDy = pReference->position.y - pPlayer->position.y;
                    constexpr float kClearlyInvalidHorizontalDistanceSquared = 2048.0f * 2048.0f;
                    const bool clearlyInvalidPosition =
                        playerDx * playerDx + playerDy * playerDy > kClearlyInvalidHorizontalDistanceSquared;

                    if (clearlyInvalidPosition)
                    {
                        const uint32_t retiredFormId = pending.LocalFormId;
                        Inventory::Entry restoredItem = pending.Item;
                        restoredItem.Count = -restoredItem.Count;

                        ScopedInventoryOverride inventoryOverride;
                        pReference->Disable();
                        m_retiredWorldReferences.insert(retiredFormId);
                        pPlayer->AddOrRemoveItem(restoredItem);

                        NiPoint3 replacementPosition = pPlayer->position;
                        replacementPosition.z += 64.0f;
                        NiPoint3 replacementRotation = pPlayer->rotation;
                        TESObjectREFR* pReplacement =
                            pPlayer->DropOrPickUpObject(pending.Item, &replacementPosition, &replacementRotation);

                        pending.RecreationAttempts = 1;
                        if (pReplacement)
                        {
                            const uint32_t replacementFormId = pReplacement->formID;
                            if (m_retiredWorldReferences.erase(replacementFormId) != 0)
                                pReplacement->Enable();

                            m_formIdToWorldEntity.erase(retiredFormId);
                            if (const auto oldBinding = m_formIdToWorldEntity.find(replacementFormId);
                                oldBinding != m_formIdToWorldEntity.end())
                            {
                                m_worldEntityToFormId.erase(oldBinding->second);
                                m_formIdToWorldEntity.erase(oldBinding);
                            }

                            m_worldEntityToFormId[it->first] = replacementFormId;
                            m_formIdToWorldEntity[replacementFormId] = it->first;
                            pending.LocalFormId = replacementFormId;
                            pending.LastX = pReplacement->position.x;
                            pending.LastY = pReplacement->position.y;
                            pending.LastZ = pReplacement->position.z;
                            pending.StableSamples = 0;
                            pending.StartedAt = now;
                            pending.LastSampleAt = now;

                            spdlog::warn("[STRE][WorldSync] drop_ref_recreated entity={} retiredForm={:08X} replacementForm={:08X} position=({:.2f},{:.2f},{:.2f})",
                                         it->first, retiredFormId, replacementFormId,
                                         pReplacement->position.x, pReplacement->position.y, pReplacement->position.z);
                            ++it;
                            continue;
                        }

                        pReference->Enable();
                        m_retiredWorldReferences.erase(retiredFormId);
                        Inventory::Entry removeRestoredItem = restoredItem;
                        removeRestoredItem.Count = -removeRestoredItem.Count;
                        pPlayer->AddOrRemoveItem(removeRestoredItem);
                        spdlog::error("[STRE][WorldSync] drop_ref_recreate_failed entity={} localForm={:08X}",
                                      it->first, retiredFormId);
                    }
                }
            }

            const float dx = pReference->position.x - pending.LastX;
            const float dy = pReference->position.y - pending.LastY;
            const float dz = pReference->position.z - pending.LastZ;
            const float distanceSquared = dx * dx + dy * dy + dz * dz;
            pending.StableSamples =
                distanceSquared <= kStableDistanceSquared ? static_cast<uint8_t>(pending.StableSamples + 1) : 0;
            pending.LastX = pReference->position.x;
            pending.LastY = pReference->position.y;
            pending.LastZ = pReference->position.z;
            pending.LastSampleAt = now;

            const bool stable = now - pending.StartedAt >= kMinimumAge && pending.StableSamples >= 3;
            const bool timedOut = now - pending.StartedAt >= kMaximumAge;
            if (!stable && !timedOut)
            {
                ++it;
                continue;
            }

            pending.LocalSettled = true;

            if (pending.IsAuthority)
            {
                if (timedOut && !stable)
                {
                    spdlog::warn("[STRE][WorldSync] settled_timeout_fallback entity={} localForm={:08X} position=({:.2f},{:.2f},{:.2f})",
                                 it->first, pending.LocalFormId,
                                 pReference->position.x, pReference->position.y, pReference->position.z);
                }

                RequestInventoryChanges request;
                request.WorldEntityId = it->first;
                request.TransformUpdate = true;
                request.PositionX = pReference->position.x;
                request.PositionY = pReference->position.y;
                request.PositionZ = pReference->position.z;
                request.RotationX = pReference->rotation.x;
                request.RotationY = pReference->rotation.y;
                request.RotationZ = pReference->rotation.z;
                m_transport.Send(request);

                spdlog::info("[STRE][WorldSync] settled_transform_send entity={} localForm={:08X} position=({:.2f},{:.2f},{:.2f}) stable={} timeout={}",
                             it->first, pending.LocalFormId,
                             request.PositionX, request.PositionY, request.PositionZ, stable, timedOut);
                it = m_pendingDropStabilization.erase(it);
                continue;
            }
        }

        if (!pending.HasAuthoritativeTransform)
        {
            if (now - pending.StartedAt >= kAuthoritativeTransformWait)
            {
                spdlog::warn("[STRE][WorldSync] reconcile_abandoned entity={} localForm={:08X} reason=authoritative-transform-timeout",
                             it->first, pending.LocalFormId);
                it = m_pendingDropStabilization.erase(it);
                continue;
            }

            ++it;
            continue;
        }

        const float dx = pReference->position.x - pending.AuthoritativePositionX;
        const float dy = pReference->position.y - pending.AuthoritativePositionY;
        const float dz = pReference->position.z - pending.AuthoritativePositionZ;
        const float distanceSquared = dx * dx + dy * dy + dz * dz;

        if (distanceSquared <= cWorldEntityMaximumSettledDriftSquared)
        {
            spdlog::info("[STRE][WorldSync] reconcile_skipped entity={} localForm={:08X} distance={:.2f} threshold={:.2f}",
                         it->first, pending.LocalFormId, std::sqrt(distanceSquared), cWorldEntityMaximumSettledDrift);
            it = m_pendingDropStabilization.erase(it);
            continue;
        }

        const float distance = std::sqrt(distanceSquared);
        const bool corrected = RecreateWorldEntityAtAuthoritativeTransform(it->first, pending);
        if (corrected)
        {
            spdlog::warn("[STRE][WorldSync] reconcile_applied entity={} localForm={:08X} distance={:.2f} threshold={:.2f} mode=recreate",
                         it->first, pending.LocalFormId, distance, cWorldEntityMaximumSettledDrift);
        }
        else
        {
            spdlog::error("[STRE][WorldSync] reconcile_failed entity={} localForm={:08X} distance={:.2f} threshold={:.2f}",
                          it->first, pending.LocalFormId, distance, cWorldEntityMaximumSettledDrift);
        }

        // Reconciliation is intentionally one-shot. Never fight local Havok with
        // repeated corrections while the entity is otherwise free.
        it = m_pendingDropStabilization.erase(it);
    }
}

void InventoryService::StoreAuthoritativeTransform(uint64_t aWorldEntityId, const NotifyInventoryChanges& acMessage) noexcept
{
    const auto pendingIt = m_pendingDropStabilization.find(aWorldEntityId);
    if (pendingIt == m_pendingDropStabilization.end())
    {
        spdlog::debug("[STRE][WorldSync] settled_transform_ignored entity={} reason=no-local-settling-tracker",
                      aWorldEntityId);
        return;
    }

    auto& pending = pendingIt->second;
    if (pending.IsAuthority)
        return;

    pending.HasAuthoritativeTransform = true;
    pending.AuthoritativePositionX = acMessage.PositionX;
    pending.AuthoritativePositionY = acMessage.PositionY;
    pending.AuthoritativePositionZ = acMessage.PositionZ;
    pending.AuthoritativeRotationX = acMessage.RotationX;
    pending.AuthoritativeRotationY = acMessage.RotationY;
    pending.AuthoritativeRotationZ = acMessage.RotationZ;

    spdlog::info("[STRE][WorldSync] settled_transform_received entity={} position=({:.2f},{:.2f},{:.2f})",
                 aWorldEntityId, acMessage.PositionX, acMessage.PositionY, acMessage.PositionZ);
}

bool InventoryService::RecreateWorldEntityAtAuthoritativeTransform(
    uint64_t aWorldEntityId, PendingDropStabilization& arPending) noexcept
{
    auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(arPending.LocalFormId));
    Actor* pSourceActor = Utils::GetByServerId<Actor>(arPending.SourceServerId);
    if (!pReference || !pSourceActor)
        return false;

    const uint32_t retiredFormId = arPending.LocalFormId;

    Inventory::Entry restoredItem = arPending.Item;
    restoredItem.Count = -restoredItem.Count;

    NiPoint3 position{};
    position.x = arPending.AuthoritativePositionX;
    position.y = arPending.AuthoritativePositionY;
    position.z = arPending.AuthoritativePositionZ + 4.0f;

    NiPoint3 rotation{};
    rotation.x = arPending.AuthoritativeRotationX;
    rotation.y = arPending.AuthoritativeRotationY;
    rotation.z = arPending.AuthoritativeRotationZ;

    ScopedInventoryOverride inventoryOverride;
    pReference->Disable();
    m_retiredWorldReferences.insert(retiredFormId);
    pSourceActor->AddOrRemoveItem(restoredItem);

    TESObjectREFR* pReplacement = pSourceActor->DropOrPickUpObject(arPending.Item, &position, &rotation);
    if (!pReplacement)
    {
        pReference->Enable();
        m_retiredWorldReferences.erase(retiredFormId);

        Inventory::Entry removeRestoredItem = restoredItem;
        removeRestoredItem.Count = -removeRestoredItem.Count;
        pSourceActor->AddOrRemoveItem(removeRestoredItem);
        return false;
    }

    const uint32_t replacementFormId = pReplacement->formID;
    if (m_retiredWorldReferences.erase(replacementFormId) != 0)
        pReplacement->Enable();

    m_formIdToWorldEntity.erase(retiredFormId);
    if (const auto oldBinding = m_formIdToWorldEntity.find(replacementFormId);
        oldBinding != m_formIdToWorldEntity.end())
    {
        m_worldEntityToFormId.erase(oldBinding->second);
        m_formIdToWorldEntity.erase(oldBinding);
    }

    m_worldEntityToFormId[aWorldEntityId] = replacementFormId;
    m_formIdToWorldEntity[replacementFormId] = aWorldEntityId;
    arPending.LocalFormId = replacementFormId;
    return true;
}
