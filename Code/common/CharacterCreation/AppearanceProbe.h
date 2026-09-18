#pragma once

#include <Messages/CharacterAppearanceUpdate.h>
#include <optional>

namespace STRE::CharacterCreation
{
inline bool IsPrivatePlayerCreation(bool aPlayer, bool aCreatedPrivateNpc, uint32_t aActorId, uint32_t aBaseId) noexcept
{
    return aPlayer && aCreatedPrivateNpc && aActorId >= 0xFF000000 && aBaseId >= 0xFF000000;
}
struct AppearanceBaseIdentity
{
    uint32_t ActorFormId{};
    uint32_t BaseFormId{};

    bool Matches(uint32_t aActorId, uint32_t aBaseId) const noexcept
    {
        return ActorFormId >= 0xFF000000 && BaseFormId >= 0xFF000000 && ActorFormId == aActorId && BaseFormId == aBaseId;
    }
};

struct AppearanceNodes
{
    uintptr_t ThirdPerson{};
    uintptr_t Face{};
    uintptr_t Head{};
    bool operator==(const AppearanceNodes&) const = default;
};

inline bool RenewedAppearanceGeometry(AppearanceNodes before, AppearanceNodes now) noexcept
{
    return now.ThirdPerson && now.Face && now.Head && now.ThirdPerson != before.ThirdPerson && (now.Face != before.Face || now.Head != before.Head);
}

// Diagnostic observation only. Changed pointers never establish readiness for
// applying a different appearance. One reset per component lifetime, even when
// notifications arrive during/after the probe.
struct AppearanceProbe
{
    static constexpr uint32_t MaxTicks = 120;
    NotifyCharacterAppearanceUpdate Latest;
    AppearanceNodes Before;
    AppearanceNodes Previous;
    uint32_t Ticks{};
    uint32_t FirstTransitionTick{};
    bool Started{};
    bool Complete{};
    bool SawNull{};
    bool SawReturn{};
    bool SawTransition{};

    void Receive(const NotifyCharacterAppearanceUpdate& acSnapshot) { Latest = acSnapshot; }
    void Rebind()
    {
        auto latest = std::move(Latest);
        *this = AppearanceProbe{};
        Latest = std::move(latest);
    }
    bool Start(AppearanceNodes aNodes) noexcept
    {
        if (Started || Complete)
            return false;
        Started = true;
        Before = Previous = aNodes;
        return true;
    }
    void Observe(AppearanceNodes aNodes) noexcept
    {
        if (Complete)
            return;
        ++Ticks;
        if (Started)
        {
            if (!aNodes.ThirdPerson || !aNodes.Face || !aNodes.Head)
                SawNull = true;
            else if (SawNull)
                SawReturn = true;
            if (!(aNodes == Before))
            {
                if (!SawTransition)
                    FirstTransitionTick = Ticks;
                SawTransition = true;
            }
            Previous = aNodes;
        }
        Complete = Ticks >= MaxTicks;
    }
};

struct PendingAppearanceFinal
{
    std::optional<RequestCharacterAppearanceUpdate> Snapshot;
    void Reset() noexcept { Snapshot.reset(); }
    void Queue(bool aConnected, RequestCharacterAppearanceUpdate aSnapshot)
    {
        Reset();
        if (aConnected && aSnapshot.IsValid())
            Snapshot = std::move(aSnapshot);
    }
    template <class TSend> bool Flush(bool aConnected, std::optional<uint32_t> aServerId, TSend&& aSend)
    {
        if (!aConnected)
            Reset();
        if (!Snapshot || !aServerId)
            return false;
        Snapshot->ActorId = *aServerId;
        if (!aSend(*Snapshot))
            return false;
        Reset();
        return true;
    }
};
} // namespace STRE::CharacterCreation
