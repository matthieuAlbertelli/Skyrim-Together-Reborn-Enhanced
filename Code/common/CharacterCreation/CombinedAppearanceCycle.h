#pragma once
#include <CharacterCreation/AppearanceProbe.h>
#include <CharacterCreation/AppearancePostState.h>

namespace STRE::CharacterCreation
{
// Two native rebuilds for one immutable network snapshot. No final bytes or tints
// are consumed by the race/source-sex stage. Each wait has its own bounded budget.
struct CombinedAppearanceCycle
{
    enum class Stage
    {
        PendingCombined,
        RaceSwitchRequested,
        RaceSwitchVerified,
        RaceResetRequested,
        WaitingForRaceGeometry,
        RaceStageReady,
        FinalDeserializeRequested,
        FinalDeserializeVerified,
        SexResetRequested,
        WaitingForFinalGeometry,
        FinalStageReady,
        ApplyingTints,
        Applied,
        Failed
    };
    static constexpr uint32_t MaxStageTicks = 120;
    bool Enabled{}, FinalPhase{}, NativeCallInProgress{};
    Stage State{Stage::PendingCombined};
    AppearancePostState RaceExpected, FinalExpected;
    AppearanceNodes BeforeRaceReset, BeforeFinalReset, TintTarget;
    uint32_t Ticks{}, SwitchCount{}, DeserializeCount{}, ResetCount{}, FaceGenCount{};
    // CharacterService update identity, not an elapsed-time or frame-delay budget.
    uint64_t RaceReadyUpdate{};
    int64_t InventoryCount{};
    size_t EquippedCount{};

    void Begin(AppearancePostState source, uint32_t targetRace, uint8_t targetSex, float weight)
    {
        *this = {};
        Enabled = true;
        RaceExpected = source;
        RaceExpected.RuntimeRace = RaceExpected.BaseRace = targetRace;
        FinalExpected = RaceExpected;
        FinalExpected.Sex = targetSex;
        FinalExpected.Weight = weight;
    }
    const AppearancePostState& Expected() const { return FinalPhase ? FinalExpected : RaceExpected; }
    bool RequestSwitch()
    {
        if (!Enabled || State != Stage::PendingCombined)
            return false;
        State = Stage::RaceSwitchRequested;
        ++SwitchCount;
        return true;
    }
    bool VerifyRace(bool passed)
    {
        if (State != Stage::RaceSwitchRequested)
            return false;
        State = passed ? Stage::RaceSwitchVerified : Stage::Failed;
        return passed;
    }
    bool RequestRaceReset(AppearanceNodes nodes)
    {
        if (State != Stage::RaceSwitchVerified)
            return false;
        BeforeRaceReset = nodes;
        Ticks = 0;
        ++ResetCount;
        State = Stage::RaceResetRequested;
        return true;
    }
    bool RaceResetReturned(bool issued)
    {
        if (State != Stage::RaceResetRequested)
            return false;
        State = issued ? Stage::WaitingForRaceGeometry : Stage::Failed;
        return issued;
    }
    bool ObserveRace(AppearanceNodes nodes, uint64_t serviceUpdate)
    {
        if (State != Stage::WaitingForRaceGeometry || !RenewedAppearanceGeometry(BeforeRaceReset, nodes))
            return false;
        RaceReadyUpdate = serviceUpdate;
        State = Stage::RaceStageReady;
        return true;
    }
    bool RequestFinalDeserialize(uint64_t serviceUpdate)
    {
        if (State != Stage::RaceStageReady || serviceUpdate == RaceReadyUpdate)
            return false;
        FinalPhase = true;
        ++DeserializeCount;
        State = Stage::FinalDeserializeRequested;
        return true;
    }
    bool VerifyFinal(bool passed)
    {
        if (State != Stage::FinalDeserializeRequested)
            return false;
        State = passed ? Stage::FinalDeserializeVerified : Stage::Failed;
        return passed;
    }
    bool RequestFinalReset(AppearanceNodes nodes)
    {
        if (State != Stage::FinalDeserializeVerified)
            return false;
        BeforeFinalReset = nodes;
        Ticks = 0;
        ++ResetCount;
        State = Stage::SexResetRequested;
        return true;
    }
    bool FinalResetReturned(bool issued)
    {
        if (State != Stage::SexResetRequested)
            return false;
        State = issued ? Stage::WaitingForFinalGeometry : Stage::Failed;
        return issued;
    }
    bool FinalGeometryReady(AppearanceNodes nodes) const { return RenewedAppearanceGeometry(BeforeFinalReset, nodes); }
    bool ObserveFinal(AppearanceNodes nodes)
    {
        if (State != Stage::WaitingForFinalGeometry || !FinalGeometryReady(nodes))
            return false;
        TintTarget = nodes;
        State = Stage::FinalStageReady;
        return true;
    }
    bool RequestFaceGen()
    {
        if (State != Stage::FinalStageReady)
            return false;
        ++FaceGenCount;
        State = Stage::ApplyingTints;
        return true;
    }
    bool Complete(bool generated)
    {
        if (State != Stage::ApplyingTints || !generated)
            return false;
        State = Stage::Applied;
        return true;
    }
    bool Tick()
    {
        if (State != Stage::WaitingForRaceGeometry && State != Stage::WaitingForFinalGeometry && State != Stage::ApplyingTints)
            return false;
        if (Ticks >= MaxStageTicks)
        {
            State = Stage::Failed;
            return false;
        }
        ++Ticks;
        return true;
    }
    bool InventoryLost(int64_t count, size_t equipped) const { return count < InventoryCount || equipped < EquippedCount; }
};
} // namespace STRE::CharacterCreation
