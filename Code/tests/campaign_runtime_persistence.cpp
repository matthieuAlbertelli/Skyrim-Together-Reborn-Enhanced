#include <campaign_runtime_test_helpers.h>

#include <catch2/catch.hpp>

#include <optional>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

TEST_CASE(
    "Campaign runtime service persists mutable Lobby roster commands deterministically",
    "[campaign.runtime][persistence][roster]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CampaignRuntimeService service(*store);

    CampaignCommandResult created = service.CreateLobbyCampaign(
        MakeRuntimeCampaignCommand());
    REQUIRE(created.Succeeded());
    REQUIRE(created.Version == 1);

    CampaignLoadResult emptyLobby = service.LoadCampaign(
        CampaignId{"campaign-runtime"});
    REQUIRE(emptyLobby.Succeeded());
    REQUIRE(emptyLobby.Campaign.Phase == CampaignPhase::Lobby);
    REQUIRE_FALSE(emptyLobby.Campaign.RosterSealed);
    REQUIRE(emptyLobby.Campaign.Roster.empty());
    REQUIRE(emptyLobby.RuntimeState ==
            CampaignRuntimeState::WAITING_FOR_ROSTER);

    REQUIRE(service.AddRosterSlot(
        {CampaignId{"campaign-runtime"},
         1,
         MutationId{"mutation-add-2"},
         MakeRuntimeSlot(2)}).Succeeded());
    REQUIRE(service.AddRosterSlot(
        {CampaignId{"campaign-runtime"},
         2,
         MutationId{"mutation-add-1"},
         MakeRuntimeSlot(1)}).Succeeded());

    CampaignLoadResult configured = service.LoadCampaign(
        CampaignId{"campaign-runtime"});
    REQUIRE(configured.Succeeded());
    REQUIRE(configured.Campaign.Version == 3);
    REQUIRE(configured.Campaign.Roster.size() == 2);
    REQUIRE(configured.Campaign.Roster[0].Slot ==
            CampaignSlotId{"slot-1"});
    REQUIRE(configured.Campaign.Roster[1].Slot ==
            CampaignSlotId{"slot-2"});

    CampaignSlotRecord duplicateSlot = MakeRuntimeSlot(3);
    duplicateSlot.Slot = CampaignSlotId{"slot-1"};
    REQUIRE(service.AddRosterSlot(
        {CampaignId{"campaign-runtime"},
         3,
         MutationId{"mutation-duplicate-slot"},
         duplicateSlot}).Error == CampaignError::DuplicateSlot);

    CampaignSlotRecord duplicatePlayer = MakeRuntimeSlot(3);
    duplicatePlayer.Player = PlayerId{"player-1"};
    REQUIRE(service.AddRosterSlot(
        {CampaignId{"campaign-runtime"},
         3,
         MutationId{"mutation-duplicate-player"},
         duplicatePlayer}).Error == CampaignError::DuplicatePlayer);

    CampaignSlotRecord duplicateBinding = MakeRuntimeSlot(3);
    duplicateBinding.CharacterBinding = CharacterBindingId{"binding-1"};
    REQUIRE(service.AddRosterSlot(
        {CampaignId{"campaign-runtime"},
         3,
         MutationId{"mutation-duplicate-binding"},
         duplicateBinding}).Error ==
        CampaignError::DuplicateCharacterBinding);

    CampaignSlotRecord replacement = MakeRuntimeSlot(20);
    replacement.Slot = CampaignSlotId{"slot-2"};
    CampaignCommandResult replaced = service.ReplaceRosterSlot(
        {CampaignId{"campaign-runtime"},
         3,
         MutationId{"mutation-replace-2"},
         replacement});
    REQUIRE(replaced.Succeeded());
    REQUIRE(replaced.Version == 4);

    REQUIRE(service.RemoveRosterSlot(
        {CampaignId{"campaign-runtime"},
         4,
         MutationId{"mutation-remove-1"},
         CampaignSlotId{"slot-1"}}).Succeeded());
    REQUIRE(service.RemoveRosterSlot(
        {CampaignId{"campaign-runtime"},
         5,
         MutationId{"mutation-remove-2"},
         CampaignSlotId{"slot-2"}}).Succeeded());

    CampaignCommandResult emptySeal = service.CommitCampaignStart(
        {CampaignId{"campaign-runtime"},
         6,
         MutationId{"mutation-empty-seal"},
         PlayerId{"player-20"}});
    REQUIRE(emptySeal.Error == CampaignError::InvalidRoster);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 6);
}

TEST_CASE(
    "CommitCampaignStart is atomic journaled outboxed and durably idempotent",
    "[campaign.runtime][persistence][seal][idempotency]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CampaignRuntimeService service(*store);
    REQUIRE(service.CreateLobbyCampaign(
        MakeRuntimeCampaignCommand(2)).Succeeded());

    const CommitCampaignStartCommand commit{
        CampaignId{"campaign-runtime"},
        1,
        MutationId{"mutation-commit-start"},
        PlayerId{"player-1"}};
    CampaignCommandResult committed = service.CommitCampaignStart(commit);
    REQUIRE(committed.Succeeded());
    REQUIRE(committed.Applied);
    REQUIRE(committed.Version == 2);

    CampaignLoadResult loaded = service.LoadCampaign(
        CampaignId{"campaign-runtime"});
    REQUIRE(loaded.Succeeded());
    REQUIRE(loaded.Campaign.Version == 2);
    REQUIRE(loaded.Campaign.RosterSealed);
    REQUIRE(loaded.Campaign.Phase == CampaignPhase::CharacterCreation);
    REQUIRE(loaded.Campaign.SessionManager == PlayerId{"player-1"});
    REQUIRE(loaded.Campaign.Roster.size() == 2);

    auto journal = store->LoadJournal(CampaignId{"campaign-runtime"});
    REQUIRE(journal.Succeeded());
    REQUIRE(journal.Value.size() == 2);
    REQUIRE(journal.Value.back().Kind == "CommitCampaignStart");
    REQUIRE(journal.Value.back().ResultingRevision == 2);
    auto outbox = store->LoadPendingOutbox(CampaignId{"campaign-runtime"});
    REQUIRE(outbox.Succeeded());
    REQUIRE(outbox.Value.size() == 2);
    auto snapshot = RuntimeCodec::DecodeSnapshotIntent(
        outbox.Value.back().Payload);
    REQUIRE(snapshot.Succeeded());
    REQUIRE(snapshot.Value == loaded.Campaign);

    SetCampaignReadyCommand ready{
        CampaignId{"campaign-runtime"},
        2,
        MutationId{"mutation-ready-1"},
        MakeRuntimeIdentity(1),
        true};
    CampaignCommandResult readied = service.SetReady(ready);
    REQUIRE(readied.Succeeded());
    REQUIRE(readied.Version == 3);

    CampaignCommandResult duplicateReady = service.SetReady(
        {CampaignId{"campaign-runtime"},
         3,
         MutationId{"mutation-ready-duplicate-value"},
         MakeRuntimeIdentity(1),
         true});
    REQUIRE(duplicateReady.Succeeded());
    REQUIRE_FALSE(duplicateReady.Applied);
    REQUIRE(duplicateReady.Version == 3);

    CampaignCommandResult withdrawn = service.SetReady(
        {CampaignId{"campaign-runtime"},
         3,
         MutationId{"mutation-ready-withdraw"},
         MakeRuntimeIdentity(1),
         false});
    REQUIRE(withdrawn.Succeeded());
    REQUIRE(withdrawn.Version == 4);

    const auto rosterBeforeTransfer = service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Roster;
    CampaignCommandResult transferred = service.TransferSessionManager(
        {CampaignId{"campaign-runtime"},
         4,
         MutationId{"mutation-transfer-manager"},
         PlayerId{"player-1"},
         PlayerId{"player-2"}});
    REQUIRE(transferred.Succeeded());
    REQUIRE(transferred.Version == 5);
    CampaignLoadResult afterTransfer = service.LoadCampaign(
        CampaignId{"campaign-runtime"});
    REQUIRE(afterTransfer.Campaign.SessionManager == PlayerId{"player-2"});
    REQUIRE(afterTransfer.Campaign.Roster == rosterBeforeTransfer);

    CampaignCommandResult oldReplay = service.CommitCampaignStart(commit);
    REQUIRE(oldReplay.Succeeded());
    REQUIRE(oldReplay.IdempotentReplay);
    REQUIRE(oldReplay.Version == 2);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 5);

    CommitCampaignStartCommand conflict = commit;
    conflict.SessionManager = PlayerId{"player-2"};
    CampaignCommandResult conflicting = service.CommitCampaignStart(conflict);
    REQUIRE(conflicting.PersistenceError ==
            StoreError::IdempotencyConflict);

    CampaignCommandResult stale = service.SetReady(
        {CampaignId{"campaign-runtime"},
         1,
         MutationId{"mutation-stale-ready"},
         MakeRuntimeIdentity(2),
         true});
    REQUIRE(stale.Error == CampaignError::StaleRevision);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 5);

    CampaignCommandResult sealedAdd = service.AddRosterSlot(
        {CampaignId{"campaign-runtime"},
         5,
         MutationId{"mutation-add-after-seal"},
         MakeRuntimeSlot(3)});
    REQUIRE(sealedAdd.Error == CampaignError::RosterSealed);

    CampaignMemberIdentity outsider = MakeRuntimeIdentity(9);
    CampaignCommandResult nonMemberReady = service.SetReady(
        {CampaignId{"campaign-runtime"},
         5,
         MutationId{"mutation-outsider-ready"},
         outsider,
         true});
    REQUIRE(nonMemberReady.Error == CampaignError::NotCampaignMember);

    CampaignAggregate exact = afterTransfer.Campaign;
    CampaignLoadResult active = service.LoadCampaign(
        CampaignId{"campaign-runtime"},
        MakeFullPresence(exact));
    REQUIRE(active.RuntimeState == CampaignRuntimeState::ACTIVE);
}

TEST_CASE(
    "CommitCampaignStart persistence failure leaves no partial seal phase or revision",
    "[campaign.runtime][persistence][atomicity]")
{
    TemporaryDatabase database;
    std::optional<TransactionStage> injectedStage;
    SqliteCampaignStoreOptions options;
    options.FaultInjector = [&injectedStage](TransactionStage aStage)
    {
        return injectedStage == aStage;
    };
    auto store = OpenStore(database, options);
    CampaignRuntimeService service(*store);
    REQUIRE(service.CreateLobbyCampaign(
        MakeRuntimeCampaignCommand(2)).Succeeded());

    injectedStage = TransactionStage::AfterJournal;
    const CommitCampaignStartCommand commit{
        CampaignId{"campaign-runtime"},
        1,
        MutationId{"mutation-faulted-commit"},
        PlayerId{"player-1"}};
    CampaignCommandResult failed = service.CommitCampaignStart(commit);
    REQUIRE(failed.Error == CampaignError::PersistenceFailure);
    REQUIRE(failed.PersistenceError == StoreError::FaultInjected);

    CampaignLoadResult unchanged = service.LoadCampaign(
        CampaignId{"campaign-runtime"});
    REQUIRE(unchanged.Succeeded());
    REQUIRE(unchanged.Campaign.Version == 1);
    REQUIRE_FALSE(unchanged.Campaign.RosterSealed);
    REQUIRE(unchanged.Campaign.Phase == CampaignPhase::Lobby);
    REQUIRE_FALSE(unchanged.Campaign.SessionManager);
    REQUIRE(unchanged.Campaign.Roster.size() == 2);
    REQUIRE(store->LoadJournal(
        CampaignId{"campaign-runtime"}).Value.size() == 1);
    REQUIRE(store->LoadPendingOutbox(
        CampaignId{"campaign-runtime"}).Value.size() == 1);

    injectedStage.reset();
    CampaignCommandResult retry = service.CommitCampaignStart(commit);
    REQUIRE(retry.Succeeded());
    REQUIRE(retry.Version == 2);
}

TEST_CASE(
    "Accepted ready no-op durably reserves its MutationId",
    "[campaign.runtime][persistence][idempotency][noop]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CampaignRuntimeService service(*store);
    REQUIRE(service.CreateLobbyCampaign(
        MakeRuntimeCampaignCommand(1)).Succeeded());
    REQUIRE(service.CommitCampaignStart(
        {CampaignId{"campaign-runtime"},
         1,
         MutationId{"mutation-start-noop-regression"},
         PlayerId{"player-1"}}).Succeeded());
    REQUIRE(service.SetReady(
        {CampaignId{"campaign-runtime"},
         2,
         MutationId{"mutation-ready-before-noop"},
         MakeRuntimeIdentity(1),
         true}).Succeeded());

    const SetCampaignReadyCommand acceptedNoOp{
        CampaignId{"campaign-runtime"},
        3,
        MutationId{"mutation-accepted-ready-noop"},
        MakeRuntimeIdentity(1),
        true};
    CampaignCommandResult noOp = service.SetReady(acceptedNoOp);
    REQUIRE(noOp.Succeeded());
    REQUIRE_FALSE(noOp.Applied);
    REQUIRE(noOp.Version == 3);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 3);
    auto journalAfterNoOp = store->LoadJournal(
        CampaignId{"campaign-runtime"});
    REQUIRE(journalAfterNoOp.Succeeded());
    REQUIRE(journalAfterNoOp.Value.size() == 4);
    REQUIRE(journalAfterNoOp.Value.back().ExpectedRevision == 3);
    REQUIRE(journalAfterNoOp.Value.back().ResultingRevision == 3);
    REQUIRE(store->LoadPendingOutbox(
        CampaignId{"campaign-runtime"}).Value.size() == 3);

    CampaignCommandResult replay = service.SetReady(acceptedNoOp);
    REQUIRE(replay.Succeeded());
    REQUIRE(replay.IdempotentReplay);
    REQUIRE_FALSE(replay.Applied);
    REQUIRE(replay.Version == 3);
    REQUIRE(store->LoadJournal(
        CampaignId{"campaign-runtime"}).Value.size() == 4);

    SetCampaignReadyCommand conflicting = acceptedNoOp;
    conflicting.Ready = false;
    CampaignCommandResult conflict = service.SetReady(conflicting);
    REQUIRE(conflict.PersistenceError == StoreError::IdempotencyConflict);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 3);

    CampaignCommandResult laterMutation = service.SetReady(
        {CampaignId{"campaign-runtime"},
         3,
         MutationId{"mutation-ready-after-noop"},
         MakeRuntimeIdentity(1),
         false});
    REQUIRE(laterMutation.Succeeded());
    REQUIRE(laterMutation.Applied);
    REQUIRE(laterMutation.Version == 4);
}

TEST_CASE(
    "Accepted Session Manager self-transfer durably reserves its MutationId",
    "[campaign.runtime][persistence][idempotency][noop]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CampaignRuntimeService service(*store);
    REQUIRE(service.CreateLobbyCampaign(
        MakeRuntimeCampaignCommand(2)).Succeeded());
    REQUIRE(service.CommitCampaignStart(
        {CampaignId{"campaign-runtime"},
         1,
         MutationId{"mutation-start-manager-noop"},
         PlayerId{"player-1"}}).Succeeded());

    const TransferSessionManagerCommand acceptedNoOp{
        CampaignId{"campaign-runtime"},
        2,
        MutationId{"mutation-manager-self-transfer"},
        PlayerId{"player-1"},
        PlayerId{"player-1"}};
    CampaignCommandResult noOp =
        service.TransferSessionManager(acceptedNoOp);
    REQUIRE(noOp.Succeeded());
    REQUIRE_FALSE(noOp.Applied);
    REQUIRE(noOp.Version == 2);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 2);
    REQUIRE(store->LoadJournal(
        CampaignId{"campaign-runtime"}).Value.size() == 3);
    REQUIRE(store->LoadPendingOutbox(
        CampaignId{"campaign-runtime"}).Value.size() == 2);

    CampaignCommandResult replay =
        service.TransferSessionManager(acceptedNoOp);
    REQUIRE(replay.Succeeded());
    REQUIRE(replay.IdempotentReplay);
    REQUIRE(replay.Version == 2);

    TransferSessionManagerCommand conflicting = acceptedNoOp;
    conflicting.NewManager = PlayerId{"player-2"};
    REQUIRE(service.TransferSessionManager(conflicting).PersistenceError ==
            StoreError::IdempotencyConflict);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 2);

    CampaignCommandResult laterMutation =
        service.TransferSessionManager(
            {CampaignId{"campaign-runtime"},
             2,
             MutationId{"mutation-manager-after-noop"},
             PlayerId{"player-1"},
             PlayerId{"player-2"}});
    REQUIRE(laterMutation.Succeeded());
    REQUIRE(laterMutation.Version == 3);
}

TEST_CASE(
    "Accepted identical roster replacement durably reserves its MutationId",
    "[campaign.runtime][persistence][idempotency][noop]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CampaignRuntimeService service(*store);
    REQUIRE(service.CreateLobbyCampaign(
        MakeRuntimeCampaignCommand(2)).Succeeded());

    const ReplaceRosterSlotCommand acceptedNoOp{
        CampaignId{"campaign-runtime"},
        1,
        MutationId{"mutation-identical-roster-replacement"},
        MakeRuntimeSlot(1)};
    CampaignCommandResult noOp = service.ReplaceRosterSlot(acceptedNoOp);
    REQUIRE(noOp.Succeeded());
    REQUIRE_FALSE(noOp.Applied);
    REQUIRE(noOp.Version == 1);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 1);
    REQUIRE(store->LoadJournal(
        CampaignId{"campaign-runtime"}).Value.size() == 2);
    REQUIRE(store->LoadPendingOutbox(
        CampaignId{"campaign-runtime"}).Value.size() == 1);

    CampaignCommandResult replay = service.ReplaceRosterSlot(acceptedNoOp);
    REQUIRE(replay.Succeeded());
    REQUIRE(replay.IdempotentReplay);
    REQUIRE(replay.Version == 1);

    ReplaceRosterSlotCommand conflicting = acceptedNoOp;
    conflicting.Slot = MakeRuntimeSlot(9);
    conflicting.Slot.Slot = CampaignSlotId{"slot-1"};
    REQUIRE(service.ReplaceRosterSlot(conflicting).PersistenceError ==
            StoreError::IdempotencyConflict);
    REQUIRE(service.LoadCampaign(
        CampaignId{"campaign-runtime"}).Campaign.Version == 1);

    ReplaceRosterSlotCommand later = conflicting;
    later.Mutation = MutationId{"mutation-roster-after-noop"};
    CampaignCommandResult laterMutation = service.ReplaceRosterSlot(later);
    REQUIRE(laterMutation.Succeeded());
    REQUIRE(laterMutation.Version == 2);
}

TEST_CASE(
    "Campaign core codec preserves readiness and rejects revision mismatch",
    "[campaign.runtime][codec]")
{
    CampaignAggregate campaign = MakeSealedRuntimeCampaign(4);
    REQUIRE(CampaignStateMachine::SetReady(
        campaign, MakeRuntimeIdentity(1), true).Succeeded());
    REQUIRE(CampaignStateMachine::SetReady(
        campaign, MakeRuntimeIdentity(4), true).Succeeded());

    Bytes payload;
    REQUIRE(RuntimeCodec::EncodeCoreState(campaign, payload).Succeeded());
    auto decoded = RuntimeCodec::DecodeCoreState(
        campaign.Id,
        campaign.RosterSealed,
        ToRuntimeRosterRecords(campaign),
        campaign.Version,
        payload);
    REQUIRE(decoded.Succeeded());
    REQUIRE(decoded.Value == campaign);

    auto stale = RuntimeCodec::DecodeCoreState(
        campaign.Id,
        campaign.RosterSealed,
        ToRuntimeRosterRecords(campaign),
        campaign.Version + 1,
        payload);
    REQUIRE(stale.Error == StoreError::IntegrityFailure);
}
