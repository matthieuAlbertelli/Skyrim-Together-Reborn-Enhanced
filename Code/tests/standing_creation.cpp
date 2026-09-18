#include <CharacterCreation/StandingCreation.h>
#include <catch2/catch.hpp>
#include <algorithm>
#include <array>
#include <limits>

using namespace STRE::CharacterCreation;

TEST_CASE("Standing creation PlayerId ranks are unique stable and independent of roster order", "[character-creation]")
{
    std::array<std::string_view, 10> players{"j", "h", "f", "d", "b", "i", "g", "e", "c", "a"};
    for (const auto local : players)
    {
        const auto rank = StandingCreationPositionIndex(players, local);
        REQUIRE(rank);
        REQUIRE(*rank == static_cast<size_t>(local.front() - 'a'));
        auto reversed = players;
        std::reverse(reversed.begin(), reversed.end());
        REQUIRE(StandingCreationPositionIndex(reversed, local) == rank);
    }
}

TEST_CASE("Standing creation rejects incomplete or contradictory player identity", "[character-creation]")
{
    const std::array<std::string_view, 2> valid{"b", "a"}, duplicate{"a", "a"}, empty{"a", ""};
    REQUIRE_FALSE(StandingCreationPositionIndex({}, "a"));
    REQUIRE_FALSE(StandingCreationPositionIndex(valid, ""));
    REQUIRE_FALSE(StandingCreationPositionIndex(valid, "c"));
    REQUIRE_FALSE(StandingCreationPositionIndex(duplicate, "a"));
    REQUIRE_FALSE(StandingCreationPositionIndex(empty, "a"));
    const std::array<std::string_view, 11> overflow{"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k"};
    REQUIRE_FALSE(StandingCreationPositionIndex(overflow, "a"));
    REQUIRE(StandingCreationPositionIndex(std::span(valid).first(1), "b") == 0);
}

TEST_CASE("Standing placement leaves both chair rows behind their furniture without changing floor", "[character-creation]")
{
    const auto nearRow = StandingCreationPosition({100.f, -2500.f, 12.f}, 0.f);
    const auto farRow = StandingCreationPosition({100.f, -2300.f, 12.f}, 3.14159265f);
    REQUIRE(nearRow.x == Approx(100.f));
    REQUIRE(nearRow.y == Approx(-2596.f));
    REQUIRE(nearRow.z == 12.f);
    REQUIRE(farRow.x == Approx(100.f));
    REQUIRE(farRow.y == Approx(-2204.f));
    REQUIRE(farRow.z == 12.f);
    REQUIRE(farRow.y - nearRow.y > 200.f);
}

TEST_CASE("Creation move validation accepts nearby coordinates and rejects missed or nonfinite moves", "[character-creation]")
{
    const glm::vec3 target{100.f, 200.f, 12.f};
    REQUIRE(CreationPositionReached(target, target));
    REQUIRE(CreationPositionReached(target + glm::vec3{16.f, -16.f, 16.f}, target));
    REQUIRE(CreationPositionReached(target + glm::vec3{32.f, 0.f, 0.f}, target));
    REQUIRE_FALSE(CreationPositionReached(target + glm::vec3{33.f, 0.f, 0.f}, target));
    REQUIRE_FALSE(CreationPositionReached(target + glm::vec3{24.f, 24.f, 0.f}, target));
    REQUIRE_FALSE(CreationPositionReached(target + glm::vec3{0.f, 0.f, 33.f}, target));
    for (const auto invalid : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
        for (unsigned axis = 0; axis < 3; ++axis)
        {
            auto bad = target;
            bad[axis] = invalid;
            REQUIRE_FALSE(CreationPositionReached(bad, target));
            REQUIRE_FALSE(CreationPositionReached(target, bad));
        }
}

TEST_CASE("Deferred player movement waits for actual cell and position and has a bounded timeout", "[character-creation]")
{
    const glm::vec3 target{100.f, 200.f, 12.f}, before{100.f, 100.f, 12.f};
    REQUIRE(ObserveCreationMove(true, before, target, 0.0) == CreationMoveStatus::Pending);
    REQUIRE(ObserveCreationMove(true, before, target, 4.99) == CreationMoveStatus::Pending);
    REQUIRE(ObserveCreationMove(false, target, target, 1.0) == CreationMoveStatus::Pending);
    REQUIRE(ObserveCreationMove(true, target, target, 0.1) == CreationMoveStatus::Reached);
    REQUIRE(ObserveCreationMove(true, before, target, 5.0) == CreationMoveStatus::TimedOut);
    REQUIRE(ObserveCreationMove(false, target, target, 5.0) == CreationMoveStatus::TimedOut);
    REQUIRE(ObserveCreationMove(true, {std::numeric_limits<float>::quiet_NaN(), 0.f, 0.f}, target, 0.0) == CreationMoveStatus::InvalidPosition);
}
