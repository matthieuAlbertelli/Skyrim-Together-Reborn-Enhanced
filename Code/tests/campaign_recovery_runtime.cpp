#include <CampaignRuntimeService.h>
#include <Structs/NativeSaveBundle.h>
#include <campaign_persistence_test_helpers.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <memory>
#include <unordered_set>

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
    const CampaignSlotRecord slot = Slot(aIndex);
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

struct RecoveryFixture
{
    explicit RecoveryFixture(std::size_t aRosterSize = 2)
        : RosterSize(aRosterSize)
        , Store(OpenStore(Database))
        , Runtime(*Store)
    {
        std::vector<CampaignSlotRecord> roster;
        for (std::size_t index = 1; index <= aRosterSize; ++index)
            roster.push_back(Slot(index));
        REQUIRE(Runtime.CreateLobbyCampaign(
            {Campaign, MutationId{"create-recovery"}, roster}).Succeeded());
        REQUIRE(Runtime.CommitCampaignStart(
            {Campaign, 1, MutationId{"seal-recovery"}, PlayerId{"player-1"}})
                    .Succeeded());
    }

    CampaignCheckpointCommandResult BeginCheckpoint(
        std::string aCheckpoint)
    {
        return Runtime.BeginCheckpoint(
            {Campaign,
             CheckpointId{aCheckpoint},
             "stre-" + aCheckpoint,
             Presence(Campaign, RosterSize)});
    }

    CampaignCheckpointCommandResult AckCheckpoint(
        std::string aCheckpoint,
        std::size_t aSlot,
        const NativeSaveBundleArtifact& acArtifact)
    {
        return Runtime.RecordCheckpointSave(
            {Campaign,
             CheckpointId{aCheckpoint},
             "stre-" + aCheckpoint,
             Identity(Campaign, aSlot),
             std::string(kNativeSaveFingerprintAlgorithm),
             kNativeSaveFingerprintVersion,
             Bytes(acArtifact.Fingerprint.begin(), acArtifact.Fingerprint.end()),
             kNativeSaveMetadataCodecVersion,
             acArtifact.Metadata});
    }

    void CommitCheckpoint(std::string aCheckpoint)
    {
        REQUIRE(BeginCheckpoint(aCheckpoint).Succeeded());
        for (std::size_t index = 1; index <= RosterSize; ++index)
        {
            const auto acknowledged = AckCheckpoint(
                aCheckpoint,
                index,
                Artifact(
                    "stre-" + aCheckpoint,
                    static_cast<std::uint8_t>(index)));
            REQUIRE(acknowledged.Succeeded());
            REQUIRE(acknowledged.Committed == (index == RosterSize));
        }
    }

    CampaignRecoveryCommandResult Disconnect(std::size_t aSlot = 2)
    {
        auto presence = Presence(Campaign, RosterSize);
        presence.erase(presence.begin() + static_cast<std::ptrdiff_t>(aSlot - 1));
        return Runtime.BeginRecovery(
            {Campaign,
             Identity(Campaign, aSlot),
             std::move(presence),
             CampaignRuntimeState::ACTIVE});
    }

    StateVersion RecoverCommittedCheckpoint(std::string aCheckpoint)
    {
        const auto begun = Disconnect(RosterSize);
        REQUIRE(begun.Succeeded());
        REQUIRE(begun.Activity);
        const auto prepared = Runtime.PrepareRecovery(
            {Campaign, Presence(Campaign, RosterSize)});
        REQUIRE(prepared.Succeeded());
        REQUIRE(prepared.Dispatch == CampaignRecoveryDispatch::NativeLoad);

        CampaignRecoveryCommandResult restored;
        for (std::size_t index = 1; index <= RosterSize; ++index)
        {
            const auto artifact = Artifact(
                "stre-" + aCheckpoint,
                static_cast<std::uint8_t>(index));
            const auto loaded = Runtime.RecordRecoveryLoaded(
                LoadedCommand(*prepared.Activity, index, artifact));
            REQUIRE(loaded.Succeeded());
            REQUIRE(loaded.FirstBarrierCompleted == (index == RosterSize));
            if (index == RosterSize)
                restored = loaded;
        }
        REQUIRE(restored.Dispatch ==
            CampaignRecoveryDispatch::RestoredSnapshot);
        REQUIRE(restored.Activity);
        REQUIRE(restored.Activity->RestoreRevision);
        const StateVersion restoreRevision =
            *restored.Activity->RestoreRevision;

        const auto authoritative = Runtime.LoadCampaign(
            Campaign, Presence(Campaign, RosterSize));
        REQUIRE(authoritative.Succeeded());
        REQUIRE(authoritative.Campaign.Version == restoreRevision);
        REQUIRE(authoritative.RuntimeState ==
            CampaignRuntimeState::RESTORING_CHECKPOINT);

        for (std::size_t index = 1; index <= RosterSize; ++index)
        {
            const auto applied = Runtime.RecordRecoverySnapshotApplied(
                {Campaign,
                 restored.Activity->Attempt,
                 *restored.Activity->Checkpoint,
                 restoreRevision,
                 Identity(Campaign, index),
                 Presence(Campaign, RosterSize)});
            REQUIRE(applied.Succeeded());
            REQUIRE(applied.RecoveryCompleted == (index == RosterSize));
        }
        REQUIRE_FALSE(Runtime.GetRecoveryActivity(Campaign));
        return restoreRevision;
    }

    RecordCampaignRecoveryLoadedCommand LoadedCommand(
        const CampaignRecoveryActivity& acActivity,
        std::size_t aSlot,
        const NativeSaveBundleArtifact& acArtifact) const
    {
        return {
            Campaign,
            acActivity.Attempt,
            *acActivity.Checkpoint,
            Identity(Campaign, aSlot),
            true,
            acArtifact.Bundle.LogicalIdentity,
            std::string(kNativeSaveFingerprintAlgorithm),
            kNativeSaveFingerprintVersion,
            Bytes(acArtifact.Fingerprint.begin(), acArtifact.Fingerprint.end()),
            kNativeSaveMetadataCodecVersion,
            acArtifact.Metadata,
            Presence(Campaign, RosterSize)};
    }

    std::size_t RosterSize;
    TemporaryDatabase Database;
    std::unique_ptr<SqliteCampaignStore> Store;
    CampaignRuntimeService Runtime;
    CampaignId Campaign{"campaign-recovery"};
};
}

TEST_CASE(
    "Restore revisions remain canonical across consecutive checkpoint recovery cycles",
    "[campaign.recovery][runtime][sequential][canonical-snapshot]")
{
    for (const std::size_t rosterSize : {1u, 2u})
    {
        RecoveryFixture fixture(rosterSize);
        std::optional<StateVersion> previousRestore;
        for (std::size_t cycle = 1; cycle <= 3; ++cycle)
        {
            const std::string checkpoint =
                "cp-cycle-" + std::to_string(cycle);
            fixture.CommitCheckpoint(checkpoint);
            const auto durable = fixture.Store->LoadCheckpoint(
                fixture.Campaign, CheckpointId{checkpoint});
            REQUIRE(durable.Succeeded());
            REQUIRE(durable.Value.SnapshotCoreStateCodecVersion > 0);
            REQUIRE_FALSE(durable.Value.SnapshotCoreStatePayload.empty());
            if (previousRestore)
                REQUIRE(durable.Value.SourceRevision == *previousRestore);

            const StateVersion restored =
                fixture.RecoverCommittedCheckpoint(checkpoint);
            if (previousRestore)
                REQUIRE(restored > *previousRestore);
            previousRestore = restored;
            const auto active = fixture.Runtime.LoadCampaign(
                fixture.Campaign,
                Presence(fixture.Campaign, rosterSize));
            REQUIRE(active.Succeeded());
            REQUIRE(active.Campaign.Version == restored);
            REQUIRE(active.RuntimeState == CampaignRuntimeState::ACTIVE);
        }

        const auto journal = fixture.Store->LoadJournal(fixture.Campaign);
        REQUIRE(journal.Succeeded());
        REQUIRE(std::count_if(
            journal.Value.begin(),
            journal.Value.end(),
            [](const JournalRecord& acRecord)
            {
                return acRecord.Kind == "RestoreCheckpoint";
            }) == 3);
    }
}

TEST_CASE(
    "Required sealed-roster disconnect durably enters recovery lock and fences mutations",
    "[campaign.recovery][runtime][disconnect][fence]")
{
    RecoveryFixture fixture;
    const auto begun = fixture.Disconnect();
    REQUIRE(begun.Succeeded());
    REQUIRE(begun.Activity);
    REQUIRE(begun.Activity->EntryRevision == begun.Command.Version);
    REQUIRE_FALSE(begun.Activity->Attempt.Value.empty());

    const auto repeatedDisconnect = fixture.Disconnect();
    REQUIRE(repeatedDisconnect.Succeeded());
    REQUIRE(repeatedDisconnect.Command.IdempotentReplay);
    REQUIRE(repeatedDisconnect.Activity->Attempt == begun.Activity->Attempt);

    auto incomplete = Presence(fixture.Campaign, 2);
    incomplete.pop_back();
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, incomplete).RuntimeState ==
        CampaignRuntimeState::RECOVERY_LOCK);
    REQUIRE(fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, incomplete}).Command.Error ==
        CampaignError::RosterIncomplete);
    REQUIRE(fixture.Runtime.SetReady(
        {fixture.Campaign,
         begun.Command.Version,
         MutationId{"ready-during-recovery"},
         Identity(fixture.Campaign, 1),
         true}).Error == CampaignError::RecoveryInProgress);
    REQUIRE(fixture.Runtime.BeginCheckpoint(
        {fixture.Campaign,
         CheckpointId{"cp-forbidden"},
         "stre-cp-forbidden",
         Presence(fixture.Campaign, 2)}).Command.Error ==
        CampaignError::RecoveryInProgress);

    const auto noCheckpoint = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(noCheckpoint.Command.Error ==
        CampaignError::NoCommittedCheckpoint);
    REQUIRE(noCheckpoint.Activity->Reason ==
        CampaignRecoveryReason::NoCommittedCheckpoint);
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, Presence(fixture.Campaign, 2)).RuntimeState ==
        CampaignRuntimeState::RECOVERY_LOCK);
}

TEST_CASE(
    "Explicit campaign load opens the existing recovery from ACTIVE with full roster",
    "[campaign.load][campaign.recovery][runtime]")
{
    RecoveryFixture fixture;
    fixture.CommitCheckpoint("cp-load");
    const auto begun = fixture.Runtime.BeginRecovery({
        fixture.Campaign,
        Identity(fixture.Campaign, 1),
        Presence(fixture.Campaign, 2),
        CampaignRuntimeState::ACTIVE,
        true});
    REQUIRE(begun.Succeeded());
    REQUIRE(begun.Activity);
    REQUIRE(begun.Activity->Reason ==
        CampaignRecoveryReason::CampaignLoadRequested);

    const auto prepared = fixture.Runtime.PrepareRecovery({
        fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(prepared.Succeeded());
    REQUIRE(prepared.Dispatch == CampaignRecoveryDispatch::NativeLoad);
    REQUIRE(prepared.Checkpoint->Id == CheckpointId{"cp-load"});
}

TEST_CASE(
    "A one-member sealed roster crosses the generic recovery barriers",
    "[campaign.recovery][runtime][barriers][roster-one]")
{
    RecoveryFixture fixture(1);
    fixture.CommitCheckpoint("cp-one");

    const auto begun = fixture.Disconnect(1);
    REQUIRE(begun.Succeeded());
    REQUIRE(begun.Activity);
    const RestoreAttemptId attempt = begun.Activity->Attempt;

    const auto prepared = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 1)});
    REQUIRE(prepared.Succeeded());
    REQUIRE(prepared.Dispatch == CampaignRecoveryDispatch::NativeLoad);
    REQUIRE(prepared.Activity->Attempt == attempt);

    const auto artifact = Artifact("stre-cp-one", 1);
    auto stale = fixture.LoadedCommand(
        *prepared.Activity, 1, artifact);
    stale.Attempt.Value += "-stale";
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(stale).Command.Error ==
        CampaignError::RecoveryMismatch);

    const auto restored = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 1, artifact));
    REQUIRE(restored.Succeeded());
    REQUIRE(restored.FirstBarrierCompleted);
    REQUIRE(restored.Dispatch ==
        CampaignRecoveryDispatch::RestoredSnapshot);
    REQUIRE(restored.Activity->RestoreRevision);

    const auto loadedReplay = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 1, artifact));
    REQUIRE(loadedReplay.Succeeded());
    REQUIRE(loadedReplay.Command.IdempotentReplay);
    REQUIRE(loadedReplay.Activity->Attempt == attempt);
    REQUIRE(loadedReplay.Activity->RestoreRevision ==
        restored.Activity->RestoreRevision);

    const StateVersion restoreRevision =
        *restored.Activity->RestoreRevision;
    const auto completed = fixture.Runtime.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         attempt,
         *restored.Activity->Checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 1),
         Presence(fixture.Campaign, 1)});
    REQUIRE(completed.Succeeded());
    REQUIRE(completed.RecoveryCompleted);
    REQUIRE_FALSE(fixture.Runtime.GetRecoveryActivity(fixture.Campaign));
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, Presence(fixture.Campaign, 1)).RuntimeState ==
        CampaignRuntimeState::ACTIVE);

    const auto appliedReplay =
        fixture.Runtime.RecordRecoverySnapshotApplied(
            {fixture.Campaign,
             attempt,
             *restored.Activity->Checkpoint,
             restoreRevision,
             Identity(fixture.Campaign, 1),
             Presence(fixture.Campaign, 1)});
    REQUIRE(appliedReplay.Succeeded());
    REQUIRE(appliedReplay.Command.IdempotentReplay);
    REQUIRE(appliedReplay.RecoveryCompleted);

    const auto journal = fixture.Store->LoadJournal(fixture.Campaign);
    REQUIRE(journal.Succeeded());
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "BeginRecovery";
        }) == 1);
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "RestoreCheckpoint";
        }) == 1);
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "CompleteRecovery";
        }) == 1);
}

TEST_CASE(
    "Collective recovery restores once and completes only after both exact barriers",
    "[campaign.recovery][runtime][barriers][idempotence]")
{
    RecoveryFixture fixture;
    fixture.CommitCheckpoint("cp-1");
    const StateVersion checkpointRevision =
        fixture.Store->LoadCampaign(fixture.Campaign).Value.CurrentRevision;

    const auto begun = fixture.Disconnect();
    REQUIRE(begun.Succeeded());
    const auto prepared = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(prepared.Succeeded());
    REQUIRE(prepared.Dispatch == CampaignRecoveryDispatch::NativeLoad);
    REQUIRE(prepared.Activity->Stage ==
        CampaignRecoveryStage::LoadingNativeSaves);
    REQUIRE(prepared.Checkpoint->Id == CheckpointId{"cp-1"});

    const auto firstArtifact = Artifact("stre-cp-1", 1);
    const auto secondArtifact = Artifact("stre-cp-1", 2);
    auto wrongCheckpoint = fixture.LoadedCommand(
        *prepared.Activity, 1, firstArtifact);
    wrongCheckpoint.Checkpoint = CheckpointId{"cp-stale"};
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(wrongCheckpoint).Command.Error ==
        CampaignError::RecoveryMismatch);

    auto wrongBinding = fixture.LoadedCommand(
        *prepared.Activity, 1, firstArtifact);
    wrongBinding.Actor.CharacterBinding = CharacterBindingId{"binding-wrong"};
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(wrongBinding).Command.Error ==
        CampaignError::NotCampaignMember);

    auto wrongProof = fixture.LoadedCommand(
        *prepared.Activity, 1, firstArtifact);
    wrongProof.Fingerprint.front() ^= 0xFF;
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(wrongProof).Command.Error ==
        CampaignError::InvalidCheckpointArtifact);

    auto failedLoad = fixture.LoadedCommand(
        *prepared.Activity, 1, firstArtifact);
    failedLoad.Succeeded = false;
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(failedLoad).Command.Error ==
        CampaignError::InvalidCheckpointArtifact);
    REQUIRE(fixture.Runtime.GetRecoveryActivity(fixture.Campaign)->Reason ==
        CampaignRecoveryReason::ClientLoadFailed);

    const auto first = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 1, firstArtifact));
    REQUIRE(first.Succeeded());
    REQUIRE_FALSE(first.FirstBarrierCompleted);
    const auto duplicate = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 1, firstArtifact));
    REQUIRE(duplicate.Succeeded());
    REQUIRE(duplicate.Command.IdempotentReplay);

    auto wrongAttempt = fixture.LoadedCommand(
        *prepared.Activity, 2, secondArtifact);
    wrongAttempt.Attempt.Value += "-stale";
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(wrongAttempt).Command.Error ==
        CampaignError::RecoveryMismatch);

    const auto restored = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 2, secondArtifact));
    REQUIRE(restored.Succeeded());
    REQUIRE(restored.FirstBarrierCompleted);
    REQUIRE(restored.Dispatch == CampaignRecoveryDispatch::RestoredSnapshot);
    REQUIRE(restored.Activity->RestoreRevision);
    REQUIRE(*restored.Activity->RestoreRevision > checkpointRevision);
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, Presence(fixture.Campaign, 2)).RuntimeState ==
        CampaignRuntimeState::RESTORING_CHECKPOINT);

    const auto delayedLoaded = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 1, firstArtifact));
    REQUIRE(delayedLoaded.Succeeded());
    REQUIRE(delayedLoaded.Command.IdempotentReplay);
    REQUIRE(delayedLoaded.Dispatch ==
        CampaignRecoveryDispatch::RestoredSnapshot);

    const StateVersion restoreRevision = *restored.Activity->RestoreRevision;
    REQUIRE(fixture.Runtime.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         restored.Activity->Attempt,
         *restored.Activity->Checkpoint,
         restoreRevision + 1,
         Identity(fixture.Campaign, 1),
         Presence(fixture.Campaign, 2)}).Command.Error ==
        CampaignError::RecoveryMismatch);
    const auto firstApplied = fixture.Runtime.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         restored.Activity->Attempt,
         *restored.Activity->Checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 1),
         Presence(fixture.Campaign, 2)});
    REQUIRE(firstApplied.Succeeded());
    REQUIRE_FALSE(firstApplied.RecoveryCompleted);
    const auto firstAppliedReplay =
        fixture.Runtime.RecordRecoverySnapshotApplied(
            {fixture.Campaign,
             restored.Activity->Attempt,
             *restored.Activity->Checkpoint,
             restoreRevision,
             Identity(fixture.Campaign, 1),
             Presence(fixture.Campaign, 2)});
    REQUIRE(firstAppliedReplay.Command.IdempotentReplay);

    const auto completed = fixture.Runtime.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         restored.Activity->Attempt,
         *restored.Activity->Checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 2),
         Presence(fixture.Campaign, 2)});
    REQUIRE(completed.Succeeded());
    REQUIRE(completed.RecoveryCompleted);
    REQUIRE_FALSE(fixture.Runtime.GetRecoveryActivity(fixture.Campaign));
    REQUIRE(fixture.Runtime.LoadCampaign(
        fixture.Campaign, Presence(fixture.Campaign, 2)).RuntimeState ==
        CampaignRuntimeState::ACTIVE);

    const auto delayedDuplicate =
        fixture.Runtime.RecordRecoverySnapshotApplied(
            {fixture.Campaign,
             restored.Activity->Attempt,
             *restored.Activity->Checkpoint,
             restoreRevision,
             Identity(fixture.Campaign, 2),
             Presence(fixture.Campaign, 2)});
    REQUIRE(delayedDuplicate.Succeeded());
    REQUIRE(delayedDuplicate.Command.IdempotentReplay);

    const auto journal = fixture.Store->LoadJournal(fixture.Campaign);
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "RestoreCheckpoint";
        }) == 1);
    const auto restore = std::find_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "RestoreCheckpoint";
        });
    REQUIRE(restore != journal.Value.end());
    REQUIRE(restore->RestoredFromRevision ==
        restored.Checkpoint->SourceRevision);
    REQUIRE(journal.Value.back().Kind == "CompleteRecovery");
}

TEST_CASE(
    "Recovery restart reconstructs the correct barrier without duplicating restore",
    "[campaign.recovery][runtime][restart]")
{
    TemporaryDatabase database;
    const CampaignId campaign{"campaign-restart-recovery"};
    RestoreAttemptId attempt;
    CheckpointId checkpoint;
    StateVersion restoreRevision{};
    {
        auto store = OpenStore(database);
        CampaignRuntimeService runtime(*store);
        REQUIRE(runtime.CreateLobbyCampaign(
            {campaign,
             MutationId{"create-restart-recovery"},
             {Slot(1), Slot(2)}}).Succeeded());
        REQUIRE(runtime.CommitCampaignStart(
            {campaign, 1, MutationId{"seal-restart-recovery"},
             PlayerId{"player-1"}}).Succeeded());
        REQUIRE(runtime.BeginCheckpoint(
            {campaign, CheckpointId{"cp-restart"},
             "stre-cp-restart", Presence(campaign, 2)}).Succeeded());
        const auto firstArtifact = Artifact("stre-cp-restart", 1);
        const auto secondArtifact = Artifact("stre-cp-restart", 2);
        REQUIRE(runtime.RecordCheckpointSave(
            {campaign, CheckpointId{"cp-restart"}, "stre-cp-restart",
             Identity(campaign, 1), std::string(kNativeSaveFingerprintAlgorithm),
             kNativeSaveFingerprintVersion,
             Bytes(firstArtifact.Fingerprint.begin(), firstArtifact.Fingerprint.end()),
             kNativeSaveMetadataCodecVersion, firstArtifact.Metadata}).Succeeded());
        REQUIRE(runtime.RecordCheckpointSave(
            {campaign, CheckpointId{"cp-restart"}, "stre-cp-restart",
             Identity(campaign, 2), std::string(kNativeSaveFingerprintAlgorithm),
             kNativeSaveFingerprintVersion,
             Bytes(secondArtifact.Fingerprint.begin(), secondArtifact.Fingerprint.end()),
             kNativeSaveMetadataCodecVersion, secondArtifact.Metadata}).Committed);

        auto missing = Presence(campaign, 2);
        missing.pop_back();
        const auto begun = runtime.BeginRecovery(
            {campaign,
             Identity(campaign, 2),
             missing,
             CampaignRuntimeState::ACTIVE});
        REQUIRE(begun.Succeeded());
        attempt = begun.Activity->Attempt;
    }
    {
        auto store = OpenStore(database);
        CampaignRuntimeService restarted(*store);
        REQUIRE(restarted.LoadCampaign(
            campaign, Presence(campaign, 2)).RuntimeState ==
            CampaignRuntimeState::RECOVERY_LOCK);
        const auto prepared = restarted.PrepareRecovery(
            {campaign, Presence(campaign, 2)});
        REQUIRE(prepared.Activity->Attempt == attempt);
        checkpoint = *prepared.Activity->Checkpoint;
        const auto firstArtifact = Artifact("stre-cp-restart", 1);
        const auto secondArtifact = Artifact("stre-cp-restart", 2);
        const auto command = [&](std::size_t aSlot,
                                 const NativeSaveBundleArtifact& acArtifact)
        {
            return RecordCampaignRecoveryLoadedCommand{
                campaign, attempt, checkpoint, Identity(campaign, aSlot), true,
                "stre-cp-restart", std::string(kNativeSaveFingerprintAlgorithm),
                kNativeSaveFingerprintVersion,
                Bytes(acArtifact.Fingerprint.begin(), acArtifact.Fingerprint.end()),
                kNativeSaveMetadataCodecVersion, acArtifact.Metadata,
                Presence(campaign, 2)};
        };
        REQUIRE(restarted.RecordRecoveryLoaded(
            command(1, firstArtifact)).Succeeded());
        const auto restored = restarted.RecordRecoveryLoaded(
            command(2, secondArtifact));
        REQUIRE(restored.FirstBarrierCompleted);
        restoreRevision = *restored.Activity->RestoreRevision;
    }
    {
        auto store = OpenStore(database);
        CampaignRuntimeService restarted(*store);
        REQUIRE(restarted.LoadCampaign(
            campaign, Presence(campaign, 2)).RuntimeState ==
            CampaignRuntimeState::RECOVERY_LOCK);
        const auto prepared = restarted.PrepareRecovery(
            {campaign, Presence(campaign, 2)});
        REQUIRE(prepared.Dispatch ==
            CampaignRecoveryDispatch::NativeLoad);
        REQUIRE(prepared.Activity->Attempt == attempt);
        REQUIRE(prepared.Activity->RestoreRevision == restoreRevision);
        REQUIRE(prepared.Activity->LoadedSlots.empty());
        REQUIRE(prepared.Activity->SnapshotAppliedSlots.empty());

        const auto firstArtifact = Artifact("stre-cp-restart", 1);
        const auto secondArtifact = Artifact("stre-cp-restart", 2);
        const auto command = [&](std::size_t aSlot,
                                 const NativeSaveBundleArtifact& acArtifact)
        {
            return RecordCampaignRecoveryLoadedCommand{
                campaign, attempt, checkpoint, Identity(campaign, aSlot), true,
                "stre-cp-restart", std::string(kNativeSaveFingerprintAlgorithm),
                kNativeSaveFingerprintVersion,
                Bytes(acArtifact.Fingerprint.begin(), acArtifact.Fingerprint.end()),
                kNativeSaveMetadataCodecVersion, acArtifact.Metadata,
                Presence(campaign, 2)};
        };
        REQUIRE_FALSE(restarted.RecordRecoveryLoaded(
            command(1, firstArtifact)).FirstBarrierCompleted);
        const auto replayedRestore = restarted.RecordRecoveryLoaded(
            command(2, secondArtifact));
        REQUIRE(replayedRestore.Succeeded());
        REQUIRE(replayedRestore.FirstBarrierCompleted);
        REQUIRE(replayedRestore.Command.IdempotentReplay);
        REQUIRE(replayedRestore.Dispatch ==
            CampaignRecoveryDispatch::RestoredSnapshot);
        REQUIRE(replayedRestore.Activity->Attempt == attempt);
        REQUIRE(replayedRestore.Activity->RestoreRevision == restoreRevision);

        REQUIRE(restarted.RecordRecoverySnapshotApplied(
            {campaign, attempt, checkpoint, restoreRevision,
             Identity(campaign, 1), Presence(campaign, 2)}).Succeeded());
        REQUIRE(restarted.RecordRecoverySnapshotApplied(
            {campaign, attempt, checkpoint, restoreRevision,
             Identity(campaign, 2), Presence(campaign, 2)}).RecoveryCompleted);
        const auto journal = store->LoadJournal(campaign);
        REQUIRE(std::count_if(
            journal.Value.begin(), journal.Value.end(),
            [](const JournalRecord& acRecord)
            {
                return acRecord.Kind == "RestoreCheckpoint";
            }) == 1);
    }
}

TEST_CASE(
    "Recovery restart preserves a partial Loaded barrier but requires current-session reproval",
    "[campaign.recovery][runtime][restart][loaded-barrier]")
{
    RecoveryFixture fixture;
    fixture.CommitCheckpoint("cp-restart-loaded-partial");
    const auto begun = fixture.Disconnect();
    REQUIRE(begun.Succeeded());
    const auto prepared = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(prepared.Dispatch == CampaignRecoveryDispatch::NativeLoad);
    const auto firstArtifact = Artifact("stre-cp-restart-loaded-partial", 1);
    const auto secondArtifact = Artifact("stre-cp-restart-loaded-partial", 2);
    const auto first = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 1, firstArtifact));
    REQUIRE(first.Succeeded());
    REQUIRE_FALSE(first.Command.IdempotentReplay);
    REQUIRE_FALSE(first.FirstBarrierCompleted);
    REQUIRE(first.Activity->DurableLoadedSlots.contains("slot-1"));

    CampaignRuntimeService restarted(*fixture.Store);
    const auto rehydrated = restarted.GetRecoveryActivity(fixture.Campaign);
    REQUIRE(rehydrated);
    REQUIRE(rehydrated->Attempt == begun.Activity->Attempt);
    REQUIRE(rehydrated->DurableLoadedSlots ==
        std::unordered_set<std::string>{"slot-1"});
    REQUIRE(rehydrated->LoadedSlots.empty());
    const auto replay = restarted.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(replay.Dispatch == CampaignRecoveryDispatch::NativeLoad);

    auto loaded = [&](std::size_t aSlot,
                      const NativeSaveBundleArtifact& acArtifact)
    {
        return restarted.RecordRecoveryLoaded(
            {fixture.Campaign,
             replay.Activity->Attempt,
             *replay.Activity->Checkpoint,
             Identity(fixture.Campaign, aSlot),
             true,
             "stre-cp-restart-loaded-partial",
             std::string(kNativeSaveFingerprintAlgorithm),
             kNativeSaveFingerprintVersion,
             Bytes(acArtifact.Fingerprint.begin(), acArtifact.Fingerprint.end()),
             kNativeSaveMetadataCodecVersion,
             acArtifact.Metadata,
             Presence(fixture.Campaign, 2)});
    };
    const auto replayedFirst = loaded(1, firstArtifact);
    REQUIRE(replayedFirst.Succeeded());
    REQUIRE(replayedFirst.Command.IdempotentReplay);
    REQUIRE_FALSE(replayedFirst.FirstBarrierCompleted);
    const auto second = loaded(2, secondArtifact);
    REQUIRE(second.Succeeded());
    REQUIRE_FALSE(second.Command.IdempotentReplay);
    REQUIRE(second.FirstBarrierCompleted);
    REQUIRE(second.Dispatch == CampaignRecoveryDispatch::RestoredSnapshot);

    const auto journal = fixture.Store->LoadJournal(fixture.Campaign);
    REQUIRE(journal.Succeeded());
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "RecoveryLoaded";
        }) == 2);
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "RestoreCheckpoint";
        }) == 1);
}

TEST_CASE(
    "Recovery restart preserves partial Applied evidence and replays both barriers safely",
    "[campaign.recovery][runtime][restart][applied-barrier]")
{
    RecoveryFixture fixture;
    fixture.CommitCheckpoint("cp-restart-applied-partial");
    const auto begun = fixture.Disconnect();
    const auto prepared = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    const auto firstArtifact = Artifact("stre-cp-restart-applied-partial", 1);
    const auto secondArtifact = Artifact("stre-cp-restart-applied-partial", 2);
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 1, firstArtifact)).Succeeded());
    const auto restored = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(*prepared.Activity, 2, secondArtifact));
    REQUIRE(restored.FirstBarrierCompleted);
    const StateVersion restoreRevision = *restored.Activity->RestoreRevision;
    const CheckpointId checkpoint = *restored.Activity->Checkpoint;
    const auto firstApplied = fixture.Runtime.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         begun.Activity->Attempt,
         checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 1),
         Presence(fixture.Campaign, 2)});
    REQUIRE(firstApplied.Succeeded());
    REQUIRE_FALSE(firstApplied.RecoveryCompleted);
    REQUIRE(firstApplied.Activity->DurableSnapshotAppliedSlots.contains(
        "slot-1"));

    CampaignRuntimeService restarted(*fixture.Store);
    const auto rehydrated = restarted.GetRecoveryActivity(fixture.Campaign);
    REQUIRE(rehydrated);
    REQUIRE(rehydrated->Attempt == begun.Activity->Attempt);
    REQUIRE(rehydrated->RestoreRevision == restoreRevision);
    REQUIRE(rehydrated->DurableLoadedSlots.size() == 2);
    REQUIRE(rehydrated->DurableSnapshotAppliedSlots ==
        std::unordered_set<std::string>{"slot-1"});
    REQUIRE(rehydrated->LoadedSlots.empty());
    REQUIRE(rehydrated->SnapshotAppliedSlots.empty());

    const auto replay = restarted.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(replay.Dispatch == CampaignRecoveryDispatch::NativeLoad);
    const auto loaded = [&](std::size_t aSlot,
                            const NativeSaveBundleArtifact& acArtifact)
    {
        return restarted.RecordRecoveryLoaded(
            {fixture.Campaign,
             replay.Activity->Attempt,
             checkpoint,
             Identity(fixture.Campaign, aSlot),
             true,
             "stre-cp-restart-applied-partial",
             std::string(kNativeSaveFingerprintAlgorithm),
             kNativeSaveFingerprintVersion,
             Bytes(acArtifact.Fingerprint.begin(), acArtifact.Fingerprint.end()),
             kNativeSaveMetadataCodecVersion,
             acArtifact.Metadata,
             Presence(fixture.Campaign, 2)});
    };
    const auto replayedFirstLoaded = loaded(1, firstArtifact);
    REQUIRE(replayedFirstLoaded.Command.IdempotentReplay);
    REQUIRE_FALSE(replayedFirstLoaded.FirstBarrierCompleted);
    const auto replayedRestore = loaded(2, secondArtifact);
    REQUIRE(replayedRestore.Command.IdempotentReplay);
    REQUIRE(replayedRestore.FirstBarrierCompleted);
    REQUIRE(replayedRestore.Activity->RestoreRevision == restoreRevision);

    const auto replayedFirstApplied =
        restarted.RecordRecoverySnapshotApplied(
            {fixture.Campaign,
             begun.Activity->Attempt,
             checkpoint,
             restoreRevision,
             Identity(fixture.Campaign, 1),
             Presence(fixture.Campaign, 2)});
    REQUIRE(replayedFirstApplied.Succeeded());
    REQUIRE(replayedFirstApplied.Command.IdempotentReplay);
    REQUIRE_FALSE(replayedFirstApplied.RecoveryCompleted);
    const auto completed = restarted.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         begun.Activity->Attempt,
         checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 2),
         Presence(fixture.Campaign, 2)});
    REQUIRE(completed.Succeeded());
    REQUIRE(completed.RecoveryCompleted);

    const auto journal = fixture.Store->LoadJournal(fixture.Campaign);
    REQUIRE(journal.Succeeded());
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "RecoverySnapshotApplied";
        }) == 2);
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "RestoreCheckpoint";
        }) == 1);
}

TEST_CASE(
    "Persistence reload preserves a restore revision for the next checkpoint recovery",
    "[campaign.recovery][runtime][sequential][restart]")
{
    TemporaryDatabase database;
    const CampaignId campaign{"campaign-sequential-restart"};
    StateVersion firstRestoreRevision{};
    CheckpointId secondCheckpoint;
    RestoreAttemptId secondAttempt;
    StateVersion secondRestoreRevision{};

    {
        auto store = OpenStore(database);
        CampaignRuntimeService runtime(*store);
        REQUIRE(runtime.CreateLobbyCampaign(
            {campaign,
             MutationId{"create-sequential-restart"},
             {Slot(1), Slot(2)}}).Succeeded());
        REQUIRE(runtime.CommitCampaignStart(
            {campaign,
             1,
             MutationId{"seal-sequential-restart"},
             PlayerId{"player-1"}}).Succeeded());

        const CheckpointId checkpoint{"cp-before-restart"};
        REQUIRE(runtime.BeginCheckpoint(
            {campaign,
             checkpoint,
             "stre-cp-before-restart",
             Presence(campaign, 2)}).Succeeded());
        for (std::size_t index = 1; index <= 2; ++index)
        {
            const auto artifact = Artifact(
                "stre-cp-before-restart",
                static_cast<std::uint8_t>(index));
            const auto saved = runtime.RecordCheckpointSave(
                {campaign,
                 checkpoint,
                 "stre-cp-before-restart",
                 Identity(campaign, index),
                 std::string(kNativeSaveFingerprintAlgorithm),
                 kNativeSaveFingerprintVersion,
                 Bytes(artifact.Fingerprint.begin(), artifact.Fingerprint.end()),
                 kNativeSaveMetadataCodecVersion,
                 artifact.Metadata});
            REQUIRE(saved.Succeeded());
            REQUIRE(saved.Committed == (index == 2));
        }

        REQUIRE(runtime.BeginRecovery(
            {campaign,
             Identity(campaign, 2),
             {Presence(campaign, 2).front()},
             CampaignRuntimeState::ACTIVE}).Succeeded());
        const auto prepared = runtime.PrepareRecovery(
            {campaign, Presence(campaign, 2)});
        REQUIRE(prepared.Succeeded());
        CampaignRecoveryCommandResult restored;
        for (std::size_t index = 1; index <= 2; ++index)
        {
            const auto artifact = Artifact(
                "stre-cp-before-restart",
                static_cast<std::uint8_t>(index));
            restored = runtime.RecordRecoveryLoaded(
                {campaign,
                 prepared.Activity->Attempt,
                 checkpoint,
                 Identity(campaign, index),
                 true,
                 "stre-cp-before-restart",
                 std::string(kNativeSaveFingerprintAlgorithm),
                 kNativeSaveFingerprintVersion,
                 Bytes(artifact.Fingerprint.begin(), artifact.Fingerprint.end()),
                 kNativeSaveMetadataCodecVersion,
                 artifact.Metadata,
                 Presence(campaign, 2)});
            REQUIRE(restored.Succeeded());
        }
        firstRestoreRevision = *restored.Activity->RestoreRevision;
        for (std::size_t index = 1; index <= 2; ++index)
        {
            REQUIRE(runtime.RecordRecoverySnapshotApplied(
                {campaign,
                 restored.Activity->Attempt,
                 checkpoint,
                 firstRestoreRevision,
                 Identity(campaign, index),
                 Presence(campaign, 2)}).Succeeded());
        }
        REQUIRE_FALSE(runtime.GetRecoveryActivity(campaign));
    }

    {
        auto store = OpenStore(database);
        CampaignRuntimeService runtime(*store);
        const auto active = runtime.LoadCampaign(
            campaign, Presence(campaign, 2));
        REQUIRE(active.Succeeded());
        REQUIRE(active.Campaign.Version == firstRestoreRevision);
        REQUIRE(active.RuntimeState == CampaignRuntimeState::ACTIVE);

        secondCheckpoint = CheckpointId{"cp-after-restart"};
        const auto begun = runtime.BeginCheckpoint(
            {campaign,
             secondCheckpoint,
             "stre-cp-after-restart",
             Presence(campaign, 2)});
        REQUIRE(begun.Succeeded());
        REQUIRE(begun.Activity->SourceRevision == firstRestoreRevision);
        for (std::size_t index = 1; index <= 2; ++index)
        {
            const auto artifact = Artifact(
                "stre-cp-after-restart",
                static_cast<std::uint8_t>(index));
            REQUIRE(runtime.RecordCheckpointSave(
                {campaign,
                 secondCheckpoint,
                 "stre-cp-after-restart",
                 Identity(campaign, index),
                 std::string(kNativeSaveFingerprintAlgorithm),
                 kNativeSaveFingerprintVersion,
                 Bytes(artifact.Fingerprint.begin(), artifact.Fingerprint.end()),
                 kNativeSaveMetadataCodecVersion,
                 artifact.Metadata}).Succeeded());
        }
        auto incomplete = Presence(campaign, 2);
        incomplete.pop_back();
        const auto opened = runtime.BeginRecovery(
            {campaign,
             Identity(campaign, 2),
             incomplete,
             CampaignRuntimeState::ACTIVE});
        REQUIRE(opened.Succeeded());
        secondAttempt = opened.Activity->Attempt;
        const auto prepared = runtime.PrepareRecovery(
            {campaign, Presence(campaign, 2)});
        CampaignRecoveryCommandResult restored;
        for (std::size_t index = 1; index <= 2; ++index)
        {
            const auto artifact = Artifact(
                "stre-cp-after-restart",
                static_cast<std::uint8_t>(index));
            restored = runtime.RecordRecoveryLoaded(
                {campaign,
                 secondAttempt,
                 secondCheckpoint,
                 Identity(campaign, index),
                 true,
                 "stre-cp-after-restart",
                 std::string(kNativeSaveFingerprintAlgorithm),
                 kNativeSaveFingerprintVersion,
                 Bytes(artifact.Fingerprint.begin(), artifact.Fingerprint.end()),
                 kNativeSaveMetadataCodecVersion,
                 artifact.Metadata,
                 Presence(campaign, 2)});
            REQUIRE(restored.Succeeded());
        }
        REQUIRE(restored.FirstBarrierCompleted);
        secondRestoreRevision = *restored.Activity->RestoreRevision;
        REQUIRE(runtime.LoadCampaign(
            campaign, Presence(campaign, 2)).Succeeded());
    }

    {
        auto store = OpenStore(database);
        CampaignRuntimeService runtime(*store);
        const auto prepared = runtime.PrepareRecovery(
            {campaign, Presence(campaign, 2)});
        REQUIRE(prepared.Succeeded());
        REQUIRE(prepared.Dispatch ==
            CampaignRecoveryDispatch::NativeLoad);
        REQUIRE(prepared.Activity->Attempt == secondAttempt);
        REQUIRE(prepared.Activity->RestoreRevision == secondRestoreRevision);
        CampaignRecoveryCommandResult replayedRestore;
        for (std::size_t index = 1; index <= 2; ++index)
        {
            const auto artifact = Artifact(
                "stre-cp-after-restart",
                static_cast<std::uint8_t>(index));
            replayedRestore = runtime.RecordRecoveryLoaded(
                {campaign,
                 secondAttempt,
                 secondCheckpoint,
                 Identity(campaign, index),
                 true,
                 "stre-cp-after-restart",
                 std::string(kNativeSaveFingerprintAlgorithm),
                 kNativeSaveFingerprintVersion,
                 Bytes(artifact.Fingerprint.begin(), artifact.Fingerprint.end()),
                 kNativeSaveMetadataCodecVersion,
                 artifact.Metadata,
                 Presence(campaign, 2)});
            REQUIRE(replayedRestore.Succeeded());
            REQUIRE(replayedRestore.Command.IdempotentReplay);
        }
        REQUIRE(replayedRestore.FirstBarrierCompleted);
        REQUIRE(replayedRestore.Dispatch ==
            CampaignRecoveryDispatch::RestoredSnapshot);
        REQUIRE(replayedRestore.Activity->RestoreRevision ==
            secondRestoreRevision);
        REQUIRE(runtime.LoadCampaign(
            campaign, Presence(campaign, 2)).Succeeded());
        REQUIRE(runtime.RecordRecoverySnapshotApplied(
            {campaign,
             secondAttempt,
             secondCheckpoint,
             secondRestoreRevision,
             Identity(campaign, 1),
             Presence(campaign, 2)}).Succeeded());
        REQUIRE(runtime.RecordRecoverySnapshotApplied(
            {campaign,
             secondAttempt,
             secondCheckpoint,
             secondRestoreRevision,
             Identity(campaign, 2),
             Presence(campaign, 2)}).RecoveryCompleted);
        const auto journal = store->LoadJournal(campaign);
        REQUIRE(journal.Succeeded());
        REQUIRE(std::count_if(
            journal.Value.begin(), journal.Value.end(),
            [](const JournalRecord& acRecord)
            {
                return acRecord.Kind == "RestoreCheckpoint";
            }) == 2);
    }
}

TEST_CASE(
    "Missing checkpoint snapshot fails closed before either recovery barrier",
    "[campaign.recovery][runtime][snapshot][fail-closed]")
{
    TemporaryDatabase database;
    const CampaignId campaign{"campaign-corrupt-snapshot"};
    const CheckpointId checkpoint{"cp-corrupt-snapshot"};
    {
        auto store = OpenStore(database);
        CampaignRuntimeService runtime(*store);
        REQUIRE(runtime.CreateLobbyCampaign(
            {campaign,
             MutationId{"create-corrupt-snapshot"},
             {Slot(1), Slot(2)}}).Succeeded());
        REQUIRE(runtime.CommitCampaignStart(
            {campaign,
             1,
             MutationId{"seal-corrupt-snapshot"},
             PlayerId{"player-1"}}).Succeeded());
        REQUIRE(runtime.BeginCheckpoint(
            {campaign,
             checkpoint,
             "stre-cp-corrupt-snapshot",
             Presence(campaign, 2)}).Succeeded());
        for (std::size_t index = 1; index <= 2; ++index)
        {
            const auto artifact = Artifact(
                "stre-cp-corrupt-snapshot",
                static_cast<std::uint8_t>(index));
            REQUIRE(runtime.RecordCheckpointSave(
                {campaign,
                 checkpoint,
                 "stre-cp-corrupt-snapshot",
                 Identity(campaign, index),
                 std::string(kNativeSaveFingerprintAlgorithm),
                 kNativeSaveFingerprintVersion,
                 Bytes(artifact.Fingerprint.begin(), artifact.Fingerprint.end()),
                 kNativeSaveMetadataCodecVersion,
                 artifact.Metadata}).Succeeded());
        }
    }
    REQUIRE(ExecuteRaw(
        database.Path,
        "DROP TRIGGER campaign_snapshot_no_update; "
        "UPDATE campaign_snapshots SET checksum='0000000000000000' "
        "WHERE snapshot_id='snapshot-cp-corrupt-snapshot';"));

    auto store = OpenStore(database);
    CampaignRuntimeService runtime(*store);
    auto incomplete = Presence(campaign, 2);
    incomplete.pop_back();
    REQUIRE(runtime.BeginRecovery(
        {campaign,
         Identity(campaign, 2),
         incomplete,
         CampaignRuntimeState::ACTIVE}).Succeeded());
    const auto prepared = runtime.PrepareRecovery(
        {campaign, Presence(campaign, 2)});
    REQUIRE(prepared.Command.Error == CampaignError::IntegrityFailure);
    REQUIRE(prepared.Dispatch == CampaignRecoveryDispatch::None);
    const auto locked = runtime.GetRecoveryActivity(campaign);
    REQUIRE(locked);
    REQUIRE_FALSE(locked->RestoreRevision);
}

TEST_CASE(
    "Disconnect while applying snapshot replays native barrier without a new restore",
    "[campaign.recovery][runtime][client-crash][restart]")
{
    RecoveryFixture fixture;
    fixture.CommitCheckpoint("cp-applying-crash");
    const auto begun = fixture.Disconnect();
    REQUIRE(begun.Succeeded());
    const auto prepared = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    const auto firstArtifact = Artifact("stre-cp-applying-crash", 1);
    const auto secondArtifact = Artifact("stre-cp-applying-crash", 2);
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(
            *prepared.Activity, 1, firstArtifact)).Succeeded());
    const auto restored = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(
            *prepared.Activity, 2, secondArtifact));
    REQUIRE(restored.FirstBarrierCompleted);
    const RestoreAttemptId attempt = restored.Activity->Attempt;
    const StateVersion restoreRevision =
        *restored.Activity->RestoreRevision;

    REQUIRE(fixture.Runtime.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         attempt,
         *restored.Activity->Checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 1),
         Presence(fixture.Campaign, 2)}).Succeeded());

    auto incomplete = Presence(fixture.Campaign, 2);
    incomplete.pop_back();
    const auto reopened = fixture.Runtime.BeginRecovery(
        {fixture.Campaign,
         Identity(fixture.Campaign, 2),
         incomplete,
         CampaignRuntimeState::RESTORING_CHECKPOINT});
    REQUIRE(reopened.Succeeded());
    REQUIRE(reopened.Command.IdempotentReplay);
    REQUIRE(reopened.Activity->Attempt == attempt);
    REQUIRE(reopened.Activity->Stage ==
        CampaignRecoveryStage::RecoveryLock);

    const auto replay = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(replay.Dispatch == CampaignRecoveryDispatch::NativeLoad);
    REQUIRE(replay.Activity->Attempt == attempt);
    REQUIRE(replay.Activity->RestoreRevision == restoreRevision);
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(
            *replay.Activity, 1, firstArtifact)).Succeeded());
    const auto replayedRestore = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(
            *replay.Activity, 2, secondArtifact));
    REQUIRE(replayedRestore.FirstBarrierCompleted);
    REQUIRE(replayedRestore.Command.IdempotentReplay);
    REQUIRE(replayedRestore.Activity->Attempt == attempt);
    REQUIRE(replayedRestore.Activity->RestoreRevision == restoreRevision);

    const auto journal = fixture.Store->LoadJournal(fixture.Campaign);
    REQUIRE(std::count_if(
        journal.Value.begin(), journal.Value.end(),
        [](const JournalRecord& acRecord)
        {
            return acRecord.Kind == "RestoreCheckpoint";
        }) == 1);
}

TEST_CASE(
    "Completed recovery acknowledgement replays after server restart",
    "[campaign.recovery][runtime][complete-crash][restart]")
{
    RecoveryFixture fixture;
    fixture.CommitCheckpoint("cp-complete-crash");
    REQUIRE(fixture.Disconnect().Succeeded());
    const auto prepared = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    const auto firstArtifact = Artifact("stre-cp-complete-crash", 1);
    const auto secondArtifact = Artifact("stre-cp-complete-crash", 2);
    REQUIRE(fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(
            *prepared.Activity, 1, firstArtifact)).Succeeded());
    const auto restored = fixture.Runtime.RecordRecoveryLoaded(
        fixture.LoadedCommand(
            *prepared.Activity, 2, secondArtifact));
    const RestoreAttemptId attempt = restored.Activity->Attempt;
    const CheckpointId checkpoint = *restored.Activity->Checkpoint;
    const StateVersion restoreRevision =
        *restored.Activity->RestoreRevision;
    REQUIRE(fixture.Runtime.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         attempt,
         checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 1),
         Presence(fixture.Campaign, 2)}).Succeeded());
    REQUIRE(fixture.Runtime.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         attempt,
         checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 2),
         Presence(fixture.Campaign, 2)}).RecoveryCompleted);

    CampaignRuntimeService restarted(*fixture.Store);
    const auto replayed = restarted.RecordRecoverySnapshotApplied(
        {fixture.Campaign,
         attempt,
         checkpoint,
         restoreRevision,
         Identity(fixture.Campaign, 1),
         Presence(fixture.Campaign, 2)});
    REQUIRE(replayed.Succeeded());
    REQUIRE(replayed.Command.IdempotentReplay);
    REQUIRE(replayed.RecoveryCompleted);
    REQUIRE_FALSE(replayed.Activity);
}

TEST_CASE(
    "Disconnect ordering selects only the checkpoint committed before recovery",
    "[campaign.recovery][runtime][checkpoint-race]")
{
    RecoveryFixture fixture;
    fixture.CommitCheckpoint("cp-1");
    REQUIRE(fixture.BeginCheckpoint("cp-2").Succeeded());
    REQUIRE(fixture.AckCheckpoint(
        "cp-2", 1, Artifact("stre-cp-2", 3)).Succeeded());

    const auto begun = fixture.Disconnect();
    REQUIRE(begun.Succeeded());
    REQUIRE(fixture.AckCheckpoint(
        "cp-2", 2, Artifact("stre-cp-2", 4)).Command.Error ==
        CampaignError::RecoveryInProgress);
    const auto prepared = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(prepared.Checkpoint->Id == CheckpointId{"cp-1"});
    REQUIRE(fixture.Store->LoadCheckpoint(
        fixture.Campaign, CheckpointId{"cp-2"}).Value.State ==
        CheckpointState::Candidate);
}

TEST_CASE(
    "Checkpoint committed before disconnect becomes the collective rollback point",
    "[campaign.recovery][runtime][checkpoint-race]")
{
    RecoveryFixture fixture;
    fixture.CommitCheckpoint("cp-1");
    fixture.CommitCheckpoint("cp-2");

    const auto begun = fixture.Disconnect();
    REQUIRE(begun.Succeeded());
    const auto prepared = fixture.Runtime.PrepareRecovery(
        {fixture.Campaign, Presence(fixture.Campaign, 2)});
    REQUIRE(prepared.Succeeded());
    REQUIRE(prepared.Checkpoint->Id == CheckpointId{"cp-2"});
}
