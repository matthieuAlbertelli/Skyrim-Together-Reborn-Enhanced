#pragma once

#include <CampaignCoreCodec.h>
#include <CampaignStore.h>

#include <optional>
#include <string>
#include <unordered_map>
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

    CampaignLoadResult LoadCampaign(
        const CampaignId& acCampaign,
        const std::vector<CampaignMemberPresence>& acPresence = {}) noexcept;
    CampaignCreationLookupResult FindCampaignCreation(
        const PlayerId& acPlayer,
        const MutationId& acMutation) noexcept;

private:
    [[nodiscard]] std::optional<CampaignCommandResult> CheckMutationFence(
        const CampaignId& acCampaign) const noexcept;

    ICampaignStore& m_store;
    std::unordered_map<std::string, CampaignCheckpointActivity>
        m_activeCheckpoints;
};
}
