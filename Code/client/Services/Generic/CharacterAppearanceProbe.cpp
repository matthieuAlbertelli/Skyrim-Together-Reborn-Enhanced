#include <SexChangeProbe.h>
#include <TiltedOnlinePCH.h>
#include <Services/CharacterService.h>
#include <Services/RemoteRespawnLab.h>
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

CharacterService::~CharacterService() noexcept
{
    if (auto* events = EventDispatcherManager::Get())
        events->switchRaceCompleteEvent.UnRegisterSink(this);
}

BSTEventResult CharacterService::OnEvent(const TESSwitchRaceCompleteEvent* event, const EventDispatcher<TESSwitchRaceCompleteEvent>*)
{
    if (event && event->actor)
        SignalSexChangeProbe(event->actor, "completion-event");
    if (event && event->actor && event->actor->GetExtension()->IsRemotePlayer())
        m_world.GetRunner().Trigger(
            RaceAppearanceCompleteEvent{event->actor->formID, reinterpret_cast<uintptr_t>(event->actor), event->actor->race ? event->actor->race->formID : 0});
    return BSTEventResult::kOk;
}

void CharacterService::OnRaceAppearanceComplete(const RaceAppearanceCompleteEvent& event) noexcept
{
    for (const auto entity : m_world.view<RemoteAppearanceProbeComponent>())
    {
        auto& apply = m_world.get<RemoteAppearanceProbeComponent>(entity);
        if (!apply.Active || !apply.Race.Enabled || apply.Race.Expected.ActorId != event.ActorId || apply.Race.Expected.ActorPointer != event.ActorToken ||
            apply.Race.TargetRace != event.RuntimeRace)
            continue;
        apply.Race.EventSeen = true;
        spdlog::info("[CharacterService][RaceAppearanceApply] switch-race-complete-event serverId={} milestoneOnly=true", apply.Race.Expected.ServerId);
    }
}

void CharacterService::OnLocalAppearanceUpdate(const RequestLocalAppearanceUpdateEvent& aEvent) noexcept
{
    if (!aEvent.FinalBuildRevision)
        return;
    const auto attempt = ++m_appearanceAttemptSeq;
    auto* tracePlayer = PlayerCharacter::Get();
    auto* traceNpc = tracePlayer ? Cast<TESNPC>(tracePlayer->baseForm) : nullptr;
    spdlog::info(
        "[STRE][AppearanceTrace][Publisher] phase=begin attemptSeq={} connected={} runtimeLocked={} playerPtr={:X} npcPtr={:X}", attempt, m_transport.IsConnected(),
        AppearanceRuntimeLocked(), reinterpret_cast<uintptr_t>(tracePlayer), reinterpret_cast<uintptr_t>(traceNpc));
    const auto rejected = [&](const char* reason)
    {
        spdlog::info("[STRE][AppearanceTrace][Publisher] phase=rejected attemptSeq={} reason={}", attempt, reason);
    };
    if (!m_transport.IsConnected() || AppearanceRuntimeLocked())
    {
        rejected(!m_transport.IsConnected() ? "transport-disconnected" : "runtime-locked");
        m_appearanceFinal.Reset();
        return;
    }
    m_appearanceFinal.Reset();
    auto* player = PlayerCharacter::Get();
    auto* npc = player ? Cast<TESNPC>(player->baseForm) : nullptr;
    if (!npc)
    {
        rejected(player ? "no-npc" : "no-player");
        return;
    }

    RequestCharacterAppearanceUpdate snapshot;
    snapshot.FinalBuildRevision = aEvent.FinalBuildRevision;
    // Fresh canonical capture, never the diagnostic's previous sample.
    if (!npc->raceForm.race)
    {
        rejected("no-race");
        return;
    }
    if (!m_world.GetModSystem().GetServerModId(npc->raceForm.race->formID, snapshot.Descriptor.Race))
    {
        rejected("server-race-unresolved");
        return;
    }
    snapshot.Descriptor.Sex = npc->actorData.IsFemale() ? 1 : 0;
    snapshot.Descriptor.Weight = npc->weight;
    snapshot.ChangeFlags = npc->GetChangeFlags();
    npc->Serialize(&snapshot.AppearanceBuffer);
    const auto& tints = player->GetTints();
    if (tints.length > tints.capacity || tints.length > CharacterAppearanceUpdate::MaxTintCount || (tints.length && !tints.data))
    {
        spdlog::warn("[CharacterService][AppearanceSync] final capture rejected reason=tint-array");
        rejected("tint-array");
        m_appearanceFinal.Reset();
        return;
    }
    for (uint32_t i = 0; i < tints.length; ++i)
    {
        const auto* tint = tints[i];
        if (!tint)
        {
            m_appearanceFinal.Reset();
            spdlog::warn("[CharacterService][AppearanceSync] final capture rejected reason=null-tint");
            rejected("null-tint");
            return;
        }
        Tints::Entry entry{};
        entry.Alpha = tint->alpha;
        entry.Color = tint->color;
        entry.Type = tint->type;
        const char* name = tint->texture ? tint->texture->name.AsAscii() : nullptr;
        const size_t size = name ? strnlen(name, CharacterAppearanceUpdate::MaxTextureNameBytes + 1) : 0;
        if (size > CharacterAppearanceUpdate::MaxTextureNameBytes)
        {
            m_appearanceFinal.Reset();
            spdlog::warn("[CharacterService][AppearanceSync] final capture rejected reason=texture-name");
            rejected("texture-name");
            return;
        }
        if (name)
            entry.Name.assign(name, size);
        snapshot.FaceTints.Entries.push_back(std::move(entry));
    }
    if (!snapshot.IsValid())
    {
        m_appearanceFinal.Reset();
        spdlog::warn("[CharacterService][AppearanceSync] final capture rejected reason=payload-bounds");
        rejected("payload-invalid");
        return;
    }
    spdlog::info(
        "[STRE][AppearanceTrace][Publisher] phase=captured attemptSeq={} descriptorRace={:X}:{:X} descriptorSex={} descriptorWeight={} changeFlags={:X} bytes={} tints={}", attempt,
        snapshot.Descriptor.Race.ModId, snapshot.Descriptor.Race.BaseId, snapshot.Descriptor.Sex, snapshot.Descriptor.Weight, snapshot.ChangeFlags,
        snapshot.AppearanceBuffer.size(), snapshot.FaceTints.Entries.size());
    uint64_t traceParts = 14695981039346656037ull;
    if (npc->headparts)
        for (uint32_t i = 0; i < npc->headpartsCount; ++i)
            traceParts = (traceParts ^ (npc->headparts[i] ? npc->headparts[i]->formID : 0)) * 1099511628211ull;
    const auto* traceHair = npc->headData ? npc->headData->hairColor : nullptr;
    spdlog::info("[STRE][AppearanceTrace][Publisher] phase=captured-native attemptSeq={} headparts={} headpartsHash={:X} hairColor={:X} bodyColor={},{},{}", attempt, npc->headpartsCount, traceParts, traceHair ? traceHair->formID : 0, npc->color.red, npc->color.green, npc->color.blue);
    LogAppearance("publisher-captured", 0, player, npc);
    m_appearanceFinal.Queue(true, std::move(snapshot));
    FlushAppearanceFinal();
    if (m_appearanceFinal.Snapshot)
        spdlog::info("[CharacterService][AppearanceSync] queued pending assignment or transport");
}

void CharacterService::FlushAppearanceFinal() noexcept
{
    if (AppearanceRuntimeLocked())
    {
        if (m_appearanceFinal.Snapshot)
            spdlog::info("[STRE][AppearanceTrace][Publisher] phase=flush attemptSeq={} reason=runtime-locked snapshotPending=true", m_appearanceAttemptSeq);
        m_appearanceFinal.Reset();
        return;
    }
    std::optional<uint32_t> id;
    const auto view = m_world.view<FormIdComponent, LocalComponent>();
    for (const auto entity : view)
        if (view.get<FormIdComponent>(entity).Id == 0x14)
        {
            id = view.get<LocalComponent>(entity).Id;
            break;
        }
    if (m_appearanceFinal.Snapshot)
    {
        spdlog::info(
            "[STRE][AppearanceTrace][Publisher] phase=flush attemptSeq={} connected={} localAssignmentPresent={} localServerId={} snapshotPending=true", m_appearanceAttemptSeq,
            m_transport.IsConnected(), id.has_value(), id.value_or(0));
        if (!m_transport.IsConnected())
            spdlog::info("[STRE][AppearanceTrace][Publisher] phase=pending-no-transport attemptSeq={} existingPolicy=discard", m_appearanceAttemptSeq);
        else if (!id)
            spdlog::info("[STRE][AppearanceTrace][Publisher] phase=pending-no-assignment attemptSeq={}", m_appearanceAttemptSeq);
    }
    m_appearanceFinal.Flush(
        m_transport.IsConnected(), id,
        [&](const RequestCharacterAppearanceUpdate& acSnapshot)
        {
            spdlog::info("[STRE][AppearanceTrace][Publisher] phase=send-attempt attemptSeq={} actorId={}", m_appearanceAttemptSeq, acSnapshot.ActorId);
            if (!m_transport.Send(acSnapshot))
            {
                spdlog::info("[STRE][AppearanceTrace][Publisher] phase=send-failed attemptSeq={} actorId={}", m_appearanceAttemptSeq, acSnapshot.ActorId);
                return false;
            }
            spdlog::info("[STRE][AppearanceTrace][Publisher] phase=sent attemptSeq={} actorId={}", m_appearanceAttemptSeq, acSnapshot.ActorId);
            spdlog::info(
                "[CharacterService][AppearanceSync] sent actor={} bytes={} tints={} final-only", acSnapshot.ActorId, acSnapshot.AppearanceBuffer.size(),
                acSnapshot.FaceTints.Entries.size());
            return true;
        });
}

void CharacterService::OnAppearanceProbe(const NotifyCharacterAppearanceUpdate& acSnapshot) noexcept
{
    // Official final rematerialization fails closed; legacy hot dispatch stays bypassed.
    STRE::RemoteRespawnLab::ReceiveFinal(m_world, acSnapshot);
}

void CharacterService::ApplyAppearanceSnapshots() noexcept
{
    using Apply = STRE::CharacterCreation::AppearanceApply;
    if (!m_transport.IsConnected() || AppearanceRuntimeLocked())
    {
        m_world.clear<RemoteAppearanceProbeComponent, RemotePlayerAppearanceBaseComponent>();
        return;
    }
    const auto view = m_world.view<RemoteAppearanceProbeComponent>();
    Vector<entt::entity> entities(view.begin(), view.end());
    for (const auto entity : entities)
    {
        auto* apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
        if (!apply || (!apply->Latest && !apply->Active))
            continue;
        const auto* remote = m_world.try_get<RemoteComponent>(entity);
        const auto* form = m_world.try_get<FormIdComponent>(entity);
        const auto* provenance = m_world.try_get<RemotePlayerAppearanceBaseComponent>(entity);
        auto* actor = form ? Cast<Actor>(TESForm::GetById(form->Id)) : nullptr;
        auto* base = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
        if (!remote || !m_world.all_of<PlayerComponent>(entity) || !provenance || !actor || !base || !actor->GetExtension()->IsRemotePlayer() ||
            !provenance->Matches(actor->formID, base->formID) || remote->CachedRefId != actor->formID || !base->IsTemporary())
        {
            if (!apply->ProvenanceWarning)
                spdlog::warn("[CharacterService][AppearanceApply] skipped reason=private-base-not-proven pendingRetained=true");
            apply->ProvenanceWarning = true;
            if (apply->Active)
                apply->Finish(false);
            continue;
        }
        if (apply->Active && apply->Combined.Enabled)
        {
            RunCombinedAppearance(entity);
            continue;
        }
        if (apply->Active && apply->Sex.Enabled)
        {
            RunSexAppearance(entity);
            continue;
        }
        const uint32_t serverId = remote->Id;
        const auto nodes = ReadNodes(actor);
        if (!apply->Active)
        {
            // WaitingFor3D can already own an Actor. Keep the received snapshot
            // pending until ordinary initial setup finishes; never run a second
            // reconstruction or apply new tints during that initial setup.
            if (m_world.all_of<WaitingFor3D>(entity) || !nodes.ThirdPerson || !nodes.Face || !nodes.Head)
                continue;
            if (actor->IsMount() || actor->extraData.Contains(ExtraDataType::Interaction) || VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0")
            {
                spdlog::warn("[CharacterService][AppearanceApply] skipped reason=mounted-or-unsupported-runtime");
                apply->Latest.reset();
                continue;
            }
            const auto& descriptor = apply->Latest->Descriptor;
            GameId actorRace{}, baseRace{};
            const bool racesResolved = actor->race && base->raceForm.race && m_world.GetModSystem().GetServerModId(actor->race->formID, actorRace) &&
                                       m_world.GetModSystem().GetServerModId(base->raceForm.race->formID, baseRace);
            spdlog::info(
                "[CharacterService][AppearanceApply] descriptor serverId={} race={:X}:{:X} sex={} weight={}", serverId, descriptor.Race.ModId, descriptor.Race.BaseId,
                descriptor.Sex, descriptor.Weight);
            using Classification = STRE::CharacterCreation::AppearanceClassification;
            const auto targetId = m_world.GetModSystem().GetGameId(descriptor.Race);
            auto* target = targetId ? Cast<TESRace>(TESForm::GetById(targetId)) : nullptr;
            const auto classification = STRE::CharacterCreation::ClassifyAppearanceUpdate(
                descriptor, actorRace, baseRace, base->actorData.IsFemale() ? 1 : 0, racesResolved && target && !target->IsTemporary(),
                provenance->Matches(actor->formID, base->formID));
            const bool raceChange = classification == Classification::RaceChangeSameSex;
            if (classification == Classification::Invalid || !IsSupportedVanillaHumanoidAppearanceRace(actor->race) || !IsSupportedVanillaHumanoidAppearanceRace(target))
            {
                spdlog::warn("[CharacterService][AppearanceApply] skipped serverId={} reason=invalid-state-or-unsupported-domain nativeMutation=false", serverId);
                apply->Latest.reset();
                continue;
            }
            spdlog::info(
                "[CharacterService][AppearanceApply] classification={} sourceRace={:X} targetRace={:X} sourceSex={} targetSex={} sourceModel={} targetModel={} serverId={}",
                static_cast<unsigned>(classification), actor->race->formID, targetId, base->actorData.IsFemale(), descriptor.Sex,
                AppearanceRaceModel(actor->race, base->actorData.IsFemale()), AppearanceRaceModel(target, descriptor.Sex), serverId);
            if (classification == Classification::RaceAndSexChange)
            {
                RunCombinedAppearance(entity);
                continue;
            }
            if (classification == Classification::SameRaceSexChange)
            {
                RunSexAppearance(entity);
                continue;
            }
            if (apply->Latest->FaceTints.Entries.empty())
            {
                spdlog::warn("[CharacterService][AppearanceApply] skipped serverId={} reason=empty-tints-clear-unproven", serverId);
                apply->Latest.reset();
                continue;
            }
            auto* settings = INISettingCollection::Get();
            if (!settings || !settings->GetSetting("bUseFaceGenPreprocessedHeads:General"))
            {
                spdlog::warn("[CharacterService][AppearanceApply] skipped reason=missing-facegen-setting");
                apply->Latest.reset();
                continue;
            }
            if (!raceChange)
                spdlog::info("[CharacterService][AppearanceApply] private-base verified; same-race-sex verified serverId={}", serverId);
            LogAppearance("before", serverId, actor, base);
            const auto before = ReadPostState(m_world, entity, actor, base);
            apply->Begin(nodes);
            // Copy active bytes across engine calls; callbacks may invalidate ECS.
            const auto snapshot = *apply->Active;
            const auto actorId = actor->formID;
            if (raceChange)
            {
                auto expected = before;
                expected.Sex = snapshot.Descriptor.Sex;
                expected.Weight = snapshot.Descriptor.Weight;
                apply->Race.Begin(targetId, expected);
                const auto inventory = actor->GetActorInventory();
                for (const auto& entry : inventory.Entries)
                {
                    apply->Race.InventoryCount += entry.Count;
                    apply->Race.EquippedCount += entry.IsWorn() ? 1 : 0;
                }
                spdlog::info(
                    "[CharacterService][RaceAppearanceApply] switch-race requested serverId={} from={:X} to={:X} argPlayer=false", serverId, actor->race->formID, targetId);
                const bool issued = apply->Race.RequestSwitch() && actor->SwitchRace(target, STRE::CharacterCreation::RaceSwitchPlayerArgument);
                apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
                auto* returnedActor = Cast<Actor>(TESForm::GetById(actorId));
                auto* returnedBase = returnedActor ? Cast<TESNPC>(returnedActor->baseForm) : nullptr;
                if (!apply || !apply->Active || !apply->Race.Enabled)
                {
                    spdlog::error("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=active-state-lost-after-switch", serverId);
                    continue;
                }
                const auto actual = ReadPostState(m_world, entity, returnedActor, returnedBase);
                auto switchExpected = apply->Race.Expected;
                switchExpected.Sex = before.Sex;
                const bool valid = STRE::CharacterCreation::CheckAppearancePostState(
                    before, switchExpected, actual,
                    [&](const char* field, auto pre, auto wanted, auto value, bool matches)
                    {
                        spdlog::info(
                            "[CharacterService][RaceAppearanceApply] post-return {} pre={} expected={} actual={} match={} diagnosticOnly={}", field, pre, wanted, value, matches,
                            std::string_view(field) == "Weight");
                    });
                if (!apply->Race.Returned(issued && valid))
                {
                    spdlog::error("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=post-switch-invariants-or-call-failed no-rollback=true", serverId);
                    apply->Finish(false);
                    continue;
                }
                spdlog::info(
                    "[CharacterService][RaceAppearanceApply] switch-race returned serverId={} runtimeRace={:X} baseRace={:X} overlayRace={:X}", serverId, actual.RuntimeRace,
                    actual.BaseRace, returnedBase->overlayRace ? returnedBase->overlayRace->formID : 0);
                // Reacquired identity passed. Reserve the single Deserialize for either race strategy.
                actor = returnedActor;
                base = returnedBase;
                if (!apply->Race.RequestDeserialize())
                {
                    apply->Finish(false);
                    continue;
                }
            }
            spdlog::info(
                "[CharacterService][AppearanceApply] deserialize serverId={} changeFlags={:08X} bytes={} descriptorSex={} descriptorWeight={}", serverId, snapshot.ChangeFlags,
                snapshot.AppearanceBuffer.size(), snapshot.Descriptor.Sex, snapshot.Descriptor.Weight);
            base->Deserialize(snapshot.AppearanceBuffer, snapshot.ChangeFlags);
            apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
            auto* currentActor = Cast<Actor>(TESForm::GetById(actorId));
            auto* currentBase = currentActor ? Cast<TESNPC>(currentActor->baseForm) : nullptr;
            const auto actual = ReadPostState(m_world, entity, currentActor, currentBase);
            auto expected = before;
            if (raceChange)
                expected.RuntimeRace = expected.BaseRace = targetId;
            expected.Sex = snapshot.Descriptor.Sex;
            expected.Weight = snapshot.Descriptor.Weight;
            // Emit all comparisons even if the native call removed the active state.
            const bool passed = STRE::CharacterCreation::CheckAppearancePostState(
                before, expected, actual,
                [&](const char* field, auto pre, auto wanted, auto value, bool matches)
                {
                    if (std::string_view(field) == "Weight")
                    {
                        spdlog::info(
                            "[CharacterService][AppearanceApply] postcheck diagnostic Weight pre={} expected={} actual={} nonfatal=true match={} serverId={}", pre, wanted, value,
                            matches, serverId);
                        return;
                    }
                    spdlog::info(
                        "[CharacterService][AppearanceApply] postcheck {} {} pre={} expected={} actual={} serverId={}", matches ? "match" : "mismatch", field, pre, wanted, value,
                        serverId);
                });
            if (currentActor && currentBase)
                LogAppearance("post-deserialize-observed", serverId, currentActor, currentBase);
            if (!apply || !apply->Active)
            {
                spdlog::error("[CharacterService][AppearanceApply] postcheck mismatch active-state expected=present actual=absent serverId={} no-rollback-attempted", serverId);
                continue;
            }
            if (!passed)
            {
                spdlog::error("[CharacterService][AppearanceApply] stopped serverId={} reason=post-deserialize-invariant-mismatch no-rollback-attempted", serverId);
                apply->Finish(false);
                continue;
            }
            if (raceChange)
            {
                if (!apply->Race.Enabled || !apply->Race.Deserialized(true))
                {
                    spdlog::error("[CharacterService][RaceAppearanceApply] stopped reason=race-state-lost-after-deserialize serverId={}", serverId);
                    apply->Finish(false);
                    continue;
                }
                spdlog::info("[CharacterService][RaceAppearanceApply] npc-deserialized postcheck passed serverId={}", serverId);
                LogAppearance("race-npc-deserialized", serverId, actor, base);
                const auto preReset = ReadNodes(currentActor);
                if (!apply->RequestRaceReset(preReset))
                {
                    spdlog::error("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=reset-state-invalid", serverId);
                    apply->Finish(false);
                    continue;
                }
                spdlog::info(
                    "[CharacterService][RaceAppearanceApply] reset3d requested reason=race-head-rebuild-required serverId={} preResetRoot={:X} preResetFace={:X} preResetHead={:X}",
                    serverId, preReset.ThirdPerson, preReset.Face, preReset.Head);
                const bool resetIssued = currentActor->QueueUpdate();
                apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
                if (!apply || !apply->RaceResetReturned(resetIssued))
                    spdlog::error("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=reset-failed-or-state-lost", serverId);
                continue;
            }
            spdlog::info("[CharacterService][AppearanceApply] postcheck passed serverId={}", serverId);
            LogAppearance("npc-deserialized", serverId, actor, base);
            spdlog::info(
                "[CharacterService][AppearanceApply] weight before-reset serverId={} actual={} descriptor={} diagnosticOnly=true", serverId, base->weight,
                snapshot.Descriptor.Weight);
            if (apply->RequestReset() && actor->QueueUpdate())
                spdlog::info("[CharacterService][AppearanceApply] reset3d requested serverId={} updateWeight=true", serverId);
            else if (auto* current = m_world.try_get<RemoteAppearanceProbeComponent>(entity))
                current->Finish(false);
            continue;
        }
        if (apply->Race.Enabled)
        {
            const auto actual = ReadPostState(m_world, entity, actor, base);
            const bool valid = STRE::CharacterCreation::CheckAppearancePostState(
                apply->Race.Expected, apply->Race.Expected, actual,
                [&](const char* field, auto, auto wanted, auto value, bool matches)
                {
                    if (!matches)
                        spdlog::warn(
                            "[CharacterService][RaceAppearanceApply] transition invariant={} expected={} actual={} diagnosticOnly={}", field, wanted, value,
                            std::string_view(field) == "Weight");
                });
            int64_t count{};
            size_t equipped{};
            for (const auto& entry : actor->GetActorInventory().Entries)
            {
                count += entry.Count;
                equipped += entry.IsWorn() ? 1 : 0;
            }
            if (!valid || apply->Race.InventoryLost(count, equipped))
            {
                spdlog::error(
                    "[CharacterService][RaceAppearanceApply] stopped serverId={} reason={} inventoryCount={} equippedEntries={}", serverId,
                    valid ? "logical-inventory-or-equipment-loss" : "race-transition-invariant", count, equipped);
                apply->Finish(false);
                continue;
            }
        }
        if (apply->State == Apply::Stage::WaitingForHead)
        {
            if (apply->Race.Enabled)
                spdlog::info(
                    "[CharacterService][RaceAppearanceApply] race-transition serverId={} tick={} rootChanged={} faceChanged={} headChanged={} root={:X} face={:X} head={:X}",
                    serverId, apply->Ticks + 1, nodes.ThirdPerson != apply->Before.ThirdPerson, nodes.Face != apply->Before.Face, nodes.Head != apply->Before.Head,
                    nodes.ThirdPerson, nodes.Face, nodes.Head);
            if (apply->ObserveHead(nodes))
            {
                if (apply->Race.Enabled)
                    spdlog::info(
                        "[CharacterService][RaceAppearanceApply] race-head-ready race-transition serverId={} tick={} eventSeen={} rootOld={:X} rootNew={:X} faceOld={:X} "
                        "faceNew={:X} headOld={:X} headNew={:X}",
                        serverId, apply->Ticks, apply->Race.EventSeen, apply->Before.ThirdPerson, nodes.ThirdPerson, apply->Before.Face, nodes.Face, apply->Before.Head,
                        nodes.Head);
                spdlog::info("[CharacterService][AppearanceApply] transition serverId={} tick={}", serverId, apply->Ticks);
                spdlog::info(
                    "[CharacterService][AppearanceApply] weight transition serverId={} actual={} descriptor={} diagnosticOnly=true", serverId, base->weight,
                    apply->Active->Descriptor.Weight);
                const auto* oldFace = m_world.try_get<FaceGenComponent>(entity);
                const bool hadFace = oldFace != nullptr;
                const bool generatedBefore = oldFace && oldFace->Generated;
                spdlog::info(
                    "[CharacterService][AppearanceApply] facegen begin serverId={} tintCount={} tintHash={:X} face={:X} head={:X} generatedBefore={} bodyColor={},{},{}", serverId,
                    apply->Active->FaceTints.Entries.size(), STRE::CharacterCreation::AppearanceTintHash(apply->Active->FaceTints), nodes.Face, nodes.Head, generatedBefore,
                    base->color.red, base->color.green, base->color.blue);
                FaceGenSystem::Setup(m_world, entity, apply->Active->FaceTints);
                const auto* setupFace = m_world.try_get<FaceGenComponent>(entity);
                if (apply->Race.Enabled)
                    spdlog::info(
                        "[CharacterService][RaceAppearanceApply] facegen begin; facegen setup serverId={} generated={} tintCount={} tintHash={:X}", serverId,
                        setupFace && setupFace->Generated, setupFace ? setupFace->FaceTints.Entries.size() : 0,
                        setupFace ? STRE::CharacterCreation::AppearanceTintHash(setupFace->FaceTints) : 0);
                spdlog::info(
                    "[CharacterService][AppearanceApply] facegen setup serverId={} replacedExisting={} componentPresent={} generatedBefore={} generatedAfter={} tintCount={} "
                    "tintHash={:X}",
                    serverId, hadFace, setupFace != nullptr, generatedBefore, setupFace && setupFace->Generated, setupFace ? setupFace->FaceTints.Entries.size() : 0,
                    setupFace ? STRE::CharacterCreation::AppearanceTintHash(setupFace->FaceTints) : 0);
            }
            else
            {
                if (apply->Ticks >= Apply::MaxTicks)
                {
                    spdlog::warn("[CharacterService][AppearanceApply] outcome=inconclusive serverId={} reason=head-transition-timeout ticks=120", serverId);
                    if (apply->Race.Enabled)
                        spdlog::warn("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=race-head-transition-timeout ticks=120", serverId);
                    apply->Finish(false);
                }
                continue;
            }
        }
        else
            ++apply->Ticks;
        // Head can change again while waiting for its material. Restart tint
        // generation against that head, never accept Generated from an old one.
        auto* face = m_world.try_get<FaceGenComponent>(entity);
        if (!face || (apply->Race.Enabled ? !STRE::CharacterCreation::RaceAppearanceCycle::GeometryReady(apply->Before, nodes)
                                          : (!nodes.Face || !nodes.Head || (nodes.Face == apply->Before.Face && nodes.Head == apply->Before.Head))))
        {
            if (apply->Ticks >= Apply::MaxTicks)
            {
                spdlog::warn("[CharacterService][AppearanceApply] outcome=inconclusive serverId={} reason=new-head-unavailable ticks=120", serverId);
                if (apply->Race.Enabled)
                    spdlog::warn("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=new-race-head-unavailable ticks=120", serverId);
                apply->Finish(false);
            }
            continue;
        }
        if (nodes.Face != apply->TintTarget.Face || nodes.Head != apply->TintTarget.Head || (apply->Race.Enabled && nodes.ThirdPerson != apply->TintTarget.ThirdPerson))
        {
            apply->TintTarget = nodes;
            face->Generated = false;
        }
        const bool raceCycle = apply->Race.Enabled;
        const auto updateActorId = actor->formID;
        FaceGenSystem::Update(m_world, actor, *face);
        if (raceCycle)
        {
            apply = m_world.try_get<RemoteAppearanceProbeComponent>(entity);
            face = m_world.try_get<FaceGenComponent>(entity);
            actor = Cast<Actor>(TESForm::GetById(updateActorId));
            base = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
            if (!apply || !apply->Active || !apply->Race.Enabled || !face)
            {
                spdlog::error("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=state-lost-after-facegen", serverId);
                if (apply)
                    apply->Finish(false);
                continue;
            }
            const auto actual = ReadPostState(m_world, entity, actor, base);
            const bool valid = STRE::CharacterCreation::CheckAppearancePostState(
                apply->Race.Expected, apply->Race.Expected, actual,
                [&](const char* field, auto, auto wanted, auto value, bool matches)
                {
                    if (!matches)
                        spdlog::warn(
                            "[CharacterService][RaceAppearanceApply] post-facegen invariant={} expected={} actual={} diagnosticOnly={}", field, wanted, value,
                            std::string_view(field) == "Weight");
                });
            if (!valid)
            {
                spdlog::error("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=post-facegen-invariant", serverId);
                apply->Finish(false);
                continue;
            }
            int64_t count{};
            size_t equipped{};
            for (const auto& entry : actor->GetActorInventory().Entries)
            {
                count += entry.Count;
                equipped += entry.IsWorn() ? 1 : 0;
            }
            if (apply->Race.InventoryLost(count, equipped))
            {
                spdlog::error("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=logical-loss-after-facegen", serverId);
                apply->Finish(false);
                continue;
            }
            spdlog::info("[CharacterService][RaceAppearanceApply] facegen update generated={} serverId={}", face->Generated, serverId);
        }
        const auto afterNodes = ReadNodes(actor);
        // Never report completion if Update's target changed underneath it.
        if (afterNodes.Face != apply->TintTarget.Face || afterNodes.Head != apply->TintTarget.Head ||
            (apply->Race.Enabled && afterNodes.ThirdPerson != apply->TintTarget.ThirdPerson))
            face->Generated = false;
        spdlog::info(
            "[CharacterService][AppearanceApply] facegen update generated={} serverId={} tick={} face={:X} head={:X} sameTarget={} tintCount={} tintHash={:X} activeTintHash={:X}",
            face->Generated, serverId, apply->Ticks, afterNodes.Face, afterNodes.Head, afterNodes.Face == apply->TintTarget.Face && afterNodes.Head == apply->TintTarget.Head,
            face->FaceTints.Entries.size(), STRE::CharacterCreation::AppearanceTintHash(face->FaceTints), STRE::CharacterCreation::AppearanceTintHash(apply->Active->FaceTints));
        spdlog::info(
            "[CharacterService][AppearanceApply] facegen geometry serverId={} sameTransition={} bodyColor={},{},{}", serverId,
            afterNodes.Face == apply->Transition.Face && afterNodes.Head == apply->Transition.Head, base->color.red, base->color.green, base->color.blue);
        if (apply->CompleteTints(face->Generated))
        {
            spdlog::info("[CharacterService][AppearanceApply] face-tints-applied serverId={} tintCount={}", serverId, face->FaceTints.Entries.size());
            LogAppearance("applied", serverId, actor, base);
            if (apply->Race.Enabled)
                spdlog::info("[CharacterService][RaceAppearanceApply] face-tints-applied; applied serverId={} targetRace={:X} generated=true", serverId, apply->Race.TargetRace);
        }
        else if (apply->Ticks >= Apply::MaxTicks)
        {
            spdlog::warn("[CharacterService][AppearanceApply] outcome=inconclusive serverId={} reason=facegen-timeout ticks=120", serverId);
            if (apply->Race.Enabled)
                spdlog::warn("[CharacterService][RaceAppearanceApply] stopped serverId={} reason=facegen-timeout ticks=120", serverId);
            apply->Finish(false);
        }
    }
}
