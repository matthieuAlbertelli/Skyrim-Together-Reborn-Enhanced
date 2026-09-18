#pragma once
#include <cstdint>
struct World;
struct Actor;
// Diagnostic observations only; never stores/dereferences an actor across calls.
void TraceAppearanceActor(World& world, entt::entity entity, uint32_t serverId, Actor* actor, const char* source, uint64_t tick);

void ResetAppearanceTrace(World& world);
