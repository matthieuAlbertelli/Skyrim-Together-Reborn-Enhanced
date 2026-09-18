#pragma once
#include <cstdint>

// Request a fresh final capture in generic CharacterService. No network payload
// or engine pointer belongs in the CharacterCreation event.
struct RequestLocalAppearanceUpdateEvent
{
    uint64_t FinalBuildRevision{}; // Authoritatively Applied initial Character Build only.
};
