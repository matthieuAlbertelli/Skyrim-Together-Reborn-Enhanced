#pragma once

#include <CampaignClientAdmissionState.h>
#include <CampaignIdentityStore.h>

#include <cstdint>
#include <optional>
#include <string>

namespace STRE::Campaign
{
struct CampaignCheckpointClientRequest
{
    std::string CampaignId;
    std::string CheckpointId;
    std::uint64_t SourceRevision{};
    std::string NativeSaveIdentity;

    bool operator==(const CampaignCheckpointClientRequest&) const noexcept = default;
};

enum class CampaignCheckpointClientActionKind
{
    Wait,
    StartNativeSave,
    ValidateExisting,
    SendFailure
};

struct CampaignCheckpointClientAction
{
    CampaignCheckpointClientActionKind Kind{
        CampaignCheckpointClientActionKind::SendFailure};
    CampaignCheckpointClientRequest Request;
    std::optional<NativeSaveBundleArtifact> ExpectedArtifact;
};

struct CampaignCheckpointClientResult
{
    CampaignCheckpointClientRequest Request;
    bool Succeeded{};
    std::optional<NativeSaveBundleArtifact> Artifact;
};

class CampaignCheckpointClient final
{
public:
    explicit CampaignCheckpointClient(CampaignIdentityStore& aStore) noexcept;

    [[nodiscard]] CampaignCheckpointClientAction HandleRequest(
        CampaignCheckpointClientRequest aRequest,
        const std::optional<CampaignClientAdmission>& acAdmission) noexcept;
    [[nodiscard]] std::optional<CampaignCheckpointClientResult>
    CompleteNativeSave(const NativeSaveBundleArtifact& acArtifact) noexcept;
    [[nodiscard]] std::optional<CampaignCheckpointClientResult>
    FailNativeSave() noexcept;
    [[nodiscard]] const std::optional<CampaignCheckpointClientRequest>&
    GetActiveRequest() const noexcept { return m_active; }

private:
    CampaignIdentityStore& m_store;
    std::optional<CampaignCheckpointClientRequest> m_active;
};
}
