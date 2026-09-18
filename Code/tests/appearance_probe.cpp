#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>
#include <optional>
#include <CharacterCreation/AppearanceProbe.h>
#include <catch2/catch.hpp>

using namespace STRE::CharacterCreation;

namespace
{
RequestCharacterAppearanceUpdate Final(const char* apBytes)
{
    RequestCharacterAppearanceUpdate snapshot;
    snapshot.AppearanceBuffer = apBytes;
    snapshot.Descriptor = {GameId{0, 0x13746}, 0, 50.f};
    return snapshot;
}
} // namespace

TEST_CASE("Appearance provenance checks both temporary actor and private base identity", "[appearance-probe]")
{
    const AppearanceBaseIdentity provenance{0xFF001234, 0xFF005678};
    REQUIRE(IsPrivatePlayerCreation(true, true, provenance.ActorFormId, provenance.BaseFormId));
    REQUIRE_FALSE(IsPrivatePlayerCreation(false, true, provenance.ActorFormId, provenance.BaseFormId));
    REQUIRE_FALSE(IsPrivatePlayerCreation(true, false, provenance.ActorFormId, provenance.BaseFormId));
    REQUIRE_FALSE(IsPrivatePlayerCreation(true, true, 0x14, provenance.BaseFormId));
    REQUIRE_FALSE(IsPrivatePlayerCreation(true, true, provenance.ActorFormId, 0x1234));
    REQUIRE(provenance.Matches(0xFF001234, 0xFF005678));
    REQUIRE_FALSE(provenance.Matches(0xFF001235, 0xFF005678));
    REQUIRE_FALSE(provenance.Matches(0xFF001234, 0xFF005679));
    REQUIRE_FALSE(AppearanceBaseIdentity{0x14, 0xFF005678}.Matches(0x14, 0xFF005678));
    REQUIRE_FALSE(AppearanceBaseIdentity{0xFF001234, 0x1234}.Matches(0xFF001234, 0x1234));
    REQUIRE_FALSE(AppearanceBaseIdentity{}.Matches(0xFF001234, 0xFF005678));
}

TEST_CASE("Appearance final waits for assignment and sends exactly once", "[appearance-probe]")
{
    PendingAppearanceFinal pending;
    int sent{};
    auto send = [&](const RequestCharacterAppearanceUpdate& acSnapshot)
    {
        ++sent;
        REQUIRE(acSnapshot.ActorId == 7);
        REQUIRE(acSnapshot.AppearanceBuffer == "latest");
        return true;
    };
    pending.Queue(true, Final("old"));
    REQUIRE_FALSE(pending.Flush(true, std::nullopt, send));
    pending.Queue(true, Final("latest"));
    REQUIRE(pending.Flush(true, 7, send));
    REQUIRE_FALSE(pending.Flush(true, 7, send));
    REQUIRE(sent == 1);
    REQUIRE_FALSE(pending.Snapshot);
}

TEST_CASE("Appearance final drops offline and recovery state and retains rejected sends", "[appearance-probe]")
{
    PendingAppearanceFinal pending;
    int sends{};
    auto send = [&](const RequestCharacterAppearanceUpdate&)
    {
        ++sends;
        return false;
    };
    pending.Queue(false, Final("solo"));
    REQUIRE_FALSE(pending.Flush(false, 7, send));
    REQUIRE(sends == 0);
    pending.Queue(true, Final("online"));
    REQUIRE_FALSE(pending.Flush(true, 7, send));
    REQUIRE(pending.Snapshot);
    REQUIRE_FALSE(pending.Flush(false, 7, send));
    REQUIRE_FALSE(pending.Snapshot);
    REQUIRE(sends == 1);
    pending.Queue(true, Final("recovery"));
    pending.Reset();
    REQUIRE_FALSE(pending.Flush(true, 7, send));
    REQUIRE(sends == 1);
}

TEST_CASE("Appearance probe coalesces without rearming and stops after 120 ticks", "[appearance-probe]")
{
    AppearanceProbe probe;
    NotifyCharacterAppearanceUpdate snapshot;
    snapshot.AppearanceBuffer = "old";
    probe.Receive(snapshot);
    REQUIRE(probe.Start({1, 2, 3}));
    for (uint32_t tick = 1; tick <= AppearanceProbe::MaxTicks; ++tick)
    {
        snapshot.AppearanceBuffer = "latest";
        probe.Receive(snapshot);
        REQUIRE_FALSE(probe.Start({4, 5, 6}));
        probe.Observe({1, 2, 3});
        REQUIRE(probe.Ticks == tick);
        REQUIRE(probe.Complete == (tick == AppearanceProbe::MaxTicks));
    }
    REQUIRE_FALSE(probe.SawTransition);
    REQUIRE(probe.Latest.AppearanceBuffer == "latest");
    probe.Receive(snapshot);
    probe.Observe({7, 8, 9});
    REQUIRE_FALSE(probe.Start({7, 8, 9}));
    REQUIRE(probe.Ticks == 120);
    REQUIRE_FALSE(probe.SawTransition);
}

TEST_CASE("Appearance probe records null return and pointer transitions without claiming readiness", "[appearance-probe]")
{
    AppearanceProbe probe;
    REQUIRE(probe.Start({1, 2, 3}));
    probe.Observe({0, 0, 0});
    REQUIRE(probe.SawNull);
    REQUIRE(probe.SawTransition);
    REQUIRE(probe.FirstTransitionTick == 1);
    probe.Observe({1, 2, 3});
    REQUIRE(probe.SawReturn);
    REQUIRE_FALSE(probe.Complete);
    probe.Observe({1, 2, 9});
    REQUIRE(probe.Previous.Head == 9);
    REQUIRE(probe.FirstTransitionTick == 1);
}

TEST_CASE("Appearance probe initial wait is bounded and new actor binding retains only latest payload", "[appearance-probe]")
{
    AppearanceProbe probe;
    NotifyCharacterAppearanceUpdate snapshot;
    snapshot.AppearanceBuffer = "pending";
    probe.Receive(snapshot);
    for (uint32_t i = 0; i < AppearanceProbe::MaxTicks; ++i)
        probe.Observe({});
    REQUIRE(probe.Complete);
    REQUIRE_FALSE(probe.Start({1, 2, 3}));
    probe.Rebind();
    REQUIRE(probe.Latest.AppearanceBuffer == "pending");
    REQUIRE_FALSE(probe.Complete);
    REQUIRE(probe.Ticks == 0);
    REQUIRE(probe.Start({4, 5, 6}));
    AppearanceProbe delayed;
    for (uint32_t i = 0; i < 60; ++i)
        delayed.Observe({});
    REQUIRE(delayed.Start({1, 2, 3}));
    for (uint32_t i = 0; i < 60; ++i)
        delayed.Observe({1, 2, 3});
    REQUIRE(delayed.Complete);
    REQUIRE(delayed.Ticks == 120);
}
