#pragma once
#include <glm/vec3.hpp>

struct TESObjectCELL;
struct TESWorldSpace;

// Native placement only; no ECS identity, ownership or network lifecycle.
struct ActorSpawnLocation
{
    TESObjectCELL* Cell{};
    TESWorldSpace* WorldSpace{};
    glm::vec3 Position{};
    glm::vec3 Rotation{};
};
