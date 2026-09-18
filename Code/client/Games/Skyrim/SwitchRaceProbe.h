#pragma once
struct Actor;
// Logs a read-only snapshot with latest observed call for this actor, if known.
void ObserveSwitchRaceProbe(Actor* actor, const char* phase) noexcept;
