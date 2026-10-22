#include "../client/MainMenu/NativeAdapter.h"
#include <catch2/catch.hpp>
#include <vector>

using namespace STRE::MainMenu;

TEST_CASE("Presentation native admission is exact and independent", "[main-menu][runtime-port]")
{
    REQUIRE(SupportsPresentation("1.7.104.0"));
    for (auto version : {"", "1.6.1170.0", "1.7.99.0", "1.7.104", "1.7.105.0", "1.7.104.1"})
        REQUIRE_FALSE(SupportsPresentation(version));
    using namespace STRE::Runtime;
    auto contracts = NativeContracts;
    contracts[static_cast<std::size_t>(NativeContract::MenuPresentation)].Available = NativeEvidence::Missing;
    REQUIRE(NativeAdmissionReady(contracts)); // optional presentation is not a campaign bypass
    contracts[static_cast<std::size_t>(NativeContract::LoadFunction)].Available = NativeEvidence::Missing;
    REQUIRE_FALSE(NativeAdmissionReady(contracts));
}

TEST_CASE("Presentation music call validation rejects changed bytes and unreachable hooks", "[main-menu]")
{
    const auto site = GENERATE(0u, 1u);
    const std::array<std::uintptr_t, 2> sites{0x14095820A, 0x14095A26D};
    constexpr std::uintptr_t original = 0x14095EA10;
    const auto address = sites[site];
    auto bytes = cMusicCalls[site];
    std::int32_t displacement = 42;
    REQUIRE(PrepareMusicCall(site, bytes, address, original, address + 105, displacement));
    REQUIRE(displacement == 100);
    for (std::size_t i = 0; i < bytes.size(); ++i)
    {
        auto altered = bytes;
        altered[i] ^= 1;
        REQUIRE_FALSE(PrepareMusicCall(site, altered, address, original, address + 105, displacement));
        REQUIRE(displacement == 100);
    }
    REQUIRE_FALSE(PrepareMusicCall(site, {bytes.data(), 4}, address, original, address, displacement));
    REQUIRE_FALSE(PrepareMusicCall(2, bytes, address, original, address, displacement));
    REQUIRE_FALSE(PrepareMusicCall(site, bytes, address, original + 1, address, displacement));
    REQUIRE_FALSE(PrepareMusicCall(site, bytes, address, original, 0, displacement));
    REQUIRE_FALSE(PrepareMusicCall(site, bytes, address, original, address + 5 + 0x80000000ull, displacement));
    REQUIRE_FALSE(PrepareMusicCall(site, bytes, address, original, address + 5 - 0x80000001ull, displacement));
    REQUIRE(PrepareMusicCall(site, bytes, address, original, address + 5 + 0x7FFFFFFFull, displacement));
    REQUIRE(displacement == std::numeric_limits<std::int32_t>::max());
    REQUIRE(PrepareMusicCall(site, bytes, address, original, address + 5 - 0x80000000ull, displacement));
    REQUIRE(displacement == std::numeric_limits<std::int32_t>::min());
}

TEST_CASE("Presentation write permissions are transactional and originals survive rejection", "[main-menu]")
{
    // Same transaction as the live adapter, without any Skyrim address or call.
    const auto failAt = GENERATE(0u, 1u, 2u, 3u, 4u);
    const std::array<std::uintptr_t, 5> original{101, 202, 303, 404, 505};
    auto sites = original;
    std::array<std::uintptr_t, 5> saved{};
    std::array<bool, 4> writable{};
    std::vector<std::size_t> restored;
    unsigned commits{};
    const bool installed = WithWritablePatches(writable.size(),
        [&](std::size_t i) { if (i == failAt) return false; writable[i] = true; return true; },
        [&](std::size_t i) { writable[i] = false; restored.push_back(i); },
        [&]() { ++commits; saved = sites; sites = {11, 22, 33, 44, 55}; });
    REQUIRE(installed == (failAt == 4));
    REQUIRE(commits == (installed ? 1 : 0));
    REQUIRE(writable == std::array<bool, 4>{});
    REQUIRE(restored.size() == failAt);
    for (std::size_t i = 0; i < restored.size(); ++i)
        REQUIRE(restored[i] == failAt - 1 - i);
    if (installed)
    {
        REQUIRE(saved == original);
        REQUIRE(sites != original);
    }
    else
    {
        REQUIRE(sites == original);
        REQUIRE(saved == std::array<std::uintptr_t, 5>{});
    }
    REQUIRE_FALSE(WithWritablePatches(0, [](auto) { return true; }, [](auto) {}, [&]() { ++commits; }));
    REQUIRE(commits == (installed ? 1 : 0));
}
