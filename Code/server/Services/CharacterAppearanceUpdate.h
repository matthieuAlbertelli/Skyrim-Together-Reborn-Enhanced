#pragma once

#include <Messages/CharacterAppearanceUpdate.h>
#include <optional>

inline bool MatchesAppliedCharacterBuild(const CharacterAppearanceUpdate& aFinal, bool aApplied, uint64_t aRevision,
                                        GameId aRace, const std::optional<CharacterAppearanceUpdate>& aCommitted) noexcept
{
    return aApplied && aFinal.FinalBuildRevision && aFinal.FinalBuildRevision == aRevision && aFinal.Descriptor.Race == aRace &&
           (!aCommitted || *aCommitted == aFinal);
}

// The service resolves the entity and owner first. Keep this policy independent
// of World so rejection, canonical replacement and the exact outgoing snapshot
// can be exercised without starting a network server.
template <class TPlayer, class TCharacter, class TBroadcast>
bool ApplyCharacterAppearanceUpdate(
    const RequestCharacterAppearanceUpdate& acRequest, const TPlayer* apOwner, const TPlayer* apSender, TCharacter& aCharacter, TBroadcast&& aBroadcast)
{
    if (!apSender || apOwner != apSender || !acRequest.IsValid())
        return false;

    aCharacter.SaveBuffer = acRequest.AppearanceBuffer;
    aCharacter.ChangeFlags = acRequest.ChangeFlags;
    aCharacter.FaceTints = acRequest.FaceTints;

    NotifyCharacterAppearanceUpdate notify;
    notify.ActorId = acRequest.ActorId;
    notify.FinalBuildRevision = acRequest.FinalBuildRevision;
    notify.AppearanceBuffer = aCharacter.SaveBuffer;
    notify.ChangeFlags = aCharacter.ChangeFlags;
    notify.FaceTints = aCharacter.FaceTints;
    notify.Descriptor = acRequest.Descriptor;
    aBroadcast(notify);
    return true;
}
