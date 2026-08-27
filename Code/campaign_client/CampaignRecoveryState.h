#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace STRE::Campaign
{
enum class CampaignClientRecoveryStage : std::uint8_t
{
    Idle,
    RecoveryLocked,
    LoadingNativeSave,
    NativeSaveLoaded,
    SnapshotApplied,
    Completed,
    Failed
};

enum class CampaignRecoveryClientAction : std::uint8_t
{
    Reject,
    Ignore,
    StartNativeLoad,
    ResendLoaded,
    ApplySnapshot,
    ResendSnapshotApplied,
    Release
};

enum class CampaignRecoveryDisconnectContext : std::uint8_t
{
    GameplayWorld,
    MainMenuRuntimeDeparture
};

enum class CampaignRecoveryLocalGateAction : std::uint8_t
{
    LockGameplay,
    SkipNoGameplay
};

// Server recovery authority and local gameplay presentation are separate. A
// correlated Main Menu departure still disconnects from the server, but has no
// loaded world for the native gameplay gate to fence.
[[nodiscard]] constexpr CampaignRecoveryLocalGateAction
ProjectRecoveryDisconnectGate(
    CampaignRecoveryDisconnectContext aContext) noexcept
{
    return aContext ==
            CampaignRecoveryDisconnectContext::MainMenuRuntimeDeparture
        ? CampaignRecoveryLocalGateAction::SkipNoGameplay
        : CampaignRecoveryLocalGateAction::LockGameplay;
}

struct CampaignRecoveryCorrelation
{
    std::string CampaignId;
    std::string RestoreAttemptId;
    std::string CheckpointId;
    std::uint64_t SourceRevision{};
    std::uint64_t RestoreRevision{};

    bool operator==(const CampaignRecoveryCorrelation&) const noexcept =
        default;
};

class CampaignRecoveryState final
{
public:
    [[nodiscard]] bool Lock(std::string aCampaignId) noexcept;
    [[nodiscard]] bool LockProvisional(
        std::string aCampaignId) noexcept;
    [[nodiscard]] CampaignRecoveryClientAction ObserveAuthoritativeActive(
        std::string_view acCampaignId) noexcept;
    [[nodiscard]] CampaignRecoveryClientAction ObserveLoadRequest(
        CampaignRecoveryCorrelation aCorrelation) noexcept;
    [[nodiscard]] bool FinishNativeLoad(bool aSucceeded) noexcept;
    [[nodiscard]] CampaignRecoveryClientAction ObserveSnapshot(
        const CampaignRecoveryCorrelation& acCorrelation) noexcept;
    [[nodiscard]] bool FinishSnapshotApply() noexcept;
    [[nodiscard]] CampaignRecoveryClientAction ObserveComplete(
        const CampaignRecoveryCorrelation& acCorrelation) noexcept;

    [[nodiscard]] CampaignClientRecoveryStage GetStage() const noexcept
    {
        return m_stage;
    }
    [[nodiscard]] const CampaignRecoveryCorrelation& GetCorrelation()
        const noexcept
    {
        return m_correlation;
    }

private:
    [[nodiscard]] bool LockInternal(
        std::string aCampaignId,
        bool aAuthoritative) noexcept;
    [[nodiscard]] bool MatchesBase(
        const CampaignRecoveryCorrelation& acCorrelation) const noexcept;

    CampaignClientRecoveryStage m_stage{CampaignClientRecoveryStage::Idle};
    CampaignRecoveryCorrelation m_correlation;
    bool m_authoritativeLock{};
};
}
