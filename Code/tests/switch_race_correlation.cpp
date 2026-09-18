#include <CharacterCreation/SwitchRaceCorrelation.h>
#include <catch2/catch.hpp>
using STRE::CharacterCreation::SwitchRaceCorrelation;

TEST_CASE("SwitchRace correlation distinguishes in-call and returned observations", "[appearance-probe]")
{
    SwitchRaceCorrelation history;
    REQUIRE(history.Latest(42).Sequence == 0);
    const auto sequence = history.Enter(42);
    REQUIRE(sequence == 1);
    REQUIRE(history.Latest(42).InFlight);
    REQUIRE(history.Latest(7).Sequence == 0);
    history.Return(sequence);
    REQUIRE(history.Latest(42).Sequence == sequence);
    REQUIRE_FALSE(history.Latest(42).InFlight);
}

TEST_CASE("SwitchRace correlation keeps actors separate and labels latest nested call", "[appearance-probe]")
{
    SwitchRaceCorrelation history;
    const auto outer = history.Enter(42);
    const auto other = history.Enter(7);
    const auto inner = history.Enter(42);
    REQUIRE(inner > other);
    REQUIRE(other > outer);
    history.Return(inner);
    REQUIRE(history.Latest(42).Sequence == inner);
    REQUIRE_FALSE(history.Latest(42).InFlight);
    REQUIRE(history.Latest(7).Sequence == other);
    REQUIRE(history.Latest(7).InFlight);
    history.Return(outer);
    REQUIRE(history.Latest(42).Sequence == inner);
}

TEST_CASE("SwitchRace correlation storage is bounded and old associations expire", "[appearance-probe]")
{
    SwitchRaceCorrelation history;
    const auto old = history.Enter(1);
    for (uintptr_t i = 2; i <= 65; ++i)
        history.Enter(i);
    history.Return(old);
    REQUIRE(history.Latest(1).Sequence == 0);
    REQUIRE(history.Latest(65).Sequence == 65);
    REQUIRE(history.Latest(65).InFlight);
    REQUIRE(history.Latest(0).Sequence == 0);
}
