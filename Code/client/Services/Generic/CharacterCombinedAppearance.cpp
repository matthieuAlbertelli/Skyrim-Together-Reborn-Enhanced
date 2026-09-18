#include <SexChangeProbe.h>
#include <TiltedOnlinePCH.h>
#include <Services/CharacterService.h>
#include <Services/AppearanceTrace.h>
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

void CharacterService::RunCombinedAppearance(entt::entity entity) noexcept
{
    using Cycle = STRE::CharacterCreation::CombinedAppearanceCycle;
    using Stage = Cycle::Stage;
    auto* apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
    const auto* traceRemote = m_world.try_get<RemoteComponent>(entity);
    spdlog::info(
        "[STRE][AppearanceTrace][Combined] phase=enter active={} latest={} serverId={} nativeInProgress={}", apply && apply->Active.has_value(), apply && apply->Latest.has_value(),
        traceRemote ? traceRemote->Id : 0, apply && apply->Combined.NativeCallInProgress);
    if (apply && apply->Combined.Enabled && apply->Combined.NativeCallInProgress)
        return;
    const auto* form = m_world.try_get<FormIdComponent>(entity);
    const auto* remote = m_world.try_get<RemoteComponent>(entity);
    const auto actorId = form ? form->Id : 0;
    const auto serverId = remote ? remote->Id : 0;
    auto* actor = actorId ? Cast<Actor>(TESForm::GetById(actorId)) : nullptr;
    auto* base = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
    const auto stop = [&](const char* reason)
    {
        spdlog::warn("[CharacterService][CombinedAppearanceApply] stopped serverId={} reason={}", serverId, reason);
        if (auto* current = m_world.try_get<RemoteAppearanceProbeComponent>(entity))
            current->Finish(false);
    };
    if (!apply || !remote || !actor || !base || !m_transport.IsConnected() || AppearanceRuntimeLocked())
    {
        stop("combined-state-or-transport-unavailable");
        return;
    }
    const auto inventory = [](Actor* current)
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
    const auto observe = [&](const char* step)
    {
        TraceAppearanceActor(m_world, entity, serverId, actor, step, m_appearanceTraceTick);
        LogAppearance(step, serverId, actor, base);
        const auto nodes = ReadNodes(actor);
        spdlog::info(
            "[CharacterService][CombinedAppearanceApply] {} serverId={} overlayRace={:X} root={:X} face={:X} head={:X} maleModel={} femaleModel={}", step, serverId,
            base->overlayRace ? base->overlayRace->formID : 0, nodes.ThirdPerson, nodes.Face, nodes.Head, AppearanceRaceModel(actor->race, 0), AppearanceRaceModel(actor->race, 1));
    };
    // Reacquire after every native call. All fifteen fatal fields are checked for
    // the current phase: race/source-sex first, then race/target-sex. Weight is diagnostic.
    const auto check = [&](const char* step)
    {
        apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
        actor = Cast<Actor>(TESForm::GetById(actorId));
        base = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
        if (!apply || !apply->Active || !apply->Combined.Enabled || !m_transport.IsConnected() || AppearanceRuntimeLocked())
            return false;
        apply->Combined.NativeCallInProgress = false;
        const auto expected = apply->Combined.Expected();
        const auto actual = ReadPostState(m_world, entity, actor, base);
        const bool passed = STRE::CharacterCreation::CheckAppearancePostState(
            expected, expected, actual,
            [&](const char* field, auto, auto wanted, auto value, bool matches)
            {
                spdlog::info(
                    "[CharacterService][CombinedAppearanceApply] {} field={} expected={} actual={} match={} diagnosticOnly={} serverId={}", step, field, wanted, value, matches,
                    std::string_view(field) == "Weight", serverId);
            });
        if (!passed || !actor || !base)
            return false;
        const auto [count, worn] = inventory(actor);
        return !apply->Combined.InventoryLost(count, worn);
    };
    if (apply->Combined.Enabled && apply->Combined.State == Stage::RaceStageReady && !apply->Active)
    {
        stop("combined-race-ready-active-lost");
        return;
    }
    if (!apply->Active)
    {
        if (!apply->Latest)
            return;
        const auto descriptor = apply->Latest->Descriptor;
        const auto targetId = m_world.GetModSystem().GetGameId(descriptor.Race);
        auto* target = targetId ? Cast<TESRace>(TESForm::GetById(targetId)) : nullptr;
        const auto source = ReadPostState(m_world, entity, actor, base);
        const auto nodes = ReadNodes(actor);
        auto* settings = INISettingCollection::Get();
        const bool privateBase = source.RemoteMarker && source.PlayerMarker && source.PrivateBase && source.ActorId == source.CachedRefId &&
                                 source.ActorId == source.ProvenanceActorId && source.BaseId == source.ProvenanceBaseId;
        const std::pair<const char*, bool> guards[] = {
            {"payloadValid", apply->Latest->IsValid()},
            {"privateBase", privateBase},
            {"sourceSupported", IsSupportedVanillaHumanoidAppearanceRace(actor->race)},
            {"targetSupported", IsSupportedVanillaHumanoidAppearanceRace(target)},
            {"raceActuallyDiffers", actor->race != target},
            {"runtimeBaseRaceCoherent", actor->race == base->raceForm.race},
            {"sexActuallyDiffers", descriptor.Sex != source.Sex},
            {"notWaitingFor3D", !m_world.all_of<WaitingFor3D>(entity)},
            {"rootPresent", nodes.ThirdPerson != 0},
            {"facePresent", nodes.Face != 0},
            {"headPresent", nodes.Head != 0},
            {"notMounted", !actor->IsMount()},
            {"noInteraction", !actor->extraData.Contains(ExtraDataType::Interaction)},
            {"facegenSetting", settings && settings->GetSetting("bUseFaceGenPreprocessedHeads:General")},
            {"tintsPresent", !apply->Latest->FaceTints.Entries.empty()}};
        bool failed = false;
        for (const auto& [guard, passed] : guards)
        {
            spdlog::info("[STRE][AppearanceTrace][Combined] phase=preguard serverId={} guard={} passed={}", serverId, guard, passed);
            if (!passed)
            {
                spdlog::info("[STRE][AppearanceTrace][Combined] phase=skip serverId={} failedGuard={}", serverId, guard);
                failed = true;
            }
        }
        if (failed)
        {
            apply->Latest.reset();
            return;
        }
        spdlog::info(
            "[CharacterService][CombinedAppearanceApply] classification=RaceAndSexChange combined-race-stage begin serverId={} sourceRace={:X} targetRace={:X} sourceSex={} "
            "targetSex={}",
            serverId, source.RuntimeRace, targetId, source.Sex, descriptor.Sex);
        observe("combined-race-stage before-switch");
        if (!apply->Begin(nodes))
            return;
        apply->Combined.Begin(source, targetId, descriptor.Sex, descriptor.Weight);
        const auto [count, worn] = inventory(actor);
        apply->Combined.InventoryCount = count;
        apply->Combined.EquippedCount = worn;
        if (!apply->Combined.RequestSwitch())
        {
            stop("combined-switch-state-invalid");
            return;
        }
        spdlog::info("[CharacterService][CombinedAppearanceApply] switch-race requested serverId={} targetRace={:X} argPlayer=false", serverId, targetId);
        apply->Combined.NativeCallInProgress = true;
        const bool issued = actor->SwitchRace(target, false);
        const bool valid = check("combined-race-stage post-switch");
        if (!valid || !apply->Combined.VerifyRace(issued && valid))
        {
            stop("combined-race-stage-switch-or-postcheck-failed");
            return;
        }
        observe("combined-race-stage post-switch");
        if (!apply->Combined.RequestRaceReset(ReadNodes(actor)))
        {
            stop("combined-race-reset-state-invalid");
            return;
        }
        spdlog::info("[CharacterService][CombinedAppearanceApply] combined-race-stage reset3d requested ordinal=1 serverId={} finalDeserialize=false", serverId);
        apply->Combined.NativeCallInProgress = true;
        const bool reset = actor->QueueUpdate();
        const bool preserved = check("combined-race-stage post-reset");
        if (!preserved || !apply->Combined.RaceResetReturned(reset))
        {
            stop("combined-race-stage-reset-failed-or-invariants");
            return;
        }
        observe("combined-race-stage after-reset");
        return;
    }
    if (!apply->Combined.Enabled)
        return;
    // In-call reservation states must not advance through reentrant update callbacks.
    const auto state = apply->Combined.State;
    if (state == Stage::RaceStageReady)
    {
        // The update identity also fences reentry in the same service passage.
        if (apply->Combined.RaceReadyUpdate == m_appearanceTraceTick)
            return;
        if (!check("combined-final-stage resume-invariant") || m_world.all_of<WaitingFor3D>(entity) ||
            !STRE::CharacterCreation::RenewedAppearanceGeometry(apply->Combined.BeforeRaceReset, ReadNodes(actor)))
        {
            stop("combined-race-ready-invariants-or-geometry-lost");
            return;
        }
        observe("combined-final-stage resume-from-race-ready");
        const auto resumeNodes = ReadNodes(actor);
        spdlog::info(
            "[CharacterService][CombinedAppearanceApply] combined-final-stage resume-from-race-ready serverId={} serviceUpdate={} race={:X} sex={} root={:X} face={:X} head={:X}",
            serverId, m_appearanceTraceTick, actor->race->formID, base->actorData.IsFemale(), resumeNodes.ThirdPerson, resumeNodes.Face, resumeNodes.Head);
        // Final payload is used only now, after observable race/source-sex readiness.
        const auto snapshot = *apply->Active;
        if (!apply->Combined.RequestFinalDeserialize(m_appearanceTraceTick))
        {
            stop("combined-final-deserialize-state-invalid");
            return;
        }
        spdlog::info(
            "[CharacterService][CombinedAppearanceApply] combined-final-stage deserialize serverId={} bytes={} changeFlags={:X} targetSex={}", serverId,
            snapshot.AppearanceBuffer.size(), snapshot.ChangeFlags, snapshot.Descriptor.Sex);
        apply->Combined.NativeCallInProgress = true;
        base->Deserialize(snapshot.AppearanceBuffer, snapshot.ChangeFlags);
        const bool valid = check("combined-final-stage post-deserialize");
        if (!valid || !apply->Combined.VerifyFinal(valid))
        {
            stop("combined-final-deserialize-invariants-or-target-sex");
            return;
        }
        observe("combined-final-stage deserialized");
        if (!apply->Combined.RequestFinalReset(ReadNodes(actor)))
        {
            stop("combined-final-reset-state-invalid");
            return;
        }
        spdlog::info("[CharacterService][CombinedAppearanceApply] combined-final-stage reset3d requested ordinal=2 serverId={}", serverId);
        apply->Combined.NativeCallInProgress = true;
        const bool reset = actor->QueueUpdate();
        const bool preserved = check("combined-final-stage post-reset");
        if (!preserved || !apply->Combined.FinalResetReturned(reset))
        {
            stop("combined-final-reset-failed-or-invariants");
            return;
        }
        observe("combined-final-stage after-reset");
        return;
    }
    if (state != Stage::WaitingForRaceGeometry && state != Stage::WaitingForFinalGeometry && state != Stage::ApplyingTints)
        return;
    const bool racePhase = state == Stage::WaitingForRaceGeometry;
    if (!check(racePhase ? "combined-race-stage invariant" : "combined-final-stage invariant"))
    {
        stop("combined-phase-invariants-or-logical-loss");
        return;
    }
    const auto timeout = racePhase ? "combined-race-stage-head-timeout" : "combined-final-stage-head-or-facegen-timeout";
    if (!apply->Combined.Tick())
    {
        stop(timeout);
        return;
    }
    const auto nodes = ReadNodes(actor);
    const auto before = racePhase ? apply->Combined.BeforeRaceReset : apply->Combined.BeforeFinalReset;
    spdlog::info(
        "[CharacterService][CombinedAppearanceApply] {} transition serverId={} tick={} race={:X} sex={} root={:X} face={:X} head={:X} rootChanged={} faceChanged={} headChanged={}",
        racePhase ? "combined-race-stage" : "combined-final-stage", serverId, apply->Combined.Ticks, actor->race->formID, base->actorData.IsFemale(), nodes.ThirdPerson, nodes.Face,
        nodes.Head, nodes.ThirdPerson != before.ThirdPerson, nodes.Face != before.Face, nodes.Head != before.Head);
    if (racePhase)
    {
        if (!apply->Combined.ObserveRace(nodes, m_appearanceTraceTick))
        {
            if (apply->Combined.Ticks >= Cycle::MaxStageTicks)
                stop("combined-race-stage-head-timeout");
            return;
        }
        observe("combined-race-stage ready");
        spdlog::info(
            "[CharacterService][CombinedAppearanceApply] combined-race-stage ready phase-boundary=return-to-service serverId={} serviceUpdate={}", serverId, m_appearanceTraceTick);
        return;
    }

    if (apply->Combined.State == Stage::WaitingForFinalGeometry)
    {
        if (!apply->Combined.ObserveFinal(nodes))
        {
            if (apply->Combined.Ticks >= Cycle::MaxStageTicks)
                stop("combined-final-stage-head-timeout");
            return;
        }
        observe("combined-final-stage ready");
        if (!apply->Combined.RequestFaceGen())
        {
            stop("combined-facegen-state-invalid");
            return;
        }
        const auto tints = apply->Active->FaceTints;
        spdlog::info(
            "[CharacterService][CombinedAppearanceApply] FaceGen setup final-only serverId={} tintCount={} tintHash={:X}", serverId, tints.Entries.size(),
            STRE::CharacterCreation::AppearanceTintHash(tints));
        apply->Combined.NativeCallInProgress = true;
        FaceGenSystem::Setup(m_world, entity, tints);
        if (!check("combined-final-stage post-facegen-setup"))
        {
            stop("combined-facegen-setup-invariants");
            return;
        }
    }
    auto* face = m_world.try_get<FaceGenComponent>(entity);
    const auto currentNodes = ReadNodes(actor);
    if (!face || !apply->Combined.FinalGeometryReady(currentNodes))
    {
        if (apply->Combined.Ticks >= Cycle::MaxStageTicks)
            stop("combined-final-stage-facegen-timeout");
        return;
    }
    if (!(currentNodes == apply->Combined.TintTarget))
    {
        apply->Combined.TintTarget = currentNodes;
        face->Generated = false;
    }
    const auto tintTarget = apply->Combined.TintTarget;
    apply->Combined.NativeCallInProgress = true;
    FaceGenSystem::Update(m_world, actor, *face);
    if (!check("combined-final-stage post-facegen-update"))
    {
        stop("combined-facegen-invariants");
        return;
    }
    face = m_world.try_get<FaceGenComponent>(entity);
    if (!face)
    {
        stop("combined-facegen-component-lost");
        return;
    }
    const auto after = ReadNodes(actor);
    if (!apply->Combined.FinalGeometryReady(after) || !(after == tintTarget))
        face->Generated = false;
    spdlog::info("[CharacterService][CombinedAppearanceApply] FaceGen update serverId={} Generated={} tick={}", serverId, face->Generated, apply->Combined.Ticks);
    if (apply->Combined.Complete(face->Generated))
    {
        observe("combined-applied");
        spdlog::info(
            "[CharacterService][CombinedAppearanceApply] face-tints-applied Applied serverId={} resets={} deserialize={} switches={}", serverId, apply->Combined.ResetCount,
            apply->Combined.DeserializeCount, apply->Combined.SwitchCount);
        apply->Finish(true);
    }
    else if (apply->Combined.Ticks >= Cycle::MaxStageTicks)
        stop("combined-final-stage-facegen-timeout");
}
