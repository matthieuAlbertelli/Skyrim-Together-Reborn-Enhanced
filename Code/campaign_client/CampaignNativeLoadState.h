#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace STRE::Campaign
{
enum class CampaignNativeLoadState : std::uint8_t
{
    Idle,
    ValidatingArtifact,
    ReadyToInvoke,
    Invoking,
    AwaitingPostLoad,
    AwaitingSafetyProof,
    Completed,
    Failed
};

enum class CampaignNativeLoadFailure : std::uint8_t
{
    None,
    InvalidIdentity,
    CampaignAdmissionUnavailable,
    ArtifactUnavailable,
    ArtifactInvalid,
    NativeSaveBusy,
    ArtifactValidationRejected,
    ArtifactValidationFailed,
    GateArmFailed,
    NativeBoundaryUnavailable,
    UnexpectedNativeLoad,
    NativeLoadRejected,
    PostLoadMissing,
    GateNotLocked,
    GuardMenuUnavailable,
    GameNotPaused,
    TransportUnavailable,
    Timeout,
    InternalFailure
};

struct CampaignNativeLoadSnapshot
{
    CampaignNativeLoadState State{CampaignNativeLoadState::Idle};
    std::string Identity;
    CampaignNativeLoadFailure Failure{CampaignNativeLoadFailure::None};
    bool ArtifactValidated{};
    bool NativeEntered{};
    bool NativeReturned{};
    bool NativeAccepted{};
    bool PostLoadObserved{};
    bool GateLocked{};
    bool GuardMenuObserved{};
    bool GamePaused{};
    bool TransportAlive{};
};

class CampaignNativeLoadRequest final
{
public:
    [[nodiscard]] bool Request(std::string aIdentity) noexcept;
    [[nodiscard]] bool MarkArtifactValidated() noexcept;
    [[nodiscard]] bool BeginInvocation() noexcept;
    [[nodiscard]] bool OnNativeLoadEnter(
        std::string_view acActualIdentity) noexcept;
    void OnNativeLoadReturn(bool aManaged, bool aSucceeded) noexcept;
    [[nodiscard]] bool OnPostLoad() noexcept;
    void ObserveGateLocked() noexcept;
    void ObserveGuardMenu(bool aGamePaused) noexcept;
    void ObserveTransportAlive() noexcept;
    [[nodiscard]] bool Fail(CampaignNativeLoadFailure aFailure) noexcept;
    [[nodiscard]] bool Reset() noexcept;

    [[nodiscard]] const CampaignNativeLoadSnapshot& Snapshot() const noexcept
    {
        return m_snapshot;
    }
    [[nodiscard]] bool IsActive() const noexcept;

private:
    void CompleteIfProven() noexcept;

    CampaignNativeLoadSnapshot m_snapshot;
};

[[nodiscard]] const char* ToString(
    CampaignNativeLoadFailure aFailure) noexcept;
}
