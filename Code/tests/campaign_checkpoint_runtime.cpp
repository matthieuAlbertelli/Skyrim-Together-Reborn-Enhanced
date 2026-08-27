#include <CampaignRuntimeService.h>
#include <Structs/NativeSaveBundle.h>
#include <campaign_persistence_test_helpers.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <memory>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

namespace
{
CampaignSlotRecord Slot(std::size_t aIndex)
{
    const std::string suffix = std::to_string(aIndex);
    return {
        CampaignSlotId{"slot-" + suffix},
        PlayerId{"player-" + suffix},
        CharacterBindingId{"binding-" + suffix}};
}

CampaignMemberIdentity Identity(
    const CampaignId& acCampaign,
    std::size_t aIndex)
{
    const auto slot = Slot(aIndex);
    return {
        acCampaign, slot.Slot, slot.Player, slot.CharacterBinding};
}

std::vector<CampaignMemberPresence> Presence(
    const CampaignId& acCampaign,
    std::size_t aCount)
{
    std::vector<CampaignMemberPresence> result;
    for (std::size_t index = 1; index <= aCount; ++index)
        result.push_back({Identity(acCampaign, index), true, true});
    return result;
}

NativeSaveBundleArtifact Artifact(
    const std::string& acIdentity,
    std::uint8_t aMarker)
{
    std::vector<NativeSaveBundleMember> members(2);
    members[0].Role = NativeSaveMemberRole::Ess;
    members[0].Size = 100 + aMarker;
    members[0].Sha256.fill(aMarker);
    members[1].Role = NativeSaveMemberRole::Skse;
    members[1].Size = 200 + aMarker;
    members[1].Sha256.fill(static_cast<std::uint8_t>(aMarker + 1));
    const auto artifact = BuildNativeSaveBundleArtifact(
        acIdentity, std::move(members));
    REQUIRE(artifact.Succeeded());
    return artifact.Value;
}

struct CheckpointFixture
{
    explicit CheckpointFixture(
        std::size_t aRosterSize,
        std::string aCampaign = "campaign-checkpoint")
        : Store(OpenStore(Database))
        , Runtime(*Store)
        , Campaign{std::move(aCampaign)}
        , RosterSize(aRosterSize)
    {
        std::vector<CampaignSlotRecord> roster;
        for (std::size_t index = 1; index <= aRosterSize; ++index)
            roster.push_back(Slot(index));
        REQUIRE(Runtime.CreateLobbyCampaign(
            {Campaign, MutationId{"create-" + Campaign.Value}, roster})
                    .Succeeded());
        if (aRosterSize != 0)
        {
            REQUIRE(Runtime.CommitCampaignStart(
                {Campaign,
                 1,
                 MutationId{"seal-" + Campaign.Value},
                 PlayerId{"player-1"}})
                        .Succeeded());
        }
    }

    CampaignCheckpointCommandResult Begin(
        std::string aCheckpoint = "cp-1",
        std::vector<CampaignMemberPresence> aPresence = {})
    {
        if (aPresence.empty() && RosterSize != 0)
            aPresence = Presence(Campaign, RosterSize);
        return Runtime.BeginCheckpoint(
            {Campaign,
             CheckpointId{aCheckpoint},
             "stre-" + aCheckpoint,
             std::move(aPresence)});
    }

    CampaignCheckpointCommandResult Ack(
        std::size_t aSlot,
        const NativeSaveBundleArtifact& acArtifact,
        std::string aCheckpoint = "cp-1")
    {
        return Runtime.RecordCheckpointSave(
            {Campaign,
             CheckpointId{aCheckpoint},
             "stre-" + aCheckpoint,
             Identity(Campaign, aSlot),
             std::string(kNativeSaveFingerprintAlgorithm),
             kNativeSaveFingerprintVersion,
             Bytes(acArtifact.Fingerprint.begin(),
                   acArtifact.Fingerprint.end()),
             kNativeSaveMetadataCodecVersion,
             acArtifact.Metadata});
    }

    TemporaryDatabase Database;
    std::unique_ptr<SqliteCampaignStore> Store;
    CampaignRuntimeService Runtime;
    CampaignId Campaign;
    std::size_t RosterSize{};
};
}

TEST_CASE(
    "Checkpoint begin requires an exact sealed admitted roster",
    "[campaign.checkpoint][runtime][begin]")
{
    CheckpointFixture fixture(2);
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, Presence(fixture.Campaign, 2)).RuntimeState ==
        CampaignRuntimeState::ACTIVE);

    const auto missing = fixture.Runtime.BeginCheckpoint(
        {CampaignId{"missing"},
         CheckpointId{"cp-missing"},
         "stre-cp-missing",
         {}});
    REQUIRE_FALSE(missing.Succeeded());
    REQUIRE(missing.Command.PersistenceError == StoreError::NotFound);

    auto incompletePresence = Presence(fixture.Campaign, 2);
    incompletePresence.pop_back();
    const auto incomplete = fixture.Begin("cp-incomplete", incompletePresence);
    REQUIRE(incomplete.Command.Error == CampaignError::RosterIncomplete);

    const auto begun = fixture.Begin();
    REQUIRE(begun.Succeeded());
    REQUIRE(begun.Activity);
    REQUIRE(begun.Activity->SourceRevision == 2);
    REQUIRE(begun.Activity->Checkpoint == CheckpointId{"cp-1"});
    REQUIRE(begun.Activity->NativeSaveIdentity == "stre-cp-1");
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, Presence(fixture.Campaign, 2)).RuntimeState ==
        CampaignRuntimeState::CHECKPOINTING);

    const auto candidate = fixture.Store->LoadCheckpoint(
        fixture.Campaign, CheckpointId{"cp-1"});
    REQUIRE(candidate.Succeeded());
    REQUIRE(candidate.Value.State == CheckpointState::Candidate);
    REQUIRE(candidate.Value.SourceRevision == 2);
    REQUIRE(candidate.Value.Slots.size() == 2);
    const auto coalesced = fixture.Begin("cp-2");
    REQUIRE(coalesced.Command.Error == CampaignError::CheckpointInProgress);
    REQUIRE(coalesced.Activity);
    REQUIRE(coalesced.Activity->Checkpoint == CheckpointId{"cp-1"});
    REQUIRE(fixture.Store->LoadCheckpoint(
        fixture.Campaign, CheckpointId{"cp-2"}).Error ==
        StoreError::NotFound);
}

TEST_CASE(
    "Unsealed and empty campaigns cannot begin checkpoints",
    "[campaign.checkpoint][runtime][begin]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CampaignRuntimeService runtime(*store);
    const CampaignId campaign{"campaign-lobby"};
    REQUIRE(runtime.CreateLobbyCampaign(
        {campaign, MutationId{"create-lobby"}, {}}).Succeeded());
    const auto empty = runtime.BeginCheckpoint(
        {campaign, CheckpointId{"cp-1"}, "stre-cp-1", {}});
    REQUIRE(empty.Command.Error == CampaignError::RosterNotSealed);
}

TEST_CASE(
    "Checkpoint mutation fence is per campaign and permits only checkpoint progress",
    "[campaign.checkpoint][runtime][fence][multi-campaign]")
{
    CheckpointFixture first(2, "campaign-a");
    REQUIRE(first.Begin().Succeeded());
    const auto fenced = first.Runtime.SetReady(
        {first.Campaign,
         3,
         MutationId{"normal-mutation"},
         Identity(first.Campaign, 1),
         true});
    REQUIRE(fenced.Error == CampaignError::CheckpointInProgress);

    const CampaignId second{"campaign-b"};
    REQUIRE(first.Runtime.CreateLobbyCampaign(
        {second, MutationId{"create-b"}, {Slot(1)}}).Succeeded());
    REQUIRE(first.Runtime.CommitCampaignStart(
        {second, 1, MutationId{"seal-b"}, PlayerId{"player-1"}})
                .Succeeded());
    REQUIRE(first.Runtime.SetReady(
        {second,
         2,
         MutationId{"ready-b"},
         Identity(second, 1),
         true}).Succeeded());
    REQUIRE(first.Runtime.LoadCampaign(
        second, Presence(second, 1)).RuntimeState ==
        CampaignRuntimeState::ACTIVE);
}

TEST_CASE(
    "Checkpoint ACK authority validation is exact idempotent and fail closed",
    "[campaign.checkpoint][runtime][ack][robustness]")
{
    CheckpointFixture fixture(2);
    REQUIRE(fixture.Begin().Succeeded());
    const auto firstArtifact = Artifact("stre-cp-1", 1);
    const auto secondArtifact = Artifact("stre-cp-1", 2);

    auto wrongCampaign = fixture.Runtime.RecordCheckpointSave(
        {CampaignId{"other"},
         CheckpointId{"cp-1"},
         "stre-cp-1",
         Identity(fixture.Campaign, 1),
         std::string(kNativeSaveFingerprintAlgorithm),
         kNativeSaveFingerprintVersion,
         Bytes(firstArtifact.Fingerprint.begin(), firstArtifact.Fingerprint.end()),
         kNativeSaveMetadataCodecVersion,
         firstArtifact.Metadata});
    REQUIRE(wrongCampaign.Command.Error == CampaignError::CheckpointNotActive);

    REQUIRE(fixture.Ack(1, firstArtifact, "cp-wrong").Command.Error ==
        CampaignError::CheckpointMismatch);
    auto wrongIdentity = fixture.Runtime.RecordCheckpointSave(
        {fixture.Campaign,
         CheckpointId{"cp-1"},
         "stre-other",
         Identity(fixture.Campaign, 1),
         std::string(kNativeSaveFingerprintAlgorithm),
         kNativeSaveFingerprintVersion,
         Bytes(firstArtifact.Fingerprint.begin(), firstArtifact.Fingerprint.end()),
         kNativeSaveMetadataCodecVersion,
         firstArtifact.Metadata});
    REQUIRE(wrongIdentity.Command.Error == CampaignError::CheckpointMismatch);

    auto outsider = Identity(fixture.Campaign, 1);
    outsider.Player = PlayerId{"player-other"};
    auto wrongActor = fixture.Runtime.RecordCheckpointSave(
        {fixture.Campaign,
         CheckpointId{"cp-1"},
         "stre-cp-1",
         outsider,
         std::string(kNativeSaveFingerprintAlgorithm),
         kNativeSaveFingerprintVersion,
         Bytes(firstArtifact.Fingerprint.begin(), firstArtifact.Fingerprint.end()),
         kNativeSaveMetadataCodecVersion,
         firstArtifact.Metadata});
    REQUIRE(wrongActor.Command.Error == CampaignError::NotCampaignMember);

    const auto first = fixture.Ack(1, firstArtifact);
    REQUIRE(first.Succeeded());
    REQUIRE_FALSE(first.Committed);
    const auto replay = fixture.Ack(1, firstArtifact);
    REQUIRE(replay.Succeeded());
    REQUIRE(replay.Command.IdempotentReplay);

    const auto conflict = fixture.Ack(1, secondArtifact);
    REQUIRE_FALSE(conflict.Succeeded());
    REQUIRE(conflict.Command.PersistenceError ==
        StoreError::IdempotencyConflict);

    auto missingFingerprint = fixture.Runtime.RecordCheckpointSave(
        {fixture.Campaign,
         CheckpointId{"cp-1"},
         "stre-cp-1",
         Identity(fixture.Campaign, 2),
         std::string(kNativeSaveFingerprintAlgorithm),
         kNativeSaveFingerprintVersion,
         {},
         kNativeSaveMetadataCodecVersion,
         secondArtifact.Metadata});
    REQUIRE(missingFingerprint.Command.Error ==
        CampaignError::InvalidCheckpointArtifact);
    auto missingMetadata = secondArtifact;
    missingMetadata.Metadata.clear();
    REQUIRE(fixture.Ack(2, missingMetadata).Command.Error ==
        CampaignError::InvalidCheckpointArtifact);

    auto invalidCodec = fixture.Runtime.RecordCheckpointSave(
        {fixture.Campaign,
         CheckpointId{"cp-1"},
         "stre-cp-1",
         Identity(fixture.Campaign, 2),
         std::string(kNativeSaveFingerprintAlgorithm),
         kNativeSaveFingerprintVersion,
         Bytes(secondArtifact.Fingerprint.begin(), secondArtifact.Fingerprint.end()),
         99,
         secondArtifact.Metadata});
    REQUIRE(invalidCodec.Command.Error ==
        CampaignError::InvalidCheckpointArtifact);

    auto oversized = secondArtifact.Metadata;
    oversized.resize(kMaximumNativeSaveMetadataSize + 1);
    auto oversizedResult = fixture.Runtime.RecordCheckpointSave(
        {fixture.Campaign,
         CheckpointId{"cp-1"},
         "stre-cp-1",
         Identity(fixture.Campaign, 2),
         std::string(kNativeSaveFingerprintAlgorithm),
         kNativeSaveFingerprintVersion,
         Bytes(secondArtifact.Fingerprint.begin(), secondArtifact.Fingerprint.end()),
         kNativeSaveMetadataCodecVersion,
         std::move(oversized)});
    REQUIRE(oversizedResult.Command.Error ==
        CampaignError::InvalidCheckpointArtifact);

    Bytes oversizedFingerprint(
        secondArtifact.Fingerprint.begin(),
        secondArtifact.Fingerprint.end());
    oversizedFingerprint.push_back(0);
    auto oversizedFingerprintResult = fixture.Runtime.RecordCheckpointSave(
        {fixture.Campaign,
         CheckpointId{"cp-1"},
         "stre-cp-1",
         Identity(fixture.Campaign, 2),
         std::string(kNativeSaveFingerprintAlgorithm),
         kNativeSaveFingerprintVersion,
         std::move(oversizedFingerprint),
         kNativeSaveMetadataCodecVersion,
         secondArtifact.Metadata});
    REQUIRE(oversizedFingerprintResult.Command.Error ==
        CampaignError::InvalidCheckpointArtifact);

    const auto committed = fixture.Ack(2, secondArtifact);
    REQUIRE(committed.Succeeded());
    REQUIRE(committed.Committed);
    REQUIRE_FALSE(fixture.Runtime.GetActiveCheckpoint(fixture.Campaign));
    const auto stored = fixture.Store->LoadLastCommittedCheckpoint(
        fixture.Campaign);
    REQUIRE(stored.Succeeded());
    REQUIRE(stored.Value.Id == CheckpointId{"cp-1"});
}

TEST_CASE(
    "Server restart does not resume an unfinished checkpoint activity",
    "[campaign.checkpoint][runtime][restart]")
{
    TemporaryDatabase database;
    const CampaignId campaign{"campaign-restart"};
    {
        auto store = OpenStore(database);
        CampaignRuntimeService runtime(*store);
        REQUIRE(runtime.CreateLobbyCampaign(
            {campaign, MutationId{"create-restart"}, {Slot(1)}})
                    .Succeeded());
        REQUIRE(runtime.CommitCampaignStart(
            {campaign,
             1,
             MutationId{"seal-restart"},
             PlayerId{"player-1"}})
                    .Succeeded());
        REQUIRE(runtime.BeginCheckpoint(
            {campaign,
             CheckpointId{"cp-before-crash"},
             "stre-cp-before-crash",
             Presence(campaign, 1)})
                    .Succeeded());
    }
    {
        auto store = OpenStore(database);
        CampaignRuntimeService restarted(*store);
        REQUIRE_FALSE(restarted.GetActiveCheckpoint(campaign));
        const auto loaded = restarted.LoadCampaign(
            campaign, Presence(campaign, 1));
        REQUIRE(loaded.Succeeded());
        REQUIRE(loaded.RuntimeState == CampaignRuntimeState::ACTIVE);
        const auto candidate = store->LoadCheckpoint(
            campaign, CheckpointId{"cp-before-crash"});
        REQUIRE(candidate.Succeeded());
        REQUIRE(candidate.Value.State == CheckpointState::Candidate);
        REQUIRE_FALSE(store->LoadLastCommittedCheckpoint(campaign).Succeeded());
    }
}

TEST_CASE(
    "Checkpoint commits exact rosters at supported campaign sizes",
    "[campaign.checkpoint][runtime][commit][scale]")
{
    for (const std::size_t rosterSize : {2u, 4u, 10u})
    {
        DYNAMIC_SECTION("roster=" << rosterSize)
        {
            CheckpointFixture fixture(rosterSize);
            REQUIRE(fixture.Begin().Succeeded());
            for (std::size_t index = rosterSize; index >= 1; --index)
            {
                const auto ack = fixture.Ack(
                    index,
                    Artifact("stre-cp-1", static_cast<std::uint8_t>(index)));
                REQUIRE(ack.Succeeded());
                REQUIRE(ack.Committed == (index == 1));
            }
            REQUIRE(fixture.Store->LoadLastCommittedCheckpoint(
                fixture.Campaign).Value.Slots.size() == rosterSize);
        }
    }
}

TEST_CASE(
    "Failed or disconnected checkpoint preserves the previous commit",
    "[campaign.checkpoint][runtime][failure][disconnect]")
{
    CheckpointFixture fixture(2);
    REQUIRE(fixture.Begin("cp-1").Succeeded());
    REQUIRE(fixture.Ack(1, Artifact("stre-cp-1", 1), "cp-1").Succeeded());
    REQUIRE(fixture.Ack(2, Artifact("stre-cp-1", 2), "cp-1").Committed);

    REQUIRE(fixture.Begin("cp-2").Succeeded());
    REQUIRE(fixture.Ack(1, Artifact("stre-cp-2", 3), "cp-2").Succeeded());
    const auto failed = fixture.Runtime.FailCheckpoint(
        {fixture.Campaign,
         CheckpointId{"cp-2"},
         "stre-cp-2",
         Identity(fixture.Campaign, 2)});
    REQUIRE(failed.Succeeded());
    REQUIRE(fixture.Store->LoadLastCommittedCheckpoint(
        fixture.Campaign).Value.Id == CheckpointId{"cp-1"});
    REQUIRE(fixture.Store->LoadCheckpoint(
        fixture.Campaign, CheckpointId{"cp-2"}).Value.State ==
        CheckpointState::Candidate);
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, Presence(fixture.Campaign, 2)).RuntimeState ==
        CampaignRuntimeState::ACTIVE);

    REQUIRE(fixture.Begin("cp-3").Succeeded());
    fixture.Runtime.AbandonCheckpoint(fixture.Campaign);
    auto disconnectedPresence = Presence(fixture.Campaign, 2);
    disconnectedPresence.pop_back();
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, disconnectedPresence).RuntimeState ==
        CampaignRuntimeState::WAITING_FOR_ROSTER);
    REQUIRE(fixture.Store->LoadCheckpoint(
        fixture.Campaign, CheckpointId{"cp-3"}).Value.State ==
        CheckpointState::Candidate);
    REQUIRE(fixture.Runtime.RecordCheckpointSave(
        {fixture.Campaign,
         CheckpointId{"cp-3"},
         "stre-cp-3",
         Identity(fixture.Campaign, 1),
         {}, 0, {}, 0, {}}).Command.Error ==
        CampaignError::CheckpointNotActive);
}
