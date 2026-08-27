#include <CampaignClientAdmissionState.h>

#include <utility>

namespace STRE::Campaign
{
void CampaignClientAdmissionState::Accept(
    CampaignClientAdmission aAdmission) noexcept
{
    std::scoped_lock lock(m_mutex);
    m_resumeCandidate = aAdmission.CampaignId;
    m_admission = std::move(aAdmission);
    m_resumeInFlight = false;
}

std::optional<CampaignClientAdmission>
CampaignClientAdmissionState::GetAdmission() const noexcept
{
    std::scoped_lock lock(m_mutex);
    return m_admission;
}

void CampaignClientAdmissionState::ObserveSnapshot(
    std::string_view acCampaignId,
    bool aRosterSealed,
    bool aRuntimeActive,
    std::size_t aRosterSize,
    std::size_t aPresentCount) noexcept
{
    std::scoped_lock lock(m_mutex);
    m_runtimeProjection = RuntimeProjection{
        std::string(acCampaignId), aRosterSealed, aRuntimeActive,
        aRosterSize, aPresentCount};
}

CampaignHelgenReadinessView
CampaignClientAdmissionState::GetHelgenReadinessView() const noexcept
{
    std::scoped_lock lock(m_mutex);
    const bool canSignal = m_admission && m_runtimeProjection &&
        m_admission->CampaignId == m_runtimeProjection->CampaignId &&
        m_runtimeProjection->RosterSealed &&
        m_runtimeProjection->RuntimeActive &&
        m_runtimeProjection->RosterSize > 0 &&
        m_runtimeProjection->PresentCount == m_runtimeProjection->RosterSize;
    return CampaignHelgenReadinessView{m_admission, canSignal};
}

std::optional<std::string> CampaignClientAdmissionState::Disconnect() noexcept
{
    std::scoped_lock lock(m_mutex);
    return ClearRuntimeSession(true);
}

std::optional<std::string>
CampaignClientAdmissionState::EndRuntimeSession() noexcept
{
    std::scoped_lock lock(m_mutex);
    return ClearRuntimeSession(false);
}

std::optional<std::string>
CampaignClientAdmissionState::ClearRuntimeSession(
    bool aKeepReconnectCandidate) noexcept
{
    if (m_admission)
        m_resumeCandidate = m_admission->CampaignId;
    const std::optional<std::string> campaign = m_resumeCandidate;
    if (!aKeepReconnectCandidate)
        m_resumeCandidate.reset();
    m_admission.reset();
    m_runtimeProjection.reset();
    m_resumeInFlight = false;
    return campaign;
}

std::optional<std::string> CampaignClientAdmissionState::BeginResume() noexcept
{
    std::scoped_lock lock(m_mutex);
    if (m_admission || !m_resumeCandidate || m_resumeInFlight)
        return std::nullopt;
    m_resumeInFlight = true;
    return m_resumeCandidate;
}

void CampaignClientAdmissionState::ResumeRejected() noexcept
{
    std::scoped_lock lock(m_mutex);
    m_resumeInFlight = false;
}

void CampaignClientAdmissionState::Leave(
    std::string_view acCampaignId) noexcept
{
    std::scoped_lock lock(m_mutex);
    if (m_admission && m_admission->CampaignId == acCampaignId)
        m_admission.reset();
    if (m_resumeCandidate && *m_resumeCandidate == acCampaignId)
        m_resumeCandidate.reset();
    if (m_runtimeProjection &&
        m_runtimeProjection->CampaignId == acCampaignId)
    {
        m_runtimeProjection.reset();
    }
    m_resumeInFlight = false;
}
}
