#pragma once

#include <CampaignRuntimeService.h>
#include <Structs/Campaign.h>

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

    [[nodiscard]] std::optional<CampaignSnapshotData> BuildSnapshot(
        const CampaignId& acCampaign) noexcept;
    [[nodiscard]] const CampaignAdmissionRecord* FindConnection(
        CampaignConnectionHandle aConnection) const noexcept;
    [[nodiscard]] std::vector<CampaignConnectionHandle>
    GetAdmittedConnections(const CampaignId& acCampaign) const;

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
