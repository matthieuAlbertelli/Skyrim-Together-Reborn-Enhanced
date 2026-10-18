#include <TiltedCore/Serialization.hpp>
#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <CharacterCreation/CreationSeating.h>
#include <CharacterCreation/SeatingEntryEvidence.h>
#include <CharacterCreation/SeatingObservationWindow.h>
#include <CharacterCreation/SeatingIntentRegistry.h>
#include <CharacterCreation/SeatingLifetime.h>
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

TEST_CASE("Local observation waits at most InitialMs without Enter", "[creation-seating]")
{
    constexpr int64_t start = 1000;
    REQUIRE(SeatingObservationWindow::Deadline(start, 0) == start + SeatingObservationWindow::InitialMs);
    REQUIRE(SeatingObservationWindow::Active(start + 40000, start, 0));
    REQUIRE_FALSE(SeatingObservationWindow::Active(start + 40001, start, 0));
    REQUIRE_FALSE(SeatingObservationWindow::Active(start - 1, start, 0));
    REQUIRE_FALSE(SeatingObservationWindow::Active(start, 0, 0));
}

TEST_CASE("Early and late Enter each leave only AfterEnterMs of local observation", "[creation-seating]")
{
    constexpr int64_t start = 1000;
    // Includes the old +30.75s diagnostic and an Enter on the initial deadline.
    for (const auto elapsed : {1, 1000, 30750, 39000, 40000})
    {
        const auto enter = start + elapsed;
        REQUIRE(SeatingObservationWindow::Deadline(start, enter) == enter + 5000);
        REQUIRE(SeatingObservationWindow::Active(enter + 4999, start, enter));
        REQUIRE(SeatingObservationWindow::Active(enter + 5000, start, enter));
        REQUIRE_FALSE(SeatingObservationWindow::Active(enter + 5001, start, enter));
    }
    REQUIRE(SeatingObservationWindow::Deadline(start, start + 1000) < start + SeatingObservationWindow::InitialMs);
    REQUIRE(SeatingObservationWindow::Deadline(start, start + 39000) > start + SeatingObservationWindow::InitialMs);
}

TEST_CASE("Observation closes once with no periodic samples or late Enter reopening", "[creation-seating]")
{
    constexpr int64_t start = 1000;
    const auto enter = GENERATE(int64_t{0}, int64_t{2000}, int64_t{40000});
    const auto deadline = SeatingObservationWindow::Deadline(start, enter);
    SeatingObservationWindow window;
    unsigned samples{}, endings{}, samplesAfterEnd{};
    for (auto now = start; now <= deadline + 2000; ++now)
    {
        // Events are not visible before their actual timestamp.
        const auto action = window.Poll(now, start, now >= enter ? enter : 0);
        if (action == SeatingObservationAction::Sample) ++samples;
        if (action == SeatingObservationAction::End) ++endings;
        if (now > deadline && action == SeatingObservationAction::Sample) ++samplesAfterEnd;
    }
    REQUIRE(samples == static_cast<unsigned>((deadline - start) / SeatingObservationWindow::SampleMs + 1));
    REQUIRE(endings == 1);
    REQUIRE(samplesAfterEnd == 0);
    REQUIRE(window.Closed);
    // A callback after closure cannot extend an already terminal observation.
    REQUIRE(window.Poll(deadline + 3000, start, deadline + 3000) == SeatingObservationAction::None);
    REQUIRE(window.Poll(deadline + 9000, start, deadline + 3000) == SeatingObservationAction::None);
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

TEST_CASE("Local entry requires composite native evidence and not logical state or Enter alone", "[creation-seating]")
{
    SeatingEntryEvidence e;
    e.IdentityMatches = true;
    e.CorrectFurniture = e.LogicalSitState = true;
    REQUIRE(e.LogicalSeated());
    REQUIRE_FALSE(e.EntryConfirmed());
    SeatProjection p;
    REQUIRE(p.Observe(0x14, 11, true, true, e.EntryConfirmed(), false, true) == SeatAction::AwaitEntry);
    REQUIRE_FALSE(p.Completed);
    REQUIRE_FALSE(p.Issued); // an occupied chair must not be activated again
    e.RootReady = e.GraphReady = e.EnterEvent = true;
    REQUIRE_FALSE(e.EntryConfirmed()); // event/root alone cannot confirm entry
    e.FurnitureVariableValid = e.InFurniture = true;
    REQUIRE(e.EntryConfirmed()); // unknown sitting graph variable is diagnostic
    e.SittingVariableValid = true;
    REQUIRE(e.EntryConfirmed()); // explicitly false isIdleSitting
    e.IdleSitting = true;
    REQUIRE(e.EntryConfirmed());
    REQUIRE(p.Observe(0x14, 11, true, true, e.EntryConfirmed(), false, true) == SeatAction::Complete);
    SeatingEntryEvidence enterOnly;
    enterOnly.IdentityMatches = enterOnly.EnterEvent = true;
    REQUIRE_FALSE(enterOnly.EntryConfirmed());
}

TEST_CASE("Local entry with isIdleSitting false rejects each missing required observation", "[creation-seating]")
{
    const SeatingEntryEvidence complete{true, true, true, true, true, true, true, true, false, true};
    REQUIRE(complete.EntryConfirmed());
    const std::array<bool SeatingEntryEvidence::*, 8> fields{
        &SeatingEntryEvidence::CorrectFurniture, &SeatingEntryEvidence::LogicalSitState,
        &SeatingEntryEvidence::RootReady, &SeatingEntryEvidence::GraphReady, &SeatingEntryEvidence::EnterEvent,
        &SeatingEntryEvidence::FurnitureVariableValid, &SeatingEntryEvidence::InFurniture,
        &SeatingEntryEvidence::IdentityMatches};
    for (const auto field : fields)
    {
        auto missing = complete;
        missing.*field = false;
        REQUIRE_FALSE(missing.EntryConfirmed());
        SeatProjection pending;
        REQUIRE(pending.Observe(0x14, 11, true, false, false, false, false) == SeatAction::Activate);
        pending.Issued = true;
        REQUIRE(pending.Observe(0x14, 11, true, missing.CorrectFurniture, missing.EntryConfirmed(), false, true) == SeatAction::AwaitEntry);
        REQUIRE_FALSE(pending.Completed);
    }
    SeatProjection p;
    REQUIRE(p.Observe(0x14, 11, true, true, complete.EntryConfirmed(), false, true) == SeatAction::Complete);
    REQUIRE(p.Observe(0x14, 11, true, false, false, false, false) == SeatAction::Complete);
    REQUIRE(p.Observe(0x14, 22, true, true, false, false, true) == SeatAction::AwaitEntry);
    REQUIRE_FALSE(p.Completed); // old token's completion is not reused
}

TEST_CASE("Furniture evidence belongs to the current actor token and assigned seat", "[creation-seating]")
{
    const SeatingEntryIdentity observed{0x14, 0xBF3DD, 11, 100};
    auto current = observed;
    SECTION("matching identity") { REQUIRE(current.Matches(observed)); return; }
    SECTION("other actor") { current.Actor = 0x15; }
    SECTION("new token for same actor") { current.ActorToken = 22; }
    SECTION("other chair") { current.Seat = 0xBF3DC; }
    SECTION("replaced chair token") { current.SeatToken = 101; }
    SECTION("no actor") { current.Actor = 0; }
    SECTION("no actor token") { current.ActorToken = 0; }
    SECTION("no chair") { current.Seat = 0; }
    SECTION("no chair token") { current.SeatToken = 0; }
    REQUIRE_FALSE(current.Matches(observed));
    SeatingEntryEvidence evidence{true, true, true, true, true, true, true, true, false, current.Matches(observed)};
    REQUIRE_FALSE(evidence.EntryConfirmed());
    SeatProjection projection;
    REQUIRE(projection.Observe(current.Actor, current.ActorToken, true, true, evidence.EntryConfirmed(), false, true) != SeatAction::Complete);
    REQUIRE_FALSE(projection.Completed);
}

TEST_CASE("Confirmed local entry can never time out or activate again on the same token", "[creation-seating]")
{
    SeatProjection projection;
    REQUIRE(projection.Observe(0x14, 11, true, false, false, false, false) == SeatAction::Activate);
    projection.Issued = true;
    const SeatingEntryEvidence evidence{true, true, true, true, true, true, true, true, false, true};
    REQUIRE(projection.Observe(0x14, 11, true, true, evidence.EntryConfirmed(), false, true) == SeatAction::Complete);
    // Later voluntary exit or loss of graph/readiness cannot undo completion.
    for (const auto now : {2000, 11001, 41001, 100000})
    {
        REQUIRE_FALSE(projection.PendingEntryTimedOut(now, 1000));
        REQUIRE(projection.Observe(0x14, 11, true, false, false, false, false) == SeatAction::Complete);
        REQUIRE(projection.Observe(0x14, 11, false, false, false, false, false) != SeatAction::Activate);
    }
}

TEST_CASE("An unconfirmed entry retains the real timeout without permitting reactivation", "[creation-seating]")
{
    SeatProjection projection;
    REQUIRE_FALSE(projection.PendingEntryTimedOut(50000, 1000));
    REQUIRE(projection.Observe(0x14, 11, true, false, false, false, false) == SeatAction::Activate);
    projection.Issued = true;
    REQUIRE_FALSE(projection.PendingEntryTimedOut(11000, 1000));
    REQUIRE(projection.PendingEntryTimedOut(11001, 1000));
    REQUIRE(projection.Observe(0x14, 11, true, false, false, false, false) == SeatAction::AwaitEntry);
    // A late native entry within the observation window may still confirm.
    REQUIRE(projection.Observe(0x14, 11, true, true, true, false, true) == SeatAction::Complete);
    REQUIRE_FALSE(projection.PendingEntryTimedOut(40000, 1000));
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

namespace
{
struct AppliedSeatingIntent
{
    uint32_t Server{7}, Player{4};
    uint64_t Revision{2};
    std::string Campaign{"campaign"}, DurablePlayer{"durable-player"};
    SeatProjection Projection;
    SeatingLifetime Lifetime;
    SeatingObservationWindow Observation;
};
}

TEST_CASE("Applied replay retains the same seating attempt and terminal tombstone", "[creation-seating]")
{
    std::map<uint32_t, AppliedSeatingIntent> intents;
    const auto added = AdmitSeatingIntent(intents, AppliedSeatingIntent{});
    REQUIRE(added.Admission == SeatingIntentAdmission::Added);
    auto& intent = *added.Intent;
    REQUIRE(intent.Projection.Observe(0x14, 11, true, false, false, false, false) == SeatAction::Activate);
    intent.Projection.Issued = true;
    REQUIRE(intent.Observation.Poll(1000, 1000, 0) == SeatingObservationAction::Sample);

    SECTION("pending entry") {}
    SECTION("entry confirmed with a passive tail")
    {
        REQUIRE(intent.Projection.Observe(0x14, 11, true, true, true, false, true) == SeatAction::Complete);
    }
    SECTION("confirmed observation ended")
    {
        REQUIRE(intent.Projection.Observe(0x14, 11, true, true, true, false, true) == SeatAction::Complete);
        REQUIRE(intent.Observation.Poll(7001, 1000, 2000) == SeatingObservationAction::End);
        intent.Lifetime.Terminal = true;
    }
    SECTION("unconfirmed observation timed out")
    {
        REQUIRE(intent.Projection.PendingEntryTimedOut(41001, 1000));
        REQUIRE(intent.Observation.Poll(41001, 1000, 0) == SeatingObservationAction::End);
        intent.Lifetime.Terminal = true;
    }
    SECTION("creation cell left")
    {
        REQUIRE_FALSE(intent.Lifetime.Observe(10, 20, 10, true, 2000));
        REQUIRE(intent.Lifetime.Terminal);
    }
    SECTION("recovery lock")
    {
        // The native gate writes this tombstone before any observation or
        // activation; the adapter's ordering is checked in test_creation_seating.
        intent.Lifetime.Terminal = true;
    }
    const auto before = intent;
    for (int n = 0; n < 3; ++n)
    {
        const auto replay = AdmitSeatingIntent(intents, AppliedSeatingIntent{});
        REQUIRE(replay.Admission == SeatingIntentAdmission::Duplicate);
        REQUIRE(replay.Intent == added.Intent);
        REQUIRE(intents.size() == 1);
        REQUIRE(intent.Projection.Actor == before.Projection.Actor);
        REQUIRE(intent.Projection.Token == before.Projection.Token);
        REQUIRE(intent.Projection.Issued);
        REQUIRE(intent.Projection.Completed == before.Projection.Completed);
        REQUIRE(intent.Lifetime.Terminal == before.Lifetime.Terminal);
        REQUIRE(intent.Observation.Closed == before.Observation.Closed);
        REQUIRE(intent.Observation.LastSampleMs == before.Observation.LastSampleMs);
        REQUIRE(intent.Projection.Observe(0x14, 11, true, false, false, false, false) != SeatAction::Activate);
    }
}

TEST_CASE("Applied admission cannot replace an existing identity or exceed the sealed roster bound", "[creation-seating]")
{
    std::map<uint32_t, AppliedSeatingIntent> intents;
    const auto first = AdmitSeatingIntent(intents, AppliedSeatingIntent{});
    REQUIRE(first.Admission == SeatingIntentAdmission::Added);
    first.Intent->Lifetime.Terminal = true;
    auto incoming = AppliedSeatingIntent{};
    SECTION("changed revision") { ++incoming.Revision; }
    SECTION("changed transport player") { ++incoming.Player; }
    SECTION("changed campaign") { incoming.Campaign = "other-campaign"; }
    SECTION("changed durable player") { incoming.DurablePlayer = "other-player"; }
    SECTION("same durable player on another server binding") { ++incoming.Server; }
    SECTION("ten distinct players is the limit")
    {
        for (uint32_t n = 1; n < 10; ++n)
        {
            incoming.Server = 7 + n;
            incoming.DurablePlayer = "player-" + std::to_string(n);
            REQUIRE(AdmitSeatingIntent(intents, incoming).Admission == SeatingIntentAdmission::Added);
        }
        REQUIRE(AdmitSeatingIntent(intents, AppliedSeatingIntent{}).Admission == SeatingIntentAdmission::Duplicate);
        incoming.Server = 99;
        incoming.DurablePlayer = "eleventh-player";
        REQUIRE(AdmitSeatingIntent(intents, incoming).Admission == SeatingIntentAdmission::Full);
        REQUIRE(intents.size() == 10);
        return;
    }
    const auto rejected = AdmitSeatingIntent(intents, incoming);
    REQUIRE(rejected.Admission == (incoming.Server == 7 ? SeatingIntentAdmission::IdentityConflict : SeatingIntentAdmission::DuplicatePlayer));
    REQUIRE(rejected.Intent == first.Intent);
    REQUIRE(first.Intent->Lifetime.Terminal);
    REQUIRE(first.Intent->Revision == 2);
    REQUIRE(first.Intent->Server == 7);
    REQUIRE(first.Intent->Campaign == "campaign");
    REQUIRE(first.Intent->DurablePlayer == "durable-player");
    REQUIRE(intents.size() == 1);
}

TEST_CASE("A new native token resets projection only after readiness and before lifetime termination", "[creation-seating]")
{
    SeatProjection projection;
    REQUIRE(projection.Observe(0x14, 11, true, true, true, false, true) == SeatAction::Complete);
    REQUIRE(projection.Observe(0x14, 22, false, false, false, false, false) == SeatAction::Pending);
    REQUIRE(projection.Token == 11);
    REQUIRE(projection.Completed);
    REQUIRE(projection.Observe(0x14, 22, true, false, false, false, false) == SeatAction::Activate);
    REQUIRE(projection.Token == 22);
    REQUIRE_FALSE(projection.Completed);
    REQUIRE_FALSE(projection.Issued);
    projection.Issued = true;
    REQUIRE(projection.Observe(0x14, 22, true, false, false, false, false) == SeatAction::AwaitEntry);
    // The existing lifetime gate suppresses even a new token after cell exit.
    SeatingLifetime lifetime;
    REQUIRE_FALSE(lifetime.Observe(10, 20, 10, true, 2000));
    REQUIRE_FALSE(lifetime.Observe(10, 10, 10, true, 3000));
    REQUIRE(lifetime.Terminal);
}

TEST_CASE("Creation seating observation is terminal on cell exit and cannot rearm", "[seating]")
{
    using STRE::CharacterCreation::SeatingLifetime;
    SeatingLifetime lifetime;
    REQUIRE_FALSE(lifetime.Observe(10, 10, 0, false, 100));
    REQUIRE(lifetime.Observe(10, 10, 10, true, 200));
    SECTION("Observer leaves first") { REQUIRE_FALSE(lifetime.Observe(10, 20, 10, true, 300)); }
    SECTION("Remote leaves first") { REQUIRE_FALSE(lifetime.Observe(10, 10, 20, true, 300)); }
    SECTION("Committed binding disappears") { REQUIRE_FALSE(lifetime.Observe(10, 10, 0, false, 300)); }
    SECTION("Diagnostic deadline ends without following the player forever")
    {
        REQUIRE(lifetime.Observe(10, 10, 10, true, 40199));
        REQUIRE_FALSE(lifetime.Observe(10, 10, 10, true, 40200));
    }
    REQUIRE(lifetime.Terminal);
    unsigned nativeBindingCalls{};
    for (int i = 0; i < 100; ++i)
        if (!lifetime.Terminal && lifetime.Observe(10, 10, 10, true, 500 + i)) ++nativeBindingCalls;
    REQUIRE(nativeBindingCalls == 0);
}
