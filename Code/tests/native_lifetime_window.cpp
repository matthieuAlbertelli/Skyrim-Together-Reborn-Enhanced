#include <CharacterCreation/NativeLifetimeWindow.h>
#include <catch2/catch.hpp>

using STRE::CharacterCreation::NativeLifetimeWindow;

TEST_CASE("Native lifetime polling is throttled without catch-up bursts", "[native-lifetime]")
{
    NativeLifetimeWindow window;
    REQUIRE_FALSE(window.PollDue(0));
    REQUIRE_FALSE(window.PollDue(99));
    REQUIRE(window.PollDue(100));
    REQUIRE_FALSE(window.PollDue(100));
    REQUIRE_FALSE(window.PollDue(199));
    REQUIRE(window.PollDue(200));
    REQUIRE(window.PollDue(10000));
    REQUIRE_FALSE(window.PollDue(10000));
    REQUIRE_FALSE(window.PollDue(10099));
    REQUIRE(window.PollDue(10100));
}

TEST_CASE("Native lifetime observation ends even when removal never arrives", "[native-lifetime]")
{
    NativeLifetimeWindow window;
    REQUIRE_FALSE(window.Expired(179999));
    REQUIRE(window.Expired(180000));
    REQUIRE_FALSE(window.PollDue(180000));
    REQUIRE_FALSE(window.PollDue(UINT64_MAX));
    REQUIRE_FALSE(window.RemovalObserved); // Timeout is not an engine event.
}

TEST_CASE("First removal bounds observation and duplicates cannot extend it", "[native-lifetime]")
{
    NativeLifetimeWindow window;
    window.ObserveRemoval(1000);
    window.ObserveRemoval(20000);
    window.ObserveRemoval(30999);
    REQUIRE(window.RemovalMs == 1000);
    REQUIRE_FALSE(window.Expired(30999));
    REQUIRE(window.Expired(31000));
    REQUIRE_FALSE(window.PollDue(31000));
}

TEST_CASE("Removal at creation and near the global deadline stay bounded", "[native-lifetime]")
{
    NativeLifetimeWindow window;
    SECTION("zero is a valid removal timestamp")
    {
        window.ObserveRemoval(0);
        REQUIRE(window.RemovalObserved);
        REQUIRE_FALSE(window.Expired(29999));
        REQUIRE(window.Expired(30000));
    }
    SECTION("late removal cannot extend the global deadline")
    {
        window.ObserveRemoval(179000);
        REQUIRE_FALSE(window.Expired(179999));
        REQUIRE(window.Expired(180000));
    }
}

TEST_CASE("Backward sample times cannot underflow or trigger extra polls", "[native-lifetime]")
{
    NativeLifetimeWindow window;
    REQUIRE(window.PollDue(500));
    window.ObserveRemoval(1000);
    REQUIRE_FALSE(window.Expired(999));
    REQUIRE_FALSE(window.PollDue(0));
    REQUIRE_FALSE(window.PollDue(499));
    REQUIRE_FALSE(window.PollDue(599));
    REQUIRE(window.PollDue(600));
}

TEST_CASE("High frequency service ticks have a finite native lookup budget", "[native-lifetime]")
{
    NativeLifetimeWindow window;
    uint32_t polls = 0;
    for (uint64_t ms = 0; ms <= 200000; ++ms)
        if (window.PollDue(ms))
            ++polls;
    REQUIRE(polls == 1799);
    REQUIRE(window.Expired(200000));
}
