#include <catch2/catch.hpp>
#include <CharacterCreation/ValenDialogueProjection.h>
#include <set>

TEST_CASE("Valen has 21 disjoint local class stages and a safe fallback", "[valen]")
{
    std::set<std::uint16_t> stages;
    for (const auto classId : STRE::Valen::kClasses)
    {
        const auto stage = STRE::Valen::ClassStage(classId);
        CHECK(stage >= 101);
        CHECK(stage <= 121);
        CHECK(stages.insert(stage).second);
    }
    CHECK(stages.size() == 21);
    CHECK(STRE::Valen::ClassStage("class.warrior") == 107);
    CHECK(STRE::Valen::ClassStage("class.mage") == 117);
    CHECK(STRE::Valen::ClassStage("class.thief") == 114);
    for (const auto unknown : {"", "class.unknown", "class.Warrior", "warrior"})
        CHECK(STRE::Valen::ClassStage(unknown) == 100);
    CHECK(STRE::Valen::kGroupStage < 100);
}
