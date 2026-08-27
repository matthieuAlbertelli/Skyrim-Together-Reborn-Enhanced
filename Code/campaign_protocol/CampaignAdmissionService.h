#pragma once

#include <CampaignRuntimeService.h>
#include <Structs/Campaign.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace STRE::Campaign
{
using CampaignConnectionHandle = std::uint64_t;

enum class CampaignConnectionRegistration
{
    Accepted,
    InvalidPlayerId,
    DuplicateActivePlayerId
};

struct CampaignAdmissionRecord
{
    CampaignConnectionHandle Connection{};
    PlayerId Player;
    std::optional<CampaignMemberIdentity> AdmittedIdentity;
    std::optional<CampaignMemberIdentity> PreviousAdmission;
};

enum class CampaignRecoveryRecipientError : std::uint8_t
{
    None,
    InvalidCheckpointRoster,
    MissingCurrentAdmission,
    DuplicateCurrentAdmission,
    UnexpectedCurrentAdmission
};

struct CampaignRecoveryRecipient
{
    CampaignConnectionHandle Connection{};
    CampaignMemberIdentity Identity;
};

struct CampaignRecoveryRecipientPlan
{
    CampaignRecoveryRecipientError Error{
        CampaignRecoveryRecipientError::None};
    std::size_t RequiredMemberCount{};
    std::vector<CampaignRecoveryRecipient> Recipients;
    std::optional<CampaignMemberIdentity> FailedIdentity;

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == CampaignRecoveryRecipientError::None &&
            RequiredMemberCount != 0 &&
            Recipients.size() == RequiredMemberCount;
    }
};

struct CampaignProtocolCommandResult
{
    CampaignProtocolOperation Operation{CampaignProtocolOperation::Create};
    CampaignProtocolResult Result{CampaignProtocolResult::InvalidRequest};
    std::string MutationId;
    std::string CampaignId;
    StateVersion Version{};
    std::string CampaignSlotId;
    std::string CharacterBindingId;
    std::string JoinCode;
    std::optional<CampaignSnapshotData> Snapshot;

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Result == CampaignProtocolResult::Applied ||
            Result == CampaignProtocolResult::AcceptedNoOp ||
            Result == CampaignProtocolResult::IdempotentReplay;
    }
};

class CampaignAdmissionService final
{
public:
    using IdGenerator = std::function<std::string(std::string_view)>;

    explicit CampaignAdmissionService(
        CampaignRuntimeService& aRuntime,
        IdGenerator aIdGenerator = {}) noexcept;

    [[nodiscard]] CampaignConnectionRegistration RegisterConnection(
        CampaignConnectionHandle aConnection,
        std::string aPlayerId) noexcept;
    [[nodiscard]] std::optional<CampaignSnapshotData> Disconnect(
        CampaignConnectionHandle aConnection) noexcept;

    [[nodiscard]] CampaignProtocolCommandResult CreateCampaign(
        CampaignConnectionHandle aConnection,
        const std::string& acMutationId,
        bool aPartyLeader) noexcept;
    [[nodiscard]] CampaignProtocolCommandResult JoinCampaign(
        CampaignConnectionHandle aConnection,
        const std::string& acCampaignId,
        const std::string& acMutationId,
        StateVersion aExpectedRevision,
        bool aSameLiveSession) noexcept;
    [[nodiscard]] CampaignProtocolCommandResult ResumeCampaign(
        CampaignConnectionHandle aConnection,
        const std::string& acCampaignId,
        const std::string& acCharacterBindingId) noexcept;
    [[nodiscard]] CampaignRecoveryCommandResult
    BeginCampaignLoadRecovery(
        CampaignConnectionHandle aConnection,
        const CampaignId& acCampaign) noexcept;
    [[nodiscard]] CampaignProtocolCommandResult StartCampaign(
        CampaignConnectionHandle aConnection,
        const std::string& acCampaignId,
        const std::string& acMutationId,
        StateVersion aExpectedRevision,
        bool aPartyLeader,
        bool aSameLiveSession) noexcept;
    [[nodiscard]] CampaignProtocolCommandResult SetReady(
        CampaignConnectionHandle aConnection,
        const std::string& acCampaignId,
        const std::string& acMutationId,
        StateVersion aExpectedRevision,
        bool aReady) noexcept;
    [[nodiscard]] CampaignProtocolCommandResult LeaveCampaign(
        CampaignConnectionHandle aConnection,
        const std::string& acCampaignId,
        const std::string& acMutationId,
        StateVersion aExpectedRevision) noexcept;

    [[nodiscard]] CampaignCheckpointCommandResult BeginCheckpoint(
        const CampaignId& acCampaign) noexcept;
    [[nodiscard]] CampaignCheckpointCommandResult BeginCheckpoint(
        CampaignConnectionHandle aConnection) noexcept;
    [[nodiscard]] CampaignCheckpointCommandResult HandleCheckpointSaveResult(
        CampaignConnectionHandle aConnection,
        const CampaignId& acCampaign,
        const CheckpointId& acCheckpoint,
        const std::string& acNativeSaveIdentity,
        bool aSucceeded,
        std::string aFingerprintAlgorithm = {},
        std::uint32_t aFingerprintVersion = 0,
        Bytes aFingerprint = {},
        std::uint32_t aSaveMetadataCodecVersion = 0,
        Bytes aSaveMetadata = {}) noexcept;
    [[nodiscard]] std::optional<CampaignCheckpointActivity>
    GetActiveCheckpoint(const CampaignId& acCampaign) const noexcept;

    [[nodiscard]] CampaignRecoveryCommandResult PrepareRecovery(
        const CampaignId& acCampaign) noexcept;
    [[nodiscard]] CampaignRecoveryCommandResult HandleRecoveryLoaded(
        CampaignConnectionHandle aConnection,
        const CampaignId& acCampaign,
        const RestoreAttemptId& acAttempt,
        const CheckpointId& acCheckpoint,
        bool aSucceeded,
        std::string aNativeSaveIdentity,
        std::string aFingerprintAlgorithm,
        std::uint32_t aFingerprintVersion,
        Bytes aFingerprint,
        std::uint32_t aSaveMetadataCodecVersion,
        Bytes aSaveMetadata) noexcept;
    [[nodiscard]] CampaignRecoveryCommandResult
    HandleRecoverySnapshotApplied(
        CampaignConnectionHandle aConnection,
        const CampaignId& acCampaign,
        const RestoreAttemptId& acAttempt,
        const CheckpointId& acCheckpoint,
        StateVersion aRestoreRevision) noexcept;
    [[nodiscard]] std::optional<CampaignRecoveryActivity>
    GetRecoveryActivity(const CampaignId& acCampaign) noexcept;

    [[nodiscard]] std::optional<CampaignSnapshotData> BuildSnapshot(
        const CampaignId& acCampaign) noexcept;
    [[nodiscard]] const CampaignAdmissionRecord* FindConnection(
        CampaignConnectionHandle aConnection) const noexcept;
    [[nodiscard]] std::vector<CampaignConnectionHandle>
    GetAdmittedConnections(const CampaignId& acCampaign) const;
    [[nodiscard]] CampaignRecoveryRecipientPlan
    PrepareRecoveryRecipients(
        const CheckpointRecord& acCheckpoint) const noexcept;

private:
    [[nodiscard]] CampaignAdmissionRecord* FindConnection(
        CampaignConnectionHandle aConnection) noexcept;
    [[nodiscard]] std::vector<CampaignMemberPresence> BuildPresence(
        const CampaignId& acCampaign) const;
    [[nodiscard]] std::string GenerateId(std::string_view acPrefix);

    CampaignRuntimeService& m_runtime;
    IdGenerator m_idGenerator;
    std::vector<CampaignAdmissionRecord> m_connections;
};
}
