#include <SexChangeProbe.h>
#include <TiltedOnlinePCH.h>
#include <Services/CharacterService.h>
#include <Services/AppearanceDomain.h>
#include <Services/TransportService.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Events/RequestLocalAppearanceUpdateEvent.h>
#include <Events/RaceAppearanceCompleteEvent.h>
#include <Services/RunnerService.h>
#include <Components.h>
#include <World.h>
#include <Games/References.h>
#include <Forms/TESNPC.h>
#include <Forms/TESRace.h>
#include <Systems/FaceGenSystem.h>
#include <VersionDb.h>
#include <Games/TES.h>
#include <Forms/BGSHeadPart.h>
#include <Forms/BGSColorForm.h>
#include <cmath>
#include <CharacterCreation/AppearancePostState.h>
#include <CharacterCreation/AppearanceClassification.h>
#include <CharacterCreation/AppearanceTintHash.h>
#include <string_view>

namespace
{
bool AppearanceRuntimeLocked()
{
    const auto* gate = CampaignRuntimeGateService::TryGet();
    return gate && gate->IsLocked();
}

STRE::CharacterCreation::AppearanceNodes ReadNodes(Actor* apActor)
{
    return {
        reinterpret_cast<uintptr_t>(apActor->GetNiNode()), reinterpret_cast<uintptr_t>(apActor->GetFaceGenNiNode()),
        reinterpret_cast<uintptr_t>(FaceGenSystem::GetHeadGeometry(apActor))};
}

STRE::CharacterCreation::AppearancePostState ReadPostState(World& world, entt::entity entity, Actor* actor, TESNPC* base)
{
    STRE::CharacterCreation::AppearancePostState state;
    const auto* remote = world.try_get<RemoteComponent>(entity);
    const auto* form = world.try_get<FormIdComponent>(entity);
    const auto* provenance = world.try_get<RemotePlayerAppearanceBaseComponent>(entity);
    state.ActorPointer = reinterpret_cast<uintptr_t>(actor);
    state.BasePointer = reinterpret_cast<uintptr_t>(base);
    state.ActorId = actor ? actor->formID : 0;
    state.BaseId = base ? base->formID : 0;
    state.ServerId = remote ? remote->Id : 0;
    state.FormId = form ? form->Id : 0;
    state.CachedRefId = remote ? remote->CachedRefId : 0;
    state.ProvenanceActorId = provenance ? provenance->ActorFormId : 0;
    state.ProvenanceBaseId = provenance ? provenance->BaseFormId : 0;
    state.RuntimeRace = actor && actor->race ? actor->race->formID : 0;
    state.BaseRace = base && base->raceForm.race ? base->raceForm.race->formID : 0;
    state.Sex = base ? base->actorData.IsFemale() : 0;
    state.Weight = base ? base->weight : 0.f;
    state.RemoteMarker = remote && actor && actor->GetExtension()->IsRemotePlayer();
    state.PlayerMarker = world.all_of<PlayerComponent>(entity);
    state.PrivateBase = base && base->IsTemporary();
    return state;
}
void LogAppearance(const char* apStep, uint32_t aServerId, Actor* apActor, TESNPC* apBase)
{
    const auto* hair = apBase->headData ? apBase->headData->hairColor : nullptr;
    spdlog::info("[CharacterService][AppearanceApply] {} serverId={} hairColorForm={:X} hairColorABGR={:X}", apStep, aServerId, hair ? hair->formID : 0, hair ? hair->abgr : 0);
    const auto inventory = apActor->GetActorInventory();
    int64_t count{};
    size_t worn{};
    for (const auto& entry : inventory.Entries)
    {
        count += entry.Count;
        worn += entry.IsWorn() ? 1 : 0;
    }
    uint64_t parts = 14695981039346656037ull;
    if (apBase->headparts)
        for (uint32_t i = 0; i < apBase->headpartsCount; ++i)
            parts = (parts ^ (apBase->headparts[i] ? apBase->headparts[i]->formID : 0)) * 1099511628211ull;
    spdlog::info(
        "[CharacterService][AppearanceApply] {} serverId={} actor={:X} base={:X} race={:X} baseRace={:X} sex={} weight={} headparts={} headpartsHash={:X} bodyColor={},{},{} "
        "position={},{},{} inventoryEntries={} inventoryCount={} equippedEntries={} dead={} remotePlayer={} ownershipEpoch=unavailable",
        apStep, aServerId, apActor->formID, apBase->formID, apActor->race ? apActor->race->formID : 0, apBase->raceForm.race ? apBase->raceForm.race->formID : 0,
        apBase->actorData.IsFemale(), apBase->weight, apBase->headpartsCount, parts, apBase->color.red, apBase->color.green, apBase->color.blue, apActor->position.x,
        apActor->position.y, apActor->position.z, inventory.Entries.size(), count, worn, apActor->IsDead(), apActor->GetExtension()->IsRemotePlayer());
}
} // namespace

void CharacterService::RunSexAppearance(entt::entity entity) noexcept
{
    using Cycle = STRE::CharacterCreation::SexAppearanceCycle;
    auto* apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
    if (!apply)
        return;
    const auto* remote = m_world.try_get<RemoteComponent>(entity);
    const auto* form = m_world.try_get<FormIdComponent>(entity);
    const uint32_t serverId = remote ? remote->Id : 0;
    const auto stop = [&](const char* reason)
    {
        spdlog::warn("[CharacterService][SexAppearanceApply] stopped serverId={} reason={}", serverId, reason);
        if (auto* current = m_world.try_get<RemoteAppearanceProbeComponent>(entity))
            current->Finish(false);
    };
    auto* actor = form ? Cast<Actor>(TESForm::GetById(form->Id)) : nullptr;
    auto* base = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
    if (!actor || !base || !remote)
    {
        stop("actor-base-or-remote-missing");
        return;
    }
    if (!m_transport.IsConnected() || AppearanceRuntimeLocked())
    {
        stop("offline-or-recovery-lock");
        return;
    }
    const auto logCheck = [&](const auto& before, const auto& expected, const auto& actual)
    {
        return STRE::CharacterCreation::CheckAppearancePostState(
            before, expected, actual,
            [&](const char* field, auto pre, auto wanted, auto value, bool matches)
            {
                spdlog::info(
                    "[CharacterService][SexAppearanceApply] postcheck field={} pre={} expected={} actual={} match={} diagnosticOnly={} serverId={}", field, pre, wanted, value,
                    matches, std::string_view(field) == "Weight", serverId);
            });
    };
    const auto inventory = [&](Actor* current)
    {
        int64_t count{};
        size_t worn{};
        for (const auto& entry : current->GetActorInventory().Entries)
        {
            count += entry.Count;
            worn += entry.IsWorn() ? 1 : 0;
        }
        return std::pair{count, worn};
    };
    if (!apply->Active)
    {
        if (!apply->Latest)
            return;
        const auto& descriptor = apply->Latest->Descriptor;
        const auto* race = actor->race;
        const auto male = AppearanceRaceModel(race, 0), female = AppearanceRaceModel(race, 1);
        const auto pre = ReadPostState(m_world, entity, actor, base);
        const auto nodes = ReadNodes(actor);
        auto* settings = INISettingCollection::Get();
        const bool privateBase =
            pre.RemoteMarker && pre.PlayerMarker && pre.PrivateBase && pre.ActorId == pre.CachedRefId && pre.ActorId == pre.ProvenanceActorId && pre.BaseId == pre.ProvenanceBaseId;
        if (!apply->Latest->IsValid() || !privateBase || !IsSupportedVanillaHumanoidAppearanceRace(race) || base->raceForm.race != race ||
            m_world.GetModSystem().GetGameId(descriptor.Race) != race->formID || descriptor.Sex == pre.Sex || !nodes.ThirdPerson || !nodes.Face || !nodes.Head ||
            actor->IsMount() || actor->extraData.Contains(ExtraDataType::Interaction) || VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0" || !settings ||
            !settings->GetSetting("bUseFaceGenPreprocessedHeads:General") || apply->Latest->FaceTints.Entries.empty())
        {
            spdlog::warn("[CharacterService][SexAppearanceApply] skipped serverId={} reason=sex-experiment-preguards nativeMutation=false", serverId);
            apply->Latest.reset();
            return;
        }
        spdlog::info(
            "[CharacterService][SexAppearanceApply] received classification=same-race-sex-change private-base verified precheck passed serverId={} currentSex={} targetSex={} "
            "race={:X} maleModel={} femaleModel={}",
            serverId, pre.Sex, descriptor.Sex, race->formID, male, female);
        LogAppearance("sex-before", serverId, actor, base);
        auto expected = pre;
        expected.Sex = descriptor.Sex;
        expected.Weight = descriptor.Weight;
        if (!apply->Begin(nodes))
            return;
        apply->Sex.Begin(expected);
        const auto [count, worn] = inventory(actor);
        apply->Sex.InventoryCount = count;
        apply->Sex.EquippedCount = worn;
        const auto snapshot = *apply->Active;
        if (!apply->Sex.RequestDeserialize())
        {
            stop("deserialize-state-invalid");
            return;
        }
        spdlog::info(
            "[CharacterService][SexAppearanceApply] deserialize serverId={} bytes={} changeFlags={:X} targetSex={}", serverId, snapshot.AppearanceBuffer.size(),
            snapshot.ChangeFlags, expected.Sex);
        base->Deserialize(snapshot.AppearanceBuffer, snapshot.ChangeFlags);
        apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
        actor = Cast<Actor>(TESForm::GetById(static_cast<uint32_t>(expected.ActorId)));
        base = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
        const auto actual = ReadPostState(m_world, entity, actor, base);
        const bool passed = logCheck(pre, expected, actual);
        if (!apply || !apply->Active || !apply->Sex.Enabled)
        {
            stop("active-state-lost-after-deserialize");
            return;
        }
        const bool sexMatches = base && actual.Sex == expected.Sex;
        spdlog::info(
            "[CharacterService][SexAppearanceApply] sex-postcheck {} expected={} actual={} serverId={}", sexMatches ? "passed" : "mismatch", expected.Sex, actual.Sex, serverId);
        if (!apply->Sex.Verify(passed && sexMatches, static_cast<uint32_t>(actual.Sex)))
        {
            if (!sexMatches)
                spdlog::warn("[CharacterService][SexAppearanceApply] stopped reason=deserialize-did-not-apply-sex nativeReset=false serverId={}", serverId);
            stop(sexMatches ? "post-deserialize-invariants" : "deserialize-did-not-apply-sex");
            return;
        }
        LogAppearance("sex-npc-deserialized", serverId, actor, base);
        const auto [afterCount, afterWorn] = inventory(actor);
        if (apply->Sex.InventoryLost(afterCount, afterWorn))
        {
            stop("logical-inventory-or-equipment-loss");
            return;
        }
        spdlog::info("[CharacterService][SexAppearanceApply] npc-deserialized postcheck passed serverId={}", serverId);
        const auto preReset = ReadNodes(actor);
        if (!apply->Sex.RequestReset(preReset))
        {
            stop("reset-state-invalid");
            return;
        }
        spdlog::info(
            "[CharacterService][SexAppearanceApply] reset3d requested reason=sex-head-rebuild-required serverId={} targetSex={} preResetRoot={:X} preResetFace={:X} "
            "preResetHead={:X}",
            serverId, expected.Sex, preReset.ThirdPerson, preReset.Face, preReset.Head);
        const bool issued = actor->QueueUpdate();
        apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
        if (!apply || !apply->Active || !apply->Sex.Enabled || !apply->Sex.ResetReturned(issued))
            stop("reset-failed-or-state-lost");
        return;
    }
    if (!apply->Sex.Enabled)
        return;
    const auto expected = apply->Sex.Expected;
    auto actual = ReadPostState(m_world, entity, actor, base);
    if (!logCheck(expected, expected, actual))
    {
        stop("sex-transition-invariants");
        return;
    }
    const auto [count, worn] = inventory(actor);
    if (apply->Sex.InventoryLost(count, worn))
    {
        stop("logical-inventory-or-equipment-loss");
        return;
    }
    if (!apply->Sex.Tick())
    {
        stop("sex-transition-timeout");
        return;
    }
    const auto nodes = ReadNodes(actor);
    if (apply->Sex.State != Cycle::Stage::ApplyingTints)
    {
        const auto before = apply->Sex.Before;
        spdlog::info(
            "[CharacterService][SexAppearanceApply] sex-transition serverId={} tick={} rootChanged={} faceChanged={} headChanged={} actualSex={} targetSex={} root={:X} face={:X} "
            "head={:X}",
            serverId, apply->Sex.Ticks, nodes.ThirdPerson != before.ThirdPerson, nodes.Face != before.Face, nodes.Head != before.Head, actual.Sex, expected.Sex, nodes.ThirdPerson,
            nodes.Face, nodes.Head);
        if (!apply->Sex.Observe(nodes))
        {
            if (apply->Sex.Ticks >= Cycle::MaxTicks)
                stop("sex-head-transition-timeout");
            return;
        }
        spdlog::info(
            "[CharacterService][SexAppearanceApply] sex-head-ready; facegen begin serverId={} tintCount={} tintHash={:X}", serverId, apply->Active->FaceTints.Entries.size(),
            STRE::CharacterCreation::AppearanceTintHash(apply->Active->FaceTints));
        const auto tints = apply->Active->FaceTints;
        FaceGenSystem::Setup(m_world, entity, tints);
        spdlog::info("[CharacterService][SexAppearanceApply] facegen setup serverId={}", serverId);
    }
    auto* face = m_world.try_get<FaceGenComponent>(entity);
    if (!face || !apply->Sex.GeometryReady(nodes))
    {
        if (apply->Sex.Ticks >= Cycle::MaxTicks)
            stop("sex-head-unavailable");
        return;
    }
    auto& tintTarget = apply->Sex.TintTarget;
    if (nodes.ThirdPerson != tintTarget.ThirdPerson || nodes.Face != tintTarget.Face || nodes.Head != tintTarget.Head)
    {
        tintTarget = nodes;
        face->Generated = false;
    }
    FaceGenSystem::Update(m_world, actor, *face);
    apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
    face = m_world.try_get<FaceGenComponent>(entity);
    actor = Cast<Actor>(TESForm::GetById(static_cast<uint32_t>(expected.ActorId)));
    base = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
    actual = ReadPostState(m_world, entity, actor, base);
    if (!apply || !apply->Active || !apply->Sex.Enabled || !face || !logCheck(expected, expected, actual))
    {
        stop("post-facegen-state-or-invariants");
        return;
    }
    const auto [finalCount, finalWorn] = inventory(actor);
    if (apply->Sex.InventoryLost(finalCount, finalWorn))
    {
        stop("logical-loss-after-facegen");
        return;
    }
    const auto after = ReadNodes(actor);
    const auto target = apply->Sex.TintTarget;
    if (!apply->Sex.GeometryReady(after) || after.ThirdPerson != target.ThirdPerson || after.Face != target.Face || after.Head != target.Head)
        face->Generated = false;
    spdlog::info(
        "[CharacterService][SexAppearanceApply] facegen update generated={} serverId={} tick={} root={:X} face={:X} head={:X} tintCount={} tintHash={:X}", face->Generated,
        serverId, apply->Sex.Ticks, after.ThirdPerson, after.Face, after.Head, face->FaceTints.Entries.size(), STRE::CharacterCreation::AppearanceTintHash(face->FaceTints));
    if (apply->Sex.Complete(face->Generated))
    {
        LogAppearance("sex-applied", serverId, actor, base);
        spdlog::info("[CharacterService][SexAppearanceApply] face-tints-applied; applied serverId={} targetSex={}", serverId, expected.Sex);
        apply->Finish(true);
    }
    else if (apply->Sex.Ticks >= Cycle::MaxTicks)
        stop("sex-facegen-timeout");
}
