#include <TiltedOnlinePCH.h>
#include <Services/RemoteSeatingProbe.h>
#include <BranchInfo.h>
#if (!IS_MASTER)
#include <World.h>
#include <Components.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <Services/TransportService.h>
#include <CharacterCreation/SeatingObservationWindow.h>
#include <chrono>
#include <map>

namespace STRE::RemoteSeatingProbe
{
namespace
{
using namespace STRE::CharacterCreation;
struct Trace
{
    uint64_t Revision{};
    uintptr_t ActorToken{};
    int64_t Started{};
    unsigned Lines{};
    bool Closed{};
    SeatingActionTraceBudget Budget;
};
// Game-update/message consumers only. Native callbacks never access this map
// or follow an Actor/process/package/graph/furniture pointer for diagnostics.
std::map<uint32_t, Trace> s_traces;
int64_t Now()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
Trace* Find(uint32_t server)
{
    const auto it = s_traces.find(server);
    return it == s_traces.end() ? nullptr : &it->second;
}
bool Admit(Trace& t)
{
    const auto now = Now();
    if (t.Closed || !SeatingObservationWindow::Active(now, t.Started, 0) || t.Lines >= 512)
        return false;
    if (!t.Budget.Admit(now)) return false;
    ++t.Lines;
    return true;
}
}
void Applied(World& world, const NotifyCharacterBuildState& state) noexcept
{
    if (state.PlayerId == world.GetTransport().GetLocalPlayerId() || s_traces.size() >= 10) return;
    auto [it, inserted] = s_traces.try_emplace(state.ServerId);
    if (!inserted) return; // Including terminal tombstones and duplicate Applied.
    it->second.Revision = state.Revision;
    it->second.Started = Now();
}
void Binding(World&, uint32_t server, uint64_t revision, size_t index, Actor* actor) noexcept
{
    auto* t = Find(server);
    if (!t || t->Revision != revision || !Admit(*t)) return;
    const auto token = reinterpret_cast<uintptr_t>(actor); // Comparison only.
    if (t->ActorToken == token) return;
    t->ActorToken = token;
    spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=binding serverId={} revision={} rank={} actorToken={:X}", server, revision, index, token);
}
void End(uint32_t server) noexcept
{
    if (auto* t = Find(server))
    {
        t->Closed = true;
        t->ActorToken = 0;
    }
}
void Packet(World&, uint32_t, uint32_t, uint64_t) noexcept {}
void Received(World&, uint32_t server, uint32_t entity, const ActionEvent& action, uint64_t tick, size_t queue, const char* phase) noexcept
{
    auto* t = Find(server);
    if (!t || !Admit(*t)) return;
    spdlog::info("[STRE][CreationSeating][RemoteProbe] phase={} serverId={} entityVersioned={} packetTick={} actionTick={} event={} target={:X} queue={}",
        phase, server, entity, tick, action.Tick, action.EventName.c_str(), action.TargetId, queue);
}
void Replay(World&, Actor* actor, const ActionEvent& action, const char* phase, int result, size_t queue, uint64_t tick) noexcept
{
    for (auto& [server, t] : s_traces)
        if (t.ActorToken && t.ActorToken == reinterpret_cast<uintptr_t>(actor) && Admit(t))
            spdlog::info("[STRE][CreationSeating][RemoteProbe] phase={} serverId={} actorToken={:X} actionTick={} event={} target={:X} result={} queue={} tick={}",
                phase, server, t.ActorToken, action.Tick, action.EventName.c_str(), action.TargetId, result, queue, tick);
}
void QueueReset(World& world, uint32_t entity, const char* reason) noexcept
{
    const auto* remote = world.try_get<RemoteComponent>(static_cast<entt::entity>(entity));
    auto* t = remote ? Find(remote->Id) : nullptr;
    if (t && Admit(*t))
        spdlog::info("[STRE][CreationSeating][RemoteProbe] phase=queue-reset serverId={} entityVersioned={} reason={}", remote->Id, entity, reason);
    if (remote) End(remote->Id);
}
void MissingUpdateActor(World& world, uint32_t entity, uint64_t) noexcept
{
    const auto* remote = world.try_get<RemoteComponent>(static_cast<entt::entity>(entity));
    if (remote) End(remote->Id);
}
void Clear() noexcept { s_traces.clear(); }
}
#else
namespace STRE::RemoteSeatingProbe
{
void Applied(World&, const NotifyCharacterBuildState&) noexcept {}
void Binding(World&, uint32_t, uint64_t, size_t, Actor*) noexcept {}
void End(uint32_t) noexcept {}
void Packet(World&, uint32_t, uint32_t, uint64_t) noexcept {}
void Received(World&, uint32_t, uint32_t, const ActionEvent&, uint64_t, size_t, const char*) noexcept {}
void Replay(World&, Actor*, const ActionEvent&, const char*, int, size_t, uint64_t) noexcept {}
void QueueReset(World&, uint32_t, const char*) noexcept {}
void MissingUpdateActor(World&, uint32_t, uint64_t) noexcept {}
void Clear() noexcept {}
}
#endif
