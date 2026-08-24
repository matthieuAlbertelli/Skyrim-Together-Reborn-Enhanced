#include <CampaignAdmissionService.h>
#include <campaign_persistence_test_helpers.h>
#include <Structs/NativeSaveBundle.h>

#include <catch2/catch.hpp>

#include <array>
#include <memory>
#include <string>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

namespace
{
std::string TestPlayerId(std::size_t aIndex)
{
    static constexpr std::array<char, 16> cHex{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    std::string result(62, '0');
    result.push_back(cHex[(aIndex >> 4) & 0x0F]);
    result.push_back(cHex[aIndex & 0x0F]);
    return result;
}

std::int64_t CountCampaigns(const std::filesystem::path& acPath)
{
    sqlite3* pDatabase{};
    if (sqlite3_open(acPath.string().c_str(), &pDatabase) != SQLITE_OK)
    {
        if (pDatabase)
            sqlite3_close_v2(pDatabase);
        return -1;
    }
    sqlite3_stmt* pStatement{};
    std::int64_t result{-1};
    if (sqlite3_prepare_v2(
            pDatabase,
            "SELECT COUNT(*) FROM campaigns;",
            -1,
            &pStatement,
            nullptr) == SQLITE_OK &&
        sqlite3_step(pStatement) == SQLITE_ROW)
    {
        result = sqlite3_column_int64(pStatement, 0);
    }
    if (pStatement)
        sqlite3_finalize(pStatement);
    sqlite3_close_v2(pDatabase);
    return result;
}

struct ProtocolFixture
{
    ProtocolFixture()
        : Store(OpenStore(Database))
        , Runtime(*Store)
        , Admission(Runtime, [this](std::string_view acPrefix)
          {
              return std::string(acPrefix) + "test-" +
                  std::to_string(++GeneratedIds);
          })
    {
    }

    CampaignProtocolCommandResult CreateHost(
        CampaignConnectionHandle aConnection = 1)
    {
        REQUIRE(Admission.RegisterConnection(
            aConnection, TestPlayerId(aConnection)) ==
            CampaignConnectionRegistration::Accepted);
        return Admission.CreateCampaign(
            aConnection, "mutation-create", true);
    }

    TemporaryDatabase Database;
    std::unique_ptr<SqliteCampaignStore> Store;
    CampaignRuntimeService Runtime;
    std::size_t GeneratedIds{};
    CampaignAdmissionService Admission;
};

NativeSaveBundleArtifact CheckpointArtifact(
    const std::string& acIdentity,
    std::uint8_t aMarker)
{
    std::vector<NativeSaveBundleMember> members(2);
    members[0].Role = NativeSaveMemberRole::Ess;
    members[0].Size = 10 + aMarker;
    members[0].Sha256.fill(aMarker);
    members[1].Role = NativeSaveMemberRole::Skse;
    members[1].Size = 20 + aMarker;
    members[1].Sha256.fill(static_cast<std::uint8_t>(aMarker + 1));
    const auto built = BuildNativeSaveBundleArtifact(
        acIdentity, std::move(members));
    REQUIRE(built.Succeeded());
    return built.Value;
}
}

TEST_CASE(
    "Checkpoint admission derives slot authority from the connection and abandons on disconnect",
    "[campaign.admission][checkpoint]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());
    REQUIRE(fixture.Admission.StartCampaign(
        1, created.CampaignId, "mutation-start", 1, true, true)
                .Succeeded());

    const CampaignId campaign{created.CampaignId};
    const auto begun = fixture.Admission.BeginCheckpoint(campaign);
    REQUIRE(begun.Succeeded());
    REQUIRE(begun.Activity);
    const auto artifact = CheckpointArtifact(
        begun.Activity->NativeSaveIdentity, 4);

    const auto noConnection = fixture.Admission.HandleCheckpointSaveResult(
        99,
        campaign,
        begun.Activity->Checkpoint,
        begun.Activity->NativeSaveIdentity,
        true,
        std::string(kNativeSaveFingerprintAlgorithm),
        kNativeSaveFingerprintVersion,
        Bytes(artifact.Fingerprint.begin(), artifact.Fingerprint.end()),
        kNativeSaveMetadataCodecVersion,
        artifact.Metadata);
    REQUIRE(noConnection.Command.Error == CampaignError::NotCampaignMember);

    REQUIRE(fixture.Admission.RegisterConnection(2, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto notAdmitted = fixture.Admission.HandleCheckpointSaveResult(
        2,
        campaign,
        begun.Activity->Checkpoint,
        begun.Activity->NativeSaveIdentity,
        false);
    REQUIRE(notAdmitted.Command.Error == CampaignError::NotCampaignMember);

    const auto wrongCampaign = fixture.Admission.HandleCheckpointSaveResult(
        1,
        CampaignId{"campaign-other"},
        begun.Activity->Checkpoint,
        begun.Activity->NativeSaveIdentity,
        false);
    REQUIRE(wrongCampaign.Command.Error == CampaignError::CheckpointMismatch);

    const auto committed = fixture.Admission.HandleCheckpointSaveResult(
        1,
        campaign,
        begun.Activity->Checkpoint,
        begun.Activity->NativeSaveIdentity,
        true,
        std::string(kNativeSaveFingerprintAlgorithm),
        kNativeSaveFingerprintVersion,
        Bytes(artifact.Fingerprint.begin(), artifact.Fingerprint.end()),
        kNativeSaveMetadataCodecVersion,
        artifact.Metadata);
    REQUIRE(committed.Succeeded());
    REQUIRE(committed.Committed);

    const auto next = fixture.Admission.BeginCheckpoint(campaign);
    REQUIRE(next.Succeeded());
    REQUIRE(fixture.Admission.Disconnect(1));
    REQUIRE_FALSE(fixture.Admission.GetActiveCheckpoint(campaign));
    const auto snapshot = fixture.Admission.BuildSnapshot(campaign);
    REQUIRE(snapshot);
    REQUIRE(snapshot->RuntimeState ==
        static_cast<std::uint8_t>(
            CampaignRuntimeState::WAITING_FOR_ROSTER));
    REQUIRE(fixture.Store->LoadCheckpoint(
        campaign, next.Activity->Checkpoint).Value.State ==
        CheckpointState::Candidate);
}

TEST_CASE("Live campaign creation join and pre-seal leave use canonical assignments", "[campaign.admission]")
{
    ProtocolFixture fixture;
    REQUIRE(fixture.Admission.RegisterConnection(1, TestPlayerId(1)) ==
        CampaignConnectionRegistration::Accepted);
    const auto unauthorized = fixture.Admission.CreateCampaign(
        1, "mutation-create-denied", false);
    REQUIRE(unauthorized.Result == CampaignProtocolResult::Unauthorized);

    const auto created = fixture.Admission.CreateCampaign(
        1, "mutation-create", true);
    REQUIRE(created.Result == CampaignProtocolResult::Applied);
    REQUIRE(created.Version == 1);
    REQUIRE(created.CampaignSlotId == "slot-01");
    REQUIRE_FALSE(created.CharacterBindingId.empty());
    REQUIRE(created.Snapshot);
    REQUIRE(created.Snapshot->Roster.size() == 1);
    REQUIRE_FALSE(created.Snapshot->RosterSealed);
    const auto createReplay = fixture.Admission.CreateCampaign(
        1, "mutation-create", true);
    REQUIRE(createReplay.Result == CampaignProtocolResult::IdempotentReplay);
    REQUIRE(createReplay.CampaignId == created.CampaignId);
    REQUIRE(createReplay.CampaignSlotId == created.CampaignSlotId);

    REQUIRE(fixture.Admission.RegisterConnection(2, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    REQUIRE(fixture.Admission.RegisterConnection(3, TestPlayerId(2)) ==
        CampaignConnectionRegistration::DuplicateActivePlayerId);

    const auto wrongSession = fixture.Admission.JoinCampaign(
        2, created.CampaignId, "mutation-join-wrong-session", 1, false);
    REQUIRE(wrongSession.Result == CampaignProtocolResult::SessionMismatch);

    const auto joined = fixture.Admission.JoinCampaign(
        2, created.CampaignId, "mutation-join", 1, true);
    REQUIRE(joined.Result == CampaignProtocolResult::Applied);
    REQUIRE(joined.Version == 2);
    REQUIRE(joined.CampaignSlotId == "slot-02");
    REQUIRE(joined.Snapshot);
    REQUIRE(joined.Snapshot->Roster.size() == 2);

    const auto persisted = fixture.Runtime.LoadCampaign(
        CampaignId{created.CampaignId});
    REQUIRE(persisted.Succeeded());
    REQUIRE(persisted.Campaign.Roster.size() == 2);
    REQUIRE(persisted.Campaign.Roster[0].Slot.Value == "slot-01");
    REQUIRE(persisted.Campaign.Roster[1].Slot.Value == "slot-02");

    const auto left = fixture.Admission.LeaveCampaign(
        2, created.CampaignId, "mutation-leave", 2);
    REQUIRE(left.Result == CampaignProtocolResult::Applied);
    REQUIRE(left.Version == 3);
    REQUIRE(left.Snapshot);
    REQUIRE(left.Snapshot->Roster.size() == 1);

    const auto leaveReplay = fixture.Admission.LeaveCampaign(
        2, created.CampaignId, "mutation-leave", 2);
    REQUIRE(leaveReplay.Result == CampaignProtocolResult::IdempotentReplay);
    REQUIRE(leaveReplay.Version == 3);
}

TEST_CASE("Campaign create retry restores transient admission after reconnect", "[campaign.admission][reconnect]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());
    REQUIRE(fixture.Admission.Disconnect(1));
    REQUIRE(fixture.Admission.RegisterConnection(11, TestPlayerId(1)) ==
        CampaignConnectionRegistration::Accepted);

    const auto replay = fixture.Admission.CreateCampaign(
        11, "mutation-create", true);
    REQUIRE(replay.Result == CampaignProtocolResult::IdempotentReplay);
    REQUIRE(replay.CampaignId == created.CampaignId);
    const auto* admission =
        static_cast<const CampaignAdmissionService&>(fixture.Admission)
            .FindConnection(11);
    REQUIRE(admission);
    REQUIRE(admission->AdmittedIdentity);
    REQUIRE(admission->AdmittedIdentity->Campaign.Value == created.CampaignId);
    REQUIRE(admission->AdmittedIdentity->Slot.Value == created.CampaignSlotId);
}

TEST_CASE("Campaign create retry survives a full admission and store restart", "[campaign.admission][persistence][reconnect]")
{
    TemporaryDatabase database;
    std::size_t generatedIds{};
    CampaignProtocolCommandResult created;
    {
        auto store = OpenStore(database);
        CampaignRuntimeService runtime(*store);
        CampaignAdmissionService admission(
            runtime, [&generatedIds](std::string_view acPrefix)
            {
                return std::string(acPrefix) + "restart-test-" +
                    std::to_string(++generatedIds);
            });
        REQUIRE(admission.RegisterConnection(1, TestPlayerId(1)) ==
            CampaignConnectionRegistration::Accepted);
        created = admission.CreateCampaign(1, "mutation-create-restart", true);
        REQUIRE(created.Result == CampaignProtocolResult::Applied);
    }

    CampaignProtocolCommandResult replay;
    {
        auto store = OpenStore(database);
        CampaignRuntimeService runtime(*store);
        CampaignAdmissionService admission(
            runtime, [&generatedIds](std::string_view acPrefix)
            {
                return std::string(acPrefix) + "restart-test-" +
                    std::to_string(++generatedIds);
            });
        REQUIRE(admission.RegisterConnection(2, TestPlayerId(1)) ==
            CampaignConnectionRegistration::Accepted);
        replay = admission.CreateCampaign(
            2, "mutation-create-restart", true);
        REQUIRE(replay.Result == CampaignProtocolResult::IdempotentReplay);
        REQUIRE(replay.CampaignId == created.CampaignId);
        REQUIRE(replay.CampaignSlotId == created.CampaignSlotId);
        REQUIRE(replay.CharacterBindingId == created.CharacterBindingId);

        const auto durable = runtime.LoadCampaign(CampaignId{created.CampaignId});
        REQUIRE(durable.Succeeded());
        REQUIRE(durable.Campaign.Roster.size() == 1);
        REQUIRE(durable.Campaign.Roster.front().Player.Value == TestPlayerId(1));
        REQUIRE(durable.Campaign.Roster.front().Slot.Value ==
            created.CampaignSlotId);
        REQUIRE(durable.Campaign.Roster.front().CharacterBinding.Value ==
            created.CharacterBindingId);
    }

    REQUIRE(CountCampaigns(database.Path) == 1);
}

TEST_CASE("Historical create replay does not restore a removed Lobby membership", "[campaign.admission][persistence][reconnect]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());
    const auto left = fixture.Admission.LeaveCampaign(
        1, created.CampaignId, "mutation-leave-host", 1);
    REQUIRE(left.Result == CampaignProtocolResult::Applied);
    REQUIRE(left.Version == 2);
    REQUIRE(left.Snapshot);
    REQUIRE(left.Snapshot->Roster.empty());
    const std::size_t generatedBeforeReplay = fixture.GeneratedIds;

    const auto replay = fixture.Admission.CreateCampaign(
        1, "mutation-create", true);
    REQUIRE(replay.Result == CampaignProtocolResult::IdentityMismatch);
    REQUIRE(fixture.GeneratedIds == generatedBeforeReplay);
    const auto* connection =
        static_cast<const CampaignAdmissionService&>(fixture.Admission)
            .FindConnection(1);
    REQUIRE(connection);
    REQUIRE_FALSE(connection->AdmittedIdentity);

    const auto durable = fixture.Runtime.LoadCampaign(
        CampaignId{created.CampaignId});
    REQUIRE(durable.Succeeded());
    REQUIRE(durable.Campaign.Version == 2);
    REQUIRE(durable.Campaign.Roster.empty());
    REQUIRE(CountCampaigns(fixture.Database.Path) == 1);
}

TEST_CASE("Historical create replay does not restore a rebound Lobby membership", "[campaign.admission][persistence][reconnect]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());
    const auto rebound = fixture.Runtime.ReplaceRosterSlot(
        {CampaignId{created.CampaignId},
         1,
         MutationId{"mutation-rebind-host"},
         {CampaignSlotId{created.CampaignSlotId},
          PlayerId{TestPlayerId(1)},
          CharacterBindingId{"binding-rebound"}}});
    REQUIRE(rebound.Succeeded());
    REQUIRE(rebound.Version == 2);
    REQUIRE(fixture.Admission.Disconnect(1));
    REQUIRE(fixture.Admission.RegisterConnection(11, TestPlayerId(1)) ==
        CampaignConnectionRegistration::Accepted);

    const auto replay = fixture.Admission.CreateCampaign(
        11, "mutation-create", true);
    REQUIRE(replay.Result == CampaignProtocolResult::BindingMismatch);
    const auto* connection =
        static_cast<const CampaignAdmissionService&>(fixture.Admission)
            .FindConnection(11);
    REQUIRE(connection);
    REQUIRE_FALSE(connection->AdmittedIdentity);

    const auto durable = fixture.Runtime.LoadCampaign(
        CampaignId{created.CampaignId});
    REQUIRE(durable.Succeeded());
    REQUIRE(durable.Campaign.Version == 2);
    REQUIRE(durable.Campaign.Roster.size() == 1);
    REQUIRE(durable.Campaign.Roster.front().Slot.Value ==
        created.CampaignSlotId);
    REQUIRE(durable.Campaign.Roster.front().Player.Value == TestPlayerId(1));
    REQUIRE(durable.Campaign.Roster.front().CharacterBinding.Value ==
        "binding-rebound");
    REQUIRE(CountCampaigns(fixture.Database.Path) == 1);
}

TEST_CASE("Historical create replay preserves an admission to another campaign", "[campaign.admission][persistence][security]")
{
    ProtocolFixture fixture;
    const auto createdA = fixture.CreateHost();
    REQUIRE(createdA.Succeeded());
    const auto leftA = fixture.Admission.LeaveCampaign(
        1, createdA.CampaignId, "mutation-leave-a", 1);
    REQUIRE(leftA.Succeeded());

    const auto createdB = fixture.Admission.CreateCampaign(
        1, "mutation-create-b", true);
    REQUIRE(createdB.Result == CampaignProtocolResult::Applied);
    REQUIRE(createdB.CampaignId != createdA.CampaignId);
    const std::size_t generatedBeforeReplay = fixture.GeneratedIds;

    const auto replayA = fixture.Admission.CreateCampaign(
        1, "mutation-create", true);
    REQUIRE(replayA.Result == CampaignProtocolResult::NotAdmitted);
    REQUIRE(fixture.GeneratedIds == generatedBeforeReplay);
    const auto* connection =
        static_cast<const CampaignAdmissionService&>(fixture.Admission)
            .FindConnection(1);
    REQUIRE(connection);
    REQUIRE(connection->AdmittedIdentity);
    REQUIRE(connection->AdmittedIdentity->Campaign.Value ==
        createdB.CampaignId);
    REQUIRE(connection->AdmittedIdentity->Slot.Value ==
        createdB.CampaignSlotId);
    REQUIRE(connection->AdmittedIdentity->CharacterBinding.Value ==
        createdB.CharacterBindingId);

    const auto durableA = fixture.Runtime.LoadCampaign(
        CampaignId{createdA.CampaignId});
    REQUIRE(durableA.Succeeded());
    REQUIRE(durableA.Campaign.Version == 2);
    REQUIRE(durableA.Campaign.Roster.empty());
    const auto durableB = fixture.Runtime.LoadCampaign(
        CampaignId{createdB.CampaignId});
    REQUIRE(durableB.Succeeded());
    REQUIRE(durableB.Campaign.Version == 1);
    REQUIRE(durableB.Campaign.Roster.size() == 1);
    REQUIRE(CountCampaigns(fixture.Database.Path) == 2);
}

TEST_CASE("Ambiguous durable campaign creation reuse fails closed", "[campaign.admission][persistence][idempotency]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CampaignRuntimeService runtime(*store);
    const CampaignSlotRecord firstSlot{
        CampaignSlotId{"slot-01"}, PlayerId{TestPlayerId(1)},
        CharacterBindingId{"binding-first"}};
    const CampaignSlotRecord secondSlot{
        CampaignSlotId{"slot-01"}, PlayerId{TestPlayerId(1)},
        CharacterBindingId{"binding-second"}};
    REQUIRE(runtime.CreateLobbyCampaign(
        {CampaignId{"campaign-first"}, MutationId{"mutation-conflict"},
         {firstSlot}}).Succeeded());
    REQUIRE(runtime.CreateLobbyCampaign(
        {CampaignId{"campaign-second"}, MutationId{"mutation-conflict"},
         {secondSlot}}).Succeeded());

    std::size_t generatedIds{};
    CampaignAdmissionService admission(
        runtime, [&generatedIds](std::string_view acPrefix)
        {
            ++generatedIds;
            return std::string(acPrefix) + "must-not-be-generated";
        });
    REQUIRE(admission.RegisterConnection(1, TestPlayerId(1)) ==
        CampaignConnectionRegistration::Accepted);
    const auto conflict = admission.CreateCampaign(
        1, "mutation-conflict", true);
    REQUIRE(conflict.Result == CampaignProtocolResult::PersistenceFailure);
    REQUIRE(generatedIds == 0);
    REQUIRE(runtime.LoadCampaign(CampaignId{"campaign-first"}).Succeeded());
    REQUIRE(runtime.LoadCampaign(CampaignId{"campaign-second"}).Succeeded());
}

TEST_CASE("Pre-seal members resume their canonical admission without roster mutation", "[campaign.admission][reconnect]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());
    REQUIRE(fixture.Admission.RegisterConnection(2, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto joined = fixture.Admission.JoinCampaign(
        2, created.CampaignId, "mutation-join", 1, true);
    REQUIRE(joined.Succeeded());
    REQUIRE(joined.Version == 2);

    REQUIRE(fixture.Admission.Disconnect(2));
    REQUIRE(fixture.Admission.RegisterConnection(22, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto wrongBinding = fixture.Admission.ResumeCampaign(
        22, created.CampaignId, "binding-wrong");
    REQUIRE(wrongBinding.Result == CampaignProtocolResult::BindingMismatch);
    const auto resumed = fixture.Admission.ResumeCampaign(
        22, created.CampaignId, joined.CharacterBindingId);
    REQUIRE(resumed.Result == CampaignProtocolResult::Applied);
    REQUIRE(resumed.Version == 2);
    REQUIRE(resumed.CampaignSlotId == joined.CampaignSlotId);
    REQUIRE(resumed.CharacterBindingId == joined.CharacterBindingId);
    REQUIRE(resumed.Snapshot);
    REQUIRE_FALSE(resumed.Snapshot->RosterSealed);
    REQUIRE(resumed.Snapshot->Roster.size() == 2);
    REQUIRE(resumed.Snapshot->Roster[1].Present);

    const auto durable = fixture.Runtime.LoadCampaign(
        CampaignId{created.CampaignId});
    REQUIRE(durable.Succeeded());
    REQUIRE(durable.Campaign.Version == 2);
    REQUIRE(durable.Campaign.Roster.size() == 2);
}

TEST_CASE("Pre-seal host and unknown identities use the same exact resume contract", "[campaign.admission][reconnect][security]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());
    REQUIRE(fixture.Admission.Disconnect(1));
    REQUIRE(fixture.Admission.RegisterConnection(11, TestPlayerId(1)) ==
        CampaignConnectionRegistration::Accepted);
    const auto hostResumed = fixture.Admission.ResumeCampaign(
        11, created.CampaignId, created.CharacterBindingId);
    REQUIRE(hostResumed.Result == CampaignProtocolResult::Applied);
    REQUIRE(hostResumed.Version == 1);
    REQUIRE(hostResumed.CampaignSlotId == created.CampaignSlotId);

    REQUIRE(fixture.Admission.RegisterConnection(3, TestPlayerId(3)) ==
        CampaignConnectionRegistration::Accepted);
    const auto unknown = fixture.Admission.ResumeCampaign(
        3, created.CampaignId, created.CharacterBindingId);
    REQUIRE(unknown.Result == CampaignProtocolResult::IdentityMismatch);

    const auto durable = fixture.Runtime.LoadCampaign(
        CampaignId{created.CampaignId});
    REQUIRE(durable.Succeeded());
    REQUIRE(durable.Campaign.Version == 1);
    REQUIRE(durable.Campaign.Roster.size() == 1);
}

TEST_CASE("Pre-seal join replay remains idempotent while new joins require resume", "[campaign.admission][reconnect][idempotency]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());
    REQUIRE(fixture.Admission.RegisterConnection(2, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto joined = fixture.Admission.JoinCampaign(
        2, created.CampaignId, "mutation-join", 1, true);
    REQUIRE(joined.Result == CampaignProtocolResult::Applied);
    REQUIRE(fixture.Admission.Disconnect(2));

    REQUIRE(fixture.Admission.RegisterConnection(22, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto replay = fixture.Admission.JoinCampaign(
        22, created.CampaignId, "mutation-join", 1, true);
    REQUIRE(replay.Result == CampaignProtocolResult::IdempotentReplay);
    REQUIRE(replay.Version == 2);
    REQUIRE(replay.CampaignSlotId == joined.CampaignSlotId);
    REQUIRE(replay.CharacterBindingId == joined.CharacterBindingId);
    REQUIRE(fixture.Admission.Disconnect(22));

    REQUIRE(fixture.Admission.RegisterConnection(23, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto newJoin = fixture.Admission.JoinCampaign(
        23, created.CampaignId, "mutation-join-new", 2, true);
    REQUIRE(newJoin.Result ==
        CampaignProtocolResult::ExistingMembershipRequiresResume);
    REQUIRE(newJoin.Version == 2);
    const auto* connection =
        static_cast<const CampaignAdmissionService&>(fixture.Admission)
            .FindConnection(23);
    REQUIRE(connection);
    REQUIRE_FALSE(connection->AdmittedIdentity);

    const auto resumed = fixture.Admission.ResumeCampaign(
        23, created.CampaignId, joined.CharacterBindingId);
    REQUIRE(resumed.Result == CampaignProtocolResult::Applied);
    REQUIRE(resumed.Version == 2);
    const auto durable = fixture.Runtime.LoadCampaign(
        CampaignId{created.CampaignId});
    REQUIRE(durable.Succeeded());
    REQUIRE(durable.Campaign.Version == 2);
    REQUIRE(durable.Campaign.Roster.size() == 2);
}

TEST_CASE("Host-authorized seal and ready commands derive actor identity from admission", "[campaign.admission][security]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());
    REQUIRE(fixture.Admission.RegisterConnection(2, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto joined = fixture.Admission.JoinCampaign(
        2, created.CampaignId, "mutation-join", 1, true);
    REQUIRE(joined.Succeeded());

    const auto memberStart = fixture.Admission.StartCampaign(
        2, created.CampaignId, "mutation-start-member", 2, false, true);
    REQUIRE(memberStart.Result == CampaignProtocolResult::Unauthorized);

    const auto staleStart = fixture.Admission.StartCampaign(
        1, created.CampaignId, "mutation-start-stale", 1, true, true);
    REQUIRE(staleStart.Result == CampaignProtocolResult::StaleRevision);
    REQUIRE(staleStart.Version == 2);

    const auto started = fixture.Admission.StartCampaign(
        1, created.CampaignId, "mutation-start", 2, true, true);
    REQUIRE(started.Result == CampaignProtocolResult::Applied);
    REQUIRE(started.Version == 3);
    REQUIRE(started.Snapshot);
    REQUIRE(started.Snapshot->RosterSealed);
    REQUIRE(started.Snapshot->Phase ==
        static_cast<std::uint8_t>(CampaignPhase::CharacterCreation));
    REQUIRE(started.Snapshot->RuntimeState ==
        static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE));
    REQUIRE(started.Snapshot->SessionManagerPlayerId == TestPlayerId(1).c_str());

    const auto startReplay = fixture.Admission.StartCampaign(
        1, created.CampaignId, "mutation-start", 2, true, true);
    REQUIRE(startReplay.Result == CampaignProtocolResult::IdempotentReplay);
    REQUIRE(startReplay.Version == 3);
    const auto startConflict = fixture.Admission.StartCampaign(
        1, created.CampaignId, "mutation-start", 3, true, true);
    REQUIRE(startConflict.Result == CampaignProtocolResult::IdempotencyConflict);

    const auto nonMemberReady = fixture.Admission.SetReady(
        99, created.CampaignId, "mutation-spoof", 3, true);
    REQUIRE(nonMemberReady.Result == CampaignProtocolResult::NotAdmitted);

    const auto ready = fixture.Admission.SetReady(
        2, created.CampaignId, "mutation-ready", 3, true);
    REQUIRE(ready.Result == CampaignProtocolResult::Applied);
    REQUIRE(ready.Version == 4);
    REQUIRE(ready.Snapshot->Roster[1].Ready);

    const auto readyNoOp = fixture.Admission.SetReady(
        2, created.CampaignId, "mutation-ready-noop", 4, true);
    REQUIRE(readyNoOp.Result == CampaignProtocolResult::AcceptedNoOp);
    REQUIRE(readyNoOp.Version == 4);
    const auto readyReplay = fixture.Admission.SetReady(
        2, created.CampaignId, "mutation-ready-noop", 4, true);
    REQUIRE(readyReplay.Result == CampaignProtocolResult::IdempotentReplay);
    REQUIRE(readyReplay.Version == 4);
    const auto readyConflict = fixture.Admission.SetReady(
        2, created.CampaignId, "mutation-ready-noop", 4, false);
    REQUIRE(readyConflict.Result == CampaignProtocolResult::IdempotencyConflict);

    const auto withdrawn = fixture.Admission.SetReady(
        2, created.CampaignId, "mutation-ready-withdraw", 4, false);
    REQUIRE(withdrawn.Result == CampaignProtocolResult::Applied);
    REQUIRE(withdrawn.Version == 5);
    REQUIRE_FALSE(withdrawn.Snapshot->Roster[1].Ready);
}

TEST_CASE("Single-player campaign start keeps its canonical admission ACTIVE", "[campaign.admission][helgen]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(created.Succeeded());

    const auto started = fixture.Admission.StartCampaign(
        1, created.CampaignId, "mutation-start-solo-roster", 1, true, true);
    REQUIRE(started.Result == CampaignProtocolResult::Applied);
    REQUIRE(started.Snapshot);
    REQUIRE(started.Snapshot->RosterSealed);
    REQUIRE(started.Snapshot->Roster.size() == 1);
    REQUIRE(started.Snapshot->Roster.front().Present);
    REQUIRE(started.Snapshot->RuntimeState ==
        static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE));

    const auto* admission =
        static_cast<const CampaignAdmissionService&>(fixture.Admission)
            .FindConnection(1);
    REQUIRE(admission);
    REQUIRE(admission->AdmittedIdentity);
    REQUIRE(admission->AdmittedIdentity->Campaign.Value == created.CampaignId);
    REQUIRE(admission->AdmittedIdentity->Slot.Value == created.CampaignSlotId);
}

TEST_CASE("Sealed reconnect admission preserves roster and derives runtime presence", "[campaign.admission][reconnect]")
{
    ProtocolFixture fixture;
    const auto created = fixture.CreateHost();
    REQUIRE(fixture.Admission.RegisterConnection(2, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto joined = fixture.Admission.JoinCampaign(
        2, created.CampaignId, "mutation-join", 1, true);
    REQUIRE(joined.Succeeded());
    const std::string memberBinding = joined.CharacterBindingId;
    const std::string memberSlot = joined.CampaignSlotId;
    const auto started = fixture.Admission.StartCampaign(
        1, created.CampaignId, "mutation-start", 2, true, true);
    REQUIRE(started.Succeeded());
    REQUIRE(started.Snapshot->RuntimeState ==
        static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE));

    REQUIRE(fixture.Admission.RegisterConnection(20, TestPlayerId(2)) ==
        CampaignConnectionRegistration::DuplicateActivePlayerId);
    REQUIRE(fixture.Admission.RegisterConnection(3, TestPlayerId(3)) ==
        CampaignConnectionRegistration::Accepted);
    const auto sealedJoin = fixture.Admission.JoinCampaign(
        3, created.CampaignId, "mutation-extra", 3, true);
    REQUIRE(sealedJoin.Result == CampaignProtocolResult::RosterSealed);
    const auto unchangedByExtra = fixture.Admission.BuildSnapshot(
        CampaignId{created.CampaignId});
    REQUIRE(unchangedByExtra);
    REQUIRE(unchangedByExtra->RuntimeState ==
        static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE));

    const auto disconnected = fixture.Admission.Disconnect(2);
    REQUIRE(disconnected);
    REQUIRE(disconnected->RuntimeState ==
        static_cast<std::uint8_t>(CampaignRuntimeState::WAITING_FOR_ROSTER));
    REQUIRE(disconnected->Roster.size() == 2);
    REQUIRE_FALSE(disconnected->Roster[1].Present);

    const auto durable = fixture.Runtime.LoadCampaign(
        CampaignId{created.CampaignId});
    REQUIRE(durable.Succeeded());
    REQUIRE(durable.Campaign.RosterSealed);
    REQUIRE(durable.Campaign.Roster.size() == 2);

    REQUIRE(fixture.Admission.RegisterConnection(22, TestPlayerId(2)) ==
        CampaignConnectionRegistration::Accepted);
    const auto wrongBinding = fixture.Admission.ResumeCampaign(
        22, created.CampaignId, "binding-wrong");
    REQUIRE(wrongBinding.Result == CampaignProtocolResult::BindingMismatch);
    const auto resumed = fixture.Admission.ResumeCampaign(
        22, created.CampaignId, memberBinding);
    REQUIRE(resumed.Result == CampaignProtocolResult::Applied);
    REQUIRE(resumed.Version == 3);
    REQUIRE(resumed.CampaignSlotId == memberSlot);
    REQUIRE(resumed.Snapshot->RuntimeState ==
        static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE));

    const auto unknownPlayer = fixture.Admission.ResumeCampaign(
        3, created.CampaignId, memberBinding);
    REQUIRE(unknownPlayer.Result == CampaignProtocolResult::IdentityMismatch);

    const auto unchanged = fixture.Runtime.LoadCampaign(
        CampaignId{created.CampaignId});
    REQUIRE(unchanged.Succeeded());
    REQUIRE(unchanged.Campaign.Version == 3);
    REQUIRE(unchanged.Campaign.Roster.size() == 2);
}

TEST_CASE("Normal campaign protocol scales to two four and ten slots", "[campaign.admission][scale]")
{
    for (const std::size_t rosterSize : {2u, 4u, 10u})
    {
        ProtocolFixture fixture;
        const auto created = fixture.CreateHost();
        REQUIRE(created.Succeeded());
        StateVersion revision = 1;
        for (std::size_t index = 2; index <= rosterSize; ++index)
        {
            REQUIRE(fixture.Admission.RegisterConnection(
                index, TestPlayerId(index)) ==
                CampaignConnectionRegistration::Accepted);
            const auto joined = fixture.Admission.JoinCampaign(
                index, created.CampaignId,
                "mutation-join-" + std::to_string(index),
                revision, true);
            REQUIRE(joined.Result == CampaignProtocolResult::Applied);
            revision = joined.Version;
            REQUIRE(joined.CampaignSlotId ==
                (index < 10 ? "slot-0" : "slot-") +
                    std::to_string(index));
        }

        const auto started = fixture.Admission.StartCampaign(
            1, created.CampaignId, "mutation-start", revision, true, true);
        REQUIRE(started.Result == CampaignProtocolResult::Applied);
        revision = started.Version;
        REQUIRE(started.Snapshot);
        REQUIRE(started.Snapshot->Roster.size() == rosterSize);
        REQUIRE(started.Snapshot->RuntimeState ==
            static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE));
        for (const CampaignPublicSlotData& slot : started.Snapshot->Roster)
            REQUIRE(slot.Present);

        for (std::size_t index = 1; index <= rosterSize; ++index)
        {
            const auto ready = fixture.Admission.SetReady(
                index, created.CampaignId,
                "mutation-ready-" + std::to_string(index),
                revision, true);
            REQUIRE(ready.Result == CampaignProtocolResult::Applied);
            revision = ready.Version;
        }
        const auto finalSnapshot = fixture.Admission.BuildSnapshot(
            CampaignId{created.CampaignId});
        REQUIRE(finalSnapshot);
        REQUIRE(finalSnapshot->Roster.size() == rosterSize);
        for (const CampaignPublicSlotData& slot : finalSnapshot->Roster)
            REQUIRE(slot.Ready);
    }
}
