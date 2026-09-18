#pragma once
struct Actor;
// Read-only diagnostics. No mutation or publication entry points.
void TickSexChangeProbe() noexcept;
void SignalSexChangeProbe(Actor* actor, const char* phase) noexcept;

#include <cstdint>
struct SexChangeProbeTiming { uint64_t Tick{}, SexTick{}; bool Known{}; };
SexChangeProbeTiming ReadSexChangeProbeTiming(Actor* actor) noexcept;
