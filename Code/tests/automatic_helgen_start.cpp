#include <CampaignHelgenStartBarrier.h>
#include <HelgenStartProjection.h>
#include <CharacterCreation/CreationSeating.h>
#include <campaign_persistence_test_helpers.h>
#include <catch2/catch.hpp>
#include <array>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

namespace
{
struct HelgenFixture
{
    TemporaryDatabase Database;
    std::unique_ptr<SqliteCampaignStore> Store{OpenStore(Database)};
    CampaignRuntimeService Runtime{*Store};
    unsigned Next{};
    CampaignAdmissionService Admission{Runtime, [this](std::string_view prefix) {
        return std::string(prefix) + "helgen-" + std::to_string(++Next);
    }};
    CampaignHelgenStartBarrier Barrier;
    CampaignId Campaign;
    std::array<HelgenBuildEvidence, 3> Builds{HelgenBuildEvidence::Missing,
        HelgenBuildEvidence::Pending, HelgenBuildEvidence::Pending};

    HelgenFixture(unsigned count = 2, bool seal = true)
    {
        REQUIRE(Admission.RegisterConnection(1, std::string(64, '1')) == CampaignConnectionRegistration::Accepted);
        const auto created = Admission.CreateCampaign(1, "create", true);
        REQUIRE(created.Succeeded());
        Campaign = CampaignId{created.CampaignId};
        if (count == 2)
        {
            REQUIRE(Admission.RegisterConnection(2, std::string(64, '2')) == CampaignConnectionRegistration::Accepted);
            REQUIRE(Admission.JoinCampaign(2, Campaign.Value, "join", 1, true).Succeeded());
        }
        if (seal)
            REQUIRE(Admission.StartCampaign(1, Campaign.Value, "start", count, true, true).Succeeded());
    }
    HelgenStartResult Observe(unsigned connection, bool ready = false, bool checkpoint = false)
    {
        return Barrier.Observe(Admission, connection, [this](const auto& member) {
            return Builds.at(member.Connection);
        }, ready, checkpoint);
    }
};
}

TEST_CASE("Only the last exact roster Applied emits a Helgen start intent", "[helgen][character-build][campaign]")
{
    const unsigned first = GENERATE(1u, 2u);
    const unsigned last = 3 - first;
    HelgenFixture f;
    REQUIRE(f.Observe(first).Outcome == HelgenStartOutcome::Waiting);
    f.Builds[first] = HelgenBuildEvidence::Applied;
    auto result = f.Observe(first);
    REQUIRE(result.Outcome == HelgenStartOutcome::Waiting);
    REQUIRE(result.Applied == 1);
    REQUIRE_FALSE(f.Barrier.HasStarted(f.Campaign));
    // ADR-0024: the finished player's seat intent is immediately actionable.
    STRE::CharacterCreation::SeatProjection seat;
    REQUIRE(seat.Observe(first, 100 + first, true, false, false, false, false) ==
        STRE::CharacterCreation::SeatAction::Activate);
    // A retry/readiness from the finished member cannot bypass the other build.
    REQUIRE(f.Observe(first, true, true).Outcome == HelgenStartOutcome::Waiting);
    f.Builds[last] = HelgenBuildEvidence::Applied;
    result = f.Observe(last);
    REQUIRE(result.Outcome == HelgenStartOutcome::Started);
    REQUIRE(result.Applied == 2);
    REQUIRE(result.Required == 2);
    REQUIRE(f.Barrier.HasStarted(f.Campaign));
    REQUIRE(f.Observe(last).Outcome == HelgenStartOutcome::AlreadyStarted);
    REQUIRE(f.Observe(first).Outcome == HelgenStartOutcome::AlreadyStarted);
    REQUIRE(f.Observe(first, true).Outcome == HelgenStartOutcome::AlreadyStarted);
}

TEST_CASE("Helgen cannot start without sealed active exact admission and character proof", "[helgen][campaign][security]")
{
    SECTION("not sealed")
    {
        HelgenFixture f(2, false);
        f.Builds[1] = f.Builds[2] = HelgenBuildEvidence::Applied;
        REQUIRE(f.Observe(1).Outcome == HelgenStartOutcome::Rejected);
    }
    SECTION("no admission or another campaign")
    {
        HelgenFixture f;
        f.Builds[1] = f.Builds[2] = HelgenBuildEvidence::Applied;
        REQUIRE(f.Observe(99).Outcome == HelgenStartOutcome::Rejected);
        REQUIRE(f.Admission.RegisterConnection(3, std::string(64, '3')) == CampaignConnectionRegistration::Accepted);
        REQUIRE(f.Admission.CreateCampaign(3, "other", true).Succeeded());
        REQUIRE(f.Observe(3).Outcome == HelgenStartOutcome::Rejected);
        REQUIRE_FALSE(f.Barrier.HasStarted(f.Campaign));
    }
    SECTION("missing member causes recovery lock even with both proofs")
    {
        HelgenFixture f;
        f.Builds[1] = f.Builds[2] = HelgenBuildEvidence::Applied;
        const auto snapshot = f.Admission.Disconnect(2);
        REQUIRE(snapshot);
        REQUIRE(snapshot->RuntimeState != kCampaignWireRuntimeActive);
        REQUIRE(f.Observe(1).Outcome == HelgenStartOutcome::Rejected);
        REQUIRE_FALSE(f.Barrier.HasStarted(f.Campaign));
    }
    SECTION("missing build, missing character, owner or binding mismatch")
    {
        const auto evidence = GENERATE(HelgenBuildEvidence::Missing, HelgenBuildEvidence::Invalid);
        HelgenFixture f;
        f.Builds[1] = HelgenBuildEvidence::Applied;
        f.Builds[2] = evidence;
        REQUIRE(f.Observe(1).Outcome != HelgenStartOutcome::Started);
        REQUIRE(f.Observe(1, true, true).Outcome != HelgenStartOutcome::Started);
        REQUIRE_FALSE(f.Barrier.HasStarted(f.Campaign));
    }
    SECTION("full roster present again is still recovery locked")
    {
        HelgenFixture f;
        const auto* record = static_cast<const CampaignAdmissionService&>(f.Admission).FindConnection(2);
        const auto binding = record->AdmittedIdentity->CharacterBinding.Value;
        REQUIRE(f.Admission.Disconnect(2));
        REQUIRE(f.Admission.RegisterConnection(2, std::string(64, '2')) == CampaignConnectionRegistration::Accepted);
        const auto resumed = f.Admission.ResumeCampaign(2, f.Campaign.Value, binding);
        REQUIRE(resumed.Succeeded());
        REQUIRE(resumed.Snapshot);
        REQUIRE(resumed.Snapshot->Roster[0].Present);
        REQUIRE(resumed.Snapshot->Roster[1].Present);
        REQUIRE(resumed.Snapshot->RuntimeState != kCampaignWireRuntimeActive);
        f.Builds[1] = f.Builds[2] = HelgenBuildEvidence::Applied;
        REQUIRE(f.Observe(1).Outcome == HelgenStartOutcome::Rejected);
        REQUIRE_FALSE(f.Barrier.HasStarted(f.Campaign));
    }
    SECTION("single required member")
    {
        HelgenFixture f(1);
        f.Builds[1] = HelgenBuildEvidence::Applied;
        REQUIRE(f.Observe(1).Outcome == HelgenStartOutcome::Started);
    }
}

TEST_CASE("Readiness reconstructs only checkpoint sessions with no volatile builds", "[helgen][campaign][recovery]")
{
    HelgenFixture f;
    f.Builds[1] = f.Builds[2] = HelgenBuildEvidence::Missing;
    REQUIRE(f.Observe(1, true).Outcome == HelgenStartOutcome::Waiting);
    REQUIRE(f.Observe(2, true).Outcome == HelgenStartOutcome::Waiting);
    REQUIRE_FALSE(f.Barrier.HasStarted(f.Campaign));
    REQUIRE(f.Observe(1, true, true).Outcome == HelgenStartOutcome::Waiting);
    f.Barrier.Disconnect(f.Campaign);
    REQUIRE(f.Observe(2, true, true).Outcome == HelgenStartOutcome::Waiting);
    REQUIRE(f.Observe(1, true, true).Outcome == HelgenStartOutcome::Started);
    f.Barrier.Disconnect(f.Campaign);
    REQUIRE(f.Barrier.HasStarted(f.Campaign));
    REQUIRE(f.Admission.Disconnect(2));
    REQUIRE(f.Observe(1, true, true).Outcome == HelgenStartOutcome::Rejected);
}

TEST_CASE("Local Helgen projection needs finalization and one matching authorization", "[helgen][campaign.client]")
{
    HelgenStartProjection projection;
    projection.Begin("campaign");
    projection.Authorize("campaign");
    REQUIRE_FALSE(projection.Consume(true, true, true));
    projection.Finalize();
    REQUIRE_FALSE(projection.Consume(false, true, true));
    REQUIRE_FALSE(projection.Consume(true, false, true));
    REQUIRE_FALSE(projection.Consume(true, true, false));
    REQUIRE(projection.Consume(true, true, true));
    projection.Authorize("campaign");
    projection.Finalize();
    REQUIRE_FALSE(projection.Consume(true, true, true));
    projection.Reset();
    REQUIRE_FALSE(projection.Consume(false, true, true));
    projection.Begin("next");
    projection.Finalize();
    projection.Authorize("campaign");
    REQUIRE_FALSE(projection.Consume(true, true, true));
    projection.Authorize("next");
    REQUIRE(projection.Consume(true, true, true));
}

TEST_CASE("Standalone Helgen starts after local completion without server", "[helgen][campaign.client]")
{
    HelgenStartProjection projection;
    projection.Begin("");
    REQUIRE_FALSE(projection.Consume(false, false, true));
    projection.Finalize();
    REQUIRE_FALSE(projection.Consume(true, false, true));
    REQUIRE(projection.Consume(false, false, true));
    REQUIRE_FALSE(projection.Consume(false, false, true));
}
