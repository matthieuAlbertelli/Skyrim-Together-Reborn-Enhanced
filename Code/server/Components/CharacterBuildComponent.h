#pragma once

#ifndef TP_INTERNAL_COMPONENTS_GUARD
#error Include Components.h instead
#endif

#include <Structs/CharacterBuild.h>

struct CharacterBuildComponent
{
    std::uint64_t Revision{};
    CharacterBuildSnapshotData Build{};
    bool Applied{};
};
