#pragma once
#include <Games/ActorSpawnLocation.h>

struct Actor;
struct ActorValues;
struct Inventory;
struct Factions;
struct AnimationVariables;

namespace STRE::RemoteActorProjection
{
// These are the native parts of natural join, without logical identity creation.
void Place(Actor* aActor, const ActorSpawnLocation& aLocation) noexcept;
void Initialize(Actor* aActor, bool aPlayer, const ActorSpawnLocation& aLocation, const ActorValues& aValues) noexcept;
bool ReadyFor3D(Actor* aActor) noexcept;
void Complete3D(Actor* aActor, const Inventory& aInventory, const Factions& aFactions, const AnimationVariables* aVariables) noexcept;
} // namespace STRE::RemoteActorProjection
