#pragma once
#include <CharacterCreation/SexRebuildDiff.h>
struct Actor;
struct SexRebuildDiffScope
{
    STRE::CharacterCreation::SexRebuildExecution Frame;
    SexRebuildDiffScope* Previous{};
    SexRebuildDiffScope(Actor* actor, uint64_t sequence) noexcept;
    ~SexRebuildDiffScope();
    SexRebuildDiffScope(const SexRebuildDiffScope&) = delete;
    SexRebuildDiffScope& operator=(const SexRebuildDiffScope&) = delete;
};
void LogSexRebuildDiff(const char* phase, Actor* actor, uint64_t sequence, uintptr_t caller, bool updateWeight, const char* execution) noexcept;
void BindSexRebuildDiffActor(Actor* actor, uint32_t serverId) noexcept;
void ResetSexRebuildDiffActors() noexcept;
