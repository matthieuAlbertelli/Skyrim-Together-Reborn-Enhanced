#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace STRE::CharacterCreation
{
struct AppearanceMutationFingerprint
{
    enum Field
    {
        Entity,
        ServerId,
        ActorId,
        ActorPointer,
        BaseId,
        BasePointer,
        RuntimeRace,
        BaseRace,
        OverlayRace,
        Sex,
        WeightBits,
        Headparts,
        HeadpartsHash,
        HairForm,
        HairColor,
        BodyColor,
        Root,
        Face,
        Head,
        WaitingFor3D,
        ProvenancePresent,
        ProvenanceActor,
        ProvenanceBase,
        CachedRefId,
        FieldCount
    };
    inline static constexpr std::array<const char*, FieldCount> Names{
        "entity",      "serverId", "actorForm",  "actorPtr",     "baseForm",          "basePtr",         "runtimeRace",    "baseRace",
        "overlayRace", "sex",      "weightBits", "headparts",    "headpartsHash",     "hairForm",        "hairColor",      "bodyColor",
        "root",        "face",     "head",       "WaitingFor3D", "provenancePresent", "provenanceActor", "provenanceBase", "CachedRefId"};
    std::array<uint64_t, FieldCount> Values{};
    std::string Changes(const AppearanceMutationFingerprint& before) const
    {
        std::string result;
        for (size_t i = 0; i < Values.size(); ++i)
            if (Values[i] != before.Values[i])
            {
                if (!result.empty())
                    result += ',';
                result += Names[i];
            }
        return result;
    }
    const char* Outcome(const AppearanceMutationFingerprint& before) const
    {
        if (!before.Values[ActorPointer] || !Values[ActorPointer] || !before.Values[BasePointer] || !Values[BasePointer])
            return "IDENTITY_UNAVAILABLE";
        for (auto field : {ActorId, ActorPointer, BaseId, BasePointer})
            if (Values[field] != before.Values[field])
                return "REMOTE_RECREATED";
        return "IN_PLACE_MUTATION";
    }
};
} // namespace STRE::CharacterCreation
