#include <TiltedOnlinePCH.h>
#include <BranchInfo.h>
#include <Services/RemoteSeatingProbe.h>

#if (!IS_MASTER)
#include <CharacterCreation/CreationSeating.h>
#include <CharacterCreation/SeatingObservationWindow.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <Services/TransportService.h>
#include <World.h>
#include <Components.h>
#include <Games/TES.h>
#include <Actor.h>
#include <AI/AIProcess.h>
#include <Misc/MiddleProcess.h>
#include <Forms/TESObjectCELL.h>
#include <Forms/TESPackage.h>
#include <Forms/BGSAction.h>
#include <Forms/TESIdleForm.h>
#include <Games/ActorExtension.h>
#include <Games/Animation/TESActionData.h>
#include <Events/EventDispatcher.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <memory>

namespace STRE::RemoteSeatingProbe
{
namespace
{
using namespace CharacterCreation;
int64_t Now() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
struct ObservedBinding
{
    uint32_t Entity{0xFFFFFFFF}, ActorId{}, BaseId{}, CachedRef{}, SeatId{};
    uintptr_t ActorToken{}, BaseToken{}, SeatToken{};
    size_t Index{10}; // Unknown until the sealed durable rank is resolved.
    bool Committed{};
    bool operator==(const ObservedBinding&) const = default;
};
struct AppliedIdentity
{
    uint32_t Player{};
    uint64_t Revision{};
    std::string Durable;
};
struct ActionProof
{
    uint32_t ActorId{}, Idle{};
    uintptr_t ActorToken{};
    uint64_t Tick{};
    int Result{};
    std::string Event;
};
struct Trace
{
    uint32_t Server{};
    std::atomic<std::shared_ptr<const AppliedIdentity>> Identity;
    std::atomic<std::shared_ptr<const ObservedBinding>> Current;
    std::atomic<std::shared_ptr<const ActionProof>> LastReplay;
    std::atomic<int64_t> Started{}, Entered{}, LastSample{}, ClosedDeadline{};
    std::atomic<int64_t> LastGraphWait{};
    std::atomic<unsigned> ActionLines{}, Dropped{}, EventLines{}, BindingLines{};
    std::atomic<uint64_t> ReceivedCount{}, FilteredCount{}, ReplayCount{};
    std::atomic<uint64_t> Packets{}, LastPacketTick{};
    std::atomic<bool> SawCommit{};
    SeatingActionTraceBudget Budget;
};
// Writers run alongside the existing seating intention updates. Native callbacks
// load immutable identity snapshots and atomic counters, never an Intent or pointer.
std::array<std::atomic<std::shared_ptr<Trace>>, 10> s_traces;
std::atomic<unsigned> s_untrackedFiltered{};
std::shared_ptr<Trace> Find(uint32_t aServer)
{
    for (auto& slot : s_traces)
        if (auto trace = slot.load(); trace && trace->Server == aServer)
            return trace;
    return {};
}
std::shared_ptr<Trace> Track(uint32_t aServer)
{
    if (auto existing = Find(aServer)) return existing;
    auto trace = std::make_shared<Trace>();
    trace->Server = aServer;
    trace->Started = Now();
    for (auto& slot : s_traces)
    {
        std::shared_ptr<Trace> empty;
        if (slot.compare_exchange_strong(empty, trace)) return trace;
    }
    return {};
}
std::shared_ptr<Trace> Find(World& aWorld, Actor* apActor)
{
    if (!apActor) return {};
    for (auto entity : aWorld.view<RemoteComponent, PlayerComponent, FormIdComponent>())
        if (aWorld.get<FormIdComponent>(entity).Id == apActor->formID)
            return Find(aWorld.get<RemoteComponent>(entity).Id);
    return {};
}
std::shared_ptr<Trace> Find(Actor* aActor)
{
    if (!aActor) return {};
    for (auto& slot : s_traces)
        if (auto trace = slot.load())
            if (auto b = trace->Current.load(); b && b->ActorId == aActor->formID && b->ActorToken == reinterpret_cast<uintptr_t>(aActor))
                return trace;
    return {};
}
std::string Context(const Trace& t, const ObservedBinding& b)
{
    const auto identity = t.Identity.load();
    return fmt::format("serverId={} playerId={} durablePlayerId={} revision={} entityVersioned={} committedObserved={} creationPositionIndex={} "
                       "seat={:X} observedActor={:X} Base={:X} CachedRefId={:X} observedActorToken={:X} BaseToken={:X}",
                       t.Server, identity ? identity->Player : 0, identity ? identity->Durable : "unknown",
                       identity ? identity->Revision : 0, b.Entity, b.Committed, b.Index, b.SeatId,
                       b.ActorId, b.BaseId, b.CachedRef, b.ActorToken, b.BaseToken);
}
bool AdmitAction(Trace& t)
{
    const auto now = Now();
    if (!SeatingObservationWindow::Active(now, t.Started, t.Entered) ||
        !t.Budget.Admit(now) || t.ActionLines.fetch_add(1) >= 512)
    {
        ++t.Dropped;
        return false;
    }
    return true;
}
int Graph(Actor* aActor, const char* aName)
{
    if (!aActor->animationGraphHolder.IsReady()) return -1;
    BSFixedString key(aName);
    bool value{};
    return aActor->animationGraphHolder.GetVariableBool(&key, &value) ? int(value) : -1;
}
void Sample(const Trace& t, const ObservedBinding& b, Actor* actor, TESObjectREFR* seat, const char* phase, int result = -1)
{
    if (!actor || !seat || b.ActorToken != reinterpret_cast<uintptr_t>(actor) || b.SeatToken != reinterpret_cast<uintptr_t>(seat)) return;
    auto* process = actor->currentProcess;
    auto* middle = process ? process->middleProcess : nullptr;
    auto* occupied = middle ? TESObjectREFR::GetByHandle(middle->occupiedFurniture.handle.iBits) : nullptr;
    auto* runOnce = middle ? middle->runOncePackage : nullptr;
    auto* current = process ? process->package : nullptr;
    const auto targetHandle = runOnce ? middle->runOnceTargetHandle : (process ? process->packageTargetHandle : 0);
    auto* target = targetHandle ? TESObjectREFR::GetByHandle(targetHandle) : nullptr;
    const auto delta = actor->position - seat->position;
    spdlog::info("[STRE][CreationSeating][RemoteProbe] phase={} result={} elapsedMs={} actorRemote={} localPlayer=false "
        "distance={} actorPosition=({},{},{}) seatPosition=({},{},{}) actorRotation=({},{},{}) seatRotation=({},{},{}) "
        "actorCell={:X} seatCell={:X} root={:X} graphReady={} occupied={:X} sitSleepStateRaw={} isInFurniture={} isIdleSitting={} "
        "bAnimationDriven={} actorState1={:08X} actorState2={:08X} process={:X} middle={:X} currentPackage={:X} runOncePackage={:X} "
        "targetHandle={:X} targetFurniture={:X} furnitureIdle={:X} receivedPackets={} lastPacketTick={} receivedActions={} filteredActions={} forceActionCalls={} "
        "droppedDiagnosticLines={} visualConfirmation=human-required {}",
        phase, result, Now() - t.Started.load(), actor->GetExtension()->IsRemote(), std::sqrt(delta.x*delta.x + delta.y*delta.y + delta.z*delta.z),
        actor->position.x, actor->position.y, actor->position.z, seat->position.x, seat->position.y, seat->position.z,
        actor->rotation.x, actor->rotation.y, actor->rotation.z, seat->rotation.x, seat->rotation.y, seat->rotation.z,
        actor->parentCell ? actor->parentCell->formID : 0, seat->parentCell ? seat->parentCell->formID : 0,
        reinterpret_cast<uintptr_t>(actor->GetNiNode()), actor->animationGraphHolder.IsReady(), occupied ? occupied->formID : 0,
        (actor->actorState.flags1 >> 14) & 15u, Graph(actor, "isInFurniture"), Graph(actor, "isIdleSitting"), Graph(actor, "bAnimationDriven"),
        actor->actorState.flags1, actor->actorState.flags2, reinterpret_cast<uintptr_t>(process), reinterpret_cast<uintptr_t>(middle),
        current ? current->formID : 0, runOnce ? runOnce->formID : 0, targetHandle, target ? target->formID : 0,
        middle && middle->furnitureIdle ? middle->furnitureIdle->formID : 0,
        t.Packets.load(), t.LastPacketTick.load(), t.ReceivedCount.load(), t.FilteredCount.load(), t.ReplayCount.load(), t.Dropped.load(), Context(t, b));
}
void Action(Trace& t, const ObservedBinding& b, const ActionEvent& a, const char* phase, int result, Actor* actor,
            uint32_t currentForm, uint64_t packetTick = 0, size_t queue = 0)
{
    if (!AdmitAction(t)) return;
    spdlog::info("[STRE][CreationSeating][RemoteProbe] phase={} result={} actionTick={} transportOrUpdateTick={} actualActor={:X} actualActorToken={:X} "
                 "matchesObservedCommitted={} queueSize={} type={} action={:X} idle={:X} target={:X} event={} targetEvent={} "
                 "sourceState1={:08X} sourceState2={:08X} actualState1={:08X} actualState2={:08X} graphReady={} root={:X} "
                 "isInFurniture={} isIdleSitting={} elapsedMs={} {}",
                 phase, result, a.Tick, packetTick, currentForm, reinterpret_cast<uintptr_t>(actor),
                 b.Committed && b.ActorToken == reinterpret_cast<uintptr_t>(actor), queue, a.Type,
                 a.ActionId, a.IdleId, a.TargetId, a.EventName.c_str(), a.TargetEventName.c_str(), a.State1, a.State2,
                 actor ? actor->actorState.flags1 : 0, actor ? actor->actorState.flags2 : 0,
                 actor && actor->animationGraphHolder.IsReady(), actor ? reinterpret_cast<uintptr_t>(actor->GetNiNode()) : 0,
                 actor ? Graph(actor, "isInFurniture") : -1, actor ? Graph(actor, "isIdleSitting") : -1,
                 Now() - t.Started.load(), Context(t, b));
}
}
void Applied(World& world, const NotifyCharacterBuildState& state) noexcept
{
    if (state.PlayerId == world.GetTransport().GetLocalPlayerId()) return;
    auto t = Track(state.ServerId);
    if (!t || t->Identity.load()) return; // Duplicates never extend a window.
    t->Identity.store(std::make_shared<const AppliedIdentity>(AppliedIdentity{state.PlayerId, state.Revision, state.SeatingPlayerId.c_str()}));
    t->Started = Now();
    t->ActionLines = 0;
    spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=applied-intent-created serverId={} playerId={} durablePlayerId={} revision={} "
                 "receivedBeforeApplied={} droppedDiagnosticLines={} localPlayer=false",
                 t->Server, state.PlayerId, state.SeatingPlayerId.c_str(), state.Revision, t->ReceivedCount.load(), t->Dropped.load());
}
void Binding(World& world, uint32_t server, uint64_t revision, size_t index, Actor* committed) noexcept
{
    auto t = Find(server);
    if (!t) return;
    const auto identity = t->Identity.load();
    if (!identity || identity->Revision != revision) return;
    ObservedBinding b;
    b.Index = index;
    b.Committed = committed != nullptr;
    Actor* actor = committed;
    for (auto entity : world.view<RemoteComponent, FormIdComponent>())
        if (world.get<RemoteComponent>(entity).Id == server)
        {
            b.Entity = static_cast<uint32_t>(entity);
            b.CachedRef = world.get<RemoteComponent>(entity).CachedRefId;
            if (!actor) actor = Cast<Actor>(TESForm::GetById(world.get<FormIdComponent>(entity).Id)); // Evidence only, never activation.
            break;
        }
    if (actor)
    {
        b.ActorId = actor->formID;
        b.ActorToken = reinterpret_cast<uintptr_t>(actor);
        b.BaseId = actor->baseForm ? actor->baseForm->formID : 0;
        b.BaseToken = reinterpret_cast<uintptr_t>(actor->baseForm);
    }
    auto* manager = ModManager::Get();
    auto* mod = manager ? manager->GetByName("STRE_AlternateStart.esp") : nullptr;
    const auto id = CreationSeatLocalFormId(index);
    auto* seat = mod && id ? Cast<TESObjectREFR>(TESForm::GetById(mod->GetFormId(*id))) : nullptr;
    b.SeatId = seat ? seat->formID : 0;
    b.SeatToken = reinterpret_cast<uintptr_t>(seat);
    auto old = t->Current.load();
    if (!old || *old != b)
    {
        t->Current.store(std::make_shared<const ObservedBinding>(b));
        if (b.Committed && !t->SawCommit.exchange(true))
        {
            t->Started = Now();
            t->Entered = 0;
            t->ActionLines = 0;
            t->LastSample = 0;
        }
        if (t->BindingLines.fetch_add(1) < 32)
        {
          spdlog::info("[STRE][CreationSeating][RemoteProbe] phase={} {}", b.Committed ? "committed-binding-selected" : "awaiting-committed-actor", Context(*t, b));
          if (const auto* animation = world.try_get<RemoteAnimationComponent>(static_cast<entt::entity>(b.Entity)))
            spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=queue-at-binding queueSize={} lastRanTick={} lastRanEvent={} lastRanIdle={:X} {}",
                         animation->TimePoints.size(), animation->LastRanAction.Tick, animation->LastRanAction.EventName.c_str(), animation->LastRanAction.IdleId, Context(*t, b));
          if (auto replay = t->LastReplay.load())
            spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=last-force-action-at-binding actualActor={:X} actualActorToken={:X} actionTick={} "
                         "event={} idle={:X} result={} matchesNewBinding={} {}", replay->ActorId, replay->ActorToken, replay->Tick,
                         replay->Event, replay->Idle, replay->Result, replay->ActorToken == b.ActorToken, Context(*t, b));
        }
    }
    const auto now = Now();
    const auto deadline = SeatingObservationWindow::Deadline(t->Started, t->Entered);
    const bool active = SeatingObservationWindow::Active(now, t->Started, t->Entered);
    if ((active && now - t->LastSample.load() < SeatingObservationWindow::SampleMs) || (!active && t->ClosedDeadline == deadline)) return;
    t->LastSample = now;
    if (!active)
    {
        t->ClosedDeadline = deadline;
        spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=observation-window-ended receivedActions={} filteredActions={} "
                     "forceActionCalls={} droppedDiagnosticLines={} {}", t->ReceivedCount.load(), t->FilteredCount.load(),
                     t->ReplayCount.load(), t->Dropped.load(), Context(*t, b));
    }
    Sample(*t, b, actor, seat, active ? "binding-sample" : "observation-window-ended");
}
void Packet(World& world, uint32_t server, uint32_t entity, uint64_t packetTick) noexcept
{
    auto t = Find(server);
    const auto ecs = static_cast<entt::entity>(entity);
    if (!t && world.valid(ecs) && world.all_of<RemoteComponent, PlayerComponent>(ecs)) t = Track(server);
    if (!t) return;
    ++t->Packets;
    t->LastPacketTick = packetTick;
    // Establish value-only context before Applied too. Applied/commit selection
    // remains owned by Binding; packet arrival never changes that classification.
    if (!t->Identity.load())
    {
        ObservedBinding b;
        b.Entity = entity;
        const auto* remote = world.try_get<RemoteComponent>(ecs);
        const auto* form = world.try_get<FormIdComponent>(ecs);
        auto* actor = form ? Cast<Actor>(TESForm::GetById(form->Id)) : nullptr;
        b.CachedRef = remote ? remote->CachedRefId : 0;
        if (actor)
        {
            b.ActorId = actor->formID;
            b.ActorToken = reinterpret_cast<uintptr_t>(actor);
            b.BaseId = actor->baseForm ? actor->baseForm->formID : 0;
            b.BaseToken = reinterpret_cast<uintptr_t>(actor->baseForm);
        }
        auto old = t->Current.load();
        if (!old || *old != b) t->Current.store(std::make_shared<const ObservedBinding>(b));
    }
}
void Received(World& world, uint32_t server, uint32_t entity, const ActionEvent& action, uint64_t packetTick, size_t queue, const char* disposition) noexcept
{
    auto t = Find(server);
    const auto ecs = static_cast<entt::entity>(entity);
    // Observe before Applied as well, but never expand the probe to ordinary NPCs.
    if (!t && world.valid(ecs) && world.all_of<RemoteComponent, PlayerComponent>(ecs)) t = Track(server);
    if (t)
    {
        ++t->ReceivedCount;
        if (entity == 0xFFFFFFFF) ++t->FilteredCount;
        const auto* form = world.try_get<FormIdComponent>(ecs);
        auto* actor = form ? Cast<Actor>(TESForm::GetById(form->Id)) : nullptr;
        auto b = t->Current.load();
        Action(*t, b ? *b : ObservedBinding{}, action, disposition, -1, actor, form ? form->Id : 0, packetTick, queue);
    }
    else if (entity == 0xFFFFFFFF && s_untrackedFiltered.fetch_add(1) < 32)
        spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=received-untracked-filtered serverId={} packetTick={} actionTick={} "
                     "action={:X} idle={:X} event={} reason=missing-remote-animation-view", server, packetTick, action.Tick,
                     action.ActionId, action.IdleId, action.EventName.c_str());
}
void Replay(World& world, Actor* actor, const ActionEvent& action, const char* phase, int result, size_t queue, uint64_t tick) noexcept
{
    if (auto t = Find(world, actor))
    {
        if (std::string_view(phase) == "graph-not-ready" || std::string_view(phase) == "action-not-due")
        {
            const auto now = Now();
            if (now - t->LastGraphWait.load() < SeatingObservationWindow::SampleMs) return;
            t->LastGraphWait = now;
        }
        if (std::string_view(phase) == "force-action-return")
        {
            ++t->ReplayCount;
            if (SeatingObservationWindow::Active(Now(), t->Started, t->Entered))
                t->LastReplay.store(std::make_shared<const ActionProof>(ActionProof{actor->formID, action.IdleId,
                    reinterpret_cast<uintptr_t>(actor), action.Tick, result, action.EventName.c_str()}));
        }
        auto b = t->Current.load();
        Action(*t, b ? *b : ObservedBinding{}, action, phase, result, actor, actor->formID, tick, queue);
    }
}
void QueueReset(World& world, uint32_t entity, const char* reason) noexcept
{
    const auto ecs = static_cast<entt::entity>(entity);
    const auto* remote = world.try_get<RemoteComponent>(ecs);
    auto t = remote ? Find(remote->Id) : nullptr;
    if (!t || !AdmitAction(*t)) return;
    const auto* animation = world.try_get<RemoteAnimationComponent>(ecs);
    auto b = t->Current.load();
    spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=queue-reset reason={} queueBefore={} {}", reason,
                 animation ? animation->TimePoints.size() : 0, Context(*t, b ? *b : ObservedBinding{}));
}
void MissingUpdateActor(World& world, uint32_t entity, uint64_t tick) noexcept
{
    const auto ecs = static_cast<entt::entity>(entity);
    const auto* remote = world.try_get<RemoteComponent>(ecs);
    auto t = remote ? Find(remote->Id) : nullptr;
    if (!t || Now() - t->LastGraphWait.load() < SeatingObservationWindow::SampleMs) return;
    t->LastGraphWait = Now();
    if (!AdmitAction(*t)) return;
    const auto* form = world.try_get<FormIdComponent>(ecs);
    const auto* animation = world.try_get<RemoteAnimationComponent>(ecs);
    spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=update-actor-unavailable serverId={} entityVersioned={} "
                 "FormId={:X} queueSize={} updateTick={}", t->Server, entity, form ? form->Id : 0, animation ? animation->TimePoints.size() : 0, tick);
}
void NativeAction(TESActionData* action, uint8_t result, bool blocked) noexcept
{
    if (!action || !action->actor) return;
    if (auto t = Find(static_cast<Actor*>(action->actor))) if (auto b = t->Current.load(); b && AdmitAction(*t))
        spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=native-action result={} remoteBlocked={} action={:X} idle={:X} target={:X} event={} {}",
                     result, blocked, action->action ? action->action->formID : 0, action->idleForm ? action->idleForm->formID : 0,
                     action->target ? action->target->formID : 0, action->eventName.AsAscii() ? action->eventName.AsAscii() : "", Context(*t, *b));
}
void Furniture(const TESFurnitureEvent* event) noexcept
{
    if (!event || !event->character || !event->furniture) return;
    auto* actor = Cast<Actor>(event->character);
    if (auto t = Find(actor)) if (auto b = t->Current.load())
    {
        bool first = false;
        if (event->state == TESFurnitureEvent::State::Enter && b->SeatToken == reinterpret_cast<uintptr_t>(event->furniture))
        {
            int64_t empty{};
            first = t->Entered.compare_exchange_strong(empty, Now());
        }
        if (first || t->EventLines.fetch_add(1) < 8)
        {
            spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=furniture-event eventState={} furniture={:X} matchingSeat={} {}",
                         static_cast<uint32_t>(event->state), event->furniture->formID, b->SeatToken == reinterpret_cast<uintptr_t>(event->furniture), Context(*t, *b));
            Sample(*t, *b, actor, event->furniture, "furniture-event-immediate");
        }
    }
}
void Clear() noexcept
{
    const auto filtered = s_untrackedFiltered.exchange(0);
    if (filtered)
        spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=untracked-filter-summary count={} lineLimit=32", filtered);
    for (auto& slot : s_traces)
        if (auto t = slot.exchange(nullptr))
            spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=session-cleared serverId={} receivedActions={} filteredActions={} "
                         "forceActionCalls={} droppedDiagnosticLines={}", t->Server, t->ReceivedCount.load(), t->FilteredCount.load(),
                         t->ReplayCount.load(), t->Dropped.load());
}
}
#else
namespace STRE::RemoteSeatingProbe
{
void Applied(World&, const NotifyCharacterBuildState&) noexcept {}
void Binding(World&, uint32_t, uint64_t, size_t, Actor*) noexcept {}
void Packet(World&, uint32_t, uint32_t, uint64_t) noexcept {}
void Received(World&, uint32_t, uint32_t, const ActionEvent&, uint64_t, size_t, const char*) noexcept {}
void Replay(World&, Actor*, const ActionEvent&, const char*, int, size_t, uint64_t) noexcept {}
void QueueReset(World&, uint32_t, const char*) noexcept {}
void MissingUpdateActor(World&, uint32_t, uint64_t) noexcept {}
void NativeAction(TESActionData*, uint8_t, bool) noexcept {}
void Furniture(const TESFurnitureEvent*) noexcept {}
void Clear() noexcept {}
}
#endif
