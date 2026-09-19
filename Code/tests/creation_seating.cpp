#include <TiltedCore/Serialization.hpp>
#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <CharacterCreation/CreationSeating.h>
#include <CharacterCreation/SeatingEntryEvidence.h>
#include <CharacterCreation/SeatingObservationWindow.h>
#include <CharacterCreation/SeatApproach.h>
#include <CharacterCreation/StandingCreation.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <catch2/catch.hpp>
#include <limits>
using namespace STRE::CharacterCreation;

TEST_CASE("Local marker approach uses the durable rank including a one-member network roster", "[creation-seating]")
{
    const std::array<std::string_view, 1> roster{"authenticated-player"};
    const auto rank = StandingCreationPositionIndex(roster, "authenticated-player");
    REQUIRE(rank);
    REQUIRE(*rank == 0);
    REQUIRE(CreationSeatLocalFormId(*rank) == 0xBF3DD);
    REQUIRE(LocalSeatApproachRejection(true, true, false, *rank) == nullptr);
    REQUIRE_FALSE(StandingCreationPositionIndex(roster, "another-player"));
    const std::array<std::string_view, 2> duplicate{"authenticated-player", "authenticated-player"};
    REQUIRE_FALSE(StandingCreationPositionIndex(duplicate, "authenticated-player"));

    // The same local scope never substitutes Seat01 for a different durable rank.
    const std::array<std::string_view, 2> twoPlayers{"b", "a"};
    const auto rankB = StandingCreationPositionIndex(twoPlayers, "b");
    REQUIRE(rankB);
    REQUIRE(CreationSeatLocalFormId(*rankB) == 0xBF3DC);
    REQUIRE(*rankB == 1);
    REQUIRE(CreationMarkerLocalFormId(*rankB) == 0xD6B09);
    REQUIRE(LocalSeatApproachRejection(true, true, false, *rankB) == nullptr);
    for (size_t index = 0; index < 10; ++index)
        REQUIRE(LocalSeatApproachRejection(true, true, false, index) == nullptr);
    REQUIRE(LocalSeatApproachRejection(true, true, false, 10) != nullptr);
}

TEST_CASE("Local approach scope rejects remote actors and intents without local finalization", "[creation-seating]")
{
    REQUIRE(std::string_view(LocalSeatApproachRejection(false, true, false, 0)) == "approach-native-local-player-only");
    REQUIRE(std::string_view(LocalSeatApproachRejection(true, false, false, 0)) == "approach-native-local-player-only");
    REQUIRE(std::string_view(LocalSeatApproachRejection(true, true, true, 0)) == "approach-native-local-player-only");
    // Connection mode adds no separate geometry or ordering: both paths use
    // LocalSeatApproach, whose arrival/update/idempotence cases follow below.
    REQUIRE(LocalSeatApproachRejection(true, true, false, 0) == nullptr);
    REQUIRE(LocalSeatApproachRejection(false, true, false, 1) != nullptr);
    REQUIRE(LocalSeatApproachRejection(true, false, false, 1) != nullptr);
    REQUIRE(LocalSeatApproachRejection(true, true, true, 1) != nullptr);
}

TEST_CASE("Existing CK marker and seat pairs follow all ten durable ranks", "[creation-seating]")
{
    const std::array<std::string_view, 10> roster{"j", "b", "d", "a", "h", "f", "c", "i", "e", "g"};
    for (const auto player : roster)
    {
        const auto rank = StandingCreationPositionIndex(roster, player);
        REQUIRE(rank);
        const auto binding = ResolveSeatApproachBinding(*rank);
        REQUIRE(binding);
        REQUIRE(binding->Seat == CreationSeatLocalFormId(*rank));
        REQUIRE(binding->Marker == CreationMarkerLocalFormId(*rank));
        REQUIRE(binding->Marker != binding->Seat);
        REQUIRE(*rank == static_cast<size_t>(player[0] - 'a'));
        REQUIRE(LocalSeatApproachRejection(true, true, false, *rank) == nullptr);
        REQUIRE(LocalSeatApproachRejection(true, true, true, *rank) != nullptr);
    }
    REQUIRE_FALSE(ResolveSeatApproachBinding(10));
    REQUIRE_FALSE(ResolveSeatApproachBinding(std::numeric_limits<size_t>::max()));
}

TEST_CASE("Approach target preserves the exact CK marker position and orientation", "[creation-seating]")
{
    const glm::vec3 position{123.f, -456.f, 78.f}, rotation{0.f, 0.f, -1.2f};
    const auto target = SeatApproachMarkerTarget(position, rotation);
    REQUIRE(target);
    REQUIRE(target->Position == position);
    REQUIRE(target->Rotation == rotation);
    REQUIRE_FALSE(SeatApproachMarkerTarget({std::numeric_limits<float>::quiet_NaN(), 0.f, 0.f}, rotation));
    REQUIRE_FALSE(SeatApproachMarkerTarget(position, {0.f, 0.f, std::numeric_limits<float>::infinity()}));
    REQUIRE_FALSE(SeatApproachMarkerTarget(position, {0.1f, 0.f, 0.f}));
    REQUIRE_FALSE(SeatApproachMarkerTarget(position, {0.f, 0.1f, 0.f}));
}

TEST_CASE("Local marker approach observes arrival and a later engine update after facing before activating", "[creation-seating]")
{
    LocalSeatApproach approach;
    const SeatApproachTarget target{{10.f, 20.f, 0.f}, {0.f, 0.f, 1.2f}};
    approach.Start(target, 4, 1000);
    REQUIRE(approach.Observe(4, 1001, true, true, target.Position, target.Rotation) == SeatApproachAction::Wait);
    REQUIRE(approach.Observe(5, 1010, true, true, {}, {}) == SeatApproachAction::Wait);
    REQUIRE(approach.Observe(6, 1020, true, false, target.Position, {}) == SeatApproachAction::Wait);
    REQUIRE(approach.Observe(7, 1030, true, true, target.Position, {}) == SeatApproachAction::ApplyFacing);
    REQUIRE(approach.Observe(7, 1031, true, true, target.Position, target.Rotation) == SeatApproachAction::Wait);
    REQUIRE(approach.Observe(8, 1040, true, true, target.Position, target.Rotation) == SeatApproachAction::Activate);
    REQUIRE(approach.Observe(9, 1050, true, true, target.Position, target.Rotation) == SeatApproachAction::Wait);
}

TEST_CASE("Local marker approach fails closed on lost binding timeout drift or facing failure without retry", "[creation-seating]")
{
    const SeatApproachTarget target{{10.f, 20.f, 0.f}, {0.f, 0.f, 1.2f}};
    SECTION("native move remains queued or wrong cell")
    {
        LocalSeatApproach approach;
        approach.Start(target, 1, 1000);
        REQUIRE(approach.Observe(2, 5999, true, false, target.Position, target.Rotation) == SeatApproachAction::Wait);
        REQUIRE(approach.Observe(3, 6000, true, false, target.Position, target.Rotation) == SeatApproachAction::Reject);
        REQUIRE(std::string_view(approach.Failure) == "approach-arrival-or-facing-timeout");
        REQUIRE(approach.Observe(4, 6010, true, true, target.Position, target.Rotation) == SeatApproachAction::Reject);
    }
    SECTION("native identity changed")
    {
        LocalSeatApproach approach;
        approach.Start(target, 1, 1000);
        REQUIRE(approach.Observe(2, 1010, false, true, target.Position, target.Rotation) == SeatApproachAction::Reject);
        REQUIRE(std::string_view(approach.Failure) == "approach-binding-changed");
    }
    SECTION("position drift after facing")
    {
        LocalSeatApproach approach;
        approach.Start(target, 1, 1000);
        REQUIRE(approach.Observe(2, 1010, true, true, target.Position, {}) == SeatApproachAction::ApplyFacing);
        REQUIRE(approach.Observe(3, 1020, true, true, {}, target.Rotation) == SeatApproachAction::Reject);
        REQUIRE(std::string_view(approach.Failure) == "approach-position-lost-after-facing");
    }
    SECTION("rotation was not actually applied")
    {
        LocalSeatApproach approach;
        approach.Start(target, 1, 1000);
        REQUIRE(approach.Observe(2, 1010, true, true, target.Position, {}) == SeatApproachAction::ApplyFacing);
        REQUIRE(approach.Observe(3, 1020, true, true, target.Position, {}) == SeatApproachAction::Reject);
        REQUIRE(std::string_view(approach.Failure) == "approach-facing-not-observed");
    }
}

TEST_CASE("Solo capture covers the 30.75 second runtime entry and at least 40 seconds", "[creation-seating]")
{
    constexpr int64_t start = 1000, enter = start + 30750;
    REQUIRE(SeatingObservationWindow::Active(start + 10001, start, 0));
    REQUIRE(SeatingObservationWindow::Active(enter, start, 0));
    REQUIRE(SeatingObservationWindow::Active(enter + 5000, start, enter));
    REQUIRE(SeatingObservationWindow::Deadline(start, enter) == start + 40000);
    REQUIRE(SeatingObservationWindow::Active(start + 40000, start, enter));
    REQUIRE_FALSE(SeatingObservationWindow::Active(start + 40001, start, enter));
}

TEST_CASE("Late Enter extends or reopens capture without reactivating the chair", "[creation-seating]")
{
    constexpr int64_t start = 1000;
    for (const auto elapsed : {39000, 40000, 45000})
    {
        const auto enter = start + elapsed;
        REQUIRE(SeatingObservationWindow::Deadline(start, enter) == enter + 5000);
        REQUIRE(SeatingObservationWindow::Active(enter + 4999, start, enter));
        REQUIRE(SeatingObservationWindow::Active(enter + 5000, start, enter));
        REQUIRE_FALSE(SeatingObservationWindow::Active(enter + 5001, start, enter));
    }
    REQUIRE_FALSE(SeatingObservationWindow::Active(1000, 0, 0));
    REQUIRE_FALSE(SeatingObservationWindow::Active(start + 40001, start, 0));
    SeatProjection p;
    REQUIRE(p.Observe(0x14, 11, true, false, false, false, false) == SeatAction::Activate);
    p.Issued = true;
    REQUIRE(p.Observe(0x14, 11, true, false, false, false, false) == SeatAction::AwaitEntry);
    REQUIRE(p.Observe(0x14, 11, true, true, true, false, true) == SeatAction::Complete);
    // Diagnostic duration is independent of the terminal projection state.
    REQUIRE(SeatingObservationWindow::Active(start + 43000, start, start + 39000));
}

TEST_CASE("Animation trace budget renews after early movement spam", "[creation-seating]")
{
    SeatingActionTraceBudget budget;
    for (int64_t second = 1; second <= 40; ++second)
    {
        for (uint64_t n = 0; n < SeatingActionTraceBudget::PerSecond; ++n)
            REQUIRE(budget.Admit(second * 1000));
        REQUIRE_FALSE(budget.Admit(second * 1000 + 1));
    }
    budget.Reset(); // first Enter gets a fresh budget even in a saturated second
    REQUIRE(budget.Admit(40010));
    REQUIRE(budget.Admit(41000));
}

TEST_CASE("Solo logical furniture state never certifies animation entry", "[creation-seating]")
{
    SeatingEntryEvidence e;
    e.CorrectFurniture = e.LogicalSitState = true;
    REQUIRE(e.LogicalSeated());
    REQUIRE_FALSE(e.AnimationObserved());
    SeatProjection p;
    REQUIRE(p.Observe(0x14, 11, true, true, e.AnimationObserved(), false, true) == SeatAction::AwaitEntry);
    REQUIRE_FALSE(p.Completed);
    REQUIRE_FALSE(p.Issued); // an occupied chair must not be activated again
    e.RootReady = e.GraphReady = e.EnterEvent = true;
    REQUIRE_FALSE(e.AnimationObserved()); // event/root alone cannot prove animation
    e.FurnitureVariableValid = e.InFurniture = true;
    REQUIRE_FALSE(e.AnimationObserved()); // unknown sitting graph variable
    e.SittingVariableValid = true;
    REQUIRE_FALSE(e.AnimationObserved()); // explicitly false isIdleSitting
    e.IdleSitting = true;
    REQUIRE(e.AnimationObserved());
    REQUIRE(p.Observe(0x14, 11, true, true, e.AnimationObserved(), false, true) == SeatAction::Complete);
}

TEST_CASE("Solo animation evidence fails closed on every missing observation", "[creation-seating]")
{
    const SeatingEntryEvidence complete{true, true, true, true, true, true, true, true, true};
    const std::array<bool SeatingEntryEvidence::*, 9> fields{
        &SeatingEntryEvidence::CorrectFurniture, &SeatingEntryEvidence::LogicalSitState,
        &SeatingEntryEvidence::RootReady, &SeatingEntryEvidence::GraphReady, &SeatingEntryEvidence::EnterEvent,
        &SeatingEntryEvidence::FurnitureVariableValid, &SeatingEntryEvidence::InFurniture,
        &SeatingEntryEvidence::SittingVariableValid, &SeatingEntryEvidence::IdleSitting};
    for (const auto field : fields)
    {
        auto missing = complete;
        missing.*field = false;
        REQUIRE_FALSE(missing.AnimationObserved());
    }
    SeatProjection p;
    REQUIRE(p.Observe(0x14, 11, true, true, complete.AnimationObserved(), false, true) == SeatAction::Complete);
    REQUIRE(p.Observe(0x14, 11, true, false, false, false, false) == SeatAction::Complete);
    REQUIRE(p.Observe(0x14, 22, true, true, false, false, true) == SeatAction::AwaitEntry);
    REQUIRE_FALSE(p.Completed); // old token's completion is not reused
}

TEST_CASE("Individual seating never waits for another build", "[creation-seating]")
{
    SeatProjection a, b;
    REQUIRE(a.Observe(1, 11, true, false, false, false, false) == SeatAction::Activate);
    a.Issued = true;
    REQUIRE(b.Observe(0, 0, false, false, false, false, false) == SeatAction::Pending);
    REQUIRE(a.Observe(1, 11, true, false, false, false, true) == SeatAction::AwaitEntry);
    REQUIRE(a.Observe(1, 11, true, true, true, false, true) == SeatAction::Complete);
    REQUIRE(b.Observe(2, 22, true, false, false, false, false) == SeatAction::Activate);
    REQUIRE(a.Observe(3, 33, true, false, false, false, false) == SeatAction::Activate);
    REQUIRE_FALSE(a.Issued);
}
TEST_CASE("Seating occupation is fail closed and correct occupant is idempotent", "[creation-seating]")
{
    SeatProjection p;
    REQUIRE(p.Observe(1, 11, true, false, false, false, true) == SeatAction::Conflict);
    REQUIRE(p.Observe(1, 11, true, false, false, true, false) == SeatAction::Conflict);
    REQUIRE(p.Observe(1, 11, true, true, true, false, true) == SeatAction::Complete);
    REQUIRE_FALSE(p.Issued);
    REQUIRE(p.Observe(1, 11, true, false, false, false, false) == SeatAction::Complete);
    SeatProjection entering;
    REQUIRE(entering.Observe(1, 11, true, true, false, false, true) == SeatAction::AwaitEntry);
}
TEST_CASE("Creation seat mapping shares the durable rank", "[creation-seating]")
{
    REQUIRE(CreationSeatLocalFormId(0) == 0xBF3DD);
    REQUIRE(CreationSeatLocalFormId(1) == 0xBF3DC);
    REQUIRE(CreationSeatLocalFormId(9) == 0xC5178);
    REQUIRE_FALSE(CreationSeatLocalFormId(10));
    const std::array<std::string_view, 2> roster{"b", "a"}, reversed{"a", "b"};
    REQUIRE(CreationSeatLocalFormId(*StandingCreationPositionIndex(roster, "a")) == CreationSeatLocalFormId(*StandingCreationPositionIndex(reversed, "a")));
    REQUIRE_FALSE(StandingCreationPositionIndex(roster, "missing"));
}
TEST_CASE("Build seating identity is bounded and survives wire roundtrip", "[creation-seating]")
{
    NotifyCharacterBuildState sent;
    sent.State = CharacterBuildNetworkState::Applied;
    sent.PlayerId = 4;
    sent.ServerId = 7;
    sent.Revision = 2;
    sent.SeatingCampaignId = "campaign";
    sent.SeatingPlayerId = "durable-player";
    TiltedPhoques::Buffer buffer(65536);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    sent.SerializeRaw(writer);
    TiltedPhoques::Buffer::Reader reader(&buffer);
    NotifyCharacterBuildState received;
    received.DeserializeRaw(reader);
    REQUIRE(received.SeatingCampaignId == sent.SeatingCampaignId);
    REQUIRE(received.SeatingPlayerId == sent.SeatingPlayerId);
    REQUIRE(received.PlayerId == 4);
}

TEST_CASE("Seating identity rejects oversized output", "[creation-seating]")
{
    NotifyCharacterBuildState sent;
    sent.SeatingCampaignId = "campaign";
    sent.SeatingPlayerId = String(129, 'x');
    TiltedPhoques::Buffer buffer(65536);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    sent.SerializeRaw(writer);
    TiltedPhoques::Buffer::Reader reader(&buffer);
    NotifyCharacterBuildState decoded;
    decoded.DeserializeRaw(reader);
    REQUIRE(decoded.SeatingPlayerId.empty());


}
