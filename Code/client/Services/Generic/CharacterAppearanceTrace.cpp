#include <TiltedOnlinePCH.h>
#include <Services/AppearanceTrace.h>
#include <SexRebuildDiffProbe.h>
#include <Services/CharacterService.h>
#include <CharacterCreation/AppearanceMutationTrace.h>
#include <Components.h>
#include <World.h>
#include <Games/References.h>
#include <Forms/TESNPC.h>
#include <Forms/TESRace.h>
#include <Forms/BGSHeadPart.h>
#include <Forms/BGSColorForm.h>
#include <Systems/FaceGenSystem.h>
#include <bit>
#include <unordered_map>

namespace
{
using Fingerprint = STRE::CharacterCreation::AppearanceMutationFingerprint;
struct TraceStore
{
    struct Entry
    {
        Fingerprint Last, LastIdentity;
        bool HasIdentity{};
    };
    std::unordered_map<uint32_t, Entry> Entries;
};
} // namespace
void ResetAppearanceTrace(World& world)
{
    world.ctx().erase<TraceStore>();
    ResetSexRebuildDiffActors();
}
void TraceAppearanceActor(World& world, entt::entity entity, uint32_t serverId, Actor* actor, const char* source, uint64_t tick)
{
    auto* base = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
    const auto* remote = world.try_get<RemoteComponent>(entity);
    const auto* provenance = world.try_get<RemotePlayerAppearanceBaseComponent>(entity);
    if (actor && remote && world.all_of<PlayerComponent>(entity))
        BindSexRebuildDiffActor(actor, serverId);
    Fingerprint f;
    auto& v = f.Values;
    v[Fingerprint::Entity] = static_cast<uint32_t>(entity);
    v[Fingerprint::ServerId] = serverId;
    v[Fingerprint::ActorId] = actor ? actor->formID : 0;
    v[Fingerprint::ActorPointer] = reinterpret_cast<uintptr_t>(actor);
    v[Fingerprint::BaseId] = base ? base->formID : 0;
    v[Fingerprint::BasePointer] = reinterpret_cast<uintptr_t>(base);
    v[Fingerprint::RuntimeRace] = actor && actor->race ? actor->race->formID : 0;
    if (base)
    {
        v[Fingerprint::BaseRace] = base->raceForm.race ? base->raceForm.race->formID : 0;
        v[Fingerprint::OverlayRace] = base->overlayRace ? base->overlayRace->formID : 0;
        v[Fingerprint::Sex] = base->actorData.IsFemale();
        v[Fingerprint::WeightBits] = std::bit_cast<uint32_t>(base->weight);
        v[Fingerprint::Headparts] = base->headpartsCount;
        uint64_t hash = 14695981039346656037ull;
        if (base->headparts)
            for (uint32_t i = 0; i < base->headpartsCount; ++i)
                hash = (hash ^ (base->headparts[i] ? base->headparts[i]->formID : 0)) * 1099511628211ull;
        v[Fingerprint::HeadpartsHash] = hash;
        const auto* hair = base->headData ? base->headData->hairColor : nullptr;
        v[Fingerprint::HairForm] = hair ? hair->formID : 0;
        v[Fingerprint::HairColor] = hair ? hair->abgr : 0;
        v[Fingerprint::BodyColor] = uint32_t(base->color.red) | (uint32_t(base->color.green) << 8) | (uint32_t(base->color.blue) << 16);
    }
    v[Fingerprint::Root] = actor ? reinterpret_cast<uintptr_t>(actor->GetNiNode()) : 0;
    v[Fingerprint::Face] = actor ? reinterpret_cast<uintptr_t>(actor->GetFaceGenNiNode()) : 0;
    v[Fingerprint::Head] = actor ? reinterpret_cast<uintptr_t>(FaceGenSystem::GetHeadGeometry(actor)) : 0;
    v[Fingerprint::WaitingFor3D] = world.all_of<WaitingFor3D>(entity);
    v[Fingerprint::ProvenancePresent] = provenance != nullptr;
    v[Fingerprint::ProvenanceActor] = provenance ? provenance->ActorFormId : 0;
    v[Fingerprint::ProvenanceBase] = provenance ? provenance->BaseFormId : 0;
    v[Fingerprint::CachedRefId] = remote ? remote->CachedRefId : 0;
    if (!world.ctx().contains<TraceStore>())
        world.ctx().emplace<TraceStore>();
    auto& entries = world.ctx().at<TraceStore>().Entries;
    // Keep identity across missing-actor samples/rematerialization; reset on disconnect.
    // Bounded diagnostic storage only, never eviction of game components.
    if (!entries.contains(serverId) && entries.size() >= 256)
    {
        spdlog::warn("[STRE][AppearanceTrace][RemoteMutation] phase=trace-capacity serverId={}", serverId);
        return;
    }
    auto [it, first] = entries.try_emplace(serverId);
    auto& entry = it->second;
    const auto fields = first ? std::string("initial") : f.Changes(entry.Last);
    if (fields.empty())
        return;
    const char* outcome = first ? "INITIAL_OBSERVATION" : f.Outcome(entry.HasIdentity ? entry.LastIdentity : entry.Last);
    std::string values;
    for (size_t i = 0; i < v.size(); ++i)
        values += fmt::format(" {}={}({:X})", Fingerprint::Names[i], v[i], v[i]);
    spdlog::info(
        "[STRE][AppearanceTrace][RemoteMutation] phase=changed tick={} serverId={} source={} changedFields={} OUTCOME={} previousActor={:X}/{:X} previousBase={:X}/{:X}{}", tick,
        serverId, source, fields, outcome, entry.LastIdentity.Values[Fingerprint::ActorId], entry.LastIdentity.Values[Fingerprint::ActorPointer],
        entry.LastIdentity.Values[Fingerprint::BaseId], entry.LastIdentity.Values[Fingerprint::BasePointer], values);
    spdlog::info("[STRE][AppearanceTrace][RemoteMutation] phase=changed-values tick={} serverId={} weight={} actorPresent={} basePresent={}", tick, serverId, base ? base->weight : 0.f, actor != nullptr, base != nullptr);
    entry.Last = f;
    if (actor && base)
    {
        entry.LastIdentity = f;
        entry.HasIdentity = true;
    }
}
void CharacterService::TraceRemoteAppearances(const char* source) noexcept
{
    for (const auto entity : m_world.view<RemoteComponent, PlayerComponent>())
    {
        const auto& remote = m_world.get<RemoteComponent>(entity);
        const auto* form = m_world.try_get<FormIdComponent>(entity);
        const uint32_t id = form ? form->Id : remote.CachedRefId;
        auto* actor = id ? Cast<Actor>(TESForm::GetById(id)) : nullptr;
        TraceAppearanceActor(m_world, entity, remote.Id, actor, source, m_appearanceTraceTick);
    }
}
