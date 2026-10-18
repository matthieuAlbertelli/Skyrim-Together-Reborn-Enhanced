#pragma once
#include <atomic>
#include <cstdint>

namespace STRE::CharacterCreation
{
enum class SeatingObservationAction { None, Sample, End };
struct SeatingObservationWindow
{
    static constexpr int64_t InitialMs = 40000;
    static constexpr int64_t AfterEnterMs = 5000;
    static constexpr int64_t SampleMs = 500;
    int64_t LastSampleMs{};
    bool Closed{};

    static int64_t Deadline(int64_t aStart, int64_t aFirstEnter) noexcept
    {
        return aFirstEnter ? aFirstEnter + AfterEnterMs : aStart + InitialMs;
    }
    static bool Active(int64_t aNow, int64_t aStart, int64_t aFirstEnter) noexcept
    {
        return aStart > 0 && aNow >= aStart && aNow <= Deadline(aStart, aFirstEnter);
    }
    SeatingObservationAction Poll(int64_t aNow, int64_t aStart, int64_t aFirstEnter) noexcept
    {
        if (Closed || aStart <= 0 || aNow < aStart)
            return SeatingObservationAction::None;
        if (!Active(aNow, aStart, aFirstEnter))
        {
            Closed = true;
            return SeatingObservationAction::End;
        }
        if (LastSampleMs && aNow - LastSampleMs < SampleMs)
            return SeatingObservationAction::None;
        LastSampleMs = aNow;
        return SeatingObservationAction::Sample;
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
