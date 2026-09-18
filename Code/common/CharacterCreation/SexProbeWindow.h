#pragma once
#include <cstdint>
#include <string_view>
#include <utility>

namespace STRE::CharacterCreation
{
inline bool SexProbeRuntime(std::string_view version)
{
    return version == "1.6.1170.0";
}
inline bool SexProbePlayer(uintptr_t actor, uintptr_t player)
{
    return actor && actor == player;
}
struct SexProbeWindow
{
    static constexpr uint64_t Duration = 180;
    bool Observed{};
    uint32_t Sex{};
    uint64_t Start{}, Until{}, Fingerprint{};
    bool SexObserved{};
    uint64_t SexTick{};
    void Arm(uint64_t tick)
    {
        Start = tick;
        Until = tick + Duration;
    }
    bool Active(uint64_t tick) const { return Until && tick <= Until; }
    struct Change
    {
        bool Baseline{}, SexChanged{}, Significant{};
        uint32_t From{};
    };
    Change Observe(uint64_t tick, uint32_t sex, uint64_t fingerprint)
    {
        Change result{!Observed, Observed && Sex != sex, !Observed || Fingerprint != fingerprint, Sex};
        if (result.SexChanged)
        {
            Arm(tick);
            SexObserved = true;
            SexTick = tick;
        }
        Observed = true;
        Sex = sex;
        Fingerprint = fingerprint;
        return result;
    }
    void Reset() { *this = {}; }
};
// Diagnostic exceptions must neither suppress nor duplicate the sole forwarded call.
// Engine exceptions remain visible; no retry or mutation callback is exposed.
template <class Before, class Original, class After> void PassiveSexProbeCall(Before&& before, Original&& original, After&& after)
{
    try
    {
        before();
    }
    catch (...)
    {
    }
    std::forward<Original>(original)();
    try
    {
        after();
    }
    catch (...)
    {
    }
}
} // namespace STRE::CharacterCreation
