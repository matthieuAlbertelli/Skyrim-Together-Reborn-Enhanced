#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

namespace STRE::CharacterCreation
{
// Bounded diagnostic history, not actor ownership or a completion contract.
// Caller supplies synchronization. Evicted/unknown actors return sequence zero.
struct SwitchRaceCorrelation
{
    struct Record
    {
        uintptr_t Actor{};
        uint64_t Sequence{};
        bool InFlight{};
    };
    std::array<Record, 64> Records{};
    uint64_t Counter{};
    size_t Next{};
    uint64_t Enter(uintptr_t actor)
    {
        const auto sequence = ++Counter;
        Records[Next] = {actor, sequence, true};
        Next = (Next + 1) % Records.size();
        return sequence;
    }
    void Return(uint64_t sequence)
    {
        for (auto& record : Records)
            if (record.Sequence == sequence)
                record.InFlight = false;
    }
    Record Latest(uintptr_t actor) const
    {
        Record found;
        if (!actor)
            return found;
        for (const auto& record : Records)
            if (record.Actor == actor && record.Sequence > found.Sequence)
                found = record;
        return found;
    }
};
} // namespace STRE::CharacterCreation
