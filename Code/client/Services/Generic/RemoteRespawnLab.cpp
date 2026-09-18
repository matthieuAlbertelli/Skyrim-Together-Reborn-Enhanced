#include <TiltedOnlinePCH.h>
#include <Services/RemoteRespawnLab.h>
#include <BranchInfo.h>
#include <CharacterCreation/FinalRespawn.h>
#include <CharacterCreation/RemoteMaterializationLifecycle.h>
#include <Services/RemoteActorProjection.h>
#include <Services/CharacterService.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Services/CampaignService.h>
#include <Services/TransportService.h>
#include <Services/InventoryService.h>
#include <Systems/FaceGenSystem.h>
#include <Messages/CharacterAppearanceUpdate.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <Components.h>
#include <World.h>
#include <Actor.h>
#include <Games/ActorExtension.h>
#include <Games/Overrides.h>
#include <Forms/TESNPC.h>
#include <Forms/TESRace.h>
#include <Forms/TESObjectCELL.h>
#include <Forms/TESWorldSpace.h>
#include <Forms/BGSHeadPart.h>
#include <chrono>
#include <map>
#include <memory>

namespace STRE::RemoteRespawnLab
{
namespace
{
using namespace CharacterCreation;
static_assert(offsetof(ActorState, flags1) == 0x8);
static_assert(offsetof(ActorState, flags2) == 0xC);
using Clock = std::chrono::steady_clock;
uint64_t s_session{1}, s_generation{};
struct NativePair
{
    MaterializationForms Forms{};
    uintptr_t ActorToken{}, BaseToken{};
    bool DeleteIssued{};
};
struct Job
{
    uint32_t Server{}, Player{};
    entt::entity Entity{entt::null};
    NotifyCharacterBuildState Build;
    std::optional<NotifyCharacterAppearanceUpdate> Final;
    RemoteMaterializationLifecycle Lifecycle;
    NativePair Old, Candidate;
    uintptr_t ExpectedBase{};
    FaceGenComponent Tints;
    Clock::time_point Started{}, RetirementStarted{};
    uint32_t Cell{}, Space{};
    glm::vec3 Position{}, Rotation{};
    ActorValues Values;
    bool Done{}, Restored{}, RetirementLogged{}, EarlyDiscovery{}, CandidateCleanupPending{};
};
std::map<uint32_t, std::unique_ptr<Job>> s_jobs;
thread_local Job* s_creating{};

void Log(const Job& aJob, const char* aPhase, const char* aDetail = "")
{
    spdlog::info(
        "[STRE][RemoteRespawnLAB] phase={} serverId={} entityVersioned={} session={} generation={} finalRevision={} oldActor={:X} oldBase={:X} newActor={:X} newBase={:X} "
        "detail={}",
        aPhase, aJob.Server, static_cast<uint32_t>(aJob.Entity), aJob.Lifecycle.Key().Session, aJob.Lifecycle.Key().Generation, aJob.Final ? aJob.Final->FinalBuildRevision : 0,
        aJob.Old.Forms.Actor, aJob.Old.Forms.Base, aJob.Candidate.Forms.Actor, aJob.Candidate.Forms.Base, aDetail);
}
bool Locked()
{
    const auto* gate = CampaignRuntimeGateService::TryGet();
    return gate && gate->IsLocked();
}
Actor* Resolve(const NativePair& aPair)
{
    auto* actor = Cast<Actor>(TESForm::GetById(aPair.Forms.Actor));
    auto* base = Cast<TESNPC>(TESForm::GetById(aPair.Forms.Base));
    return actor && base && reinterpret_cast<uintptr_t>(actor) == aPair.ActorToken && reinterpret_cast<uintptr_t>(base) == aPair.BaseToken && actor->baseForm == base ? actor
                                                                                                                                                                      : nullptr;
}
std::string ActorStateDiagnostic(uint32_t aFlags1, uint32_t aFlags2)
{
    const auto state = DecodeFinalRespawnActorState(aFlags1, aFlags2);
    return fmt::format(
        "stateDecode=observed stateVeto={} lifeState={} lifeName={} knockState={} attackState={} flyState={} weaponState={} recoilState={} staggered={} sprinting={} swimming={} "
        "lifeAllowed={} knockIdle={} attackIdle={} flyIdle={} weaponSheathed={} recoilIdle={} notStaggered={} notSprinting={} notSwimming={}",
        state.VetoReason(), state.Life, state.LifeName(), state.Knock, state.Attack, state.Fly, state.Weapon, state.Recoil, state.Staggered, state.Sprinting, state.Swimming,
        state.LifeAllowsReplacement(), state.KnockIdle(), state.AttackIdle(), state.FlyIdle(), state.WeaponSheathed(), state.RecoilIdle(), state.NotStaggered(), state.NotSprinting(),
        state.NotSwimming());
}
bool SafeActor(Actor* aActor, const Job& aJob, const char* aBoundary)
{
    if (!aActor || !aActor->GetExtension()->IsRemotePlayer() || aActor->IsDead() || aActor->IsDisabled() || aActor->IsInCombat() || aActor->IsMount())
        return false;
    const auto flags1 = aActor->actorState.flags1;
    const auto flags2 = aActor->actorState.flags2;
    if (FinalRespawnActorStateSafe(flags1, flags2))
        return true;
    spdlog::warn(
        "[STRE][RemoteRespawnLAB] phase=actor-state-rejected detail=unsafe-actor-state boundary={} serverId={} entityVersioned={} Actor={:X} ActorToken={:X} "
        "safeState=0 actorStateFlags1={:08X} actorStateFlags2={:08X} {}",
        aBoundary, aJob.Server, static_cast<uint32_t>(aJob.Entity), aActor->formID, reinterpret_cast<uintptr_t>(aActor), flags1, flags2, ActorStateDiagnostic(flags1, flags2));
    return false;
}
bool Bound(World& aWorld, const Job& aJob, const NativePair& aPair)
{
    if (!aWorld.valid(aJob.Entity) || aWorld.any_of<LocalComponent, WaitingForAssignmentComponent, WaitingFor3D>(aJob.Entity))
        return false;
    const auto* remote = aWorld.try_get<RemoteComponent>(aJob.Entity);
    const auto* player = aWorld.try_get<PlayerComponent>(aJob.Entity);
    const auto* form = aWorld.try_get<FormIdComponent>(aJob.Entity);
    return remote && player && form && remote->Id == aJob.Server && player->Id == aJob.Player && remote->CachedRefId == aPair.Forms.Actor && form->Id == aPair.Forms.Actor &&
           Resolve(aPair);
}
void Retire(Job& aJob, NativePair& aPair, bool aOld)
{
    if (!aPair.Forms.Actor || aPair.DeleteIssued)
        return;
    aPair.DeleteIssued = true; // Reserve before the existing native call, including reentry.
    if (Resolve(aPair) && (aOld || aPair.Forms.Actor != aJob.Old.Forms.Actor || aPair.ActorToken != aJob.Old.ActorToken))
    {
        Log(aJob, aOld ? "old-retirement-requested" : "candidate-retirement-requested");
        CharacterService::DeleteTempActor(aPair.Forms.Actor);
    }
    else
        Log(aJob, "retirement-binding-unavailable", "no-delete-on-stale-or-reused-identity");
    if (aOld)
        aJob.RetirementStarted = Clock::now();
}
void DispatchRetirement(Job& aJob)
{
    const auto intents = aJob.Lifecycle.TakeRetirementIntents(aJob.Lifecycle.Key());
    if (intents & 1)
        Retire(aJob, aJob.Old, true);
    if (intents & 2)
        Retire(aJob, aJob.Candidate, false);
    if (aJob.CandidateCleanupPending)
    {
        // Also covers a native candidate rejected before RecordCandidate could
        // accept its IDs. Retire refuses any alias of the still-bound old Actor.
        if (aJob.Candidate.Forms.Valid())
            Retire(aJob, aJob.Candidate, false);
        aJob.CandidateCleanupPending = false;
    }
}
void Abort(Job& aJob, const char* aReason)
{
    aJob.Lifecycle.Abort(aJob.Lifecycle.Key());
    aJob.Done = true;
    aJob.CandidateCleanupPending = true;
    Log(aJob, "abort", aReason);
    if (Locked() || !World::Get().GetTransport().IsConnected())
        return; // Including recovery entered reentrantly by a native placement call.
    DispatchRetirement(aJob);
}
// One read-only observation is both the gate input and its rejection evidence.
// -1 denotes a native value unavailable because no Actor was resolved.
struct BindingObservation
{
    entt::entity Entity{entt::null}, Collision{entt::null};
    bool Valid{}, Remote{}, Player{}, Form{}, Local{}, Assignment{}, Waiting3D{}, Interpolation{}, Animation{};
    bool RecoveryLocked{}, SnapshotPresent{}, Provenance{}, ProvenanceMatches{};
    int RuntimeState{-1};
    uint32_t Server{}, PlayerId{}, FormId{}, CachedRefId{}, ActorFormId{}, BaseFormId{}, Flags1{}, Flags2{};
    Actor* NativeActor{};
    TESForm* RawBase{};
    TESNPC* Base{};
    TESForm* BaseLookup{};
    TESObjectCELL* Cell{};
    uint32_t CurrentCell{};
    int RemotePlayer{-1}, Dead{-1}, Disabled{-1}, Combat{-1}, Mounted{-1}, SafeState{-1}, RootReady{-1}, ActorTemporary{-1}, BaseTemporary{-1};
};
BindingObservation ObserveBinding(World& aWorld, entt::entity aEntity, bool aReadNative = true)
{
    BindingObservation o;
    o.Entity = aEntity;
    o.Valid = aWorld.valid(aEntity);
    o.RecoveryLocked = Locked();
    const auto& snapshot = aWorld.GetCampaignService().GetLatestSnapshot();
    o.SnapshotPresent = snapshot.has_value();
    if (snapshot)
        o.RuntimeState = snapshot->RuntimeState;
    if (!o.Valid)
        return o;
    const auto* remote = aWorld.try_get<RemoteComponent>(aEntity);
    const auto* player = aWorld.try_get<PlayerComponent>(aEntity);
    const auto* form = aWorld.try_get<FormIdComponent>(aEntity);
    o.Remote = remote != nullptr;
    o.Player = player != nullptr;
    o.Form = form != nullptr;
    o.Server = remote ? remote->Id : 0;
    o.CachedRefId = remote ? remote->CachedRefId : 0;
    o.PlayerId = player ? player->Id : 0;
    o.FormId = form ? form->Id : 0;
    o.Local = aWorld.all_of<LocalComponent>(aEntity);
    o.Assignment = aWorld.all_of<WaitingForAssignmentComponent>(aEntity);
    o.Waiting3D = aWorld.all_of<WaitingFor3D>(aEntity);
    o.Interpolation = aWorld.all_of<InterpolationComponent>(aEntity);
    o.Animation = aWorld.all_of<RemoteAnimationComponent>(aEntity);
    if (!aReadNative)
        return o;
    o.NativeActor = form ? Cast<Actor>(TESForm::GetById(form->Id)) : nullptr;
    if (o.NativeActor)
    {
        auto* actor = o.NativeActor;
        o.ActorFormId = actor->formID;
        o.RawBase = actor->baseForm;
        o.BaseFormId = o.RawBase ? o.RawBase->formID : 0;
        o.Base = Cast<TESNPC>(o.RawBase);
        o.BaseLookup = o.RawBase ? TESForm::GetById(o.BaseFormId) : nullptr;
        o.Flags1 = actor->actorState.flags1;
        o.Flags2 = actor->actorState.flags2;
        o.SafeState = FinalRespawnActorStateSafe(o.Flags1, o.Flags2);
        o.RemotePlayer = actor->GetExtension()->IsRemotePlayer();
        o.Dead = actor->IsDead();
        o.Disabled = actor->IsDisabled();
        o.Combat = actor->IsInCombat();
        o.Mounted = actor->IsMount();
        o.ActorTemporary = actor->IsTemporary();
        o.BaseTemporary = o.Base ? int(o.Base->IsTemporary()) : -1;
        o.Cell = actor->GetParentCell(); // Preserve existing LAB guard; log current cell separately.
        o.CurrentCell = actor->parentCell ? actor->parentCell->formID : 0;
        o.RootReady = RemoteActorProjection::ReadyFor3D(actor);
    }
    const auto* provenance = aWorld.try_get<RemotePlayerAppearanceBaseComponent>(aEntity);
    o.Provenance = provenance != nullptr;
    o.ProvenanceMatches = provenance && o.NativeActor && o.Base && provenance->Matches(o.NativeActor->formID, o.Base->formID);
    return o;
}
void LogBindingRejection(const Job& aJob, const BindingObservation& o, const char* aPhase, const char* aReason)
{
    spdlog::warn(
        "[STRE][RemoteRespawnLAB] phase={} detail={} serverId={} entityVersioned={} entity={} entityVersion={} entityValid={} collisionEntityVersioned={} "
        "remotePresent={} remoteServerId={} playerPresent={} playerId={} expectedPlayerId={} formPresent={} FormId={:X} CachedRefId={:X} "
        "Actor={:X} Base={:X} ActorToken={:X} BaseToken={:X} baseLookupToken={:X} baseIsTESNPC={} WaitingFor3D={} Local={} Assignment={} "
        "recoveryLocked={} snapshotPresent={} recoveryState={} actorRemotePlayer={} actorDead={} actorDisabled={} combat={} mounted={} "
        "safeState={} actorStateFlags1={:08X} actorStateFlags2={:08X} actorTemporary={} baseTemporary={} rootReady={} "
        "cell={:X} currentCell={:X} interpolation={} animation={} provenancePresent={} provenanceMatches={} {}",
        aPhase, aReason, aJob.Server, static_cast<uint32_t>(o.Entity), entt::to_entity(o.Entity), entt::to_version(o.Entity), o.Valid, static_cast<uint32_t>(o.Collision),
        o.Remote, o.Server, o.Player, o.PlayerId, aJob.Build.PlayerId, o.Form, o.FormId, o.CachedRefId,
        o.ActorFormId, o.BaseFormId, reinterpret_cast<uintptr_t>(o.NativeActor), reinterpret_cast<uintptr_t>(o.RawBase),
        reinterpret_cast<uintptr_t>(o.BaseLookup), o.Base != nullptr, o.Waiting3D, o.Local, o.Assignment, o.RecoveryLocked, o.SnapshotPresent, o.RuntimeState,
        o.RemotePlayer, o.Dead, o.Disabled, o.Combat, o.Mounted, o.SafeState, o.Flags1, o.Flags2, o.ActorTemporary, o.BaseTemporary, o.RootReady,
        o.Cell ? o.Cell->formID : 0, o.CurrentCell, o.Interpolation, o.Animation, o.Provenance, o.ProvenanceMatches,
        o.NativeActor ? ActorStateDiagnostic(o.Flags1, o.Flags2) : "stateDecode=unavailable");
}
entt::entity FindDiagnosticEntity(World& aWorld, const Job& aJob)
{
    if (aWorld.valid(aJob.Entity))
        return aJob.Entity;
    for (auto entity : aWorld.view<RemoteComponent>())
        if (aWorld.get<RemoteComponent>(entity).Id == aJob.Server)
            return entity;
    // Diagnostic fallback only: never admit a player lacking RemoteComponent.
    for (auto entity : aWorld.view<PlayerComponent>())
        if (aWorld.get<PlayerComponent>(entity).Id == aJob.Build.PlayerId)
            return entity;
    return entt::null;
}

bool Capture(World& aWorld, Job& aJob)
{
    if (VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0")
    {
        LogBindingRejection(aJob, ObserveBinding(aWorld, FindDiagnosticEntity(aWorld, aJob), false), "gate-rejected", "unsupported-runtime");
        return false;
    }
    entt::entity found = entt::null;
    for (auto entity : aWorld.view<RemoteComponent, PlayerComponent>())
        if (aWorld.get<RemoteComponent>(entity).Id == aJob.Server)
        {
            if (found != entt::null)
            {
                auto observed = ObserveBinding(aWorld, found);
                observed.Collision = entity;
                LogBindingRejection(aJob, observed, "gate-rejected", "remote-serverid-collision");
                return false;
            }
            found = entity;
        }
    if (found == entt::null)
    {
        const auto observed = ObserveBinding(aWorld, FindDiagnosticEntity(aWorld, aJob));
        LogBindingRejection(aJob, observed, "gate-rejected", observed.Remote ? "player-component-missing" : "remote-component-missing");
        return false;
    }
    aJob.Entity = found;
    auto observed = ObserveBinding(aWorld, found);
    const auto reject = [&](const char* reason)
    {
        LogBindingRejection(aJob, observed, "gate-rejected", reason);
        return false;
    };
    if (!observed.Form)
        return reject("formid-missing");
    if (!observed.Player)
        return reject("player-component-missing");
    if (observed.PlayerId != aJob.Build.PlayerId)
        return reject("player-id-mismatch");
    aJob.Player = observed.PlayerId;
    auto* actor = observed.NativeActor;
    auto* base = observed.Base;
    if (!actor)
        return reject("actor-lookup-failed");
    if (!observed.RemotePlayer)
        return reject("actor-not-remote-player");
    if (observed.Dead)
        return reject("actor-dead");
    if (observed.Disabled)
        return reject("actor-disabled");
    if (observed.Combat)
        return reject("combat");
    if (observed.Mounted)
        return reject("mounted");
    if (!observed.SafeState)
        return reject("unsafe-actor-state");
    if (!base)
        return reject("base-not-tesnpc");
    if (!observed.ActorTemporary)
        return reject("actor-not-temporary");
    if (!observed.BaseTemporary)
        return reject("base-not-temporary");
    if (actor->formID == base->formID)
        return reject("actor-base-formid-collision");
    if (observed.BaseLookup != base)
        return reject("base-lookup-mismatch");
    aJob.Old = {{actor->formID, base->formID}, reinterpret_cast<uintptr_t>(actor), reinterpret_cast<uintptr_t>(base)};
    // Exactly the existing Bound guard, split into independently named predicates.
    if (!observed.Valid)
        return reject("entity-invalid");
    if (observed.Local)
        return reject("local-component-present");
    if (observed.Assignment)
        return reject("assignment-pending");
    if (observed.Waiting3D)
        return reject("waiting-for-3d");
    if (!observed.Remote)
        return reject("remote-component-missing");
    if (observed.Server != aJob.Server)
        return reject("remote-serverid-mismatch");
    if (observed.CachedRefId != aJob.Old.Forms.Actor)
        return reject("cachedref-mismatch");
    if (observed.FormId != aJob.Old.Forms.Actor)
        return reject("formid-actor-mismatch");
    if (!Resolve(aJob.Old))
        return reject("native-binding-token-mismatch");
    if (observed.Provenance && !observed.ProvenanceMatches)
        return reject("provenance-mismatch");
    for (auto other : aWorld.view<FormIdComponent>())
        if (other != found)
        {
            auto* otherActor = Cast<Actor>(TESForm::GetById(aWorld.get<FormIdComponent>(other).Id));
            observed.Collision = other;
            if (otherActor == actor)
                return reject("entity-alias-collision");
            if (otherActor && otherActor->baseForm == base)
                return reject("base-alias-collision");
        }
    observed.Collision = entt::null;
    auto* cell = observed.Cell;
    if (!cell)
        return reject("cell-missing");
    if (!observed.RootReady)
        return reject("root-not-ready");
    if (!observed.Interpolation)
        return reject("interpolation-missing");
    if (!observed.Animation)
        return reject("remote-animation-missing");
    aJob.Cell = cell->formID;
    auto* space = actor->GetWorldSpace();
    aJob.Space = space ? space->formID : 0;
    aJob.Position = actor->position;
    aJob.Rotation = actor->rotation;
    aJob.Values = actor->GetEssentialActorValues(); // Same capture as natural spawn ActorData.
    spdlog::info(
        "[STRE][RemoteRespawnLAB] phase=actor-state-accepted serverId={} entityVersioned={} Actor={:X} safeState={} actorStateFlags1={:08X} actorStateFlags2={:08X} {}",
        aJob.Server, static_cast<uint32_t>(aJob.Entity), observed.ActorFormId, observed.SafeState, observed.Flags1, observed.Flags2,
        ActorStateDiagnostic(observed.Flags1, observed.Flags2));
    return true;
}

bool Geometry(Job& aJob, Actor* aCandidate)
{
    auto* base = Cast<TESNPC>(aCandidate->baseForm);
    auto* target = Cast<TESRace>(TESForm::GetById(World::Get().GetModSystem().GetGameId(aJob.Final->Descriptor.Race)));
    if (!base || !target || aCandidate->race != target || base->raceForm.race != target || base->actorData.IsFemale() != (aJob.Final->Descriptor.Sex != 0) ||
        (base->headpartsCount && !base->headparts))
        return false;
    for (uint32_t i = 0; i < base->headpartsCount; ++i)
        if (!base->headparts[i] || TESForm::GetById(base->headparts[i]->formID) != base->headparts[i])
            return false;
    return RemoteActorProjection::ReadyFor3D(aCandidate);
}
void Advance(World& aWorld, Job& aJob)
{
    if (aJob.Lifecycle.State() == MaterializationState::Idle)
    {
        if (!Capture(aWorld, aJob))
        {
            aJob.Done = true;
            return;
        }
        auto* cell = Cast<TESObjectCELL>(TESForm::GetById(aJob.Cell));
        auto* space = aJob.Space ? Cast<TESWorldSpace>(TESForm::GetById(aJob.Space)) : nullptr;
        auto* target = Cast<TESRace>(TESForm::GetById(aWorld.GetModSystem().GetGameId(aJob.Final->Descriptor.Race)));
        if (!cell || (aJob.Space && !space) || !target)
        {
            aJob.Done = true;
            Log(aJob, "gate-rejected", "unresolvable-final-or-placement");
            return;
        }
        const ActorSpawnLocation location{cell, space, aJob.Position, aJob.Rotation};
        const MaterializationKey key{s_session, static_cast<uint32_t>(aJob.Entity), aJob.Server, ++s_generation};
        if (!aJob.Lifecycle.Reserve(key, aJob.Old.Forms))
            return Abort(aJob, "reservation-failed");
        Log(aJob, "gate-accepted");
        Log(aJob, "transaction-reserved");
        Log(aJob, "old-binding-captured", "fresh-current-binding-independent-of-appearance-provenance");
        aJob.Started = Clock::now();
        s_creating = &aJob;
        Log(aJob, "candidate-create-enter");
        Actor* candidate = Materialize(aWorld, aJob.Entity, *aJob.Final, aJob.Tints, location);
        s_creating = nullptr;
        Log(aJob, "candidate-create-return");
        // The materializer releases its temporary reference. Never dereference its borrowed result.
        if (!aJob.Candidate.Forms.Valid() || reinterpret_cast<uintptr_t>(candidate) != aJob.Candidate.ActorToken || !Resolve(aJob.Candidate) ||
            aJob.Lifecycle.State() == MaterializationState::Aborted || aJob.Lifecycle.State() == MaterializationState::Invalidated)
            return Abort(aJob, "candidate-identity-unavailable");
        candidate = Resolve(aJob.Candidate);
        if (TESForm::GetById(aJob.Cell) != cell || (aJob.Space && TESForm::GetById(aJob.Space) != space))
            return Abort(aJob, "cell-unavailable");
        RemoteActorProjection::Initialize(candidate, true, location, aJob.Values);
        return; // Always wait for a later source-observed Discovery add and 3D.
    }
    if (!Bound(aWorld, aJob, aJob.Old) || !SafeActor(Resolve(aJob.Old), aJob, "old-binding"))
        return Abort(aJob, "old-binding-lost-or-unsafe");
    if (Clock::now() - aJob.Started > std::chrono::seconds(10))
        return Abort(aJob, "candidate-readiness-timeout");
    auto* candidate = Resolve(aJob.Candidate);
    if (!candidate || aJob.Lifecycle.State() == MaterializationState::Aborted)
        return Abort(aJob, "candidate-lost-before-commit");
    if (aJob.Lifecycle.State() != MaterializationState::CandidateDiscovered || !Geometry(aJob, candidate))
        return;
    if (!aJob.Restored)
    {
        const auto* interpolation = aWorld.try_get<InterpolationComponent>(aJob.Entity);
        auto* source = Resolve(aJob.Old);
        if (!interpolation || !source)
            return Abort(aJob, "projection-state-lost");
        const auto* variables = interpolation->TimePoints.empty() ? nullptr : &interpolation->TimePoints.back().Variables;
        RemoteActorProjection::Complete3D(candidate, aJob.Build.Build.CanonicalInventory, source->GetFactions(), variables);
        aJob.Restored = true;
        Log(aJob, "inventory-restore-status", "canonical-applied-awaiting-live-counts-and-equipment");
        return; // Equipment may rebuild geometry; validate again on a later update.
    }
    if (!InventoryService::MatchesCanonicalInventory(candidate, aJob.Build.Build.CanonicalInventory))
        return; // Deferred native equip can settle, bounded by the same staging deadline.
    if (!aJob.Tints.FaceTints.Entries.empty())
    {
        FaceGenSystem::Update(aWorld, candidate, aJob.Tints);
        if (!aJob.Tints.Generated)
            return;
    }
    candidate = Resolve(aJob.Candidate);
    auto* old = Resolve(aJob.Old);
    if (Locked())
    {
        aJob.Lifecycle.Abort(aJob.Lifecycle.Key());
        aJob.Done = true;
        Log(aJob, "abort", "recovery-before-commit-cleanup-deferred");
        return;
    }
    if (!old || !candidate || !Geometry(aJob, candidate) || !SafeActor(candidate, aJob, "precommit") || !Bound(aWorld, aJob, aJob.Old) || !old->GetParentCell() ||
        old->GetParentCell()->formID != aJob.Cell || (old->GetWorldSpace() ? old->GetWorldSpace()->formID : 0) != aJob.Space)
        return Abort(aJob, "precommit-invariant");
    candidate->SetActorValues(old->GetEssentialActorValues());
    RemoteActorProjection::Place(candidate, {old->GetParentCell(), old->GetWorldSpace(), old->position, old->rotation});
    if (Resolve(aJob.Candidate) != candidate || !Geometry(aJob, candidate) || !SafeActor(candidate, aJob, "post-placement") || !candidate->GetParentCell() ||
        candidate->GetParentCell()->formID != aJob.Cell || (candidate->GetWorldSpace() ? candidate->GetWorldSpace()->formID : 0) != aJob.Space || !Bound(aWorld, aJob, aJob.Old) ||
        Locked() || !aWorld.GetTransport().IsConnected() || aJob.Lifecycle.Key().Session != s_session)
        return Abort(aJob, "post-placement-invariant");
    if (!aJob.Lifecycle.MarkReady(aJob.Lifecycle.Key()))
        return Abort(aJob, "candidate-not-discovered");
    Log(aJob, "candidate-ready");
    if (!aJob.Lifecycle.Commit(aJob.Lifecycle.Key()))
        return Abort(aJob, "commit-fence");
    // No engine calls between the fence and publication; old tint state is
    // untouched on every precommit failure. Retirement is dispatched last.
    auto& tints = aWorld.get_or_emplace<FaceGenComponent>(aJob.Entity);
    tints = std::move(aJob.Tints);
    aWorld.emplace_or_replace<RemotePlayerAppearanceBaseComponent>(aJob.Entity, aJob.Candidate.Forms.Actor, aJob.Candidate.Forms.Base);
    aWorld.get<FormIdComponent>(aJob.Entity).Id = aJob.Candidate.Forms.Actor;
    aWorld.get<RemoteComponent>(aJob.Entity).CachedRefId = aJob.Candidate.Forms.Actor;
    aWorld.remove<RemoteAppearanceProbeComponent>(aJob.Entity);
    aJob.Done = true;
    Log(aJob, "candidate-commit");
    Log(aJob, "inventory-restore-status", "live-counts-worn-sides-and-magic-equipment-matched");
    DispatchRetirement(aJob);
    Log(aJob, "complete", "natural-join-local-representation-committed-native-retirement-observation-pending");
}
} // namespace

Actor* CommittedActor(World& aWorld, uint32_t aServerId, uint64_t aRevision) noexcept
{
    const auto found = s_jobs.find(aServerId);
    if (found == s_jobs.end())
        return nullptr;
    const auto& job = *found->second;
    if (!job.Final || job.Final->FinalBuildRevision != aRevision ||
        job.Lifecycle.State() != MaterializationState::Committed || !Bound(aWorld, job, job.Candidate))
        return nullptr;
    return Resolve(job.Candidate);
}

void ReceiveBuild(World&, const NotifyCharacterBuildState& aBuild) noexcept
{
    if (aBuild.State != CharacterBuildNetworkState::Applied || !aBuild.Revision ||
        ComputeCharacterBuildInventoryHash(aBuild.Build.CanonicalInventory) != aBuild.Build.InventoryHash)
        return;
    if (!s_jobs.contains(aBuild.ServerId) && s_jobs.size() >= 10)
        return;
    auto& job = s_jobs[aBuild.ServerId];
    if (!job)
        job = std::make_unique<Job>();
    if (job->Lifecycle.State() != MaterializationState::Idle || job->Done)
        return;
    job->Server = aBuild.ServerId;
    job->Build = aBuild;
}
void ReceiveFinal(World& aWorld, const NotifyCharacterAppearanceUpdate& aFinal) noexcept
{
    Job rejected;
    rejected.Server = aFinal.ActorId;
    rejected.Final = aFinal;
    const char* reason = nullptr;
    if (!aWorld.GetTransport().IsConnected())
        reason = "transport-disconnected";
    else if (!aFinal.FinalBuildRevision)
        reason = "missing-final-build-revision";
    else if (Locked())
        reason = "recovery-locked";
    else if (!aFinal.IsValid())
        reason = "invalid-canonical-snapshot";
    else if (!s_jobs.contains(aFinal.ActorId) && s_jobs.size() >= 10)
        reason = "transaction-capacity";
    if (reason)
    {
        Log(rejected, "gate-rejected", reason);
        return;
    }
    auto& ptr = s_jobs[aFinal.ActorId];
    if (!ptr)
        ptr = std::make_unique<Job>();
    auto& job = *ptr;
    job.Server = aFinal.ActorId;
    Log(rejected, "final-snapshot-received");
    if (job.Done || job.Lifecycle.State() != MaterializationState::Idle)
    {
        Log(job, "duplicate-final-ignored", "no-second-candidate");
        return;
    }
    if (!job.Final)
        job.Started = Clock::now(); // Duplicate delivery cannot extend the pending deadline.
    job.Final = aFinal;             // Latest valid delivery before reservation; one immutable canonical final.
}
void BeforeSpawn(Actor* aActor, TESNPC* aBase) noexcept
{
    if (!s_creating || s_creating->ExpectedBase != reinterpret_cast<uintptr_t>(aBase) || s_creating->Candidate.ActorToken)
        return;
    auto& job = *s_creating;
    // Spawn may assign or replace the provisional Actor FormID. Until its return,
    // route source observations by the allocation token, not that provisional ID.
    job.Candidate = {{0, aBase->formID}, reinterpret_cast<uintptr_t>(aActor), reinterpret_cast<uintptr_t>(aBase)};
    aActor->GetExtension()->SetPlayer(true);
    // Register allocation tokens BEFORE world exposure, even if Spawn assigns IDs.
    // The creator still owns its GamePtr during this source boundary.
}
void CandidateBase(TESNPC* aBase) noexcept
{
    if (s_creating)
        s_creating->ExpectedBase = reinterpret_cast<uintptr_t>(aBase);
}
void AfterSpawn(Actor* aActor, TESNPC* aBase) noexcept
{
    if (!s_creating || s_creating->ExpectedBase != reinterpret_cast<uintptr_t>(aBase) || s_creating->Candidate.ActorToken != reinterpret_cast<uintptr_t>(aActor))
        return;
    auto& job = *s_creating;
    job.Candidate.Forms = {aActor->formID, aBase->formID};
    if (!job.Lifecycle.RecordCandidate(job.Lifecycle.Key(), job.Candidate.Forms))
    {
        job.Lifecycle.Abort(job.Lifecycle.Key());
        Log(job, "abort", "candidate-identity-rejected-cleanup-after-create-return");
        return;
    }
    if (job.EarlyDiscovery)
        job.Lifecycle.Observe(job.Lifecycle.Key(), aActor->formID, true);
}
void DeleteRequested(const Actor* aActor) noexcept
{
    for (auto& [id, ptr] : s_jobs)
        for (auto* pair : {&ptr->Old, &ptr->Candidate})
            if (pair->ActorToken == reinterpret_cast<uintptr_t>(aActor) && (pair->Forms.Actor == aActor->formID || (s_creating == ptr.get() && pair == &ptr->Candidate)))
                pair->DeleteIssued = true;
}
bool Discovery(uint32_t aFormId, uintptr_t aToken, bool aAdded) noexcept
{
    for (auto& [id, ptr] : s_jobs)
    {
        auto& job = *ptr;
        if (job.Lifecycle.State() == MaterializationState::Idle)
            continue;
        const bool old = job.Old.Forms.Actor == aFormId && job.Old.ActorToken == aToken;
        const bool candidate = job.Candidate.ActorToken && job.Candidate.ActorToken == aToken && (!job.Candidate.Forms.Actor || job.Candidate.Forms.Actor == aFormId);
        if (!old && !candidate)
            continue;
        if (candidate && job.Lifecycle.State() == MaterializationState::CandidateReserved)
        {
            job.EarlyDiscovery = aAdded;
            if (!aAdded)
                job.Lifecycle.Abort(job.Lifecycle.Key());
            return true;
        }
        const auto route = job.Lifecycle.Observe(job.Lifecycle.Key(), aFormId, aAdded);
        if (route == DiscoveryRoute::OldRetirementObserved)
            Log(job, "old-retirement-observed", "discovery-absence-only");
        if (route == DiscoveryRoute::OldLostBeforeCommit)
            return false; // Genuine live unload keeps its normal teardown.
        if (old && route == DiscoveryRoute::LiveBinding)
            return false;
        if (route == DiscoveryRoute::LiveBindingLost)
            return false;
        return true; // Candidate staging/retirement must not reach ANY legacy subscriber.
    }
    return false;
}
bool Tracks(uint32_t aFormId, uintptr_t aToken) noexcept
{
    for (const auto& [id, ptr] : s_jobs)
    {
        if (ptr->Lifecycle.State() == MaterializationState::Idle)
            continue;
        for (const auto* pair : {&ptr->Old, &ptr->Candidate})
            if (pair->Forms.Actor == aFormId && pair->ActorToken == aToken)
                return true;
    }
    return false;
}
void Tick(World& aWorld) noexcept
{
    for (auto& [id, ptr] : s_jobs)
    {
        auto& job = *ptr;
        if (!job.Final)
            continue;
        if (!aWorld.GetTransport().IsConnected() || Locked())
        {
            if (!job.Done)
            {
                // A recovery lock is not authoritative removal of the old actor.
                job.Lifecycle.Abort(job.Lifecycle.Key());
                job.Done = true;
                LogBindingRejection(job, ObserveBinding(aWorld, FindDiagnosticEntity(aWorld, job), VersionDb::Get().GetLoadedVersionString() == "1.6.1170.0"),
                    "abort", !aWorld.GetTransport().IsConnected() ? "transport-disconnected" : "recovery-locked");
                // Native load/recovery owns its teardown. Defer our intents until
                // a later safe update; ordinary Delete calls still mark them issued.
            }
            continue;
        }
        if (!job.Done)
        {
            if (!FinalRespawnEligible(true, true, job.Build.State == CharacterBuildNetworkState::Applied, job.Final->FinalBuildRevision, job.Build.Revision))
            {
                if (Clock::now() - job.Started > std::chrono::seconds(10))
                    Abort(job, "matching-applied-build-timeout");
            }
            else
                Advance(aWorld, job);
        }
        DispatchRetirement(job);
#if (!IS_MASTER)
        if (job.Old.DeleteIssued && !job.RetirementLogged && job.RetirementStarted != Clock::time_point{} && Clock::now() - job.RetirementStarted >= std::chrono::seconds(30))
        {
            job.RetirementLogged = true;
            const bool actorPresent = TESForm::GetById(job.Old.Forms.Actor) != nullptr;
            const bool basePresent = TESForm::GetById(job.Old.Forms.Base) != nullptr;
            spdlog::info(
                "[STRE][RemoteRespawnLAB] phase=retirement-window serverId={} entityVersioned={} session={} generation={} oldActor={:X} oldBase={:X} newActor={:X} newBase={:X} "
                "actorLookupPresent={} baseLookupPresent={} conclusion=NOT-LEAK-OR-COMPLETION-PROOF",
                job.Server, static_cast<uint32_t>(job.Entity), job.Lifecycle.Key().Session, job.Lifecycle.Key().Generation, job.Old.Forms.Actor, job.Old.Forms.Base,
                job.Candidate.Forms.Actor, job.Candidate.Forms.Base, actorPresent, basePresent);
        }
#endif
    }
}
void Disconnect(World& aWorld) noexcept
{
    ++s_session;
    for (auto& [id, ptr] : s_jobs)
    {
        ptr->Lifecycle.Invalidate(ptr->Lifecycle.Key());
        ptr->Done = true;
        DispatchRetirement(*ptr);
    }
    // Tombstones survive disconnect: delayed Discovery must not become assignments.
    // A fresh process is still required for another initial-creation session.
}
} // namespace STRE::RemoteRespawnLab
