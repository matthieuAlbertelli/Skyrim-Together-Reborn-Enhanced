#pragma once

#include <CampaignCoreCodec.h>
#include <CampaignStore.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace STRE::Campaign
{
struct CampaignCommandResult
{
    CampaignError Error{CampaignError::None};
    StoreError PersistenceError{StoreError::None};
    std::string Message;
    StateVersion Version{};
    bool Applied{};
    bool IdempotentReplay{};

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == CampaignError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

struct CampaignLoadResult
{
    CampaignError Error{CampaignError::None};
    StoreError PersistenceError{StoreError::None};
    std::string Message;
    CampaignAggregate Campaign;
    CampaignRuntimeState RuntimeState{
        CampaignRuntimeState::WAITING_FOR_ROSTER};

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == CampaignError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

struct CampaignCreationLookupResult
{
    CampaignError Error{CampaignError::None};
    StoreError PersistenceError{StoreError::None};
    std::string Message;
    std::optional<CampaignMemberIdentity> Identity;
    StateVersion Version{};

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == CampaignError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

struct CreateLobbyCampaignCommand
{
    CampaignId Campaign;
    MutationId Mutation;
    std::vector<CampaignSlotRecord> InitialRoster;
};

struct AddRosterSlotCommand
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    CampaignSlotRecord Slot;
};

struct RemoveRosterSlotCommand
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    CampaignSlotId Slot;
};

struct ReplaceRosterSlotCommand
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    CampaignSlotRecord Slot;
};

struct CommitCampaignStartCommand
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    PlayerId SessionManager;
};

struct TransferSessionManagerCommand
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    PlayerId Actor;
    PlayerId NewManager;
};

struct SetCampaignReadyCommand
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    CampaignMemberIdentity Actor;
    bool Ready{};
};

struct CampaignCheckpointActivity
{
    CampaignId Campaign;
    CheckpointId Checkpoint;
    StateVersion SourceRevision{};
    std::string NativeSaveIdentity;

    bool operator==(const CampaignCheckpointActivity&) const noexcept = default;
};

struct BeginCampaignCheckpointCommand
{
    CampaignId Campaign;
    CheckpointId Checkpoint;
    std::string NativeSaveIdentity;
    std::vector<CampaignMemberPresence> Presence;
};

struct RecordCampaignCheckpointSaveCommand
{
    CampaignId Campaign;
    CheckpointId Checkpoint;
    std::string NativeSaveIdentity;
    CampaignMemberIdentity Actor;
    std::string FingerprintAlgorithm;
    std::uint32_t FingerprintVersion{};
    Bytes Fingerprint;
    std::uint32_t SaveMetadataCodecVersion{};
    Bytes SaveMetadata;
};

struct FailCampaignCheckpointCommand
{
    CampaignId Campaign;
    CheckpointId Checkpoint;
    std::string NativeSaveIdentity;
    CampaignMemberIdentity Actor;
};

struct CampaignCheckpointCommandResult
{
    CampaignCommandResult Command;
    std::optional<CampaignCheckpointActivity> Activity;
    bool Committed{};

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Command.Succeeded();
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

enum class CampaignRecoveryStage : std::uint8_t
{
    RecoveryLock,
    LoadingNativeSaves,
    ApplyingSnapshot
};

enum class CampaignRecoveryReason : std::uint8_t
{
    None,
    RosterIncomplete,
    CampaignLoadRequested,
    NoCommittedCheckpoint,
    ClientLoadFailed
};

struct CampaignRecoveryActivity
{
    CampaignId Campaign;
    RestoreAttemptId Attempt;
    StateVersion EntryRevision{};
    CampaignRecoveryStage Stage{CampaignRecoveryStage::RecoveryLock};
    CampaignRecoveryReason Reason{CampaignRecoveryReason::RosterIncomplete};
    std::optional<CheckpointId> Checkpoint;
    StateVersion SourceRevision{};
    std::optional<StateVersion> RestoreRevision;
    std::unordered_set<std::string> DurableLoadedSlots;
    std::unordered_set<std::string> DurableSnapshotAppliedSlots;
    std::unordered_set<std::string> LoadedSlots;
    std::unordered_set<std::string> SnapshotAppliedSlots;
};

enum class CampaignRecoveryDispatch : std::uint8_t
{
    None,
    NativeLoad,
    RestoredSnapshot
};

struct BeginCampaignRecoveryCommand
{
    CampaignId Campaign;
    CampaignMemberIdentity DisconnectedMember;
    std::vector<CampaignMemberPresence> Presence;
    CampaignRuntimeState RuntimeStateBeforeDisconnect{
        CampaignRuntimeState::WAITING_FOR_ROSTER};
    bool CampaignLoadRequested{};
};

struct PrepareCampaignRecoveryCommand
{
    CampaignId Campaign;
    std::vector<CampaignMemberPresence> Presence;
};

struct RecordCampaignRecoveryLoadedCommand
{
    CampaignId Campaign;
    RestoreAttemptId Attempt;
    CheckpointId Checkpoint;
    CampaignMemberIdentity Actor;
    bool Succeeded{};
    std::string NativeSaveIdentity;
    std::string FingerprintAlgorithm;
    std::uint32_t FingerprintVersion{};
    Bytes Fingerprint;
    std::uint32_t SaveMetadataCodecVersion{};
    Bytes SaveMetadata;
    std::vector<CampaignMemberPresence> Presence;
};

struct RecordCampaignRecoverySnapshotAppliedCommand
{
    CampaignId Campaign;
    RestoreAttemptId Attempt;
    CheckpointId Checkpoint;
    StateVersion RestoreRevision{};
    CampaignMemberIdentity Actor;
    std::vector<CampaignMemberPresence> Presence;
};

struct CampaignRecoveryCommandResult
{
    CampaignCommandResult Command;
    std::optional<CampaignRecoveryActivity> Activity;
    std::optional<CheckpointRecord> Checkpoint;
    CampaignRecoveryDispatch Dispatch{CampaignRecoveryDispatch::None};
    bool FirstBarrierCompleted{};
    bool RecoveryCompleted{};

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Command.Succeeded();
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

class CampaignRuntimeService final
{
public:
    explicit CampaignRuntimeService(ICampaignStore& aStore) noexcept;

    CampaignCommandResult CreateLobbyCampaign(
        const CreateLobbyCampaignCommand& acCommand) noexcept;
    CampaignCommandResult AddRosterSlot(
        const AddRosterSlotCommand& acCommand) noexcept;
    CampaignCommandResult RemoveRosterSlot(
        const RemoveRosterSlotCommand& acCommand) noexcept;
    CampaignCommandResult ReplaceRosterSlot(
        const ReplaceRosterSlotCommand& acCommand) noexcept;
    CampaignCommandResult CommitCampaignStart(
        const CommitCampaignStartCommand& acCommand) noexcept;
    CampaignCommandResult TransferSessionManager(
        const TransferSessionManagerCommand& acCommand) noexcept;
    CampaignCommandResult SetReady(
        const SetCampaignReadyCommand& acCommand) noexcept;

    CampaignCheckpointCommandResult BeginCheckpoint(
        const BeginCampaignCheckpointCommand& acCommand) noexcept;
    CampaignCheckpointCommandResult RecordCheckpointSave(
        const RecordCampaignCheckpointSaveCommand& acCommand) noexcept;
    CampaignCheckpointCommandResult FailCheckpoint(
        const FailCampaignCheckpointCommand& acCommand) noexcept;
    void AbandonCheckpoint(const CampaignId& acCampaign) noexcept;
    [[nodiscard]] std::optional<CampaignCheckpointActivity>
    GetActiveCheckpoint(const CampaignId& acCampaign) const noexcept;

    CampaignRecoveryCommandResult BeginRecovery(
        const BeginCampaignRecoveryCommand& acCommand) noexcept;
    CampaignRecoveryCommandResult PrepareRecovery(
        const PrepareCampaignRecoveryCommand& acCommand) noexcept;
    CampaignRecoveryCommandResult RecordRecoveryLoaded(
        const RecordCampaignRecoveryLoadedCommand& acCommand) noexcept;
    CampaignRecoveryCommandResult RecordRecoverySnapshotApplied(
        const RecordCampaignRecoverySnapshotAppliedCommand& acCommand) noexcept;
    [[nodiscard]] std::optional<CampaignRecoveryActivity>
    GetRecoveryActivity(const CampaignId& acCampaign) noexcept;

    CampaignLoadResult LoadCampaign(
        const CampaignId& acCampaign,
        const std::vector<CampaignMemberPresence>& acPresence = {}) noexcept;
    CampaignCreationLookupResult FindCampaignCreation(
        const PlayerId& acPlayer,
        const MutationId& acMutation) noexcept;

private:
    [[nodiscard]] std::optional<CampaignCommandResult> CheckMutationFence(
        const CampaignId& acCampaign) noexcept;
    [[nodiscard]] std::optional<CampaignCommandResult>
    ReconstructRecovery(const CampaignId& acCampaign) noexcept;

    ICampaignStore& m_store;
    std::unordered_map<std::string, CampaignCheckpointActivity>
        m_activeCheckpoints;
    std::unordered_map<std::string, CampaignRecoveryActivity> m_recoveries;
};
}
