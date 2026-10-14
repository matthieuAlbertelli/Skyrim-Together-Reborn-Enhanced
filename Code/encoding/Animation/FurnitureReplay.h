#pragma once
#include <Animation/BindingActionReplay.h>

// Pure preflight shared with tests. All fields come from fresh game-update
// resolution; tokens identify bindings and are never converted to pointers.
struct FurnitureReplayTarget
{
    uint32_t FormId{}, Cell{};
    bool IsFurniture{}, Has3D{};
};
inline bool CanApplyFurnitureReplay(AnimationBinding aActual, AnimationBinding aExpected,
    uint32_t aActorCell, bool aActor3D,
    const FurnitureReplayTarget& aTarget, bool aNewerExit = false) noexcept
{
    return aActual.FormId && aActual.Token && aActual == aExpected &&
        aActorCell && aActor3D &&
        aTarget.FormId && aTarget.IsFurniture && aTarget.Has3D &&
        aTarget.Cell == aActorCell && !aNewerExit;
}

// Some native chair events have no target. Only the current Applied creation
// assignment may supply one; an explicit target is never silently substituted.
inline uint32_t FurnitureReplayTargetId(const ActionEvent& aAction, uint32_t aAssignedCreationSeat) noexcept
{
    return aAction.TargetId ? aAction.TargetId :
        (ActionReplayCache::IsChairEntry(aAction) ? aAssignedCreationSeat : 0);
}
