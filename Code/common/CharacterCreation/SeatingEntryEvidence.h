#pragma once

namespace STRE::CharacterCreation
{
// Engine observations, not a claim about the rendered pose. Runtime acceptance
// still requires seeing the player sit and use the camera normally.
struct SeatingEntryEvidence
{
    bool CorrectFurniture{};
    bool LogicalSitState{};
    bool RootReady{};
    bool GraphReady{};
    bool EnterEvent{};
    bool FurnitureVariableValid{};
    bool InFurniture{};
    bool SittingVariableValid{};
    bool IdleSitting{};

    bool LogicalSeated() const noexcept { return CorrectFurniture && LogicalSitState; }
    bool AnimationObserved() const noexcept
    {
        return LogicalSeated() && RootReady && GraphReady && EnterEvent &&
               FurnitureVariableValid && InFurniture && SittingVariableValid && IdleSitting;
    }
};
}
