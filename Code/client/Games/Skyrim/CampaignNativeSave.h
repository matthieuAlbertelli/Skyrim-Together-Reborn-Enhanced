#pragma once

#include <Structs/NativeSaveBundle.h>

#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

enum class CampaignNativeSaveLifecycleState
{
    Idle,
    Requested,
    Processing,
    AwaitingCompletion,
    Completed,
    Failed
};

struct CampaignNativeSaveLifecycleSnapshot
{
    CampaignNativeSaveLifecycleState State{
        CampaignNativeSaveLifecycleState::Idle};
    std::string Identity;
    std::optional<STRE::Campaign::NativeSaveBundleArtifact> Artifact;
    std::string FailureReason;
};

namespace CampaignNativeSaveDetail
{
class RequestSlot final
{
public:
    [[nodiscard]] bool TryRequest(std::string aIdentity)
    {
        std::lock_guard lock(m_mutex);
        if (IsActive(m_state) || aIdentity.empty())
            return false;

        m_identity = std::move(aIdentity);
        m_artifact.reset();
        m_failureReason.clear();
        m_state = CampaignNativeSaveLifecycleState::Requested;
        return true;
    }

    [[nodiscard]] std::optional<std::string> RequestedIdentity() const
    {
        std::lock_guard lock(m_mutex);
        if (m_state != CampaignNativeSaveLifecycleState::Requested)
            return std::nullopt;
        return m_identity;
    }

    [[nodiscard]] std::optional<std::string> BeginProcessing()
    {
        std::lock_guard lock(m_mutex);
        if (m_state != CampaignNativeSaveLifecycleState::Requested)
            return std::nullopt;

        m_state = CampaignNativeSaveLifecycleState::Processing;
        return m_identity;
    }

    [[nodiscard]] bool BeginAwaitingCompletion() noexcept
    {
        std::lock_guard lock(m_mutex);
        if (m_state != CampaignNativeSaveLifecycleState::Processing)
            return false;
        m_state = CampaignNativeSaveLifecycleState::AwaitingCompletion;
        return true;
    }

    [[nodiscard]] bool Complete(
        STRE::Campaign::NativeSaveBundleArtifact aArtifact)
    {
        std::lock_guard lock(m_mutex);
        if (m_state != CampaignNativeSaveLifecycleState::AwaitingCompletion ||
            aArtifact.Bundle.LogicalIdentity != m_identity)
        {
            return false;
        }
        m_artifact = std::move(aArtifact);
        m_failureReason.clear();
        m_state = CampaignNativeSaveLifecycleState::Completed;
        return true;
    }

    [[nodiscard]] bool Fail(std::string aReason)
    {
        std::lock_guard lock(m_mutex);
        if (!IsActive(m_state) || aReason.empty())
            return false;
        m_artifact.reset();
        m_failureReason = std::move(aReason);
        m_state = CampaignNativeSaveLifecycleState::Failed;
        return true;
    }

    [[nodiscard]] CampaignNativeSaveLifecycleSnapshot Snapshot() const
    {
        std::lock_guard lock(m_mutex);
        return {m_state, m_identity, m_artifact, m_failureReason};
    }

private:
    static bool IsActive(CampaignNativeSaveLifecycleState aState) noexcept
    {
        return aState == CampaignNativeSaveLifecycleState::Requested ||
            aState == CampaignNativeSaveLifecycleState::Processing ||
            aState == CampaignNativeSaveLifecycleState::AwaitingCompletion;
    }

    mutable std::mutex m_mutex;
    CampaignNativeSaveLifecycleState m_state{
        CampaignNativeSaveLifecycleState::Idle};
    std::string m_identity;
    std::optional<STRE::Campaign::NativeSaveBundleArtifact> m_artifact;
    std::string m_failureReason;
};
}

enum class CampaignNativeSaveRequestState
{
    InvalidIdentity,
    BoundaryUnavailable,
    RequestAlreadyActive,
    InternalFailure,
    Accepted
};

struct CampaignNativeSaveRequestResult
{
    CampaignNativeSaveRequestState State{
        CampaignNativeSaveRequestState::InvalidIdentity};

    [[nodiscard]] bool WasAccepted() const noexcept
    {
        return State == CampaignNativeSaveRequestState::Accepted;
    }
};

class CampaignNativeSave final
{
public:
    // Called from STRE's game-update path. This only stores one request for the
    // engine save/load processing boundary and returns without calling Save.
    [[nodiscard]] static CampaignNativeSaveRequestResult RequestOnGameThread(
        std::string_view acNativeSaveIdentity) noexcept;

    // Re-opens an already completed checkpoint bundle for idempotent protocol
    // replay. It never calls Skyrim Save and never writes either member.
    [[nodiscard]] static CampaignNativeSaveRequestResult
    ValidateExistingOnGameThread(
        std::string_view acNativeSaveIdentity,
        const STRE::Campaign::NativeSaveBundleArtifact& acExpectedArtifact) noexcept;

    [[nodiscard]] static CampaignNativeSaveLifecycleSnapshot GetStatus();
};
