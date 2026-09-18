#pragma once
#include <cstdint>
struct RaceAppearanceCompleteEvent
{
    uint32_t ActorId{};
    uintptr_t ActorToken{};
    uint32_t RuntimeRace{};
};
