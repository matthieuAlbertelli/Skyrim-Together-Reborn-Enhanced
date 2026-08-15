#pragma once

#include <CampaignTypes.h>

namespace STRE::Campaign
{
class ICampaignStore
{
public:
    virtual ~ICampaignStore() = default;

    virtual StoreValueResult<std::uint32_t> GetSchemaVersion() noexcept = 0;
    virtual StoreResult CheckIntegrity() noexcept = 0;

    virtual MutationResult CreateCampaign(
        const CreateCampaignRequest& acRequest) noexcept = 0;
    virtual StoreValueResult<CampaignRecord> LoadCampaign(
        const CampaignId& acCampaign) noexcept = 0;
    virtual StoreValueResult<CampaignProjection> LoadCampaignProjection(
        const CampaignId& acCampaign,
        const ProjectionAudience& acAudience) noexcept = 0;
    virtual MutationResult ApplyMutation(
        const CampaignMutationRequest& acRequest) noexcept = 0;

    virtual MutationResult CreateCheckpointCandidate(
        const CreateCheckpointCandidateRequest& acRequest) noexcept = 0;
    virtual MutationResult RecordCheckpointSlotSave(
        const RecordCheckpointSlotSaveRequest& acRequest) noexcept = 0;
    virtual MutationResult CommitCheckpoint(
        const CommitCheckpointRequest& acRequest) noexcept = 0;
    virtual StoreValueResult<CheckpointRecord> LoadCheckpoint(
        const CampaignId& acCampaign,
        const CheckpointId& acCheckpoint) noexcept = 0;
    virtual StoreValueResult<CheckpointRecord> LoadLastCommittedCheckpoint(
        const CampaignId& acCampaign) noexcept = 0;
    virtual MutationResult RestoreCheckpointSnapshot(
        const RestoreCheckpointRequest& acRequest) noexcept = 0;

    virtual StoreValueResult<std::vector<JournalRecord>> LoadJournal(
        const CampaignId& acCampaign) noexcept = 0;
    virtual StoreValueResult<std::vector<JournalRecord>> LoadJournalByMutation(
        const MutationId& acMutation,
        std::string_view acKind) noexcept = 0;
    virtual StoreValueResult<std::vector<OutboxRecord>> LoadPendingOutbox(
        const CampaignId& acCampaign) noexcept = 0;
    virtual StoreResult MarkOutboxDelivered(std::uint64_t aOutboxId) noexcept = 0;
};
}
