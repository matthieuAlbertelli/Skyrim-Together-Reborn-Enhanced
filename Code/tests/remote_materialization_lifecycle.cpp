#include <CharacterCreation/RemoteMaterializationLifecycle.h>
#include <catch2/catch.hpp>

using namespace STRE::CharacterCreation;

namespace
{
constexpr MaterializationKey cKey{1, 0x1000001, 42, 7};
constexpr MaterializationForms cOld{100, 101}, cCandidate{200, 201};

RemoteMaterializationLifecycle Reserved()
{
    RemoteMaterializationLifecycle model;
    REQUIRE(model.Reserve(cKey, cOld));
    return model;
}

void Ready(RemoteMaterializationLifecycle& aModel)
{
    REQUIRE(aModel.RecordCandidate(cKey, cCandidate));
    REQUIRE(aModel.Observe(cKey, cCandidate.Actor, true) == DiscoveryRoute::CandidateStaged);
    REQUIRE(aModel.MarkReady(cKey));
}
}

TEST_CASE("Materialization candidate discovery retains logical identity and old binding", "[materialization-lifecycle]")
{
    auto model = Reserved();
    REQUIRE(model.RecordCandidate(cKey, cCandidate));
    REQUIRE(model.Observe(cKey, cCandidate.Actor, true) == DiscoveryRoute::CandidateStaged);
    REQUIRE(model.Key() == cKey);
    REQUIRE(model.HasIdentity());
    REQUIRE(model.BoundActor() == cOld.Actor);
    REQUIRE(model.TakeRetirementIntents(cKey) == 0);
}

TEST_CASE("Late old removal after commit cannot clear the new binding", "[materialization-lifecycle]")
{
    auto model = Reserved();
    Ready(model);
    REQUIRE(model.Commit(cKey));
    REQUIRE(model.TakeRetirementIntents(cKey) == 1);
    REQUIRE(model.Observe(cKey, cOld.Actor, false) == DiscoveryRoute::OldRetirementObserved);
    REQUIRE(model.BoundActor() == cCandidate.Actor);
    REQUIRE(model.HasIdentity());
    REQUIRE(model.State() == MaterializationState::Committed);
    REQUIRE(model.OldRetirement().Observed); // Not native completion.
    REQUIRE(model.Observe(cKey, cOld.Actor, true) == DiscoveryRoute::Quarantined);
}

TEST_CASE("Candidate removal after abort leaves the original binding intact", "[materialization-lifecycle]")
{
    auto model = Reserved();
    Ready(model);
    REQUIRE(model.Abort(cKey));
    REQUIRE(model.TakeRetirementIntents(cKey) == 2);
    REQUIRE(model.Observe(cKey, cCandidate.Actor, false) == DiscoveryRoute::CandidateRetirementObserved);
    REQUIRE(model.BoundActor() == cOld.Actor);
    REQUIRE(model.HasIdentity());
    REQUIRE_FALSE(model.Commit(cKey));
    REQUIRE(model.Observe(cKey, cCandidate.Actor, true) == DiscoveryRoute::Quarantined);
}

TEST_CASE("Authoritative removal invalidates every stage without resurrection", "[materialization-lifecycle]")
{
    auto model = Reserved();
    const auto stage = GENERATE(0, 1, 2, 3, 4, 5);
    if (stage >= 1)
        REQUIRE(model.RecordCandidate(cKey, cCandidate));
    if (stage >= 2)
        REQUIRE(model.Observe(cKey, cCandidate.Actor, true) == DiscoveryRoute::CandidateStaged);
    if (stage >= 3)
        REQUIRE(model.MarkReady(cKey));
    if (stage == 4)
        REQUIRE(model.Commit(cKey));
    if (stage == 5)
        REQUIRE(model.Abort(cKey));
    REQUIRE(model.Invalidate(cKey));
    REQUIRE_FALSE(model.HasIdentity());
    REQUIRE(model.BoundActor() == 0);
    REQUIRE(model.TakeRetirementIntents(cKey) == (stage ? 3 : 1));
    REQUIRE_FALSE(model.MarkReady(cKey));
    REQUIRE_FALSE(model.Commit(cKey));
    REQUIRE_FALSE(model.Abort(cKey));
    REQUIRE(model.Invalidate(cKey));
    REQUIRE(model.TakeRetirementIntents(cKey) == 0);
    REQUIRE(model.Observe(cKey, cOld.Actor, true) == DiscoveryRoute::Quarantined);
}

TEST_CASE("Stale session entity server and generation cannot mutate a transaction", "[materialization-lifecycle]")
{
    auto model = Reserved();
    Ready(model);
    auto stale = cKey;
    SECTION("generation") { --stale.Generation; }
    SECTION("entity version") { ++stale.Entity; }
    SECTION("session") { ++stale.Session; }
    SECTION("server") { ++stale.ServerId; }
    REQUIRE(model.Observe(stale, cOld.Actor, false) == DiscoveryRoute::Stale);
    REQUIRE(model.Observe(stale, cCandidate.Actor, true) == DiscoveryRoute::Stale);
    REQUIRE_FALSE(model.RecordCandidate(stale, cCandidate));
    REQUIRE_FALSE(model.MarkReady(stale));
    REQUIRE_FALSE(model.Commit(stale));
    REQUIRE_FALSE(model.Abort(stale));
    REQUIRE_FALSE(model.Invalidate(stale));
    REQUIRE(model.TakeRetirementIntents(stale) == 0);
    REQUIRE(model.State() == MaterializationState::CandidateReady);
    REQUIRE(model.BoundActor() == cOld.Actor);
    REQUIRE(model.HasIdentity());
}

TEST_CASE("Duplicate discovery and retirement intents are idempotent", "[materialization-lifecycle]")
{
    auto model = Reserved();
    Ready(model);
    REQUIRE(model.RecordCandidate(cKey, cCandidate));
    REQUIRE(model.Observe(cKey, cCandidate.Actor, true) == DiscoveryRoute::CandidateStaged);
    REQUIRE(model.State() == MaterializationState::CandidateReady);
    REQUIRE(model.Abort(cKey));
    REQUIRE(model.TakeRetirementIntents(cKey) == 2);
    REQUIRE(model.Abort(cKey));
    REQUIRE(model.TakeRetirementIntents(cKey) == 0);
    REQUIRE(model.Observe(cKey, cCandidate.Actor, false) == DiscoveryRoute::CandidateRetirementObserved);
    REQUIRE(model.Observe(cKey, cCandidate.Actor, false) == DiscoveryRoute::Duplicate);
    REQUIRE(model.Invalidate(cKey));
    REQUIRE(model.TakeRetirementIntents(cKey) == 1);
    REQUIRE(model.TakeRetirementIntents(cKey) == 0);
}

TEST_CASE("Old actor loss before commit aborts without publishing candidate", "[materialization-lifecycle]")
{
    auto model = Reserved();
    Ready(model);
    REQUIRE(model.Observe(cKey, cOld.Actor, false) == DiscoveryRoute::OldLostBeforeCommit);
    REQUIRE(model.State() == MaterializationState::Aborted);
    REQUIRE(model.BoundActor() == 0);
    REQUIRE(model.HasIdentity()); // Owning service must handle genuine unload separately.
    REQUIRE_FALSE(model.Commit(cKey));
    REQUIRE(model.TakeRetirementIntents(cKey) == 2);
    REQUIRE(model.Observe(cKey, cOld.Actor, false) == DiscoveryRoute::Duplicate);
}

TEST_CASE("Candidate loss before commit aborts and keeps live old actor", "[materialization-lifecycle]")
{
    auto model = Reserved();
    Ready(model);
    REQUIRE(model.Observe(cKey, cCandidate.Actor, false) == DiscoveryRoute::CandidateLostBeforeCommit);
    REQUIRE(model.State() == MaterializationState::Aborted);
    REQUIRE(model.BoundActor() == cOld.Actor);
    REQUIRE_FALSE(model.Commit(cKey));
}

TEST_CASE("Removal of committed candidate is a genuine current-binding loss", "[materialization-lifecycle]")
{
    auto model = Reserved();
    Ready(model);
    REQUIRE(model.Commit(cKey));
    REQUIRE(model.Observe(cKey, cCandidate.Actor, false) == DiscoveryRoute::LiveBindingLost);
    REQUIRE(model.BoundActor() == 0);
    REQUIRE_FALSE(model.Commit(cKey));
    REQUIRE(model.Observe(cKey, cOld.Actor, false) == DiscoveryRoute::OldRetirementObserved);
    REQUIRE(model.BoundActor() == 0); // Never fall back to the retiring actor.
}

TEST_CASE("Creation returning after cancellation is tracked only for cleanup", "[materialization-lifecycle]")
{
    auto model = Reserved();
    const bool removed = GENERATE(false, true);
    if (removed)
        REQUIRE(model.Invalidate(cKey));
    else
        REQUIRE(model.Abort(cKey));
    REQUIRE(model.RecordCandidate(cKey, cCandidate));
    REQUIRE(model.BoundActor() == (removed ? 0 : cOld.Actor));
    REQUIRE(model.TakeRetirementIntents(cKey) == (removed ? 3 : 2));
    REQUIRE_FALSE(model.MarkReady(cKey));
    REQUIRE_FALSE(model.Commit(cKey));
    REQUIRE(model.Observe(cKey, cCandidate.Actor, true) == DiscoveryRoute::Quarantined);
}

TEST_CASE("Reservation and publication fail closed on incomplete or ambiguous identity", "[materialization-lifecycle]")
{
    RemoteMaterializationLifecycle model;
    auto invalid = cKey;
    invalid.Generation = 0;
    REQUIRE_FALSE(model.Reserve(invalid, cOld));
    REQUIRE_FALSE(model.Reserve(cKey, {}));
    REQUIRE(model.Reserve(cKey, cOld));
    REQUIRE_FALSE(model.Reserve(cKey, cOld));
    REQUIRE_FALSE(model.RecordCandidate(cKey, cOld));
    REQUIRE_FALSE(model.RecordCandidate(cKey, {cOld.Base, 202}));
    REQUIRE_FALSE(model.RecordCandidate(cKey, {202, cOld.Actor}));
    REQUIRE_FALSE(model.Commit(cKey));
    REQUIRE_FALSE(model.MarkReady(cKey));
    REQUIRE(model.RecordCandidate(cKey, cCandidate));
    REQUIRE_FALSE(model.RecordCandidate(cKey, {300, 301}));
    REQUIRE_FALSE(model.MarkReady(cKey)); // Must first observe the known candidate.
    REQUIRE_FALSE(model.Commit(cKey));
}

TEST_CASE("Unrelated actors are never suppressed by a reservation", "[materialization-lifecycle]")
{
    auto model = Reserved();
    REQUIRE(model.Observe(cKey, 999, true) == DiscoveryRoute::Unrelated);
    REQUIRE(model.Observe(cKey, 999, false) == DiscoveryRoute::Unrelated);
    REQUIRE(model.Observe(cKey, 0, true) == DiscoveryRoute::Unrelated);
    REQUIRE(model.State() == MaterializationState::CandidateReserved);
    REQUIRE(model.BoundActor() == cOld.Actor);
}
