#pragma once
#include <cstdint>
struct World;
struct NotifyCharacterBuildState;
namespace STRE::CreationSeating
{
void Receive(World& aWorld, const NotifyCharacterBuildState& aState) noexcept;
void FinalizeLocal(World& aWorld, uint32_t aServerId, uint64_t aRevision) noexcept;
void Tick(World& aWorld) noexcept;
void Clear() noexcept;
}
