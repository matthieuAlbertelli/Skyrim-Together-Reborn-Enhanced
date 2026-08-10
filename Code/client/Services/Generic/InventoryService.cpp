#include <Services/InventoryService.h>

#include <Messages/RequestObjectInventoryChanges.h>
#include <Messages/NotifyObjectInventoryChanges.h>
#include <Messages/RequestInventoryChanges.h>
#include <Messages/NotifyInventoryChanges.h>
#include <Messages/RequestEquipmentChanges.h>
#include <Messages/NotifyEquipmentChanges.h>
#include <Messages/DrawWeaponRequest.h>
#include <Messages/NotifyDrawWeapon.h>
#include <Messages/RequestWorldEntityManipulation.h>
#include <Messages/NotifyWorldEntityManipulation.h>

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
#include <Games/Events.h>

#include <cmath>
#include <algorithm>
#include <vector>

namespace
{
// Provisional gameplay tolerance. Validate 64/96/128 Skyrim units in-game before
// promoting this to a user-facing/server setting.
constexpr float cWorldEntityMaximumSettledDrift = 96.0f;
constexpr float cWorldEntityMaximumSettledDriftSquared =
    cWorldEntityMaximumSettledDrift * cWorldEntityMaximumSettledDrift;
constexpr auto cManipulationHeartbeat = std::chrono::milliseconds(500);
constexpr wchar_t cBetterGrabbingModule[] = L"BetterGrabbing.dll";
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
    m_worldEntityManipulationConnection = m_dispatcher.sink<NotifyWorldEntityManipulation>().connect<&InventoryService::OnNotifyWorldEntityManipulation>(this);

    EventDispatcherManager::Get()->grabReleaseEvent.RegisterSink(this);
}

InventoryService::~InventoryService() noexcept
{
    EventDispatcherManager::Get()->grabReleaseEvent.UnRegisterSink(this);
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
    RunPendingRemoteWorldEntityManipulations();
    RunLocalWorldEntityManipulation();
    RunPendingDropStabilization();
}


void InventoryService::ResetWorldSyncState() noexcept
{
    if (m_worldEntityToFormId.empty() && m_formIdToWorldEntity.empty() && m_retiredWorldReferences.empty() &&
        m_pendingWorldEntitySnapshots.empty() && m_pendingRemoteWorldEntities.empty() && m_pendingDropStabilization.empty() &&
        m_worldEntityMetadata.empty() && !m_localManipulation.has_value() && m_remoteManipulations.empty() &&
        m_pendingRemoteManipulations.empty() && m_remoteAwaitingSettlementRecreate.empty())
    {
        return;
    }

    spdlog::info("[STRE][WorldSync] reset reason=disconnected entities={} pendingRemote={} pendingStabilization={} manipulated={}",
                 m_worldEntityToFormId.size(), m_pendingRemoteWorldEntities.size(), m_pendingDropStabilization.size(),
                 m_remoteManipulations.size());

    // Remote WorldEntities are disabled while another player is holding them.
    // If the connection disappears mid-grab, re-enable the local copies so a
    // multiplayer state transition can never leave an object invisible in solo.
    for (const auto& [worldEntityId, remote] : m_remoteManipulations)
    {
        if (auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(remote.LocalFormId)))
        {
            pReference->Enable();
            spdlog::warn("[STRE][WorldSync] manipulation_visibility_restored entity={} localForm={:08X} reason=disconnected",
                         worldEntityId, remote.LocalFormId);
        }
    }

    m_worldEntityToFormId.clear();
    m_formIdToWorldEntity.clear();
    m_retiredWorldReferences.clear();
    m_pendingWorldEntitySnapshots.clear();
    m_pendingRemoteWorldEntities.clear();
    m_pendingDropStabilization.clear();
    m_worldEntityMetadata.clear();
    m_localManipulation.reset();
    m_remoteManipulations.clear();
    m_pendingRemoteManipulations.clear();
    m_remoteAwaitingSettlementRecreate.clear();
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
            const auto metadataIt = m_worldEntityMetadata.find(request.WorldEntityId);
            if (metadataIt != m_worldEntityMetadata.end())
                request.PlacedReferenceId = metadataIt->second.PlacedReferenceId;

            m_pendingDropStabilization.erase(request.WorldEntityId);
            m_remoteManipulations.erase(request.WorldEntityId);
            m_pendingRemoteManipulations.erase(request.WorldEntityId);
            m_remoteAwaitingSettlementRecreate.erase(request.WorldEntityId);
            m_worldEntityMetadata.erase(request.WorldEntityId);
            if (m_localManipulation && m_localManipulation->WorldEntityId == request.WorldEntityId)
                m_localManipulation.reset();
            m_worldEntityToFormId.erase(request.WorldEntityId);
            m_formIdToWorldEntity.erase(entityIt);
            spdlog::info("[STRE][WorldSync] pickup_request entity={} localForm={:08X}", request.WorldEntityId, acEvent.DroppedFormId);
        }
        else if (auto* pPickedReference = Cast<TESObjectREFR>(TESForm::GetById(acEvent.DroppedFormId));
                 pPickedReference && !pPickedReference->IsTemporary())
        {
            GameId referenceId{};
            if (m_world.GetModSystem().GetServerModId(acEvent.DroppedFormId, referenceId))
            {
                request.PlacedReferenceId = referenceId;
                spdlog::info("[STRE][WorldSync] pickup_lazy_adoption_requested localForm={:08X} reference={:08X}:{:08X}",
                             acEvent.DroppedFormId, referenceId.ModId, referenceId.BaseId);
            }
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

    spdlog::info("[STRE][WorldSync] notify_receive actorServerId={} item={:X}:{:X} count={} drop={} entity={} placed={:08X}:{:08X} bindOnly={} lifecycleOnly={} originForm={:08X}",
                 acMessage.ServerId, acMessage.Item.BaseId.ModId, acMessage.Item.BaseId.BaseId, acMessage.Item.Count,
                 acMessage.Drop, acMessage.WorldEntityId, acMessage.PlacedReferenceId.ModId, acMessage.PlacedReferenceId.BaseId,
                 acMessage.BindOnly, acMessage.LifecycleOnly, acMessage.OriginFormId);

    if (acMessage.BindOnly)
    {
        if (acMessage.WorldEntityId == 0 || acMessage.OriginFormId == 0)
            return;
        m_worldEntityToFormId[acMessage.WorldEntityId] = acMessage.OriginFormId;
        m_formIdToWorldEntity[acMessage.OriginFormId] = acMessage.WorldEntityId;
        m_worldEntityMetadata[acMessage.WorldEntityId] = WorldEntityMetadata{acMessage.Item, acMessage.ServerId, {}};
        StartSettlementTracking(acMessage.WorldEntityId, acMessage.OriginFormId, true);
        spdlog::info("[STRE][WorldSync] bound entity={} localForm={:08X} source=origin-ack", acMessage.WorldEntityId, acMessage.OriginFormId);
        return;
    }

    if (acMessage.WorldEntityId != 0 && acMessage.PlacedReferenceId)
    {
        const bool bound = BindPlacedWorldEntity(acMessage.WorldEntityId, acMessage.PlacedReferenceId, acMessage.ServerId);
        if (acMessage.Snapshot)
        {
            if (!bound)
            {
                m_pendingWorldEntitySnapshots.push_back(acMessage);
                spdlog::info("[STRE][WorldSync] placed_snapshot_deferred entity={} reference={:08X}:{:08X}",
                             acMessage.WorldEntityId, acMessage.PlacedReferenceId.ModId, acMessage.PlacedReferenceId.BaseId);
                return;
            }

            if (acMessage.HasTransform)
            {
                WorldEntityTransform transform;
                transform.PositionX = acMessage.PositionX;
                transform.PositionY = acMessage.PositionY;
                transform.PositionZ = acMessage.PositionZ;
                transform.RotationX = acMessage.RotationX;
                transform.RotationY = acMessage.RotationY;
                transform.RotationZ = acMessage.RotationZ;
                ApplyPlacedWorldEntityTransform(acMessage.WorldEntityId, transform, true);
            }

            spdlog::info("[STRE][WorldSync] placed_snapshot_bound entity={} reference={:08X}:{:08X} transform={}",
                         acMessage.WorldEntityId, acMessage.PlacedReferenceId.ModId, acMessage.PlacedReferenceId.BaseId,
                         acMessage.HasTransform);
            return;
        }
    }

    if (acMessage.TransformUpdate && acMessage.WorldEntityId != 0)
    {
        const auto metadataIt = m_worldEntityMetadata.find(acMessage.WorldEntityId);
        if (metadataIt != m_worldEntityMetadata.end() && metadataIt->second.IsPlacedReference())
        {
            StoreAuthoritativeTransform(acMessage.WorldEntityId, acMessage);
            return;
        }

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

        if (m_remoteAwaitingSettlementRecreate.erase(acMessage.WorldEntityId) != 0)
        {
            WorldEntityTransform finalTransform;
            finalTransform.PositionX = acMessage.PositionX;
            finalTransform.PositionY = acMessage.PositionY;
            finalTransform.PositionZ = acMessage.PositionZ;
            finalTransform.RotationX = acMessage.RotationX;
            finalTransform.RotationY = acMessage.RotationY;
            finalTransform.RotationZ = acMessage.RotationZ;
            QueueNetworkDrivenRecreate(acMessage.WorldEntityId, finalTransform);
            spdlog::info("[STRE][WorldSync] manipulation_settlement_received entity={} position=({:.2f},{:.2f},{:.2f}) mode=recreate",
                         acMessage.WorldEntityId, acMessage.PositionX, acMessage.PositionY, acMessage.PositionZ);
        }
        else
        {
            StoreAuthoritativeTransform(acMessage.WorldEntityId, acMessage);
        }
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
            const auto metadataIt = m_worldEntityMetadata.find(acMessage.WorldEntityId);
            const bool isPlacedReference = metadataIt != m_worldEntityMetadata.end() && metadataIt->second.IsPlacedReference();

            if (auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(localFormId)))
            {
                pReference->Disable();
                if (!isPlacedReference)
                    m_retiredWorldReferences.insert(localFormId);
            }
            m_pendingDropStabilization.erase(acMessage.WorldEntityId);
            m_pendingRemoteWorldEntities.erase(acMessage.WorldEntityId);
            m_remoteManipulations.erase(acMessage.WorldEntityId);
            m_pendingRemoteManipulations.erase(acMessage.WorldEntityId);
            m_remoteAwaitingSettlementRecreate.erase(acMessage.WorldEntityId);
            m_worldEntityMetadata.erase(acMessage.WorldEntityId);
            if (m_localManipulation && m_localManipulation->WorldEntityId == acMessage.WorldEntityId)
                m_localManipulation.reset();
            m_formIdToWorldEntity.erase(localFormId);
            m_worldEntityToFormId.erase(entityIt);
            spdlog::info("[STRE][WorldSync] removed entity={} localForm={:08X} mode=disabled placed={}",
                         acMessage.WorldEntityId, localFormId, isPlacedReference);
        }
    }

    if (acMessage.LifecycleOnly)
        return;

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
    m_worldEntityMetadata[acMessage.WorldEntityId] = WorldEntityMetadata{acMessage.Item, acMessage.ServerId, {}};

    if (!acMessage.Snapshot)
        StartSettlementTracking(acMessage.WorldEntityId, localFormId, false);

    if (const auto pendingManipulation = m_pendingRemoteManipulations.find(acMessage.WorldEntityId);
        pendingManipulation != m_pendingRemoteManipulations.end())
    {
        const auto manipulation = pendingManipulation->second;
        m_pendingRemoteManipulations.erase(pendingManipulation);
        if (manipulation.Action == WorldEntityManipulationAction::Release)
            EndRemoteWorldEntityManipulation(manipulation);
        else if (manipulation.Action != WorldEntityManipulationAction::Rejected)
            BeginRemoteWorldEntityManipulation(manipulation);
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

        bool resolved = false;
        if (snapshot.PlacedReferenceId)
        {
            resolved = BindPlacedWorldEntity(snapshot.WorldEntityId, snapshot.PlacedReferenceId, snapshot.ServerId);
            if (resolved && snapshot.HasTransform)
            {
                WorldEntityTransform transform;
                transform.PositionX = snapshot.PositionX;
                transform.PositionY = snapshot.PositionY;
                transform.PositionZ = snapshot.PositionZ;
                transform.RotationX = snapshot.RotationX;
                transform.RotationY = snapshot.RotationY;
                transform.RotationZ = snapshot.RotationZ;
                resolved = ApplyPlacedWorldEntityTransform(snapshot.WorldEntityId, transform, true);
            }
        }
        else
        {
            resolved = TryMaterializeWorldEntity(snapshot);
        }

        if (!resolved)
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

void InventoryService::RunPendingRemoteWorldEntityManipulations() noexcept
{
    constexpr size_t kMaxAttemptsPerUpdate = 8;
    std::vector<uint64_t> pendingIds;
    pendingIds.reserve(std::min(kMaxAttemptsPerUpdate, m_pendingRemoteManipulations.size()));

    for (const auto& [worldEntityId, message] : m_pendingRemoteManipulations)
    {
        pendingIds.push_back(worldEntityId);
        if (pendingIds.size() >= kMaxAttemptsPerUpdate)
            break;
    }

    for (const uint64_t worldEntityId : pendingIds)
    {
        const auto it = m_pendingRemoteManipulations.find(worldEntityId);
        if (it == m_pendingRemoteManipulations.end())
            continue;

        const NotifyWorldEntityManipulation message = it->second;
        m_pendingRemoteManipulations.erase(it);
        if (message.Action == WorldEntityManipulationAction::Release)
            EndRemoteWorldEntityManipulation(message);
        else if (message.Action != WorldEntityManipulationAction::Rejected)
            BeginRemoteWorldEntityManipulation(message);
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
            if (pending.IsAuthority && !pending.PlacedReferenceId && pending.RecreationAttempts == 0 && now - pending.StartedAt < 1s)
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

        if (pending.ForceRecreate)
        {
            constexpr auto kRecreateRetryInterval = 250ms;
            constexpr auto kRecreateRetryWindow = 8000ms;

            if (pending.ForcedRecreateAttempts != 0 && now - pending.LastSampleAt < kRecreateRetryInterval)
            {
                ++it;
                continue;
            }

            pending.LastSampleAt = now;
            ++pending.ForcedRecreateAttempts;
            if (RecreateWorldEntityAtAuthoritativeTransform(it->first, pending))
            {
                spdlog::info("[STRE][WorldSync] manipulation_collision_restored entity={} localForm={:08X} mode=recreate attempts={}",
                             it->first, pending.LocalFormId, pending.ForcedRecreateAttempts);
                it = m_pendingDropStabilization.erase(it);
                continue;
            }

            if (now - pending.StartedAt < kRecreateRetryWindow)
            {
                spdlog::warn("[STRE][WorldSync] manipulation_collision_restore_retry entity={} localForm={:08X} attempt={}",
                             it->first, pending.LocalFormId, pending.ForcedRecreateAttempts);
                ++it;
                continue;
            }

            // Last-resort safety: a failed recreate must never leave the original
            // representation invisible forever. Re-enable it at its previous pose.
            pReference->Enable();
            m_remoteManipulations.erase(it->first);
            spdlog::error("[STRE][WorldSync] manipulation_visibility_restore_fallback entity={} localForm={:08X} attempts={}",
                          it->first, pending.LocalFormId, pending.ForcedRecreateAttempts);
            it = m_pendingDropStabilization.erase(it);
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
    if (arPending.PlacedReferenceId)
    {
        WorldEntityTransform transform;
        transform.PositionX = arPending.AuthoritativePositionX;
        transform.PositionY = arPending.AuthoritativePositionY;
        transform.PositionZ = arPending.AuthoritativePositionZ;
        transform.RotationX = arPending.AuthoritativeRotationX;
        transform.RotationY = arPending.AuthoritativeRotationY;
        transform.RotationZ = arPending.AuthoritativeRotationZ;
        const bool applied = ApplyPlacedWorldEntityTransform(aWorldEntityId, transform, true);
        if (applied)
            m_remoteManipulations.erase(aWorldEntityId);
        return applied;
    }

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

    // A hidden remote grab is complete once its visible replacement exists.
    m_remoteManipulations.erase(aWorldEntityId);
    return true;
}

WorldEntityTransform InventoryService::ReadWorldEntityTransform(const TESObjectREFR* apReference) noexcept
{
    WorldEntityTransform transform;
    if (!apReference)
        return transform;

    transform.PositionX = apReference->position.x;
    transform.PositionY = apReference->position.y;
    transform.PositionZ = apReference->position.z;
    transform.RotationX = apReference->rotation.x;
    transform.RotationY = apReference->rotation.y;
    transform.RotationZ = apReference->rotation.z;
    return transform;
}

void InventoryService::StartSettlementTracking(
    uint64_t aWorldEntityId, uint32_t aLocalFormId, bool aIsAuthority) noexcept
{
    auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(aLocalFormId));
    const auto metadataIt = m_worldEntityMetadata.find(aWorldEntityId);
    if (!pReference || metadataIt == m_worldEntityMetadata.end())
    {
        spdlog::warn("[STRE][WorldSync] settlement_tracker_skipped entity={} localForm={:08X} reason={}",
                     aWorldEntityId, aLocalFormId, pReference ? "missing-metadata" : "missing-reference");
        return;
    }

    PendingDropStabilization pending;
    pending.LocalFormId = aLocalFormId;
    pending.StartedAt = std::chrono::steady_clock::now();
    pending.LastSampleAt = pending.StartedAt;
    pending.LastX = pReference->position.x;
    pending.LastY = pReference->position.y;
    pending.LastZ = pReference->position.z;
    pending.Item = metadataIt->second.Item;
    pending.SourceServerId = metadataIt->second.SourceServerId;
    pending.PlacedReferenceId = metadataIt->second.PlacedReferenceId;
    pending.IsAuthority = aIsAuthority;
    m_pendingDropStabilization[aWorldEntityId] = pending;
}

bool InventoryService::QueueNetworkDrivenRecreate(
    uint64_t aWorldEntityId, const WorldEntityTransform& acTransform) noexcept
{
    const auto entityIt = m_worldEntityToFormId.find(aWorldEntityId);
    const auto metadataIt = m_worldEntityMetadata.find(aWorldEntityId);
    if (entityIt == m_worldEntityToFormId.end() || metadataIt == m_worldEntityMetadata.end())
    {
        spdlog::warn("[STRE][WorldSync] manipulation_recreate_deferred entity={} reason=missing-binding-or-metadata", aWorldEntityId);
        return false;
    }
    if (metadataIt->second.IsPlacedReference())
    {
        spdlog::warn("[STRE][WorldSync] manipulation_recreate_skipped entity={} reason=placed-reference", aWorldEntityId);
        return false;
    }

    PendingDropStabilization pending;
    pending.LocalFormId = entityIt->second;
    pending.StartedAt = std::chrono::steady_clock::now();
    pending.LastSampleAt = pending.StartedAt;
    pending.Item = metadataIt->second.Item;
    pending.SourceServerId = metadataIt->second.SourceServerId;
    pending.LocalSettled = true;
    pending.HasAuthoritativeTransform = true;
    pending.ForceRecreate = true;
    pending.AuthoritativePositionX = acTransform.PositionX;
    pending.AuthoritativePositionY = acTransform.PositionY;
    pending.AuthoritativePositionZ = acTransform.PositionZ;
    pending.AuthoritativeRotationX = acTransform.RotationX;
    pending.AuthoritativeRotationY = acTransform.RotationY;
    pending.AuthoritativeRotationZ = acTransform.RotationZ;
    m_pendingDropStabilization[aWorldEntityId] = pending;
    return true;
}

bool InventoryService::BindPlacedWorldEntity(
    uint64_t aWorldEntityId, const GameId& acReferenceId, uint32_t aSourceServerId) noexcept
{
    if (aWorldEntityId == 0 || !acReferenceId)
        return false;

    const uint32_t localFormId = m_world.GetModSystem().GetGameId(acReferenceId);
    if (localFormId == 0)
    {
        spdlog::warn("[STRE][WorldSync] placed_bind_deferred entity={} reference={:08X}:{:08X} reason=unmapped-mod",
                     aWorldEntityId, acReferenceId.ModId, acReferenceId.BaseId);
        return false;
    }

    auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(localFormId));
    if (!pReference)
    {
        spdlog::warn("[STRE][WorldSync] placed_bind_deferred entity={} reference={:08X}:{:08X} localForm={:08X} reason=reference-unavailable",
                     aWorldEntityId, acReferenceId.ModId, acReferenceId.BaseId, localFormId);
        return false;
    }

    if (const auto oldBinding = m_formIdToWorldEntity.find(localFormId);
        oldBinding != m_formIdToWorldEntity.end() && oldBinding->second != aWorldEntityId)
    {
        m_worldEntityToFormId.erase(oldBinding->second);
        m_worldEntityMetadata.erase(oldBinding->second);
        m_formIdToWorldEntity.erase(oldBinding);
    }

    m_worldEntityToFormId[aWorldEntityId] = localFormId;
    m_formIdToWorldEntity[localFormId] = aWorldEntityId;

    auto& metadata = m_worldEntityMetadata[aWorldEntityId];
    metadata.SourceServerId = aSourceServerId;
    metadata.PlacedReferenceId = acReferenceId;

    spdlog::info("[STRE][WorldSync] placed_bound entity={} reference={:08X}:{:08X} localForm={:08X}",
                 aWorldEntityId, acReferenceId.ModId, acReferenceId.BaseId, localFormId);
    return true;
}

bool InventoryService::ApplyPlacedWorldEntityTransform(
    uint64_t aWorldEntityId, const WorldEntityTransform& acTransform, bool aEnableAfter) noexcept
{
    const auto entityIt = m_worldEntityToFormId.find(aWorldEntityId);
    if (entityIt == m_worldEntityToFormId.end())
        return false;

    auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(entityIt->second));
    if (!pReference || !pReference->parentCell)
        return false;

    NiPoint3 position{};
    position.x = acTransform.PositionX;
    position.y = acTransform.PositionY;
    position.z = acTransform.PositionZ;

    NiPoint3 rotation{};
    rotation.x = acTransform.RotationX;
    rotation.y = acTransform.RotationY;
    rotation.z = acTransform.RotationZ;

    spdlog::info("[STRE][WorldSyncDiag] placed_apply_begin entity={} localForm={:08X} disabled={} enableAfter={} primitive=stre-moveto",
                 aWorldEntityId, entityIt->second, pReference->IsDisabled(), aEnableAfter);

    // Do not call the custom SetPosition/SetAngle wrappers that were introduced by
    // the Better Grabbing bridge. CommonLib's SetPosition is a higher-level wrapper
    // around TESObjectREFR::MoveTo_Impl; treating relocation 19363 as
    // void(TESObjectREFR*, const NiPoint3&) is ABI-incorrect and crashes.
    //
    // STR already has a reverse-engineered MoveTo wrapper with the correct internal
    // signature. Set the desired rotation in the reference data, then let MoveTo
    // apply position + rotation together using the existing cell/worldspace.
    if (aEnableAfter)
    {
        spdlog::info("[STRE][WorldSyncDiag] placed_apply_before_enable entity={} localForm={:08X}",
                     aWorldEntityId, entityIt->second);
        pReference->Enable();
        spdlog::info("[STRE][WorldSyncDiag] placed_apply_after_enable entity={} localForm={:08X}",
                     aWorldEntityId, entityIt->second);
    }

    pReference->rotation = rotation;

    spdlog::info("[STRE][WorldSyncDiag] placed_apply_before_moveto entity={} localForm={:08X} cell={:08X}",
                 aWorldEntityId, entityIt->second, pReference->parentCell->formID);
    pReference->MoveTo(pReference->parentCell, position);
    spdlog::info("[STRE][WorldSyncDiag] placed_apply_after_moveto entity={} localForm={:08X} logical=({:.2f},{:.2f},{:.2f})",
                 aWorldEntityId, entityIt->second,
                 pReference->position.x, pReference->position.y, pReference->position.z);

    spdlog::info("[STRE][WorldSyncDiag] placed_apply_complete entity={} localForm={:08X} primitive=stre-moveto",
                 aWorldEntityId, entityIt->second);
    return true;
}

void InventoryService::SendLocalManipulationRelease(const WorldEntityTransform& acTransform) noexcept
{
    if (!m_localManipulation || m_localManipulation->WorldEntityId == 0)
        return;

    const LocalManipulation manipulation = *m_localManipulation;

    RequestWorldEntityManipulation request;
    request.WorldEntityId = manipulation.WorldEntityId;
    request.PlacedReferenceId = manipulation.PlacedReferenceId;
    request.Action = WorldEntityManipulationAction::Release;
    request.Transform = acTransform;
    m_transport.Send(request);

    if (manipulation.Granted)
        StartSettlementTracking(manipulation.WorldEntityId, manipulation.LocalFormId, true);
    else
        m_pendingDropStabilization.erase(manipulation.WorldEntityId);

    m_localManipulation.reset();

    if (const auto pendingRemote = m_pendingRemoteManipulations.find(manipulation.WorldEntityId);
        pendingRemote != m_pendingRemoteManipulations.end())
    {
        const auto pendingMessage = pendingRemote->second;
        m_pendingRemoteManipulations.erase(pendingRemote);
        if (pendingMessage.Action == WorldEntityManipulationAction::Release)
            EndRemoteWorldEntityManipulation(pendingMessage);
        else if (pendingMessage.Action != WorldEntityManipulationAction::Rejected)
            BeginRemoteWorldEntityManipulation(pendingMessage);
    }

    spdlog::info("[STRE][WorldSync] manipulation_release_requested entity={} localForm={:08X} granted={}",
                 manipulation.WorldEntityId, manipulation.LocalFormId, manipulation.Granted);
}

BSTEventResult InventoryService::OnEvent(
    const TESGrabReleaseEvent* apEvent, const EventDispatcher<TESGrabReleaseEvent>*)
{
    if (!m_transport.IsConnected() || !GetModuleHandleW(cBetterGrabbingModule) || !apEvent || !apEvent->ref.object)
        return BSTEventResult::kOk;

    TESObjectREFR* pGrabbedReference = apEvent->ref.object;
    const uint32_t localFormId = pGrabbedReference->formID;
    const WorldEntityTransform transform = ReadWorldEntityTransform(pGrabbedReference);

    if (apEvent->grabbed)
    {
        uint64_t worldEntityId = 0;
        GameId placedReferenceId{};

        if (const auto entityIt = m_formIdToWorldEntity.find(localFormId);
            entityIt != m_formIdToWorldEntity.end())
        {
            worldEntityId = entityIt->second;
            if (const auto metadataIt = m_worldEntityMetadata.find(worldEntityId);
                metadataIt != m_worldEntityMetadata.end())
            {
                placedReferenceId = metadataIt->second.PlacedReferenceId;
            }
        }
        else
        {
            // Dynamic drops are assigned a WorldEntity at creation time. Only
            // stable, non-temporary placed references may be adopted lazily.
            if (pGrabbedReference->IsTemporary() ||
                !m_world.GetModSystem().GetServerModId(localFormId, placedReferenceId))
            {
                return BSTEventResult::kOk;
            }
        }

        if (worldEntityId != 0)
        {
            m_pendingDropStabilization.erase(worldEntityId);
            m_remoteManipulations.erase(worldEntityId);
            m_pendingRemoteManipulations.erase(worldEntityId);
        }

        LocalManipulation manipulation;
        manipulation.WorldEntityId = worldEntityId;
        manipulation.LocalFormId = localFormId;
        manipulation.PlacedReferenceId = placedReferenceId;
        manipulation.LastSentTransform = transform;
        manipulation.LastSentAt = std::chrono::steady_clock::now();
        m_localManipulation = manipulation;

        RequestWorldEntityManipulation request;
        request.WorldEntityId = worldEntityId;
        request.PlacedReferenceId = placedReferenceId;
        request.Action = WorldEntityManipulationAction::Start;
        request.Transform = transform;
        m_transport.Send(request);

        if (worldEntityId == 0)
        {
            spdlog::info("[STRE][WorldSync] manipulation_adoption_requested localForm={:08X} reference={:08X}:{:08X}",
                         localFormId, placedReferenceId.ModId, placedReferenceId.BaseId);
        }
        else
        {
            spdlog::info("[STRE][WorldSync] manipulation_start_requested entity={} localForm={:08X}",
                         worldEntityId, localFormId);
        }
        return BSTEventResult::kOk;
    }

    // Release may happen before the adoption grant returns on a very short grab.
    // Preserve that release locally and send it immediately after the server grants
    // the stable WorldEntity identity.
    if (m_localManipulation && m_localManipulation->LocalFormId == localFormId)
    {
        if (m_localManipulation->WorldEntityId == 0)
        {
            m_localManipulation->ReleasePending = true;
            m_localManipulation->PendingReleaseTransform = transform;
            spdlog::info("[STRE][WorldSync] manipulation_release_deferred localForm={:08X} reason=awaiting-adoption",
                         localFormId);
            return BSTEventResult::kOk;
        }

        SendLocalManipulationRelease(transform);
        return BSTEventResult::kOk;
    }

    // Defensive fallback for an already-bound entity if local manipulation state
    // was lost during a load/menu transition.
    const auto entityIt = m_formIdToWorldEntity.find(localFormId);
    if (entityIt == m_formIdToWorldEntity.end())
        return BSTEventResult::kOk;

    RequestWorldEntityManipulation request;
    request.WorldEntityId = entityIt->second;
    if (const auto metadataIt = m_worldEntityMetadata.find(entityIt->second);
        metadataIt != m_worldEntityMetadata.end())
    {
        request.PlacedReferenceId = metadataIt->second.PlacedReferenceId;
    }
    request.Action = WorldEntityManipulationAction::Release;
    request.Transform = transform;
    m_transport.Send(request);
    spdlog::info("[STRE][WorldSync] manipulation_release_requested entity={} localForm={:08X} granted=false fallback=true",
                 entityIt->second, localFormId);
    return BSTEventResult::kOk;
}

void InventoryService::OnNotifyWorldEntityManipulation(
    const NotifyWorldEntityManipulation& acMessage) noexcept
{
    // TransportService dispatches packets directly from OnConsume(). Any Skyrim
    // reference mutation must therefore be marshalled through RunnerService, whose
    // task queue is drained from the per-frame UpdateEvent.
    m_world.GetRunner().Queue([this, message = acMessage]()
    {
        ProcessNotifyWorldEntityManipulation(message);
    });
}

void InventoryService::ProcessNotifyWorldEntityManipulation(
    const NotifyWorldEntityManipulation& acMessage) noexcept
{
    spdlog::info("[STRE][WorldSyncDiag] manipulation_process_on_update entity={} action={} authority={}",
                 acMessage.WorldEntityId, static_cast<uint8_t>(acMessage.Action), acMessage.AuthorityPlayerId);
    if (!m_transport.IsConnected() || acMessage.WorldEntityId == 0)
        return;

    const bool isLocalAuthority =
        acMessage.AuthorityPlayerId != 0 && acMessage.AuthorityPlayerId == m_transport.GetLocalPlayerId();

    if (acMessage.PlacedReferenceId)
        BindPlacedWorldEntity(acMessage.WorldEntityId, acMessage.PlacedReferenceId);

    if (acMessage.Action == WorldEntityManipulationAction::Rejected)
    {
        const bool matchesPendingAdoption =
            m_localManipulation && m_localManipulation->WorldEntityId == 0 &&
            m_localManipulation->PlacedReferenceId == acMessage.PlacedReferenceId;
        const bool matchesKnownEntity =
            m_localManipulation && m_localManipulation->WorldEntityId == acMessage.WorldEntityId;

        if (matchesPendingAdoption || matchesKnownEntity)
        {
            m_localManipulation->WorldEntityId = acMessage.WorldEntityId;
            m_localManipulation->Granted = false;
            NotifyWorldEntityManipulation authoritative = acMessage;
            authoritative.Action = WorldEntityManipulationAction::Release;
            m_pendingRemoteManipulations[acMessage.WorldEntityId] = authoritative;
        }

        spdlog::warn("[STRE][WorldSync] manipulation_rejected entity={} currentAuthority={}",
                     acMessage.WorldEntityId, acMessage.AuthorityPlayerId);
        return;
    }

    if (acMessage.Action == WorldEntityManipulationAction::Start && isLocalAuthority)
    {
        if (m_localManipulation &&
            (m_localManipulation->WorldEntityId == 0 || m_localManipulation->WorldEntityId == acMessage.WorldEntityId))
        {
            m_localManipulation->WorldEntityId = acMessage.WorldEntityId;
            if (acMessage.PlacedReferenceId)
                m_localManipulation->PlacedReferenceId = acMessage.PlacedReferenceId;
            m_localManipulation->Granted = true;
            m_localManipulation->LastSentTransform = acMessage.Transform;
            m_localManipulation->LastSentAt = std::chrono::steady_clock::now();

            const bool releasePending = m_localManipulation->ReleasePending;
            const WorldEntityTransform pendingRelease = m_localManipulation->PendingReleaseTransform;

            spdlog::info("[STRE][WorldSync] manipulation_granted entity={} authority={} role=local adopted={}",
                         acMessage.WorldEntityId, acMessage.AuthorityPlayerId, acMessage.PlacedReferenceId ? true : false);

            if (releasePending)
                SendLocalManipulationRelease(pendingRelease);
        }
        return;
    }

    if (isLocalAuthority)
        return;

    switch (acMessage.Action)
    {
    case WorldEntityManipulationAction::Start:
        BeginRemoteWorldEntityManipulation(acMessage);
        break;
    case WorldEntityManipulationAction::Update:
        // Heartbeats are server-private and should never be broadcast. Ignore
        // defensively if talking to a mixed/older development build.
        break;
    case WorldEntityManipulationAction::Release:
        EndRemoteWorldEntityManipulation(acMessage);
        break;
    case WorldEntityManipulationAction::Rejected:
        break;
    }

}

void InventoryService::BeginRemoteWorldEntityManipulation(
    const NotifyWorldEntityManipulation& acMessage) noexcept
{
    if (acMessage.PlacedReferenceId)
        BindPlacedWorldEntity(acMessage.WorldEntityId, acMessage.PlacedReferenceId);

    const bool competingLocalGrab = m_localManipulation && !m_localManipulation->Granted &&
        (m_localManipulation->WorldEntityId == acMessage.WorldEntityId ||
         (m_localManipulation->WorldEntityId == 0 && acMessage.PlacedReferenceId &&
          m_localManipulation->PlacedReferenceId == acMessage.PlacedReferenceId));
    if (competingLocalGrab)
    {
        // Better Grabbing is still physically holding the local copy. Defer the
        // authoritative hide until that losing local interaction releases.
        m_pendingRemoteManipulations[acMessage.WorldEntityId] = acMessage;
        return;
    }

    const auto entityIt = m_worldEntityToFormId.find(acMessage.WorldEntityId);
    if (entityIt == m_worldEntityToFormId.end())
    {
        m_pendingRemoteManipulations[acMessage.WorldEntityId] = acMessage;
        return;
    }

    auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(entityIt->second));
    if (!pReference)
    {
        m_pendingRemoteManipulations[acMessage.WorldEntityId] = acMessage;
        return;
    }

    const bool firstStart = m_remoteManipulations.find(acMessage.WorldEntityId) == m_remoteManipulations.end();
    if (firstStart)
    {
        m_pendingDropStabilization.erase(acMessage.WorldEntityId);
        pReference->Disable();

        RemoteManipulation remote;
        remote.LocalFormId = entityIt->second;
        m_remoteManipulations[acMessage.WorldEntityId] = remote;

        spdlog::info("[STRE][WorldSync] manipulation_hidden entity={} authority={} localForm={:08X} placed={}",
                     acMessage.WorldEntityId, acMessage.AuthorityPlayerId, entityIt->second,
                     acMessage.PlacedReferenceId ? true : false);
    }

    m_pendingRemoteManipulations.erase(acMessage.WorldEntityId);
}

void InventoryService::EndRemoteWorldEntityManipulation(
    const NotifyWorldEntityManipulation& acMessage) noexcept
{
    if (acMessage.PlacedReferenceId)
        BindPlacedWorldEntity(acMessage.WorldEntityId, acMessage.PlacedReferenceId);

    const bool competingLocalGrab = m_localManipulation && !m_localManipulation->Granted &&
        (m_localManipulation->WorldEntityId == acMessage.WorldEntityId ||
         (m_localManipulation->WorldEntityId == 0 && acMessage.PlacedReferenceId &&
          m_localManipulation->PlacedReferenceId == acMessage.PlacedReferenceId));
    if (competingLocalGrab)
    {
        m_pendingRemoteManipulations[acMessage.WorldEntityId] = acMessage;
        return;
    }

    const auto entityIt = m_worldEntityToFormId.find(acMessage.WorldEntityId);
    if (entityIt == m_worldEntityToFormId.end())
    {
        m_pendingRemoteManipulations[acMessage.WorldEntityId] = acMessage;
        return;
    }

    m_pendingRemoteManipulations.erase(acMessage.WorldEntityId);
    m_pendingDropStabilization.erase(acMessage.WorldEntityId);

    const auto metadataIt = m_worldEntityMetadata.find(acMessage.WorldEntityId);
    const bool isPlacedReference =
        metadataIt != m_worldEntityMetadata.end() && metadataIt->second.IsPlacedReference();

    if (isPlacedReference)
    {
        if (!ApplyPlacedWorldEntityTransform(acMessage.WorldEntityId, acMessage.Transform, true))
        {
            m_pendingRemoteManipulations[acMessage.WorldEntityId] = acMessage;
            spdlog::warn("[STRE][WorldSync] manipulation_reappear_deferred entity={} mode=placed-reference",
                         acMessage.WorldEntityId);
            return;
        }

        m_remoteManipulations.erase(acMessage.WorldEntityId);
        m_remoteAwaitingSettlementRecreate.erase(acMessage.WorldEntityId);

        if (acMessage.AuthorityPlayerId != 0)
            StartSettlementTracking(acMessage.WorldEntityId, entityIt->second, false);

        spdlog::info("[STRE][WorldSync] manipulation_released entity={} authority={} role=remote localForm={:08X} mode=placed-reference",
                     acMessage.WorldEntityId, acMessage.AuthorityPlayerId, entityIt->second);
        return;
    }

    // Dynamic drops keep their existing recreate path. This restores a fresh
    // collision/Havok representation at the release transform without sharing the
    // held object's intermediate motion.
    if (!QueueNetworkDrivenRecreate(acMessage.WorldEntityId, acMessage.Transform))
    {
        if (auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(entityIt->second)))
            pReference->Enable();
        m_remoteManipulations.erase(acMessage.WorldEntityId);
        m_remoteAwaitingSettlementRecreate.erase(acMessage.WorldEntityId);
        spdlog::error("[STRE][WorldSync] manipulation_reappear_fallback entity={} localForm={:08X} reason=recreate-not-queued",
                      acMessage.WorldEntityId, entityIt->second);
        return;
    }

    if (acMessage.AuthorityPlayerId != 0)
        m_remoteAwaitingSettlementRecreate.insert(acMessage.WorldEntityId);
    else
        m_remoteAwaitingSettlementRecreate.erase(acMessage.WorldEntityId);

    spdlog::info("[STRE][WorldSync] manipulation_released entity={} authority={} role=remote localForm={:08X} mode=reappear-at-release awaitingSettlement={}",
                 acMessage.WorldEntityId, acMessage.AuthorityPlayerId, entityIt->second, acMessage.AuthorityPlayerId != 0);
}

void InventoryService::RunLocalWorldEntityManipulation() noexcept
{
    using namespace std::chrono;

    if (!m_localManipulation || !m_localManipulation->Granted)
        return;

    auto* pReference = Cast<TESObjectREFR>(TESForm::GetById(m_localManipulation->LocalFormId));
    if (!pReference)
    {
        m_localManipulation.reset();
        return;
    }

    const auto now = steady_clock::now();
    if (now - m_localManipulation->LastSentAt < cManipulationHeartbeat)
        return;

    // Keep the server authority lease alive and remember the latest held pose for
    // disconnect recovery. Updates are intentionally NOT broadcast to observers:
    // their representation remains disabled until Release.
    const WorldEntityTransform current = ReadWorldEntityTransform(pReference);

    RequestWorldEntityManipulation request;
    request.WorldEntityId = m_localManipulation->WorldEntityId;
    request.PlacedReferenceId = m_localManipulation->PlacedReferenceId;
    request.Action = WorldEntityManipulationAction::Update;
    request.Transform = current;
    m_transport.Send(request);

    m_localManipulation->LastSentTransform = current;
    m_localManipulation->LastSentAt = now;
}
