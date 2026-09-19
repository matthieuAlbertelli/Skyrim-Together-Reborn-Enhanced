#include <TiltedOnlinePCH.h>
#include <Services/CreationSeating.h>
#include <Services/RemoteSeatingProbe.h>
#include <CharacterCreation/CreationSeating.h>
#include <CharacterCreation/SeatingEntryEvidence.h>
#include <CharacterCreation/SeatingObservationWindow.h>
#include <CharacterCreation/SeatApproach.h>
#include <CharacterCreation/FinalRespawn.h>
#include <CharacterCreation/StandingCreation.h>
#include <Services/RemoteRespawnLab.h>
#include <Services/CampaignService.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Services/TransportService.h>
#include <Services/PapyrusService.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <Components.h>
#include <World.h>
#include <Actor.h>
#include <PlayerCharacter.h>
#include <AI/AIProcess.h>
#include <Misc/MiddleProcess.h>
#include <Forms/TESObjectCELL.h>
#include <Games/ActorExtension.h>
#include <Games/TES.h>
#include <VersionDb.h>
#include <Events/EventDispatcher.h>
#include <Games/Animation/TESActionData.h>
#include <Forms/BGSAction.h>
#include <Forms/TESIdleForm.h>
#include <Forms/TESPackage.h>
#include <AI/Movement/PlayerControls.h>
#include <map>
#include <string>
#include <chrono>
#include <atomic>
#include <cmath>
#include <memory>

namespace STRE::CreationSeating
{
namespace
{
using namespace CharacterCreation;
struct Intent
{
    uint32_t Server{}, Player{};
    uint64_t Revision{};
    size_t Index{};
    uint32_t SeatForm{}, ApproachForm{};
    std::string Campaign, DurablePlayer;
    bool Local{}, Solo{}, SessionConnected{};
    SeatProjection Projection;
    std::string LastReason;
    std::chrono::steady_clock::time_point IssuedAt{};
    int64_t LastSampleMs{}, LastClosedDeadline{};
    bool SoloProbeStarted{};
#ifndef MASTER
    LocalSeatApproach Approach;
    uintptr_t ApproachActor{}, ApproachSeat{}, ApproachCell{}, ApproachMarker{};
    glm::vec3 ApproachSeatPosition{}, ApproachSeatRotation{};
    int64_t LastApproachSample{};
#endif
};
std::map<uint32_t, Intent> s_intents;
uint64_t s_engineUpdate{};

// Native events/actions can arrive outside the VM update thread. Never read the
// intention map there and never retain borrowed engine pointers. The published
// tokens only filter observations; they are not dereferenced later.
struct SoloObservation
{
    std::atomic<uintptr_t> ActorToken{}, SeatToken{};
    std::atomic<bool> Entered{};
    std::atomic<unsigned> EventLogs{}, ActionsDropped{};
    std::atomic<int64_t> StartedAt{}, FirstEnterAt{};
    std::atomic<const void*> SitStateFunction{};
    SeatingActionTraceBudget ActionBudget;
} s_solo;
// Immutable diagnostic context shared with native callbacks. Those callbacks
// must never read Intent/string fields concurrently with the update thread.
std::atomic<std::shared_ptr<const std::string>> s_observationContext;

std::string ObservationContext()
{
    const auto context = s_observationContext.load();
    return context ? *context : "observationContext=unavailable";
}

std::string IntentContext(const Intent& aIntent)
{
    return fmt::format("sessionConnected={} localPlayer={} durablePlayerId={} creationPositionIndex={} assignedSeat={:X} assignedSeatName=Seat{:02} assignedApproach={:X} assignedApproachName=Marker{:02} serverId={} revision={}",
                       aIntent.SessionConnected, aIntent.Local, aIntent.DurablePlayer,
                       aIntent.Index, aIntent.SeatForm, aIntent.Index + 1, aIntent.ApproachForm, aIntent.Index + 1, aIntent.Server, aIntent.Revision);
}

int64_t NowMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

void BeginSoloObservation(const Intent& aIntent, Actor* apActor, TESObjectREFR* apSeat, const void* apSitState)
{
    s_solo.ActorToken = 0;
    s_observationContext.store(std::make_shared<const std::string>(IntentContext(aIntent)));
    s_solo.SeatToken = reinterpret_cast<uintptr_t>(apSeat);
    s_solo.Entered = false;
    s_solo.EventLogs = s_solo.ActionsDropped = 0;
    s_solo.FirstEnterAt = 0;
    s_solo.StartedAt = NowMilliseconds();
    s_solo.SitStateFunction = apSitState;
    s_solo.ActionBudget.Reset();
    s_solo.ActorToken = reinterpret_cast<uintptr_t>(apActor);
}

int GraphBool(Actor* apActor, const char* apName)
{
    BSFixedString name(apName);
    bool value{};
    return apActor->animationGraphHolder.GetVariableBool(&name, &value) ? int(value) : -1;
}

struct SoloSample
{
    SeatingEntryEvidence Evidence;
    uintptr_t Root{};
    int InFurniture{-1}, IdleSitting{-1}, AnimationDriven{-1}, MotionDriven{-1}, Synced{-1};
};

SoloSample ReadSoloSample(Actor* apActor, bool aCorrect, bool aSeated)
{
    SoloSample sample;
    sample.Root = reinterpret_cast<uintptr_t>(apActor->GetNiNode());
    auto& evidence = sample.Evidence;
    evidence.CorrectFurniture = aCorrect;
    evidence.LogicalSitState = aSeated;
    evidence.RootReady = sample.Root != 0;
    evidence.GraphReady = apActor->animationGraphHolder.IsReady();
    evidence.EnterEvent = s_solo.ActorToken == reinterpret_cast<uintptr_t>(apActor) && s_solo.Entered;
    if (evidence.GraphReady)
    {
        // Names from the existing Master_Behavior descriptor; failed reads stay
        // unknown (-1), never silently become a positive animation observation.
        sample.InFurniture = GraphBool(apActor, "isInFurniture");
        sample.IdleSitting = GraphBool(apActor, "isIdleSitting");
        sample.AnimationDriven = GraphBool(apActor, "bAnimationDriven");
        sample.MotionDriven = GraphBool(apActor, "bMotionDriven");
        sample.Synced = GraphBool(apActor, "bIsSynced");
    }
    evidence.FurnitureVariableValid = sample.InFurniture >= 0;
    evidence.InFurniture = sample.InFurniture == 1;
    evidence.SittingVariableValid = sample.IdleSitting >= 0;
    evidence.IdleSitting = sample.IdleSitting == 1;
    return sample;
}

void LogSoloSample(const char* apPhase, Actor* apActor, TESObjectREFR* apSeat, TESObjectREFR* apOccupied,
                   int32_t aSitState, const SoloSample& aSample)
{
    const auto now = NowMilliseconds();
    const auto started = s_solo.StartedAt.load();
    const auto entered = s_solo.FirstEnterAt.load();
    const auto delta = apActor->position - apSeat->position;
    const auto distance = std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z);
    spdlog::info("[STRE][CreationSeating][LocalProbe] phase={} actor={:X} actorToken={:X} seat={:X} seatToken={:X} "
                 "elapsedMs={} sinceEnterMs={} distance={} yawDelta={} "
                 "occupied={:X} sitState={} logicalSeated={} enterEvent={} animationObserved={} visualConfirmation=human-required "
                 "root={:X} graphReady={} isInFurniture={} isIdleSitting={} bAnimationDriven={} bMotionDriven={} bIsSynced={} "
                 "actorPosition=({},{},{}) seatPosition=({},{},{}) actorRotation=({},{},{}) seatRotation=({},{},{}) "
                 "actorCell={:X} seatCell={:X} actorState1={:08X} actorState2={:08X} actorRemote={} {}",
                 apPhase, apActor->formID, reinterpret_cast<uintptr_t>(apActor), apSeat->formID, reinterpret_cast<uintptr_t>(apSeat),
                 now - started, entered ? now - entered : -1, distance, apActor->rotation.z - apSeat->rotation.z,
                 apOccupied ? apOccupied->formID : 0, aSitState, aSample.Evidence.LogicalSeated(), aSample.Evidence.EnterEvent,
                 aSample.Evidence.AnimationObserved(), aSample.Root, aSample.Evidence.GraphReady, aSample.InFurniture,
                 aSample.IdleSitting, aSample.AnimationDriven, aSample.MotionDriven, aSample.Synced,
                 apActor->position.x, apActor->position.y, apActor->position.z, apSeat->position.x, apSeat->position.y, apSeat->position.z,
                 apActor->rotation.x, apActor->rotation.y, apActor->rotation.z, apSeat->rotation.x, apSeat->rotation.y, apSeat->rotation.z,
                 apActor->parentCell ? apActor->parentCell->formID : 0, apSeat->parentCell ? apSeat->parentCell->formID : 0,
                 apActor->actorState.flags1, apActor->actorState.flags2, apActor->GetExtension()->IsRemote(), ObservationContext());

    auto* process = apActor->currentProcess;
    auto* middle = process ? process->middleProcess : nullptr;
    auto* currentPackage = process ? process->package : nullptr;
    auto* runOnce = middle ? middle->runOncePackage : nullptr;
    // CommonLibSSE-NG AIProcess::GetRunningPackage chooses run-once first.
    auto* effective = runOnce ? runOnce : currentPackage;
    const auto targetHandle = runOnce ? middle->runOnceTargetHandle : (process ? process->packageTargetHandle : 0);
    auto* target = targetHandle ? TESObjectREFR::GetByHandle(targetHandle) : nullptr;
    const auto pathPoint = middle ? middle->furniturePathPoint : NiPoint3{};
    const auto pathDelta = apActor->position - pathPoint;
    auto* controls = PlayerControls::GetInstance();
    spdlog::info("[STRE][CreationSeating][LocalProbe] phase=process-sample context={} elapsedMs={} sinceEnterMs={} "
                 "process={:X} middle={:X} processLevelRaw={} currentPackage={:X} currentPackageToken={:X} "
                 "runOncePackage={:X} runOnceToken={:X} effectiveSource={} effectivePackage={:X} effectiveProcedureTypeRaw={} "
                 "procedureIndex={} packageDataToken={:X} targetHandle={:X} targetForm={:X} targetIsSeat={} packageStartTime={} "
                 "furniturePathPointReadable={} furniturePathPoint=({},{},{}) distanceToRawPathPoint={} pathPointSpace=unverified furnitureIdle={:X} "
                 "controlsAvailable={} moveInput=({},{}) lookInput=({},{}) autoMove={} blockInput={} movementHandlerEnabled={} "
                 "actionsDropped={} pathSolverStatus=unavailable inputProvenance=not-proven {}",
                 apPhase, now - started, entered ? now - entered : -1,
                 reinterpret_cast<uintptr_t>(process), reinterpret_cast<uintptr_t>(middle), process ? int(process->movementType) : -1,
                 currentPackage ? currentPackage->formID : 0, reinterpret_cast<uintptr_t>(currentPackage),
                 runOnce ? runOnce->formID : 0, reinterpret_cast<uintptr_t>(runOnce), runOnce ? "run-once" : (currentPackage ? "current" : "none"),
                 effective ? effective->formID : 0, effective ? effective->ePROCEDURE_TYPE : -1,
                 runOnce ? middle->runOnceProcedureIndex : (process ? process->packageProcedureIndex : -1),
                 reinterpret_cast<uintptr_t>(runOnce ? middle->runOncePackageData : (process ? process->packageData : nullptr)),
                 targetHandle, target ? target->formID : 0, target == apSeat,
                 runOnce ? middle->runOnceStartTime : (process ? process->packageStartTime : -1.0f),
                 middle != nullptr, pathPoint.x, pathPoint.y, pathPoint.z,
                 middle ? std::sqrt(pathDelta.x * pathDelta.x + pathDelta.y * pathDelta.y + pathDelta.z * pathDelta.z) : -1.0f,
                 middle && middle->furnitureIdle ? middle->furnitureIdle->formID : 0,
                 controls != nullptr, controls ? controls->Data.MoveInputVec.x : 0.0f, controls ? controls->Data.MoveInputVec.y : 0.0f,
                 controls ? controls->Data.LookInputVec.x : 0.0f, controls ? controls->Data.LookInputVec.y : 0.0f,
                 controls ? int(controls->Data.bAutoMove) : -1, controls ? int(controls->bBlockPlayerInput) : -1,
                 controls && controls->pMovementHandler ? int(controls->pMovementHandler->isEnabled) : -1, s_solo.ActionsDropped.load(), ObservationContext());
}

void CaptureSoloSample(const char* apPhase, Actor* apActor, TESObjectREFR* apSeat)
{
    auto* process = apActor->currentProcess;
    auto* occupied = process && process->middleProcess ?
                         TESObjectREFR::GetByHandle(process->middleProcess->occupiedFurniture.handle.iBits) : nullptr;
    const auto* sitState = s_solo.SitStateFunction.load();
    const auto state = sitState && GameVM::Get() && GameVM::Get()->virtualMachine ?
                           PapyrusFunction<int32_t, Actor>(sitState)(apActor) : -1;
    LogSoloSample(apPhase, apActor, apSeat, occupied, state, ReadSoloSample(apActor, occupied == apSeat, state == 3));
}

void PollSoloObservation(Intent& aIntent)
{
    const auto now = NowMilliseconds();
    const auto start = s_solo.StartedAt.load();
    const auto entered = s_solo.FirstEnterAt.load();
    const auto deadline = SeatingObservationWindow::Deadline(start, entered);
    const bool active = SeatingObservationWindow::Active(now, start, entered);
    if ((active && now - aIntent.LastSampleMs < SeatingObservationWindow::SampleMs) ||
        (!active && aIntent.LastClosedDeadline == deadline))
        return;
    aIntent.LastSampleMs = now;
    if (!active)
        aIntent.LastClosedDeadline = deadline;
    // Re-resolve each tick: diagnostic tokens are never dereferenced. This
    // observer intentionally precedes readiness/unsafe-state/completion exits.
    auto* actor = PlayerCharacter::Get();
    auto* seat = Cast<TESObjectREFR>(TESForm::GetById(aIntent.SeatForm));
    if (!actor || TESForm::GetById(actor->formID) != actor || !seat ||
        s_solo.ActorToken != reinterpret_cast<uintptr_t>(actor) || s_solo.SeatToken != reinterpret_cast<uintptr_t>(seat))
        return;
    CaptureSoloSample(active ? (entered ? "post-enter-sample" : "pending-entry-sample") : "observation-window-ended", actor, seat);
}

void Log(Intent& aIntent, const char* aReason)
{
    if (aIntent.LastReason == aReason)
        return;
    aIntent.LastReason = aReason;
    spdlog::info("[STRE][CreationSeating] phase={} serverId={} playerId={} revision={} creationPositionIndex={} seat={:X} actor={:X} sessionConnected={} localPlayer={}",
                 aReason, aIntent.Server, aIntent.DurablePlayer, aIntent.Revision, aIntent.Index, aIntent.SeatForm, aIntent.Projection.Actor,
                 aIntent.SessionConnected, aIntent.Local);
}
TESObjectREFR* Seat(uint32_t aLocalId)
{
    auto* manager = ModManager::Get();
    auto* mod = manager ? manager->GetByName("STRE_AlternateStart.esp") : nullptr;
    return mod ? Cast<TESObjectREFR>(TESForm::GetById(mod->GetFormId(aLocalId))) : nullptr;
}

#ifndef MASTER
bool PrepareLocalApproach(Intent& aIntent, Actor* apActor, TESObjectREFR* apSeat)
{
    auto& approach = aIntent.Approach;
    const auto now = NowMilliseconds();
    const auto reject = [&](const char* apReason) {
        approach.Reject(apReason);
        if (aIntent.LastReason != approach.Failure)
            spdlog::info("[STRE][CreationSeating][LocalProbe] phase=prototype-rejected reason={} {}",
                         approach.Failure, IntentContext(aIntent));
        Log(aIntent, approach.Failure);
        return false;
    };
    if (approach.State == LocalSeatApproach::Phase::Failed)
        return reject(approach.Failure);
    // Share the validated Solo pipeline with the connected native local player.
    // Session/recovery/Applied/durable-identity fences are checked on every Tick.
    if (const auto* reason = LocalSeatApproachRejection(aIntent.Local,
            apActor == PlayerCharacter::Get() && TESForm::GetById(0x14) == apActor,
            apActor->GetExtension()->IsRemote(), aIntent.Index))
        return reject(reason);
    const auto binding = ResolveSeatApproachBinding(aIntent.Index);
    if (!binding)
        return reject("approach-marker-rank-unmapped");
    if (Seat(binding->Seat) != apSeat)
        return reject("approach-assigned-seat-mismatch");
    auto* marker = Seat(binding->Marker);
    if (!marker)
        return reject("approach-marker-missing");
    aIntent.ApproachForm = marker->formID;
    auto* manager = ModManager::Get();
    auto* master = manager ? manager->GetByName("Skyrim.esm") : nullptr;
    if (!master || !marker->baseForm || marker->baseForm->formID != master->GetFormId(0x00000034))
        return reject("approach-marker-not-xmarkerheading");
    if (marker->IsDisabled())
        return reject("approach-marker-disabled");
    if (!marker->parentCell || marker->parentCell != apSeat->parentCell)
        return reject("approach-marker-cell-mismatch");
    if (!apSeat->GetNiNode())
        return reject("approach-seat-3d-unavailable");
    const auto target = SeatApproachMarkerTarget(marker->position, marker->rotation);
    if (!target)
        return reject("approach-marker-transform-invalid");
    if (approach.State == LocalSeatApproach::Phase::None)
    {
        spdlog::info("[STRE][CreationSeating][LocalProbe] phase=prototype-triggered {}", IntentContext(aIntent));
        aIntent.ApproachActor = reinterpret_cast<uintptr_t>(apActor);
        aIntent.ApproachSeat = reinterpret_cast<uintptr_t>(apSeat);
        aIntent.ApproachCell = reinterpret_cast<uintptr_t>(apSeat->parentCell);
        aIntent.ApproachMarker = reinterpret_cast<uintptr_t>(marker);
        aIntent.ApproachSeatPosition = apSeat->position;
        aIntent.ApproachSeatRotation = apSeat->rotation;
        approach.Start(*target, s_engineUpdate, now); // reserve before the only MoveTo
        s_observationContext.store(std::make_shared<const std::string>(IntentContext(aIntent)));
        spdlog::info("[STRE][CreationSeating][LocalProbe] phase=approach-target source=ck-marker "
                     "update={} targetPosition=({},{},{}) targetRotation=({},{},{}) targetCell={:X} "
                     "actorRemote={} arrivalTolerance=2 timeoutMs=5000 {}",
                     s_engineUpdate, target->Position.x, target->Position.y, target->Position.z,
                     target->Rotation.x, target->Rotation.y, target->Rotation.z, marker->parentCell->formID,
                     apActor->GetExtension()->IsRemote(), IntentContext(aIntent));
        CaptureSoloSample("before-approach-move", apActor, apSeat);
        apActor->MoveTo(marker->parentCell, NiPoint3{target->Position});
        // Re-resolve after MoveTo; never treat return from this queued call as arrival.
        if (PlayerCharacter::Get() != apActor || TESForm::GetById(0x14) != apActor ||
            TESForm::GetById(aIntent.SeatForm) != apSeat ||
            TESForm::GetById(aIntent.ApproachForm) != marker)
            return reject("approach-binding-lost-after-move");
        CaptureSoloSample("after-approach-move-request", apActor, apSeat);
        Log(aIntent, "approach-move-issued");
        return false;
    }
    const bool identity = aIntent.ApproachActor == reinterpret_cast<uintptr_t>(apActor) &&
        aIntent.ApproachSeat == reinterpret_cast<uintptr_t>(apSeat) &&
        aIntent.ApproachCell == reinterpret_cast<uintptr_t>(marker->parentCell) &&
        aIntent.ApproachMarker == reinterpret_cast<uintptr_t>(marker) &&
        aIntent.ApproachSeatPosition == glm::vec3(apSeat->position) &&
        aIntent.ApproachSeatRotation == glm::vec3(apSeat->rotation) &&
        approach.Target.Position == target->Position && approach.Target.Rotation == target->Rotation;
    const auto action = approach.Observe(s_engineUpdate, now, identity,
        apActor->parentCell == marker->parentCell, apActor->position, apActor->rotation);
    if (action == SeatApproachAction::Reject || action == SeatApproachAction::ApplyFacing ||
        action == SeatApproachAction::Activate || now - aIntent.LastApproachSample >= 500)
    {
        aIntent.LastApproachSample = now;
        const auto delta = glm::vec3(apActor->position) - approach.Target.Position;
        spdlog::info("[STRE][CreationSeating][LocalProbe] phase=approach-observation update={} moveUpdate={} facingUpdate={} "
                     "elapsedMs={} state={} distanceToTarget={} targetYaw={} actualYaw={} reason={} {}",
                     s_engineUpdate, approach.MoveUpdate, approach.FacingUpdate, now - approach.StartedMs,
                     static_cast<int>(approach.State), std::sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z),
                     approach.Target.Rotation.z, apActor->rotation.z, approach.Failure ? approach.Failure : "", IntentContext(aIntent));
        CaptureSoloSample("approach-observation", apActor, apSeat);
    }
    if (action == SeatApproachAction::Reject)
        return reject(approach.Failure);
    if (action == SeatApproachAction::ApplyFacing)
    {
        CaptureSoloSample("approach-arrived-before-facing", apActor, apSeat);
        apActor->SetRotation(approach.Target.Rotation.x, approach.Target.Rotation.y, approach.Target.Rotation.z);
        if (PlayerCharacter::Get() != apActor || TESForm::GetById(0x14) != apActor)
            return reject("approach-binding-lost-after-facing");
        CaptureSoloSample("approach-facing-applied", apActor, apSeat);
        Log(aIntent, "approach-await-next-engine-update");
        return false;
    }
    if (action == SeatApproachAction::Activate)
    {
        Log(aIntent, "approach-ready-after-engine-update");
        return true;
    }
    return false;
}
#endif
}

void Receive(World& aWorld, const NotifyCharacterBuildState& aState) noexcept
{
    if (aState.State != CharacterBuildNetworkState::Applied || !aState.Revision ||
        aState.SeatingCampaignId.empty() || aState.SeatingPlayerId.empty())
        return;
    const auto& snapshot = aWorld.GetCampaignService().GetLatestSnapshot();
    if (snapshot && snapshot->CampaignId != aState.SeatingCampaignId)
        return;
    for (auto& [server, intent] : s_intents)
        if (server != aState.ServerId && intent.Campaign == aState.SeatingCampaignId.c_str() &&
            intent.DurablePlayer == aState.SeatingPlayerId.c_str())
        {
            Log(intent, "rejected-duplicate-durable-binding");
            return;
        }
    const auto existing = s_intents.find(aState.ServerId);
    if (existing != s_intents.end())
    {
        // Never let a duplicate or contradictory identity rearm native activation.
        auto& intent = existing->second;
        if (intent.Revision != aState.Revision || intent.Player != aState.PlayerId ||
            intent.Campaign != aState.SeatingCampaignId.c_str() || intent.DurablePlayer != aState.SeatingPlayerId.c_str())
            Log(intent, "rejected-identity-or-revision");
        return;
    }
    if (s_intents.size() >= 10)
        return;
    Intent intent;
    intent.Server = aState.ServerId;
    intent.Player = aState.PlayerId;
    intent.Revision = aState.Revision;
    intent.Campaign = aState.SeatingCampaignId.c_str();
    intent.DurablePlayer = aState.SeatingPlayerId.c_str();
    s_intents.emplace(intent.Server, std::move(intent));
    RemoteSeatingProbe::Applied(aWorld, aState);
}

void FinalizeLocal(World& aWorld, uint32_t aServerId, uint64_t aRevision) noexcept
{
    if (!aWorld.GetTransport().IsConnected())
    {
        if (aRevision)
            return; // A disconnected multiplayer final is never an offline Solo final.
        Intent solo;
        solo.Local = solo.Solo = true;
        s_intents.emplace(0, std::move(solo));
        return;
    }
    const auto found = s_intents.find(aServerId);
    if (found != s_intents.end() && found->second.Revision == aRevision)
        found->second.Local = true;
}

void OnUpdate(World& aWorld) noexcept
{
    ++s_engineUpdate;
    Tick(aWorld);
}

void Tick(World& aWorld) noexcept
{
    const auto* gate = CampaignRuntimeGateService::TryGet();
    for (auto& [id, intent] : s_intents)
    {
        intent.SessionConnected = aWorld.GetTransport().IsConnected();
        bool observeLocal = intent.Solo;
#ifndef MASTER
        observeLocal = intent.Local;
#endif
        if (gate && gate->IsLocked())
        {
            Log(intent, "pending-recovery-lock");
            continue;
        }
        if (VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0")
        {
            Log(intent, "pending-runtime-unsupported");
            continue;
        }
        if (intent.Solo && aWorld.GetTransport().IsConnected())
        {
            Log(intent, "pending-session-changed");
            continue;
        }
        size_t index{};
        if (!intent.Solo)
        {
            if (!aWorld.GetTransport().IsConnected())
            {
                Log(intent, "pending-disconnected");
                continue;
            }
            const auto& snapshot = aWorld.GetCampaignService().GetLatestSnapshot();
            if (!snapshot || !snapshot->RosterSealed || snapshot->CampaignId != intent.Campaign.c_str() ||
                snapshot->Phase != kCampaignWirePhaseCharacterCreation || snapshot->RuntimeState != kCampaignWireRuntimeActive)
            {
                Log(intent, "pending-campaign");
                continue;
            }
            std::vector<std::string_view> players;
            for (const auto& member : snapshot->Roster)
                players.emplace_back(member.PlayerId.data(), member.PlayerId.size());
            const auto selected = StandingCreationPositionIndex(players, intent.DurablePlayer);
            if (!selected)
            {
                Log(intent, "rejected-player-id");
                continue;
            }
            index = *selected;
            if (intent.Player == aWorld.GetTransport().GetLocalPlayerId() && !intent.Local)
            {
                Log(intent, "pending-local-finalization");
                continue;
            }
        }
        if (intent.Local && !intent.Solo)
        {
#ifndef MASTER
            if (intent.Player != aWorld.GetTransport().GetLocalPlayerId())
            {
                Log(intent, "rejected-local-transport-identity");
                continue;
            }
#endif
            const auto durable = aWorld.GetCampaignService().GetDurablePlayerIdForAuthentication();
            if (!durable || *durable != intent.DurablePlayer.c_str())
            {
                Log(intent, "rejected-local-identity");
                continue;
            }
        }
        // Keep the passive window through native pending/completed states, but
        // only after the session, recovery, campaign and local identity fences.
        if (observeLocal && intent.SoloProbeStarted)
            PollSoloObservation(intent);
        auto* actor = intent.Local ? PlayerCharacter::Get() : RemoteRespawnLab::CommittedActor(aWorld, intent.Server, intent.Revision);
        if (!intent.Local)
        {
            intent.Index = index;
            RemoteSeatingProbe::Binding(aWorld, intent.Server, intent.Revision, index, actor);
            Log(intent, actor ? "remote-network-animation-only" : "pending-actor");
            // TESFurniture::Activate on 1.6.1170 only accepts the native local
            // PlayerCharacter. Remote presentation belongs to STR's action stream.
            continue;
        }
        if (actor != PlayerCharacter::Get() || (actor && actor->GetExtension()->IsRemote()))
        {
            Log(intent, "rejected-nonlocal-native-actor");
            continue;
        }
        if (!actor || TESForm::GetById(actor->formID) != actor || !actor->GetNiNode() ||
            actor->IsDead() || actor->IsDisabled() || actor->IsInCombat() || actor->IsMount())
        {
            Log(intent, "pending-actor");
            continue;
        }
        const auto nativeState = DecodeFinalRespawnActorState(actor->actorState.flags1, actor->actorState.flags2);
        if (!nativeState.LifeAllowsReplacement() || !nativeState.KnockIdle() || !nativeState.AttackIdle())
        {
            Log(intent, "pending-unsafe-actor-state");
            continue;
        }
        const auto localId = CreationSeatLocalFormId(index);
        auto* seat = localId ? Seat(*localId) : nullptr;
        if (!seat || !seat->baseForm || seat->baseForm->formType != FormType::Furniture || !seat->parentCell ||
            seat->IsDisabled() || seat->parentCell != actor->parentCell)
        {
            Log(intent, "pending-seat-or-cell");
            continue;
        }
        auto* manager = ModManager::Get();
        auto* mod = manager ? manager->GetByName("STRE_AlternateStart.esp") : nullptr;
        if (!mod || seat->parentCell->formID != mod->GetFormId(0x000012D1))
        {
            Log(intent, "rejected-seat-cell");
            continue;
        }
        intent.Index = index;
        intent.SeatForm = seat->formID;
        auto* process = actor->currentProcess;
        if (!process || !process->middleProcess)
        {
            Log(intent, "pending-process");
            continue;
        }
        const auto& papyrus = aWorld.ctx().at<PapyrusService>();
        const auto* inUse = papyrus.Get("ObjectReference", "IsFurnitureInUse");
        const auto* sitState = papyrus.Get("Actor", "GetSitState");
        if (!inUse || !sitState || !GameVM::Get() || !GameVM::Get()->virtualMachine)
        {
            Log(intent, "pending-native-functions");
            continue;
        }
        auto* occupied = TESObjectREFR::GetByHandle(process->middleProcess->occupiedFurniture.handle.iBits);
        const bool correct = occupied == seat;
        const auto nativeSitState = PapyrusFunction<int32_t, Actor>(sitState)(actor);
        const bool seated = nativeSitState == 3;
        const bool used = PapyrusFunction<bool, TESObjectREFR, bool>(inUse)(seat, false);
        if (observeLocal && (!intent.SoloProbeStarted || intent.Projection.Token != reinterpret_cast<uintptr_t>(actor)))
        {
            BeginSoloObservation(intent, actor, seat, sitState);
            intent.SoloProbeStarted = true;
            intent.LastSampleMs = intent.LastClosedDeadline = 0;
        }
        const auto sample = observeLocal ? ReadSoloSample(actor, correct, seated) : SoloSample{};
        // Connected local non-MASTER uses the same engine evidence as Solo.
        // Existing remote projection remains unchanged; rendered pose needs a human.
        const bool entryConfirmed = observeLocal ? sample.Evidence.AnimationObserved() : seated;
        const auto action = intent.Projection.Observe(actor->formID, reinterpret_cast<uintptr_t>(actor), true,
                                                      correct, entryConfirmed, occupied && !correct, used);
        if (action == SeatAction::Complete)
        {
            Log(intent, observeLocal ? (sample.Evidence.AnimationObserved() ? "entry-animation-observed" : "projection-already-completed") :
                           (correct && seated ? "furniture-state-confirmed" : "projection-already-completed"));
            continue;
        }
        if (action == SeatAction::Conflict)
        {
            Log(intent, "pending-occupation-conflict");
            continue;
        }
        if (action == SeatAction::AwaitEntry)
        {
            const auto now = std::chrono::steady_clock::now();
            // Keep the existing request-status timeout. It no longer owns the
            // diagnostic window, which continues through a late Enter + 5 s.
            const bool timeout = intent.Projection.Issued && now - intent.IssuedAt > std::chrono::seconds(10);
            Log(intent, timeout ? "pending-entry-timeout-no-reactivation" :
                           (observeLocal && sample.Evidence.LogicalSeated() ? "furniture-logical-only" : "pending-entry"));
            continue;
        }
        if (action != SeatAction::Activate)
            continue;
#ifndef MASTER
        if (intent.Local && !PrepareLocalApproach(intent, actor, seat))
            continue;
#endif
        // One native request per current Actor token. The existing wrapper calls
        // RealActivate under ScopedActivateOverride; no ActivateRequest is sent.
        intent.Projection.Issued = true;
        intent.IssuedAt = std::chrono::steady_clock::now();
        if (observeLocal)
        {
            // A chair can become available after a long occupancy wait. Bound
            // action capture from the actual request, not the earlier wait.
            BeginSoloObservation(intent, actor, seat, sitState);
            intent.LastSampleMs = intent.LastClosedDeadline = 0;
            LogSoloSample("before-activation", actor, seat, occupied, nativeSitState, sample);
        }
        const bool accepted = seat->Activate(actor, 0, nullptr, 1, 0);
        Log(intent, accepted ? "activation-issued" : "activation-rejected-no-reactivation");
        if (observeLocal)
        {
            auto* afterProcess = actor->currentProcess;
            auto* afterOccupied = afterProcess && afterProcess->middleProcess ?
                                      TESObjectREFR::GetByHandle(afterProcess->middleProcess->occupiedFurniture.handle.iBits) : nullptr;
            const auto afterSit = PapyrusFunction<int32_t, Actor>(sitState)(actor);
            LogSoloSample("after-activation", actor, seat, afterOccupied, afterSit, ReadSoloSample(actor, afterOccupied == seat, afterSit == 3));
        }
    }
}
void ObserveFurnitureEvent(const TESFurnitureEvent* apEvent) noexcept
{
    RemoteSeatingProbe::Furniture(apEvent);
    if (!apEvent || !apEvent->character || !apEvent->furniture ||
        s_solo.ActorToken != reinterpret_cast<uintptr_t>(apEvent->character) ||
        s_solo.SeatToken != reinterpret_cast<uintptr_t>(apEvent->furniture))
        return;
    bool firstEnter = false;
    const auto now = NowMilliseconds();
    if (apEvent->state == TESFurnitureEvent::State::Enter)
    {
        s_solo.Entered = true;
        int64_t empty{};
        firstEnter = s_solo.FirstEnterAt.compare_exchange_strong(empty, now);
        if (firstEnter)
            s_solo.ActionBudget.Reset(); // fresh budget for the useful event
    }
    else if (apEvent->state == TESFurnitureEvent::State::Exit)
        s_solo.Entered = false;
    if (firstEnter || s_solo.EventLogs.fetch_add(1) < 8)
    {
        spdlog::info("[STRE][CreationSeating][LocalProbe] phase=furniture-event actor={:X} seat={:X} event={} eventState={} "
                     "elapsedMs={} firstEnter={} visualConfirmation=human-required {}",
                     apEvent->character->formID, apEvent->furniture->formID,
                     apEvent->state == TESFurnitureEvent::State::Enter ? "enter" :
                         (apEvent->state == TESFurnitureEvent::State::Exit ? "exit" : "unknown"),
                     static_cast<uint32_t>(apEvent->state), now - s_solo.StartedAt.load(), firstEnter, ObservationContext());
        // Synchronous, sequential read-only snapshot at callback entry, before
        // returning to the engine. Do not defer the useful moment to a VM tick.
        if (auto* actor = Cast<Actor>(apEvent->character))
            CaptureSoloSample(apEvent->state == TESFurnitureEvent::State::Enter ? "furniture-enter-immediate" : "furniture-exit-immediate",
                              actor, apEvent->furniture);
    }
}

void ObserveAnimationAction(TESActionData* apAction, uint8_t aResult, bool aRemoteBlocked) noexcept
{
    RemoteSeatingProbe::NativeAction(apAction, aResult, aRemoteBlocked);
    if (!apAction || !apAction->actor || s_solo.ActorToken != reinterpret_cast<uintptr_t>(static_cast<Actor*>(apAction->actor)))
        return;
    const auto now = NowMilliseconds();
    const auto started = s_solo.StartedAt.load();
    const auto entered = s_solo.FirstEnterAt.load();
    if (!SeatingObservationWindow::Active(now, started, entered))
        return;
    if (!s_solo.ActionBudget.Admit(now))
    {
        ++s_solo.ActionsDropped;
        return;
    }
    spdlog::info("[STRE][CreationSeating][LocalProbe] phase=animation-action actor={:X} target={:X} action={:X} idle={:X} "
                 "event={} targetEvent={} input={} flags={} transitionNoAnimation={} skipped={} result={} remoteBlocked={} elapsedMs={} sinceEnterMs={} {}",
                 apAction->actor->formID, apAction->target ? apAction->target->formID : 0,
                 apAction->action ? apAction->action->formID : 0, apAction->idleForm ? apAction->idleForm->formID : 0,
                 apAction->eventName.AsAscii() ? apAction->eventName.AsAscii() : "", apAction->targetEventName.AsAscii() ? apAction->targetEventName.AsAscii() : "",
                 apAction->unkInput, apAction->someFlag, apAction->someFlag == BGSActionData::kTransitionNoAnimation,
                 apAction->someFlag == BGSActionData::kSkip, aResult, aRemoteBlocked, now - started, entered ? now - entered : -1, ObservationContext());
}

void Clear() noexcept
{
    RemoteSeatingProbe::Clear();
    s_solo.ActorToken = 0;
    s_solo.SeatToken = 0;
    s_solo.StartedAt = s_solo.FirstEnterAt = 0;
    s_solo.SitStateFunction = nullptr;
    s_observationContext.store(nullptr);
    s_intents.clear();
}
}
