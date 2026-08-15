#pragma once

#include <CampaignStore.h>

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>

struct sqlite3;

namespace STRE::Campaign
{
enum class TransactionStage
{
    AfterCurrentState,
    AfterJournal,
    AfterOutbox,
    BeforeCommit
};

struct SqliteCampaignStoreOptions
{
    std::chrono::milliseconds BusyTimeout{5000};
    std::function<bool(TransactionStage)> FaultInjector;
};

class SqliteCampaignStore final : public ICampaignStore
{
public:
    static std::unique_ptr<SqliteCampaignStore> Open(
        const std::filesystem::path& acPath,
        StoreResult& aResult,
        SqliteCampaignStoreOptions aOptions = {}) noexcept;

    ~SqliteCampaignStore() override;

    SqliteCampaignStore(const SqliteCampaignStore&) = delete;
    SqliteCampaignStore& operator=(const SqliteCampaignStore&) = delete;

    StoreValueResult<std::uint32_t> GetSchemaVersion() noexcept override;
    StoreResult CheckIntegrity() noexcept override;

    MutationResult CreateCampaign(
        const CreateCampaignRequest& acRequest) noexcept override;
    StoreValueResult<CampaignRecord> LoadCampaign(
        const CampaignId& acCampaign) noexcept override;
    StoreValueResult<CampaignProjection> LoadCampaignProjection(
        const CampaignId& acCampaign,
        const ProjectionAudience& acAudience) noexcept override;
    MutationResult ApplyMutation(
        const CampaignMutationRequest& acRequest) noexcept override;

    MutationResult CreateCheckpointCandidate(
        const CreateCheckpointCandidateRequest& acRequest) noexcept override;
    MutationResult RecordCheckpointSlotSave(
        const RecordCheckpointSlotSaveRequest& acRequest) noexcept override;
    MutationResult CommitCheckpoint(
        const CommitCheckpointRequest& acRequest) noexcept override;
    StoreValueResult<CheckpointRecord> LoadCheckpoint(
        const CampaignId& acCampaign,
        const CheckpointId& acCheckpoint) noexcept override;
    StoreValueResult<CheckpointRecord> LoadLastCommittedCheckpoint(
        const CampaignId& acCampaign) noexcept override;
    MutationResult RestoreCheckpointSnapshot(
        const RestoreCheckpointRequest& acRequest) noexcept override;

    StoreValueResult<std::vector<JournalRecord>> LoadJournal(
        const CampaignId& acCampaign) noexcept override;
    StoreValueResult<std::vector<JournalRecord>> LoadJournalByMutation(
        const MutationId& acMutation,
        std::string_view acKind) noexcept override;
    StoreValueResult<std::vector<OutboxRecord>> LoadPendingOutbox(
        const CampaignId& acCampaign) noexcept override;
    StoreResult MarkOutboxDelivered(std::uint64_t aOutboxId) noexcept override;

private:
    SqliteCampaignStore(
        sqlite3* apDatabase,
        std::filesystem::path aPath,
        SqliteCampaignStoreOptions aOptions) noexcept;

    StoreResult Initialize() noexcept;
    bool ShouldInject(TransactionStage aStage) const;

    sqlite3* m_pDatabase{};
    std::filesystem::path m_path;
    SqliteCampaignStoreOptions m_options;
};
}
