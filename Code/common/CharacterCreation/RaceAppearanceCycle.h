#pragma once
#include <CharacterCreation/AppearanceProbe.h>
#include <CharacterCreation/AppearancePostState.h>
#include <string_view>

namespace STRE::CharacterCreation
{
inline bool RaceSwitchRuntimeSupported(std::string_view version)
{
    return version == "1.6.1170.0";
}
inline constexpr bool RaceSwitchPlayerArgument = false;

struct RaceAppearanceCycle
{
    enum class Stage
    {
        PendingRaceSwitch,
        SwitchRequested,
        SwitchReturned,
        DeserializeRequested,
        Deserialized,
        ResetRequested,
        WaitingForRace3D,
        WaitingForRaceHead,
        ApplyingTints,
        Applied,
        Failed
    };
    bool Enabled{}, EventSeen{};
    uint32_t TargetRace{};
    Stage State{Stage::PendingRaceSwitch};
    AppearancePostState Expected;
    int64_t InventoryCount{};
    size_t EquippedCount{};

    void Begin(uint32_t target, AppearancePostState expected)
    {
        *this = {};
        Enabled = true;
        TargetRace = target;
        Expected = expected;
        Expected.RuntimeRace = Expected.BaseRace = target;
    }
    bool RequestSwitch()
    {
        if (!Enabled || State != Stage::PendingRaceSwitch)
            return false;
        State = Stage::SwitchRequested;
        return true;
    }
    bool Returned(bool valid)
    {
        if (State != Stage::SwitchRequested)
            return false;
        State = valid ? Stage::SwitchReturned : Stage::Failed;
        return valid;
    }
    bool RequestDeserialize()
    {
        if (!Enabled || State != Stage::SwitchReturned)
            return false;
        State = Stage::DeserializeRequested;
        return true;
    }
    bool Deserialized(bool valid)
    {
        if (State != Stage::DeserializeRequested)
            return false;
        State = valid ? Stage::Deserialized : Stage::Failed;
        return valid;
    }
    bool RequestReset()
    {
        if (!Enabled || State != Stage::Deserialized)
            return false;
        State = Stage::ResetRequested;
        return true;
    }
    bool ResetReturned(bool issued)
    {
        if (State != Stage::ResetRequested)
            return false;
        State = issued ? Stage::WaitingForRace3D : Stage::Failed;
        return issued;
    }
    static bool GeometryReady(AppearanceNodes before, AppearanceNodes now) { return RenewedAppearanceGeometry(before, now); }
    bool Observe(AppearanceNodes before, AppearanceNodes now)
    {
        if (State != Stage::WaitingForRace3D && State != Stage::WaitingForRaceHead)
            return false;
        if (!now.ThirdPerson)
        {
            State = Stage::WaitingForRace3D;
            return false;
        }
        State = Stage::WaitingForRaceHead;
        if (!GeometryReady(before, now))
            return false;
        State = Stage::ApplyingTints;
        return true;
    }
    bool InventoryLost(int64_t count, size_t equipped) const { return count < InventoryCount || equipped < EquippedCount; }
};
} // namespace STRE::CharacterCreation
