#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>

namespace STRE::CharacterCreation
{
struct SeatingObservationWindow
{
    static constexpr int64_t InitialMs = 40000;
    static constexpr int64_t AfterEnterMs = 5000;
    static constexpr int64_t SampleMs = 500;

    static int64_t Deadline(int64_t aStart, int64_t aFirstEnter) noexcept
    {
        return std::max(aStart + InitialMs, aFirstEnter ? aFirstEnter + AfterEnterMs : int64_t{});
    }
    static bool Active(int64_t aNow, int64_t aStart, int64_t aFirstEnter) noexcept
    {
        return aStart > 0 && aNow <= Deadline(aStart, aFirstEnter);
    }
};

// Per-second, not per-attempt: early turn/movement spam cannot exhaust the
// budget for a late furniture entry. Omitted actions are counted by the caller.
struct SeatingActionTraceBudget
{
    static constexpr uint64_t PerSecond = 32;
    std::atomic<uint64_t> State{};

    void Reset() noexcept { State = 0; }
    bool Admit(int64_t aNow) noexcept
    {
        const auto bucket = static_cast<uint64_t>(aNow / 1000) << 32;
        auto previous = State.load();
        for (;;)
        {
            const auto count = (previous & 0xFFFFFFFF00000000ULL) == bucket ? previous & 0xFFFFFFFFULL : 0;
            if (count >= PerSecond)
                return false;
            if (State.compare_exchange_weak(previous, bucket | (count + 1)))
                return true;
        }
    }
};
}
