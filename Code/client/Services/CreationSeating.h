#pragma once
#include <cstdint>
struct World;
struct NotifyCharacterBuildState;
struct TESFurnitureEvent;
struct TESActionData;
namespace STRE::CreationSeating
{
void Receive(World& aWorld, const NotifyCharacterBuildState& aState) noexcept;
void FinalizeLocal(World& aWorld, uint32_t aServerId, uint64_t aRevision) noexcept;
void Tick(World& aWorld) noexcept;
// Called once by the UpdateEvent listener, never by finalization's extra Tick.
void OnUpdate(World& aWorld) noexcept;
void Clear() noexcept;
// Read-only local diagnostics (connected local extension is non-MASTER);
// callbacks do not access the intention map.
void ObserveFurnitureEvent(const TESFurnitureEvent* apEvent) noexcept;
void ObserveAnimationAction(TESActionData* apAction, uint8_t aResult, bool aRemoteBlocked) noexcept;
}
