#pragma once
#include <cstdint>
#include <map>
#include <utility>

namespace STRE::CharacterCreation
{
enum class SeatingIntentAdmission { Added, Duplicate, IdentityConflict, DuplicatePlayer, Full };

// Shared by the native adapter and behavioral tests. An Applied replay must
// retain the original projection/lifetime, including its terminal tombstone.
template <class TIntent>
struct SeatingIntentResult
{
    SeatingIntentAdmission Admission;
    TIntent* Intent{};
};

template <class TIntent>
SeatingIntentResult<TIntent> AdmitSeatingIntent(std::map<uint32_t, TIntent>& aIntents, TIntent aIncoming)
{
    for (auto& [server, intent] : aIntents)
        if (server != aIncoming.Server && intent.Campaign == aIncoming.Campaign &&
            intent.DurablePlayer == aIncoming.DurablePlayer)
            return {SeatingIntentAdmission::DuplicatePlayer, &intent};

    const auto existing = aIntents.find(aIncoming.Server);
    if (existing != aIntents.end())
    {
        auto& intent = existing->second;
        const bool matches = intent.Revision == aIncoming.Revision && intent.Player == aIncoming.Player &&
            intent.Campaign == aIncoming.Campaign && intent.DurablePlayer == aIncoming.DurablePlayer;
        return {matches ? SeatingIntentAdmission::Duplicate : SeatingIntentAdmission::IdentityConflict, &intent};
    }
    if (aIntents.size() >= 10)
        return {SeatingIntentAdmission::Full};
    const auto server = aIncoming.Server;
    const auto inserted = aIntents.emplace(server, std::move(aIncoming)).first;
    return {SeatingIntentAdmission::Added, &inserted->second};
}
}
