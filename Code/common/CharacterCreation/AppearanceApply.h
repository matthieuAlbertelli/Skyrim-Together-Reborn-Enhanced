#pragma once
#include <CharacterCreation/AppearanceProbe.h>
#include <CharacterCreation/RaceAppearanceCycle.h>
#include <CharacterCreation/SexAppearanceCycle.h>
#include <CharacterCreation/CombinedAppearanceCycle.h>

namespace STRE::CharacterCreation
{
inline bool SameAppearanceRaceSex(const CharacterAppearanceDescriptor& acIncoming, const GameId& acActorRace, const GameId& acBaseRace, uint8_t aSex) noexcept
{
    return acIncoming.IsValid() && acIncoming.Race == acActorRace && acIncoming.Race == acBaseRace && acIncoming.Sex == aSex;
}

struct AppearanceApply
{
    enum class Stage
    {
        Pending,
        WaitingForHead,
        ApplyingTints
    };
    static constexpr uint32_t MaxTicks = 120;
    std::optional<NotifyCharacterAppearanceUpdate> Latest;
    std::optional<NotifyCharacterAppearanceUpdate> Active;
    Stage State{Stage::Pending};
    AppearanceNodes Before;
    AppearanceNodes TintTarget;
    AppearanceNodes Transition;
    uint32_t Ticks{};
    bool BlockFaceGen{};
    bool ProvenanceWarning{};
    bool ResetRequested{};
    RaceAppearanceCycle Race;
    SexAppearanceCycle Sex;
    CombinedAppearanceCycle Combined;

    void Receive(const NotifyCharacterAppearanceUpdate& acSnapshot)
    {
        Latest = acSnapshot;
        ProvenanceWarning = false;
    }
    void Rebind()
    {
        auto latest = Latest ? std::move(Latest) : std::move(Active);
        *this = AppearanceApply{};
        Latest = std::move(latest);
    }
    bool Begin(AppearanceNodes aNodes)
    {
        if (Active || !Latest)
            return false;
        Active = std::move(Latest);
        Latest.reset();
        Before = aNodes;
        State = Stage::WaitingForHead;
        BlockFaceGen = true;
        Ticks = 0;
        ResetRequested = false;
        Race = {};
        Sex = {};
        Combined = {};
        return true;
    }
    bool RequestReset() noexcept
    {
        if (!Active || ResetRequested || Race.Enabled || Sex.Enabled || Combined.Enabled)
            return false;
        ResetRequested = true;
        return true;
    }
    bool RequestRaceReset(AppearanceNodes preReset) noexcept
    {
        if (!Active || ResetRequested || !Race.RequestReset())
            return false;
        ResetRequested = true;
        Before = preReset;
        Ticks = 0;
        return true;
    }
    bool RaceResetReturned(bool issued) noexcept
    {
        if (!Active || !Race.ResetReturned(issued))
        {
            Finish(false);
            return false;
        }
        return true;
    }
    bool CompleteTints(bool generated)
    {
        if (!Active || Combined.Enabled || State != Stage::ApplyingTints || !generated)
            return false;
        Finish(true);
        return true;
    }
    bool ObserveHead(AppearanceNodes aNodes) noexcept
    {
        if (!Active || Combined.Enabled || State != Stage::WaitingForHead || Ticks >= MaxTicks)
            return false;
        ++Ticks;
        if (Race.Enabled)
        {
            if (!Race.Observe(Before, aNodes))
                return false;
            State = Stage::ApplyingTints;
            TintTarget = Transition = aNodes;
            return true;
        }
        if (aNodes.Face && aNodes.Head && (aNodes.Face != Before.Face || aNodes.Head != Before.Head))
        {
            State = Stage::ApplyingTints;
            TintTarget = aNodes;
            Transition = aNodes;
            return true;
        }
        return false;
    }
    void Finish(bool aApplied)
    {
        if (Race.Enabled)
            Race.State = aApplied ? RaceAppearanceCycle::Stage::Applied : RaceAppearanceCycle::Stage::Failed;
        if (Sex.Enabled)
            Sex.State = aApplied ? SexAppearanceCycle::Stage::Applied : SexAppearanceCycle::Stage::Failed;
        if (Combined.Enabled)
        {
            Combined.NativeCallInProgress = false;
            Combined.State = aApplied ? CombinedAppearanceCycle::Stage::Applied : CombinedAppearanceCycle::Stage::Failed;
        }
        Active.reset();
        State = Stage::Pending;
        Ticks = 0;
        // On failure, never let the generic loop tint an unverified old head.
        BlockFaceGen = !aApplied;
    }
};
} // namespace STRE::CharacterCreation
