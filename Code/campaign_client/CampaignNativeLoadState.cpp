#include <CampaignNativeLoadState.h>

#include <Structs/Campaign.h>

namespace STRE::Campaign
{
bool CampaignNativeLoadRequest::Request(std::string aIdentity) noexcept
{
    try
    {
        TiltedPhoques::String nativeIdentity;
        nativeIdentity.assign(aIdentity.data(), aIdentity.size());
        if (m_snapshot.State != CampaignNativeLoadState::Idle ||
            !IsValidCampaignNativeSaveIdentity(nativeIdentity))
        {
            return false;
        }

        m_snapshot = {};
        m_snapshot.State = CampaignNativeLoadState::ValidatingArtifact;
        m_snapshot.Identity = std::move(aIdentity);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool CampaignNativeLoadRequest::MarkArtifactValidated() noexcept
{
    if (m_snapshot.State != CampaignNativeLoadState::ValidatingArtifact)
        return false;

    m_snapshot.ArtifactValidated = true;
    m_snapshot.State = CampaignNativeLoadState::ReadyToInvoke;
    return true;
}

bool CampaignNativeLoadRequest::BeginInvocation() noexcept
{
    if (m_snapshot.State != CampaignNativeLoadState::ReadyToInvoke ||
        !m_snapshot.ArtifactValidated)
    {
        return false;
    }

    m_snapshot.State = CampaignNativeLoadState::Invoking;
    return true;
}

bool CampaignNativeLoadRequest::OnNativeLoadEnter(
    std::string_view acActualIdentity) noexcept
{
    if (m_snapshot.State != CampaignNativeLoadState::Invoking ||
        m_snapshot.NativeEntered)
    {
        return false;
    }
    if (acActualIdentity != m_snapshot.Identity)
    {
        (void)Fail(CampaignNativeLoadFailure::UnexpectedNativeLoad);
        return false;
    }

    m_snapshot.NativeEntered = true;
    return true;
}

void CampaignNativeLoadRequest::OnNativeLoadReturn(
    bool aManaged,
    bool aSucceeded) noexcept
{
    if (!aManaged || m_snapshot.State == CampaignNativeLoadState::Failed ||
        m_snapshot.State == CampaignNativeLoadState::Completed)
    {
        return;
    }
    if (!m_snapshot.NativeEntered)
    {
        (void)Fail(CampaignNativeLoadFailure::UnexpectedNativeLoad);
        return;
    }

    m_snapshot.NativeReturned = true;
    m_snapshot.NativeAccepted = aSucceeded;
    if (!aSucceeded)
    {
        (void)Fail(CampaignNativeLoadFailure::NativeLoadRejected);
        return;
    }

    m_snapshot.State = m_snapshot.PostLoadObserved
        ? CampaignNativeLoadState::AwaitingSafetyProof
        : CampaignNativeLoadState::AwaitingPostLoad;
    CompleteIfProven();
}

bool CampaignNativeLoadRequest::OnPostLoad() noexcept
{
    if (!IsActive() || !m_snapshot.NativeEntered ||
        m_snapshot.PostLoadObserved)
    {
        return false;
    }

    m_snapshot.PostLoadObserved = true;
    m_snapshot.State = CampaignNativeLoadState::AwaitingSafetyProof;
    CompleteIfProven();
    return true;
}

void CampaignNativeLoadRequest::ObserveGateLocked() noexcept
{
    if (!IsActive() || !m_snapshot.PostLoadObserved)
        return;
    m_snapshot.GateLocked = true;
    CompleteIfProven();
}

void CampaignNativeLoadRequest::ObserveGuardMenu(
    bool aGamePaused) noexcept
{
    (void)ObservePostLoadGuardState(true, aGamePaused);
}

CampaignNativeLoadFailure
CampaignNativeLoadRequest::ObservePostLoadGuardState(
    bool aGuardMenuOpen,
    bool aGamePaused) noexcept
{
    if (!IsActive() || !m_snapshot.GateLocked)
        return CampaignNativeLoadFailure::None;
    if (!aGuardMenuOpen)
        return CampaignNativeLoadFailure::GuardMenuUnavailable;
    m_snapshot.GuardMenuObserved = true;
    m_snapshot.GamePaused = aGamePaused;
    CompleteIfProven();
    return aGamePaused
        ? CampaignNativeLoadFailure::None
        : CampaignNativeLoadFailure::GameNotPaused;
}

void CampaignNativeLoadRequest::ObserveTransportAlive() noexcept
{
    if (!IsActive() || !m_snapshot.GateLocked)
        return;
    m_snapshot.TransportAlive = true;
    CompleteIfProven();
}

bool CampaignNativeLoadRequest::Fail(
    CampaignNativeLoadFailure aFailure) noexcept
{
    if (aFailure == CampaignNativeLoadFailure::None ||
        m_snapshot.State == CampaignNativeLoadState::Idle ||
        m_snapshot.State == CampaignNativeLoadState::Completed ||
        m_snapshot.State == CampaignNativeLoadState::Failed)
    {
        return false;
    }

    m_snapshot.Failure = aFailure;
    m_snapshot.State = CampaignNativeLoadState::Failed;
    return true;
}

bool CampaignNativeLoadRequest::Reset() noexcept
{
    if (m_snapshot.State != CampaignNativeLoadState::Completed &&
        m_snapshot.State != CampaignNativeLoadState::Failed)
    {
        return false;
    }
    m_snapshot = {};
    return true;
}

bool CampaignNativeLoadRequest::IsActive() const noexcept
{
    return m_snapshot.State != CampaignNativeLoadState::Idle &&
        m_snapshot.State != CampaignNativeLoadState::Completed &&
        m_snapshot.State != CampaignNativeLoadState::Failed;
}

void CampaignNativeLoadRequest::CompleteIfProven() noexcept
{
    if (m_snapshot.NativeReturned && m_snapshot.NativeAccepted &&
        m_snapshot.PostLoadObserved && m_snapshot.GateLocked &&
        m_snapshot.GuardMenuObserved && m_snapshot.GamePaused &&
        m_snapshot.TransportAlive)
    {
        m_snapshot.State = CampaignNativeLoadState::Completed;
    }
}

const char* ToString(CampaignNativeLoadFailure aFailure) noexcept
{
    switch (aFailure)
    {
    case CampaignNativeLoadFailure::None:
        return "none";
    case CampaignNativeLoadFailure::InvalidIdentity:
        return "invalid-identity";
    case CampaignNativeLoadFailure::CampaignAdmissionUnavailable:
        return "campaign-admission-unavailable";
    case CampaignNativeLoadFailure::ArtifactUnavailable:
        return "artifact-unavailable";
    case CampaignNativeLoadFailure::ArtifactInvalid:
        return "artifact-invalid";
    case CampaignNativeLoadFailure::NativeSaveBusy:
        return "native-save-busy";
    case CampaignNativeLoadFailure::ArtifactValidationRejected:
        return "artifact-validation-rejected";
    case CampaignNativeLoadFailure::ArtifactValidationFailed:
        return "artifact-validation-failed";
    case CampaignNativeLoadFailure::GateArmFailed:
        return "gate-arm-failed";
    case CampaignNativeLoadFailure::NativeBoundaryUnavailable:
        return "native-boundary-unavailable";
    case CampaignNativeLoadFailure::UnexpectedNativeLoad:
        return "unexpected-native-load";
    case CampaignNativeLoadFailure::NativeLoadRejected:
        return "native-load-rejected";
    case CampaignNativeLoadFailure::PostLoadMissing:
        return "post-load-missing";
    case CampaignNativeLoadFailure::GateNotLocked:
        return "gate-not-locked";
    case CampaignNativeLoadFailure::GuardMenuUnavailable:
        return "guard-menu-unavailable";
    case CampaignNativeLoadFailure::GameNotPaused:
        return "game-not-paused";
    case CampaignNativeLoadFailure::TransportUnavailable:
        return "transport-unavailable";
    case CampaignNativeLoadFailure::Timeout:
        return "timeout";
    case CampaignNativeLoadFailure::InternalFailure:
        return "internal-failure";
    }
    return "unknown";
}
}
