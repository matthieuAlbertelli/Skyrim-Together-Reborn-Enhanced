#include <CharacterCreation/NativeLifetimeBinding.h>
#include <catch2/catch.hpp>
#include <utility>

using namespace STRE::CharacterCreation;

TEST_CASE("Passive current lifetime selection accepts coherent binding without provenance", "[native-lifetime]")
{
    REQUIRE(ClassifyNativeLifetimeBinding(0xFF000D3A, 0xFF0008BD, 0xFF000D3A, false, 0, 0) == NativeLifetimeBindingEvidence::ValidatedCurrentBindingFallback);
    // Recovery recreated the same FormID with a different Base: classify the
    // current pair, never resurrect the previous allocation's provenance.
    REQUIRE(ClassifyNativeLifetimeBinding(0xFF000D3A, 0xFF000D35, 0xFF000D3A, false, 0, 0) == NativeLifetimeBindingEvidence::ValidatedCurrentBindingFallback);
}

TEST_CASE("Passive current lifetime selection never bypasses contradictory provenance", "[native-lifetime]")
{
    constexpr uint32_t actor = 0xFF000D3A, base = 0xFF000D35;
    REQUIRE(ClassifyNativeLifetimeBinding(actor, base, actor, true, actor, base) == NativeLifetimeBindingEvidence::DurableProvenance);
    REQUIRE(ClassifyNativeLifetimeBinding(actor, base, actor, true, actor, 0xFF0008BD) == NativeLifetimeBindingEvidence::Rejected);
    REQUIRE(ClassifyNativeLifetimeBinding(actor, base, actor, true, actor + 1, base) == NativeLifetimeBindingEvidence::Rejected);
    REQUIRE(ClassifyNativeLifetimeBinding(actor, base, actor, true, 0, 0) == NativeLifetimeBindingEvidence::Rejected);
}

TEST_CASE("Passive current lifetime selection rejects non-temporary or inconsistent IDs", "[native-lifetime]")
{
    const bool provenance = GENERATE(false, true);
    const auto ids = GENERATE(
        std::pair<uint32_t, uint32_t>{0, 0xFF000001}, std::pair<uint32_t, uint32_t>{0x14, 0xFF000001}, std::pair<uint32_t, uint32_t>{0xFF000001, 0},
        std::pair<uint32_t, uint32_t>{0xFF000001, 0x00000007}, std::pair<uint32_t, uint32_t>{0xFF000001, 0xFF000001});
    REQUIRE(ClassifyNativeLifetimeBinding(ids.first, ids.second, ids.first, provenance, ids.first, ids.second) == NativeLifetimeBindingEvidence::Rejected);
    REQUIRE(ClassifyNativeLifetimeBinding(0xFF000001, 0xFF000002, 0xFF000003, provenance, 0xFF000001, 0xFF000002) == NativeLifetimeBindingEvidence::Rejected);
}
