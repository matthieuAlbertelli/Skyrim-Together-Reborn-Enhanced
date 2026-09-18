#pragma once
#include <cstdint>

namespace STRE::CharacterCreation
{
// Evidence labels for passive current-binding observation only. Neither result
// grants native ownership or reconstructs allocation/generation history.
enum class NativeLifetimeBindingEvidence
{
    Rejected,
    DurableProvenance,
    ValidatedCurrentBindingFallback
};

inline NativeLifetimeBindingEvidence
ClassifyNativeLifetimeBinding(uint32_t aActor, uint32_t aBase, uint32_t aCachedActor, bool aHasProvenance, uint32_t aProvenanceActor, uint32_t aProvenanceBase) noexcept
{
    if (aActor < 0xFF000000 || aBase < 0xFF000000 || aActor == aBase || aCachedActor != aActor)
        return NativeLifetimeBindingEvidence::Rejected;
    if (aHasProvenance)
        return aProvenanceActor == aActor && aProvenanceBase == aBase ? NativeLifetimeBindingEvidence::DurableProvenance : NativeLifetimeBindingEvidence::Rejected;
    return NativeLifetimeBindingEvidence::ValidatedCurrentBindingFallback;
}
} // namespace STRE::CharacterCreation
