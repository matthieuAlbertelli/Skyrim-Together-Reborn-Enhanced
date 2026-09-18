#include <CharacterCreation/SexProbeWindow.h>
#include <catch2/catch.hpp>
#include <stdexcept>
using namespace STRE::CharacterCreation;
TEST_CASE("Sex probe detects each transition once and resets chronology", "[sex-probe]")
{
    SexProbeWindow w;
    REQUIRE(w.Observe(1, 0, 10).Baseline);
    REQUIRE_FALSE(w.Observe(2, 0, 10).Significant);
    auto change = w.Observe(3, 1, 11);
    REQUIRE(change.SexChanged);
    REQUIRE(change.From == 0);
    REQUIRE(w.Start == 3);
    REQUIRE(w.SexObserved);
    REQUIRE(w.SexTick == 3);
    w.Arm(4);
    REQUIRE(w.SexTick == 3);
    REQUIRE_FALSE(w.Observe(4, 1, 11).SexChanged);
    REQUIRE(w.Active(183));
    REQUIRE_FALSE(w.Active(185));
    REQUIRE(w.Observe(185, 0, 12).SexChanged);
    w.Reset();
    REQUIRE_FALSE(w.Observed);
    REQUIRE_FALSE(w.Active(186));
    REQUIRE(w.Observe(187, 1, 12).Baseline);
}
TEST_CASE("Sex probe runtime classification and bounded call window", "[sex-probe]")
{
    REQUIRE(SexProbeRuntime("1.6.1170.0"));
    REQUIRE_FALSE(SexProbeRuntime("1.6.640.0"));
    REQUIRE(SexProbePlayer(14, 14));
    REQUIRE_FALSE(SexProbePlayer(15, 14));
    REQUIRE_FALSE(SexProbePlayer(0, 0));
    SexProbeWindow w;
    w.Arm(9);
    REQUIRE(w.Active(189));
    REQUIRE_FALSE(w.Active(190));
    w.Arm(20);
    REQUIRE(w.Active(200));
    REQUIRE_FALSE(w.Active(201));
}
TEST_CASE("Passive reset forwarding calls original exactly once despite diagnostic exceptions", "[sex-probe]")
{
    int calls{};
    bool argument = true;
    PassiveSexProbeCall(
        [] { throw std::runtime_error("diagnostic"); },
        [&]
        {
            ++calls;
            REQUIRE(argument);
        },
        [] { throw std::runtime_error("diagnostic"); });
    REQUIRE(calls == 1);
    REQUIRE_THROWS(PassiveSexProbeCall(
        [] {},
        [&]
        {
            ++calls;
            throw std::runtime_error("engine");
        },
        [] { FAIL("after must not run"); }));
    REQUIRE(calls == 2);
}
