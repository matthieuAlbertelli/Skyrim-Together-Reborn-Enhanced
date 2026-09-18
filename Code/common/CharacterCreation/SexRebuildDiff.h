#pragma once
#include <cstdint>
namespace STRE::CharacterCreation
{
// A frame lives only during the forwarded native call, on its calling thread.
struct SexRebuildExecution
{
    uintptr_t Actor{};
    uint64_t Sequence{};
    bool Inline{}, Queued{};
    void Observe(uintptr_t actor, bool queued) noexcept
    {
        if (!Actor || actor != Actor)
            return;
        if (queued)
            Queued = true;
        else
            Inline = true;
    }
    const char* Result() const noexcept
    {
        if (Inline == Queued)
            return "unresolved";
        return Queued ? "queued-op1C" : "inline-AIProcess";
    }
};
} // namespace STRE::CharacterCreation
