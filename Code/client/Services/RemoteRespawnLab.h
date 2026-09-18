#pragma once
#include <cstdint>

struct World;
struct Actor;
struct ActorSpawnLocation;
struct TESNPC;
struct FaceGenComponent;
struct CharacterAppearanceUpdate;
struct NotifyCharacterBuildState;
struct NotifyCharacterAppearanceUpdate;

namespace STRE::RemoteRespawnLab
{
bool Enabled() noexcept;
void SetEnabled(bool aEnabled) noexcept;
void ReceiveBuild(World& aWorld, const NotifyCharacterBuildState& aBuild) noexcept;
void ReceiveFinal(World& aWorld, const NotifyCharacterAppearanceUpdate& aFinal) noexcept;
void Tick(World& aWorld) noexcept;
void Disconnect(World& aWorld) noexcept;
// Called synchronously before the existing native Spawn; no additional native call.
void BeforeSpawn(Actor* aActor, TESNPC* aBase) noexcept;
void CandidateBase(TESNPC* aBase) noexcept;
void AfterSpawn(Actor* aActor, TESNPC* aBase) noexcept;
void DeleteRequested(const Actor* aActor) noexcept;
// Source-time observation before all legacy Discovery subscribers. Tokens never dereferenced.
bool Discovery(uint32_t aFormId, uintptr_t aToken, bool aAdded) noexcept;
bool Tracks(uint32_t aFormId, uintptr_t aToken) noexcept;

// Defined beside Slice 1's helper; shared native creation pipeline, isolated tint storage.
Actor* Materialize(World& aWorld, entt::entity aEntity, const CharacterAppearanceUpdate& aFinal, FaceGenComponent& aTints, const ActorSpawnLocation& aLocation) noexcept;
} // namespace STRE::RemoteRespawnLab
