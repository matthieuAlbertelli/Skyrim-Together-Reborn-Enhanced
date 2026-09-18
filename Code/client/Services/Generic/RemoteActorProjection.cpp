#include <TiltedOnlinePCH.h>
#include <Services/RemoteActorProjection.h>
#include <Actor.h>
#include <Games/ActorExtension.h>
#include <Games/Overrides.h>

namespace STRE::RemoteActorProjection
{
void Place(Actor* aActor, const ActorSpawnLocation& aLocation) noexcept
{
    aActor->rotation = aLocation.Rotation;
    aActor->MoveTo(aLocation.Cell, aLocation.Position);
}

void Initialize(Actor* aActor, bool aPlayer, const ActorSpawnLocation& aLocation, const ActorValues& aValues) noexcept
{
    aActor->GetExtension()->SetRemote(true);
    Place(aActor, aLocation);
    aActor->SetActorValues(aValues);
    aActor->GetExtension()->SetPlayer(aPlayer);
    if (aPlayer)
    {
        aActor->SetIgnoreFriendlyHit(true);
        aActor->SetPlayerRespawnMode();
    }
}

bool ReadyFor3D(Actor* aActor) noexcept
{
    return aActor && aActor->GetNiNode();
}

void Complete3D(Actor* aActor, const Inventory& aInventory, const Factions& aFactions, const AnimationVariables* aVariables) noexcept
{
    ScopedInventoryOverride inventory;
    ScopedEquipOverride equip;
    ScopedUnequipOverride unequip;
    aActor->SetActorInventory(aInventory);
    aActor->SetFactions(aFactions);
    if (aVariables)
        aActor->LoadAnimationVariables(*aVariables);
}
} // namespace STRE::RemoteActorProjection
