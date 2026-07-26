#include <Services/CharacterBuildService.h>

#include <CharacterCreation/CharacterBuildCatalog.h>
#include <Components.h>
#include <Game/Player.h>
#include <GameServer.h>
#include <Messages/CharacterBuildAppliedRequest.h>
#include <Messages/CharacterBuildRequest.h>
#include <Messages/CharacterBuildResponse.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <Messages/NotifyPlayerLevel.h>
#include <World.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <vector>
#include <string_view>
#include <optional>
#include <string>
#include <utility>

namespace
{
using SelectionMap = std::map<std::string, std::string>;

bool TryNormalizeSelections(
    const Vector<CharacterBuildSelectionData>& acSelections,
    SelectionMap& aSelections) noexcept
{
    if (acSelections.size() >
        STRE::CharacterCreation::kMaximumSelectionCount)
    {
        return false;
    }

    for (const CharacterBuildSelectionData& selection : acSelections)
    {
        if (selection.GroupId.empty() || selection.OptionId.empty())
            return false;

        const auto [it, inserted] = aSelections.emplace(
            selection.GroupId.c_str(),
            selection.OptionId.c_str());
        (void)it;
        if (!inserted)
            return false;
    }

    return true;
}

Vector<CharacterBuildSelectionData> ToNetworkSelections(
    const SelectionMap& acSelections)
{
    Vector<CharacterBuildSelectionData> result;
    result.reserve(acSelections.size());

    for (const auto& [groupId, optionId] : acSelections)
    {
        CharacterBuildSelectionData selection;
        selection.GroupId = groupId.c_str();
        selection.OptionId = optionId.c_str();
        result.push_back(std::move(selection));
    }

    return result;
}

bool TryResolveServerModId(
    World& aWorld,
    const char* apPluginName,
    std::uint32_t& aModId) noexcept
{
    if (!apPluginName)
        return false;

    const ModsComponent& mods = aWorld.ctx().at<ModsComponent>();

    const auto findPlugin = [apPluginName, &aModId](const auto& acMods)
    {
        for (const auto& [name, entry] : acMods)
        {
            if (std::strcmp(name.c_str(), apPluginName) == 0)
            {
                aModId = entry.id;
                return true;
            }
        }

        return false;
    };

    return findPlugin(mods.GetStandardMods()) ||
        findPlugin(mods.GetLiteMods());
}

bool IsKnownServerModId(
    World& aWorld,
    std::uint32_t aModId) noexcept
{
    const ModsComponent& mods = aWorld.ctx().at<ModsComponent>();
    const auto containsId = [aModId](const auto& acMods)
    {
        return std::any_of(
            acMods.begin(),
            acMods.end(),
            [aModId](const auto& acEntry)
            {
                return acEntry.second.id == aModId;
            });
    };

    return containsId(mods.GetStandardMods()) ||
        containsId(mods.GetLiteMods());
}

bool TryBuildCanonicalInventory(
    World& aWorld,
    std::string_view aClassId,
    const SelectionMap& acSelections,
    Inventory& aInventory) noexcept
{
    aInventory = {};

    using ItemKey = std::pair<std::uint32_t, std::uint32_t>;
    std::map<ItemKey, std::int32_t> totalCounts;
    std::map<ItemKey, std::int32_t> rightWornCounts;
    std::map<ItemKey, std::int32_t> leftWornCounts;

    const auto resolveKey =
        [&aWorld](const char* apPluginName,
                  std::uint32_t aLocalFormId,
                  ItemKey& aKey) noexcept
    {
        std::uint32_t modId = 0;
        if (!TryResolveServerModId(
                aWorld,
                apPluginName,
                modId))
        {
            spdlog::error(
                "[STRE][CharacterBuild][Server] Missing plugin while building canonical inventory plugin={} localForm={:08X}",
                apPluginName,
                aLocalFormId);
            return false;
        }

        aKey = {modId, aLocalFormId};
        return true;
    };

    const std::vector<STRE::CharacterCreation::ItemGrant> grants =
        STRE::CharacterCreation::BuildItemGrants(
            aClassId,
            acSelections);

    for (const STRE::CharacterCreation::ItemGrant& grant : grants)
    {
        if (grant.Count <= 0)
            return false;

        ItemKey key;
        if (!resolveKey(
                grant.PluginName,
                grant.LocalFormId,
                key))
        {
            return false;
        }

        totalCounts[key] += grant.Count;
    }

    const std::vector<STRE::CharacterCreation::EquipmentGrant>
        equipment =
            STRE::CharacterCreation::BuildEquipmentGrants(
                aClassId,
                acSelections);

    for (const STRE::CharacterCreation::EquipmentGrant& equipped :
         equipment)
    {
        if (equipped.Count <= 0)
            return false;

        ItemKey key;
        if (!resolveKey(
                equipped.PluginName,
                equipped.LocalFormId,
                key))
        {
            return false;
        }

        const auto total = totalCounts.find(key);
        if (total == totalCounts.end())
        {
            spdlog::error(
                "[STRE][CharacterBuild][Server] Equipment plan references an item absent from canonical grants mod={} base={:08X}",
                key.first,
                key.second);
            return false;
        }

        auto& wornCounts =
            equipped.Side ==
                    STRE::CharacterCreation::EquipmentSide::Left
                ? leftWornCounts
                : rightWornCounts;
        wornCounts[key] += equipped.Count;
    }

    for (const auto& [key, totalCount] : totalCounts)
    {
        const std::int32_t rightCount = rightWornCounts[key];
        const std::int32_t leftCount = leftWornCounts[key];
        if (rightCount < 0 ||
            leftCount < 0 ||
            rightCount + leftCount > totalCount)
        {
            spdlog::error(
                "[STRE][CharacterBuild][Server] Invalid equipment plan mod={} base={:08X} total={} right={} left={}",
                key.first,
                key.second,
                totalCount,
                rightCount,
                leftCount);
            return false;
        }

        const GameId baseId{key.first, key.second};

        if (rightCount > 0)
        {
            Inventory::Entry entry;
            entry.BaseId = baseId;
            entry.Count = rightCount;
            entry.ExtraWorn = true;
            aInventory.AddOrRemoveEntry(entry);
        }

        if (leftCount > 0)
        {
            Inventory::Entry entry;
            entry.BaseId = baseId;
            entry.Count = leftCount;
            entry.ExtraWornLeft = true;
            aInventory.AddOrRemoveEntry(entry);
        }

        const std::int32_t unequippedCount =
            totalCount - rightCount - leftCount;
        if (unequippedCount > 0)
        {
            Inventory::Entry entry;
            entry.BaseId = baseId;
            entry.Count = unequippedCount;
            aInventory.AddOrRemoveEntry(entry);
        }
    }

    std::sort(
        aInventory.Entries.begin(),
        aInventory.Entries.end(),
        [](const Inventory::Entry& acLeft,
           const Inventory::Entry& acRight)
        {
            if (acLeft.BaseId.ModId != acRight.BaseId.ModId)
                return acLeft.BaseId.ModId < acRight.BaseId.ModId;
            if (acLeft.BaseId.BaseId != acRight.BaseId.BaseId)
                return acLeft.BaseId.BaseId < acRight.BaseId.BaseId;
            if (acLeft.ExtraWorn != acRight.ExtraWorn)
                return acLeft.ExtraWorn;
            return acLeft.ExtraWornLeft &&
                !acRight.ExtraWornLeft;
        });

    return true;
}

bool IsSameSubmittedBuild(
    const CharacterBuildSnapshotData& acBuild,
    const CharacterBuildRequest& acRequest,
    const Vector<CharacterBuildSelectionData>& acSelections) noexcept
{
    return acBuild.BuildVersion == acRequest.BuildVersion &&
        acBuild.RaceId == acRequest.RaceId &&
        acBuild.ClassId == acRequest.ClassId &&
        acBuild.Selections == acSelections;
}

void SendState(
    Player& aPlayer,
    std::uint32_t aServerId,
    const CharacterBuildComponent& acComponent,
    CharacterBuildNetworkState aState,
    bool aBroadcast) noexcept
{
    NotifyCharacterBuildState notify;
    notify.State = aState;
    notify.PlayerId = aPlayer.GetId();
    notify.ServerId = aServerId;
    notify.Revision = acComponent.Revision;
    notify.Build = acComponent.Build;

    if (aBroadcast)
        GameServer::Get()->SendToPlayers(notify);
    else
        aPlayer.Send(notify);
}
}

CharacterBuildService::CharacterBuildService(
    World& aWorld,
    entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_buildRequestConnection(
          aDispatcher.sink<PacketEvent<CharacterBuildRequest>>()
              .connect<&CharacterBuildService::OnCharacterBuildRequest>(this))
    , m_buildAppliedConnection(
          aDispatcher.sink<PacketEvent<CharacterBuildAppliedRequest>>()
              .connect<&CharacterBuildService::OnCharacterBuildAppliedRequest>(this))
{
}

void CharacterBuildService::OnCharacterBuildRequest(
    const PacketEvent<CharacterBuildRequest>& acPacket) noexcept
{
    Player* const pPlayer = acPacket.pPlayer;
    if (!pPlayer)
        return;

    const CharacterBuildRequest& request = acPacket.Packet;

    spdlog::info(
        "[STRE][CharacterBuild][Server] Request player={} version={} race={:016X} classId={} selections={}",
        pPlayer->GetId(),
        request.BuildVersion,
        request.RaceId.LogFormat(),
        request.ClassId.c_str(),
        request.Selections.size());

    if (request.BuildVersion !=
        STRE::CharacterCreation::kCharacterBuildVersion)
    {
        SendRejected(*pPlayer, CharacterBuildResult::RejectedVersion);
        return;
    }

    const std::optional<entt::entity> character =
        pPlayer->GetCharacter();
    if (!character || !m_world.valid(*character))
    {
        SendRejected(*pPlayer, CharacterBuildResult::RejectedNoCharacter);
        return;
    }

    if (!request.RaceId ||
        !IsKnownServerModId(m_world, request.RaceId.ModId))
    {
        SendRejected(*pPlayer, CharacterBuildResult::RejectedInvalidRace);
        return;
    }

    SelectionMap selections;
    if (!TryNormalizeSelections(request.Selections, selections) ||
        !STRE::CharacterCreation::ValidateSelections(
            request.ClassId.c_str(),
            selections))
    {
        SendRejected(*pPlayer, CharacterBuildResult::RejectedInvalidBuild);
        return;
    }

    const Vector<CharacterBuildSelectionData> networkSelections =
        ToNetworkSelections(selections);

    Inventory canonicalInventory;
    if (!TryBuildCanonicalInventory(
            m_world,
            request.ClassId.c_str(),
            selections,
            canonicalInventory))
    {
        SendRejected(*pPlayer, CharacterBuildResult::RejectedMissingPlugin);
        return;
    }

    InventoryComponent* const pInventory =
        m_world.try_get<InventoryComponent>(*character);
    if (!pInventory)
    {
        SendRejected(*pPlayer, CharacterBuildResult::RejectedNoCharacter);
        return;
    }

    if (CharacterBuildComponent* const pExisting =
            m_world.try_get<CharacterBuildComponent>(*character))
    {
        if (pExisting->Applied)
        {
            spdlog::warn(
                "[STRE][CharacterBuild][Server] Request rejected player={} reason=alreadyApplied revision={}",
                pPlayer->GetId(),
                pExisting->Revision);
            SendRejected(
                *pPlayer,
                CharacterBuildResult::RejectedAlreadyApplied);
            return;
        }

        if (IsSameSubmittedBuild(
                pExisting->Build,
                request,
                networkSelections))
        {
            CharacterBuildResponse response;
            response.Result = CharacterBuildResult::Accepted;
            response.Revision = pExisting->Revision;
            response.ServerId = World::ToInteger(*character);
            response.Build = pExisting->Build;
            pPlayer->Send(response);

            spdlog::info(
                "[STRE][CharacterBuild][Server] Accepted response retransmitted player={} revision={}",
                pPlayer->GetId(),
                pExisting->Revision);
            return;
        }

        // The client may legitimately retry from RaceMenu after a local engine
        // application failure. Until the build is acknowledged as Applied, the
        // owner is allowed to supersede the pending proposal atomically. This
        // prevents a failed local grant from permanently trapping the character
        // behind RejectedAlreadyPending for the rest of the server session.
        pExisting->Revision = m_nextRevision++;
        pExisting->Build.BuildVersion = request.BuildVersion;
        pExisting->Build.RaceId = request.RaceId;
        pExisting->Build.ClassId = request.ClassId;
        pExisting->Build.Selections = networkSelections;
        pExisting->Build.CanonicalInventory = canonicalInventory;
        pExisting->Build.InventoryHash =
            ComputeCharacterBuildInventoryHash(canonicalInventory);
        pExisting->Applied = false;

        pInventory->Content = pExisting->Build.CanonicalInventory;

        CharacterBuildResponse response;
        response.Result = CharacterBuildResult::Accepted;
        response.Revision = pExisting->Revision;
        response.ServerId = World::ToInteger(*character);
        response.Build = pExisting->Build;
        pPlayer->Send(response);

        SendState(
            *pPlayer,
            World::ToInteger(*character),
            *pExisting,
            CharacterBuildNetworkState::Accepted,
            true);

        spdlog::info(
            "[STRE][CharacterBuild][Server] Pending build replaced player={} serverId={:X} revision={} classId={} inventoryEntries={} inventoryHash={:016X}",
            pPlayer->GetId(),
            World::ToInteger(*character),
            pExisting->Revision,
            pExisting->Build.ClassId.c_str(),
            pExisting->Build.CanonicalInventory.Entries.size(),
            pExisting->Build.InventoryHash);
        return;
    }

    CharacterBuildComponent component;
    component.Revision = m_nextRevision++;
    component.Build.BuildVersion = request.BuildVersion;
    component.Build.RaceId = request.RaceId;
    component.Build.ClassId = request.ClassId;
    component.Build.Selections = networkSelections;
    component.Build.CanonicalInventory = canonicalInventory;
    component.Build.InventoryHash =
        ComputeCharacterBuildInventoryHash(canonicalInventory);
    component.Applied = false;

    // The old imported inventory is discarded atomically on the authority.
    // The client then mirrors this canonical snapshot locally under event
    // suppression, so no sequence of 95 removal packets can desynchronize it.
    pInventory->Content = component.Build.CanonicalInventory;

    CharacterBuildComponent& stored =
        m_world.emplace<CharacterBuildComponent>(
            *character,
            std::move(component));

    CharacterBuildResponse response;
    response.Result = CharacterBuildResult::Accepted;
    response.Revision = stored.Revision;
    response.ServerId = World::ToInteger(*character);
    response.Build = stored.Build;
    pPlayer->Send(response);

    SendState(
        *pPlayer,
        World::ToInteger(*character),
        stored,
        CharacterBuildNetworkState::Accepted,
        true);

    spdlog::info(
        "[STRE][CharacterBuild][Server] Build accepted player={} serverId={:X} revision={} classId={} inventoryEntries={} inventoryHash={:016X}",
        pPlayer->GetId(),
        World::ToInteger(*character),
        stored.Revision,
        stored.Build.ClassId.c_str(),
        stored.Build.CanonicalInventory.Entries.size(),
        stored.Build.InventoryHash);
}

void CharacterBuildService::OnCharacterBuildAppliedRequest(
    const PacketEvent<CharacterBuildAppliedRequest>& acPacket) noexcept
{
    Player* const pPlayer = acPacket.pPlayer;
    if (!pPlayer)
        return;

    const std::optional<entt::entity> character =
        pPlayer->GetCharacter();
    if (!character || !m_world.valid(*character))
    {
        SendRejected(*pPlayer, CharacterBuildResult::RejectedNoCharacter);
        return;
    }

    CharacterBuildComponent* const pBuild =
        m_world.try_get<CharacterBuildComponent>(*character);
    if (!pBuild || pBuild->Revision != acPacket.Packet.Revision)
    {
        SendRejected(*pPlayer, CharacterBuildResult::RejectedRevision);
        return;
    }

    if (acPacket.Packet.InventoryHash != pBuild->Build.InventoryHash)
    {
        spdlog::warn(
            "[STRE][CharacterBuild][Server] Applied acknowledgement rejected player={} revision={} expectedHash={:016X} actualHash={:016X}",
            pPlayer->GetId(),
            pBuild->Revision,
            pBuild->Build.InventoryHash,
            acPacket.Packet.InventoryHash);
        SendRejected(
            *pPlayer,
            CharacterBuildResult::RejectedInventoryHash);
        return;
    }

    pBuild->Applied = true;
    pPlayer->SetLevel(1);

    NotifyPlayerLevel levelNotify;
    levelNotify.PlayerId = pPlayer->GetId();
    levelNotify.NewLevel = 1;
    GameServer::Get()->SendToPlayers(levelNotify, pPlayer);

    SendState(
        *pPlayer,
        World::ToInteger(*character),
        *pBuild,
        CharacterBuildNetworkState::Applied,
        true);

    spdlog::info(
        "[STRE][CharacterBuild][Server] Build applied player={} serverId={:X} revision={} classId={} inventoryHash={:016X} level=1",
        pPlayer->GetId(),
        World::ToInteger(*character),
        pBuild->Revision,
        pBuild->Build.ClassId.c_str(),
        pBuild->Build.InventoryHash);
}

void CharacterBuildService::SendRejected(
    Player& aPlayer,
    CharacterBuildResult aResult) const noexcept
{
    CharacterBuildResponse response;
    response.Result = aResult;
    aPlayer.Send(response);

    spdlog::warn(
        "[STRE][CharacterBuild][Server] Build rejected player={} result={}",
        aPlayer.GetId(),
        static_cast<std::uint32_t>(aResult));
}
