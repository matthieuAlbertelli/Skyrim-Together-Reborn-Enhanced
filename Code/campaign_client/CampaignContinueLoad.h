#pragma once

#include <CampaignLoadPolicy.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace STRE::Campaign
{
struct CampaignContinueLoadClaim
{
    std::string NativeSaveIdentity;
    bool TargetReadable{};
};

enum class CampaignContinueResumePhase : std::uint8_t
{
    Idle,
    PendingNativeTransition,
    NativeTransitionAccepted
};

// Separates logical ResumeRequired ownership from its local gameplay-gate
// presentation. Only an accepted correlated native Continue followed by the
// semantic MainMenuClosed boundary can commit the transition.
class CampaignContinueResumeTransition final
{
public:
    [[nodiscard]] bool Begin(std::string aNativeSaveIdentity) noexcept;
    [[nodiscard]] bool ObserveNativeResult(bool aAccepted) noexcept;
    [[nodiscard]] std::optional<std::string> CommitMainMenuClosed() noexcept;
    void Cancel() noexcept;

    [[nodiscard]] CampaignContinueResumePhase GetPhase() const noexcept
    {
        return m_phase;
    }
    [[nodiscard]] const std::string& GetNativeSaveIdentity() const noexcept
    {
        return m_nativeSaveIdentity;
    }

private:
    std::string m_nativeSaveIdentity;
    CampaignContinueResumePhase m_phase{CampaignContinueResumePhase::Idle};
};

// Owns only the bounded semantic lifetime of one Main Menu Continue action.
// Native request ancestry is supplied by the Skyrim adapter; operation codes
// and save-list ordering deliberately play no role in this state.
class CampaignContinueLoadAttempt final
{
public:
    void Begin(std::uintptr_t aRootRequest) noexcept;
    [[nodiscard]] std::optional<CampaignContinueLoadClaim> ClaimTarget(
        std::uintptr_t aRootRequest,
        std::uintptr_t aParentRequest,
        std::uintptr_t aRequest,
        bool aExactLoadRequestType,
        std::string_view acNativeTarget,
        bool aTargetReadable) noexcept;
    void Complete(std::uintptr_t aRootRequest) noexcept;
    [[nodiscard]] bool IsActive(std::uintptr_t aRootRequest) const noexcept;

private:
    std::uintptr_t m_rootRequest{};
    bool m_targetClaimed{};
};

// Skyrim exposes its canonical load target as a native .ess filename, while
// STRE marker/cache APIs use the filename stem as NativeSaveIdentity.
[[nodiscard]] bool NormalizeSkyrimNativeSaveIdentity(
    std::string_view acNativeTarget,
    std::string& aNativeSaveIdentity) noexcept;

// Continue is a player authority seam. This helper deliberately cannot grant
// InternalRecovery, even when the target names a valid campaign checkpoint.
[[nodiscard]] CampaignLoadDecision EvaluateCampaignContinueLoad(
    const CampaignContinueLoadClaim& acClaim,
    CampaignLoadTarget aClassifiedTarget,
    bool aCampaignRuntimeSensitive) noexcept;
}
