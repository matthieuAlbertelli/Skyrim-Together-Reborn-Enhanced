#pragma once
#include <cstddef>
#include <cstdint>

struct World;
struct Actor;
struct TESFurnitureEvent;
struct TESActionData;
struct ActionEvent;
struct NotifyCharacterBuildState;

namespace STRE::RemoteSeatingProbe
{
// Non-MASTER observation only. No action is retained for replay or modified.
void Applied(World&, const NotifyCharacterBuildState&) noexcept;
void Binding(World&, uint32_t aServer, uint64_t aRevision, size_t aIndex, Actor* aCommitted) noexcept;
void Packet(World&, uint32_t aServer, uint32_t aEntity, uint64_t aPacketTick) noexcept;
void Received(World&, uint32_t aServer, uint32_t aEntity, const ActionEvent&, uint64_t aPacketTick, size_t aQueueBefore, const char* aDisposition) noexcept;
void Replay(World&, Actor*, const ActionEvent&, const char* aPhase, int aResult, size_t aQueueSize, uint64_t aUpdateTick) noexcept;
void QueueReset(World&, uint32_t aEntity, const char* aReason) noexcept;
void MissingUpdateActor(World&, uint32_t aEntity, uint64_t aUpdateTick) noexcept;
void NativeAction(TESActionData*, uint8_t aResult, bool aBlocked) noexcept;
void Furniture(const TESFurnitureEvent*) noexcept;
void Clear() noexcept;
}
