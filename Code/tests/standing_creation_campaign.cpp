#include <CampaignAdmissionService.h>
#include <CampaignBootstrapState.h>
#include <CampaignClientAdmissionState.h>
#include <CampaignLobbyDirectory.h>
#include <CampaignStandingPlacement.h>
#include <campaign_persistence_test_helpers.h>
#include <catch2/catch.hpp>
#include <array>
#include <algorithm>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;
using namespace STRE::CharacterCreation;

TEST_CASE("Create A JoinByCode B use durable PlayerId ranks after seal authorization for standing placement", "[campaign.bootstrap][character-creation]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CampaignRuntimeService runtime(*store);
    unsigned ids{};
    CampaignAdmissionService server(runtime, [&](std::string_view prefix) { return std::string(prefix) + "standing-" + std::to_string(++ids); });
    CampaignLobbyDirectory directory([] { return "ABCD"; });
    std::array<CampaignClientAdmissionState, 2> admissions;
    std::array<CampaignBootstrapState, 2> bootstraps;
    for (auto& bootstrap : bootstraps)
        bootstrap.BeginFreshGame();
    bootstraps[0].ShowCreateForm();
    bootstraps[0].BeginCreate(true);
    REQUIRE(server.RegisterConnection(1, std::string(64, 'a')) == CampaignConnectionRegistration::Accepted);
    const auto created = server.CreateCampaign(1, "standing-create", true);
    REQUIRE(created.Succeeded());
    admissions[0].Accept({created.CampaignId, created.CampaignSlotId, created.CharacterBindingId});
    bootstraps[0].OnCampaignAdmitted();
    const auto code = directory.Allocate(created.CampaignId, 17);
    REQUIRE(code);

    bootstraps[1].ShowJoinForm();
    bootstraps[1].BeginJoin(true);
    REQUIRE(server.RegisterConnection(2, std::string(64, 'b')) == CampaignConnectionRegistration::Accepted);
    const auto* lobby = directory.Resolve(*code);
    REQUIRE(lobby);
    // JoinByCode resolves the lobby alias before invoking the same server admission service.
    const auto joined = server.JoinCampaign(2, lobby->CampaignId, "standing-join", created.Version, true);
    REQUIRE(joined.Succeeded());
    REQUIRE(joined.Snapshot);
    admissions[1].Accept({joined.CampaignId, joined.CampaignSlotId, joined.CharacterBindingId});
    bootstraps[1].OnCampaignAdmitted();
    REQUIRE(created.CampaignSlotId != joined.CampaignSlotId);
    const auto beforeSeal = admissions[0].GetAdmission();
    REQUIRE(beforeSeal);
    REQUIRE_FALSE(ResolveCampaignStandingPlacement(&*joined.Snapshot, std::string(64, 'a')).Index);
    bootstraps[0].BeginStart();
    const auto started = server.StartCampaign(1, created.CampaignId, "standing-start", joined.Version, true, true);
    REQUIRE(started.Succeeded());
    REQUIRE(started.Snapshot);

    // Exercise the real wire representation of durable PlayerIds, not fabricated integer ranks.
    TiltedPhoques::Buffer buffer(8192);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    started.Snapshot->Serialize(writer);
    TiltedPhoques::Buffer::Reader reader(&buffer);
    CampaignSnapshotData snapshot;
    snapshot.Deserialize(reader);
    REQUIRE(snapshot.IsValid());
    REQUIRE(snapshot.Roster.size() == 2);
    REQUIRE(snapshot.RosterSealed);
    REQUIRE(snapshot.RuntimeState == kCampaignWireRuntimeActive);
    const auto sealedBeforePlacement = snapshot;
    const std::array<glm::vec3, 2> fixtureAnchors{{{100.f, 200.f, 0.f}, {300.f, 200.f, 0.f}}};
    std::array<size_t, 2> indices;
    std::array<glm::vec3, 2> positions;
    for (size_t i = 0; i < 2; ++i)
    {
        const auto present = std::count_if(snapshot.Roster.begin(), snapshot.Roster.end(), [](const auto& slot) { return slot.Present; });
        admissions[i].ObserveSnapshot(snapshot.CampaignId.c_str(), snapshot.RosterSealed, snapshot.RuntimeState == kCampaignWireRuntimeActive, snapshot.Roster.size(), present);
        REQUIRE(
            bootstraps[i].ObserveCanonicalState(snapshot.RosterSealed, snapshot.Phase == kCampaignWirePhaseCharacterCreation, snapshot.RuntimeState == kCampaignWireRuntimeActive));
        const auto admission = admissions[i].GetAdmission();
        REQUIRE(admission);
        REQUIRE(admission->CampaignSlotId == (i ? joined.CampaignSlotId : created.CampaignSlotId));
        const std::string playerId(64, i ? 'b' : 'a');
        const auto selected = ResolveCampaignStandingPlacement(&snapshot, playerId);
        REQUIRE(selected.Index);
        REQUIRE(selected.Reason == nullptr);
        REQUIRE(*selected.Index < fixtureAnchors.size());
        indices[i] = *selected.Index;
        REQUIRE(indices[i] == i); // Sorted a/b identities produce exactly 0/1.
        positions[i] = StandingCreationPosition(fixtureAnchors[indices[i]], 0.f);
        REQUIRE(positions[i].x == fixtureAnchors[indices[i]].x);
        REQUIRE(positions[i].y == fixtureAnchors[indices[i]].y - 96.f);
        auto reordered = snapshot;
        std::reverse(reordered.Roster.begin(), reordered.Roster.end());
        REQUIRE(ResolveCampaignStandingPlacement(&reordered, playerId).Index == selected.Index);
        for (const auto metadata : {"", "duplicate", "invalid slot text!"})
        {
            auto unrelatedSlots = snapshot;
            for (auto& member : unrelatedSlots.Roster)
                member.SlotId = metadata;
            REQUIRE(ResolveCampaignStandingPlacement(&unrelatedSlots, playerId).Index == selected.Index);
        }
    }
    REQUIRE(indices[0] != indices[1]);
    REQUIRE(positions[0] != positions[1]);
    REQUIRE(snapshot == sealedBeforePlacement);
}

TEST_CASE("Standing PlayerId selection reports missing and contradictory prerequisites", "[campaign.bootstrap][character-creation]")
{
    std::string playerId = "player-a";
    CampaignSnapshotData snapshot;
    snapshot.CampaignId = "campaign-test";
    snapshot.RosterSealed = true;
    snapshot.Phase = kCampaignWirePhaseCharacterCreation;
    snapshot.RuntimeState = kCampaignWireRuntimeActive;
    snapshot.Roster = {{"", "player-a", false, true}, {"", "player-b", false, true}};
    const auto rejects = [&](const CampaignSnapshotData* snap, std::string_view localPlayer, const char* reason)
    {
        const auto result = ResolveCampaignStandingPlacement(snap, localPlayer);
        REQUIRE_FALSE(result.Index);
        REQUIRE(std::string_view(result.Reason) == reason);
    };
    SECTION("missing local identity")
    {
        rejects(&snapshot, "", "missing-local-player-id");
    }
    SECTION("missing snapshot")
    {
        rejects(nullptr, playerId, "sealed-roster-unavailable");
    }
    SECTION("unsealed")
    {
        snapshot.RosterSealed = false;
        rejects(&snapshot, playerId, "sealed-roster-unavailable");
    }
    SECTION("missing campaign")
    {
        snapshot.CampaignId.clear();
        rejects(&snapshot, playerId, "missing-campaign-id");
    }
    SECTION("wrong phase")
    {
        snapshot.Phase = 0;
        rejects(&snapshot, playerId, "campaign-phase-not-character-creation");
    }
    SECTION("recovery")
    {
        snapshot.RuntimeState = kCampaignWireRuntimeRecoveryLock;
        rejects(&snapshot, playerId, "campaign-runtime-not-active");
    }
    SECTION("missing member")
    {
        snapshot.Roster[1].Present = false;
        rejects(&snapshot, playerId, "sealed-roster-incomplete");
    }
    SECTION("empty roster")
    {
        snapshot.Roster.clear();
        rejects(&snapshot, playerId, "sealed-roster-unavailable");
    }
    SECTION("local identity absent from roster")
    {
        rejects(&snapshot, "player-missing", "local-player-not-in-roster");
    }
    SECTION("duplicate local identity")
    {
        snapshot.Roster[1].PlayerId = playerId.c_str();
        rejects(&snapshot, playerId, "duplicate-player-id");
    }
    SECTION("duplicate other identity")
    {
        snapshot.Roster.push_back(snapshot.Roster[1]);
        rejects(&snapshot, playerId, "duplicate-player-id");
    }
    SECTION("empty roster player identity")
    {
        snapshot.Roster[1].PlayerId.clear();
        rejects(&snapshot, playerId, "missing-roster-player-id");
    }
    SECTION("too many players")
    {
        snapshot.Roster.resize(11);
        rejects(&snapshot, playerId, "creation-position-index-out-of-range");
    }
}

TEST_CASE("Ten sealed players get exactly ranks zero to nine regardless of SlotId or roster order", "[campaign.bootstrap][character-creation]")
{
    CampaignSnapshotData snapshot;
    snapshot.CampaignId = "campaign-test";
    snapshot.RosterSealed = true;
    snapshot.Phase = kCampaignWirePhaseCharacterCreation;
    snapshot.RuntimeState = kCampaignWireRuntimeActive;
    for (unsigned i = 0; i < 10; ++i)
    {
        const std::string playerId(64, static_cast<char>('0' + i));
        snapshot.Roster.push_back({"same-slot", playerId.c_str(), false, true});
    }
    std::reverse(snapshot.Roster.begin(), snapshot.Roster.end());
    const auto original = snapshot;
    for (unsigned i = 0; i < 10; ++i)
    {
        const std::string playerId(64, static_cast<char>('0' + i));
        const auto result = ResolveCampaignStandingPlacement(&snapshot, playerId);
        REQUIRE(result.Index == i);
        REQUIRE(result.Reason == nullptr);
    }
    REQUIRE(snapshot == original);
}
