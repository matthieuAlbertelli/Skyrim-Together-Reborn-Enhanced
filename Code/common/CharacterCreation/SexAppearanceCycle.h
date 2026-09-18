#pragma once
#include <CharacterCreation/AppearanceProbe.h>
#include <CharacterCreation/AppearancePostState.h>
#include <string_view>

namespace STRE::CharacterCreation
{
struct SexAppearanceCycle
{
    enum class Stage
    {
        PendingSexChange,
        DeserializedSex,
        SexVerified,
        ResetRequested,
        WaitingForSex3D,
        WaitingForSexHead,
        ApplyingTints,
        Applied,
        Failed
    };
    static constexpr uint32_t MaxTicks = 120;
    bool Enabled{};
    Stage State{Stage::PendingSexChange};
    AppearancePostState Expected;
    AppearanceNodes Before, TintTarget;
    uint32_t Ticks{};
    int64_t InventoryCount{};
    size_t EquippedCount{};
    void Begin(AppearancePostState expected)
    {
        *this = {};
        Enabled = true;
        Expected = expected;
    }
    bool RequestDeserialize()
    {
        if (!Enabled || State != Stage::PendingSexChange)
            return false;
        State = Stage::DeserializedSex;
        return true;
    }
    bool Verify(bool invariants, uint32_t actualSex)
    {
        if (State != Stage::DeserializedSex)
            return false;
        State = invariants && actualSex == Expected.Sex ? Stage::SexVerified : Stage::Failed;
        return State == Stage::SexVerified;
    }
    bool RequestReset(AppearanceNodes nodes)
    {
        if (State != Stage::SexVerified)
            return false;
        Before = nodes;
        Ticks = 0;
        State = Stage::ResetRequested;
        return true;
    }
    bool ResetReturned(bool issued)
    {
        if (State != Stage::ResetRequested)
            return false;
        State = issued ? Stage::WaitingForSex3D : Stage::Failed;
        return issued;
    }
    bool Tick()
    {
        if (Ticks >= MaxTicks)
        {
            State = Stage::Failed;
            return false;
        }
        ++Ticks;
        return true;
    }
    bool GeometryReady(AppearanceNodes now) const { return RenewedAppearanceGeometry(Before, now); }
    bool Observe(AppearanceNodes now)
    {
        if (State != Stage::WaitingForSex3D && State != Stage::WaitingForSexHead)
            return false;
        State = now.ThirdPerson && now.ThirdPerson != Before.ThirdPerson ? Stage::WaitingForSexHead : Stage::WaitingForSex3D;
        if (!GeometryReady(now))
            return false;
        State = Stage::ApplyingTints;
        TintTarget = now;
        return true;
    }
    bool Complete(bool generated)
    {
        if (State != Stage::ApplyingTints || !generated)
            return false;
        State = Stage::Applied;
        return true;
    }
    bool InventoryLost(int64_t count, size_t worn) const { return count < InventoryCount || worn < EquippedCount; }
};
} // namespace STRE::CharacterCreation
