#include <CharacterCreation/SexRebuildDiff.h>
#include <CharacterCreation/SexProbeWindow.h>
#include <catch2/catch.hpp>
#include <stdexcept>
using namespace STRE::CharacterCreation;
TEST_CASE("Sex rebuild execution requires positive matching-actor evidence", "[sex-rebuild-diff]")
{
    SexRebuildExecution frame{100, 42};
    REQUIRE(std::string(frame.Result()) == "unresolved");
    frame.Observe(200, true);
    frame.Observe(200, false);
    REQUIRE(std::string(frame.Result()) == "unresolved");
    SECTION("queue")
    {
        frame.Observe(100, true);
        REQUIRE(std::string(frame.Result()) == "queued-op1C");
    }
    SECTION("inline")
    {
        frame.Observe(100, false);
        REQUIRE(std::string(frame.Result()) == "inline-AIProcess");
    }
    SECTION("ambiguous")
    {
        frame.Observe(100, true);
        frame.Observe(100, false);
        REQUIRE(std::string(frame.Result()) == "unresolved");
    }
    REQUIRE(frame.Sequence == 42);
}
TEST_CASE("Sex rebuild diagnostic failures preserve one native invocation", "[sex-rebuild-diff]")
{
    unsigned calls{};
    PassiveSexProbeCall([] { throw std::runtime_error("entry"); }, [&] { ++calls; }, [] { throw std::runtime_error("return"); });
    REQUIRE(calls == 1);
    REQUIRE_THROWS_AS(
        PassiveSexProbeCall(
            [] {},
            [&]
            {
                ++calls;
                throw std::logic_error("native");
            },
            [] {}),
        std::logic_error);
    REQUIRE(calls == 2);
}
