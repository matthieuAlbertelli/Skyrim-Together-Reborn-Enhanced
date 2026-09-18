#pragma once
#include <cstdint>
#include <cmath>

namespace STRE::CharacterCreation
{
// Value-only observations: pointer tokens are compared, never dereferenced.
struct AppearancePostState
{
    uint64_t ActorPointer{};
    uint64_t BasePointer{};
    uint64_t ActorId{};
    uint64_t BaseId{};
    uint64_t ServerId{};
    uint64_t FormId{};
    uint64_t CachedRefId{};
    uint64_t ProvenanceActorId{};
    uint64_t ProvenanceBaseId{};
    uint64_t RuntimeRace{};
    uint64_t BaseRace{};
    uint64_t Sex{};
    uint64_t RemoteMarker{};
    uint64_t PlayerMarker{};
    uint64_t PrivateBase{};
    float Weight{};
};

// Visit every field, including passes, so a failure never hides later mismatches.
// Expected identity/races come from the verified pre-state; sex/weight from
// the incoming descriptor. Weight is diagnostic only: this path neither owns
// nor writes remote weight. The callback observes matches without deciding policy.
template <class TVisitor> bool CheckAppearancePostState(const AppearancePostState& before, const AppearancePostState& expected, const AppearancePostState& actual, TVisitor&& visit)
{
    bool passed = true;
    {
        const bool matches = expected.ActorPointer == actual.ActorPointer;
        visit("ActorPointer", before.ActorPointer, expected.ActorPointer, actual.ActorPointer, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.BasePointer == actual.BasePointer;
        visit("BasePointer", before.BasePointer, expected.BasePointer, actual.BasePointer, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.ActorId == actual.ActorId;
        visit("ActorId", before.ActorId, expected.ActorId, actual.ActorId, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.BaseId == actual.BaseId;
        visit("BaseId", before.BaseId, expected.BaseId, actual.BaseId, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.ServerId == actual.ServerId;
        visit("ServerId", before.ServerId, expected.ServerId, actual.ServerId, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.FormId == actual.FormId;
        visit("FormId", before.FormId, expected.FormId, actual.FormId, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.CachedRefId == actual.CachedRefId;
        visit("CachedRefId", before.CachedRefId, expected.CachedRefId, actual.CachedRefId, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.ProvenanceActorId == actual.ProvenanceActorId;
        visit("ProvenanceActorId", before.ProvenanceActorId, expected.ProvenanceActorId, actual.ProvenanceActorId, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.ProvenanceBaseId == actual.ProvenanceBaseId;
        visit("ProvenanceBaseId", before.ProvenanceBaseId, expected.ProvenanceBaseId, actual.ProvenanceBaseId, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.RuntimeRace == actual.RuntimeRace;
        visit("RuntimeRace", before.RuntimeRace, expected.RuntimeRace, actual.RuntimeRace, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.BaseRace == actual.BaseRace;
        visit("BaseRace", before.BaseRace, expected.BaseRace, actual.BaseRace, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.Sex == actual.Sex;
        visit("Sex", before.Sex, expected.Sex, actual.Sex, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.RemoteMarker == actual.RemoteMarker;
        visit("RemoteMarker", before.RemoteMarker, expected.RemoteMarker, actual.RemoteMarker, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.PlayerMarker == actual.PlayerMarker;
        visit("PlayerMarker", before.PlayerMarker, expected.PlayerMarker, actual.PlayerMarker, matches);
        passed &= matches;
    }
    {
        const bool matches = expected.PrivateBase == actual.PrivateBase;
        visit("PrivateBase", before.PrivateBase, expected.PrivateBase, actual.PrivateBase, matches);
        passed &= matches;
    }
    const bool weightMatches = std::isfinite(actual.Weight) && std::isfinite(expected.Weight) && std::abs(actual.Weight - expected.Weight) <= 0.01f;
    visit("Weight", before.Weight, expected.Weight, actual.Weight, weightMatches);
    return passed; // Weight never participates in the fatal decision.
}
} // namespace STRE::CharacterCreation
