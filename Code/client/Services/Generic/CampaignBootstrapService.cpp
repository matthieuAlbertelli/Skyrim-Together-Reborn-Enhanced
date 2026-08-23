#include <Services/CampaignBootstrapService.h>

#include <Services/CampaignService.h>
#include <Services/OverlayService.h>
#include <Services/TransportService.h>
#include <Services/UiSurfaceService.h>

#include <Events/CampaignBootstrapAuthorizedEvent.h>
#include <Events/ConnectedEvent.h>
#include <Events/ConnectionErrorEvent.h>
#include <Events/DisconnectedEvent.h>
#include <Events/UpdateEvent.h>

#include <OverlayApp.hpp>

#include <charconv>
#include <string_view>
#include <utility>

namespace
{
constexpr std::uint16_t cDefaultPort = 10578;

const char* ScreenName(
    STRE::Campaign::CampaignBootstrapPhase aPhase) noexcept
{
    using STRE::Campaign::CampaignBootstrapPhase;
    switch (aPhase)
    {
    case CampaignBootstrapPhase::CreateForm:
    case CampaignBootstrapPhase::ConnectingCreate:
    case CampaignBootstrapPhase::CreatingCampaign:
        return "create";
    case CampaignBootstrapPhase::JoinForm:
    case CampaignBootstrapPhase::ConnectingJoin:
    case CampaignBootstrapPhase::JoiningCampaign:
        return "join";
    case CampaignBootstrapPhase::Lobby:
    case CampaignBootstrapPhase::Starting:
        return "lobby";
    case CampaignBootstrapPhase::Error:
        return "error";
    case CampaignBootstrapPhase::Entry:
    default:
        return "entry";
    }
}

void AppendJsonString(std::string& aOutput, std::string_view acValue)
{
    aOutput.push_back('"');
    for (const unsigned char value : acValue)
    {
        switch (value)
        {
        case '"': aOutput += "\\\""; break;
        case '\\': aOutput += "\\\\"; break;
        case '\n': aOutput += "\\n"; break;
        case '\r': aOutput += "\\r"; break;
        case '\t': aOutput += "\\t"; break;
        default:
            if (value >= 0x20)
                aOutput.push_back(static_cast<char>(value));
            break;
        }
    }
    aOutput.push_back('"');
}

bool ParseEndpoint(
    const std::string& acAddress,
    std::string& aEndpoint) noexcept
{
    if (acAddress.empty() || acAddress.size() > 255)
        return false;

    std::string host = acAddress;
    std::uint16_t port = cDefaultPort;
    const std::size_t separator = acAddress.rfind(':');
    if (separator != std::string::npos)
    {
        host = acAddress.substr(0, separator);
        const std::string_view portText(
            acAddress.data() + separator + 1,
            acAddress.size() - separator - 1);
        unsigned parsed{};
        const auto result = std::from_chars(
            portText.data(), portText.data() + portText.size(), parsed);
        if (host.empty() || portText.empty() ||
            result.ec != std::errc{} ||
            result.ptr != portText.data() + portText.size() ||
            parsed == 0 || parsed > 65535)
        {
            return false;
        }
        port = static_cast<std::uint16_t>(parsed);
    }

    if (host == "localhost")
        host = "127.0.0.1";
    if (host.empty())
        return false;
    aEndpoint = host + ":" + std::to_string(port);
    return true;
}

bool Succeeded(CampaignProtocolResult aResult) noexcept
{
    return aResult == CampaignProtocolResult::Applied ||
        aResult == CampaignProtocolResult::AcceptedNoOp ||
        aResult == CampaignProtocolResult::IdempotentReplay;
}

const char* ProtocolErrorCode(CampaignProtocolResult aResult) noexcept
{
    switch (aResult)
    {
    case CampaignProtocolResult::Unauthorized:
        return "unauthorized";
    case CampaignProtocolResult::SessionMismatch:
    case CampaignProtocolResult::PartyAlignmentFailed:
        return "session_unavailable";
    case CampaignProtocolResult::CampaignNotFound:
    case CampaignProtocolResult::JoinCodeUnavailable:
        return "lobby_unavailable";
    case CampaignProtocolResult::RosterSealed:
        return "lobby_closed";
    case CampaignProtocolResult::RosterIncomplete:
        return "member_missing";
    case CampaignProtocolResult::ExistingMembershipRequiresResume:
        return "resume_required";
    case CampaignProtocolResult::InvalidJoinCode:
        return "invalid_code";
    case CampaignProtocolResult::PersistenceFailure:
        return "server_error";
    case CampaignProtocolResult::IdentityMismatch:
    case CampaignProtocolResult::BindingMismatch:
    case CampaignProtocolResult::DuplicateIdentity:
        return "identity_error";
    case CampaignProtocolResult::StaleRevision:
    case CampaignProtocolResult::IdempotencyConflict:
        return "state_changed";
    case CampaignProtocolResult::NotAdmitted:
    case CampaignProtocolResult::RosterNotSealed:
    case CampaignProtocolResult::InvalidRequest:
    default:
        return "request_rejected";
    }
}
}

CampaignBootstrapService::CampaignBootstrapService(
    entt::dispatcher& aDispatcher,
    TransportService& aTransport,
    CampaignService& aCampaignService,
    UiSurfaceService& aUiSurfaceService) noexcept
    : m_dispatcher(aDispatcher)
    , m_transport(aTransport)
    , m_campaignService(aCampaignService)
    , m_uiSurfaceService(aUiSurfaceService)
    , m_updateConnection(aDispatcher.sink<UpdateEvent>()
          .connect<&CampaignBootstrapService::OnUpdate>(this))
    , m_connectedConnection(aDispatcher.sink<ConnectedEvent>()
          .connect<&CampaignBootstrapService::OnConnected>(this))
    , m_disconnectedConnection(aDispatcher.sink<DisconnectedEvent>()
          .connect<&CampaignBootstrapService::OnDisconnected>(this))
    , m_connectionErrorConnection(aDispatcher.sink<ConnectionErrorEvent>()
          .connect<&CampaignBootstrapService::OnConnectionError>(this))
{
}

void CampaignBootstrapService::BeginFreshGame() noexcept
{
    m_state.BeginFreshGame();
    m_joinCode.clear();
    m_displayName.clear();
    m_pendingMutation.clear();
    m_lastOutcomeToken.clear();
    m_errorCode.clear();
    m_lastStateJson.clear();
    m_waitingForResume = false;

    if (m_campaignService.GetAdmission() &&
        m_campaignService.GetLobbyState())
    {
        m_state.RestoreLobby();
    }

    m_uiSurfaceService.SetSurface(UiSurface::CampaignBootstrap);
    spdlog::info(
        "[STRE][CampaignBootstrap] fresh New Game gate opened connected={}",
        m_transport.IsOnline());
    PublishState(true);
}

void CampaignBootstrapService::ChooseSolo() noexcept
{
    spdlog::info(
        "[STRE][CampaignBootstrap] Solo intent phase={}",
        static_cast<unsigned>(m_state.GetPhase()));
    if (m_campaignService.GetAdmission())
    {
        spdlog::warn(
            "[STRE][CampaignBootstrap] Solo rejected: campaign admission exists");
        SetError("campaign_already_admitted");
        return;
    }
    if (m_state.ChooseSolo())
    {
        spdlog::info(
            "[STRE][CampaignBootstrap] Solo state authorized");
        AuthorizeCharacterCreation();
    }
    else
    {
        spdlog::warn(
            "[STRE][CampaignBootstrap] Solo ignored in phase={}",
            static_cast<unsigned>(m_state.GetPhase()));
    }
}

void CampaignBootstrapService::ShowCreate() noexcept
{
    const auto before = m_state.GetPhase();
    m_errorCode.clear();
    m_state.ShowCreateForm();
    spdlog::info(
        "[STRE][CampaignBootstrap] Create form intent phaseBefore={} phaseAfter={}",
        static_cast<unsigned>(before),
        static_cast<unsigned>(m_state.GetPhase()));
    PublishState(true);
}

void CampaignBootstrapService::ShowJoin() noexcept
{
    const auto before = m_state.GetPhase();
    m_errorCode.clear();
    m_state.ShowJoinForm();
    spdlog::info(
        "[STRE][CampaignBootstrap] Join form intent phaseBefore={} phaseAfter={}",
        static_cast<unsigned>(before),
        static_cast<unsigned>(m_state.GetPhase()));
    PublishState(true);
}

void CampaignBootstrapService::Create(
    std::string aAddress,
    std::string aPassword,
    std::string aDisplayName) noexcept
{
    if (m_state.GetPhase() !=
        STRE::Campaign::CampaignBootstrapPhase::CreateForm)
    {
        return;
    }
    TiltedPhoques::String normalizedDisplayName;
    if (!NormalizeCampaignLobbyDisplayName(
            aDisplayName, normalizedDisplayName))
    {
        SetError("invalid_pseudo");
        return;
    }
    m_displayName = normalizedDisplayName.c_str();
    m_errorCode.clear();
    const bool connected = m_transport.IsOnline();
    m_state.BeginCreate(connected);
    if (connected)
        SendCreate();
    else if (!Connect(aAddress, aPassword))
    {
        m_state.Fail();
        SetError("invalid_address");
    }
    PublishState(true);
}

void CampaignBootstrapService::Join(
    std::string aAddress,
    std::string aPassword,
    std::string aJoinCode,
    std::string aDisplayName) noexcept
{
    if (m_state.GetPhase() !=
        STRE::Campaign::CampaignBootstrapPhase::JoinForm)
    {
        return;
    }
    TiltedPhoques::String normalized;
    if (!NormalizeCampaignJoinCode(aJoinCode, normalized))
    {
        SetError("invalid_code");
        return;
    }
    TiltedPhoques::String normalizedDisplayName;
    if (!NormalizeCampaignLobbyDisplayName(
            aDisplayName, normalizedDisplayName))
    {
        SetError("invalid_pseudo");
        return;
    }
    m_joinCode = normalized.c_str();
    m_displayName = normalizedDisplayName.c_str();
    m_errorCode.clear();
    const bool connected = m_transport.IsOnline();
    m_state.BeginJoin(connected);
    if (connected)
        SendJoin();
    else if (!Connect(aAddress, aPassword))
    {
        m_state.Fail();
        SetError("invalid_address");
    }
    PublishState(true);
}

void CampaignBootstrapService::Start() noexcept
{
    const auto& admission = m_campaignService.GetAdmission();
    const auto& snapshot = m_campaignService.GetLatestSnapshot();
    const auto& lobby = m_campaignService.GetLobbyState();
    if (m_state.GetPhase() !=
            STRE::Campaign::CampaignBootstrapPhase::Lobby ||
        !admission || !snapshot || !lobby || !lobby->CanStart)
    {
        return;
    }

    m_state.BeginStart();
    m_errorCode.clear();
    m_pendingMutation = m_campaignService.StartCampaign(
        admission->CampaignId, snapshot->StateVersion);
    if (m_pendingMutation.empty())
    {
        m_state.RejectStart();
        SetError("start_failed");
        return;
    }
    PublishState(true);
}

void CampaignBootstrapService::Back() noexcept
{
    m_errorCode.clear();
    m_state.Back();
    PublishState(true);
}

bool CampaignBootstrapService::Connect(
    const std::string& acAddress,
    const std::string& acPassword) noexcept
{
    if (acPassword.size() > 128)
        return false;
    std::string endpoint;
    if (!ParseEndpoint(acAddress, endpoint))
        return false;
    m_transport.SetServerPassword(acPassword);
    m_transport.Connect(endpoint);
    return true;
}

void CampaignBootstrapService::SendCreate() noexcept
{
    m_pendingMutation =
        m_campaignService.CreateCampaign(m_displayName);
    if (m_pendingMutation.empty())
    {
        m_state.Fail();
        SetError("create_failed");
    }
}

void CampaignBootstrapService::SendJoin() noexcept
{
    m_pendingMutation =
        m_campaignService.JoinCampaignByCode(
            m_joinCode, m_displayName);
    if (m_pendingMutation.empty())
    {
        m_state.Fail();
        SetError("join_failed");
    }
}

void CampaignBootstrapService::OnConnected(const ConnectedEvent&) noexcept
{
    m_state.OnConnected();
    if (m_state.GetPhase() ==
        STRE::Campaign::CampaignBootstrapPhase::CreatingCampaign)
    {
        SendCreate();
    }
    else if (m_state.GetPhase() ==
        STRE::Campaign::CampaignBootstrapPhase::JoiningCampaign)
    {
        SendJoin();
    }
    PublishState(true);
}

void CampaignBootstrapService::OnDisconnected(
    const DisconnectedEvent&) noexcept
{
    if (!m_state.IsActive())
        return;
    m_state.OnDisconnect();
    SetError("connection_lost");
}

void CampaignBootstrapService::OnConnectionError(
    const ConnectionErrorEvent&) noexcept
{
    if (!m_state.IsActive())
        return;
    m_state.Fail();
    SetError("connection_failed");
}

void CampaignBootstrapService::OnUpdate(const UpdateEvent&) noexcept
{
    if (!m_state.IsActive())
        return;
    ProcessCommandOutcome();
    ObserveCanonicalState();
    PublishState();
}

void CampaignBootstrapService::ProcessCommandOutcome() noexcept
{
    const auto& outcome = m_campaignService.GetLastCommandOutcome();
    if (!outcome)
        return;

    const std::string token = fmt::format(
        "{}:{}:{}:{}:{}",
        static_cast<unsigned>(outcome->Operation), outcome->MutationId,
        static_cast<unsigned>(outcome->Result), outcome->CampaignId,
        outcome->StateVersion);
    if (token == m_lastOutcomeToken)
        return;

    const bool isPendingMutation =
        !m_pendingMutation.empty() &&
        outcome->MutationId == m_pendingMutation;
    const bool isPendingResume =
        m_waitingForResume &&
        outcome->Operation == CampaignProtocolOperation::Resume;
    if (!isPendingMutation && !isPendingResume)
        return;
    m_lastOutcomeToken = token;

    if (outcome->Result ==
            CampaignProtocolResult::ExistingMembershipRequiresResume &&
        outcome->Operation == CampaignProtocolOperation::JoinByCode)
    {
        m_waitingForResume =
            m_campaignService.ResumeCampaign(outcome->CampaignId);
        if (!m_waitingForResume)
        {
            m_state.Fail();
            SetError("resume_failed");
        }
        return;
    }

    if (!Succeeded(outcome->Result))
    {
        if (outcome->Operation == CampaignProtocolOperation::Start)
            m_state.RejectStart();
        else
            m_state.Fail();
        SetError(fmt::format(
            "{}", ProtocolErrorCode(outcome->Result)));
        return;
    }

    if (outcome->Operation == CampaignProtocolOperation::Create ||
        outcome->Operation == CampaignProtocolOperation::JoinByCode ||
        outcome->Operation == CampaignProtocolOperation::Resume)
    {
        if (!outcome->JoinCode.empty())
            m_joinCode = outcome->JoinCode;
        m_waitingForResume = false;
        m_state.OnCampaignAdmitted();
        if (outcome->Operation == CampaignProtocolOperation::Resume)
            m_state.RestoreLobby();
    }
}

void CampaignBootstrapService::ObserveCanonicalState() noexcept
{
    const auto& snapshot = m_campaignService.GetLatestSnapshot();
    if (!snapshot)
        return;
    if (m_state.ObserveCanonicalState(
            snapshot->RosterSealed,
            snapshot->Phase == kCampaignWirePhaseCharacterCreation,
            snapshot->RuntimeState == kCampaignWireRuntimeActive))
    {
        AuthorizeCharacterCreation();
    }
}

void CampaignBootstrapService::AuthorizeCharacterCreation() noexcept
{
    m_displayName.clear();
    m_uiSurfaceService.SetSurface(UiSurface::None);
    PublishState(true);
    spdlog::info(
        "[STRE][CampaignBootstrap] emitting CharacterCreation authorization");
    m_dispatcher.trigger(CampaignBootstrapAuthorizedEvent{});
    spdlog::info(
        "[STRE][CampaignBootstrap] CharacterCreation released mode={}",
        m_state.IsMultiplayerCommitted() ? "campaign" : "solo");
}

void CampaignBootstrapService::SetError(std::string aErrorCode) noexcept
{
    m_errorCode = std::move(aErrorCode);
    PublishState(true);
}

void CampaignBootstrapService::PublishState(bool aForce) noexcept
{
    std::string json = "{\"active\":";
    json += m_state.IsActive() ? "true" : "false";
    json += ",\"screen\":";
    AppendJsonString(json, ScreenName(m_state.GetPhase()));
    json += ",\"connected\":";
    json += m_transport.IsOnline() ? "true" : "false";
    json += ",\"busy\":";
    json += m_state.IsBusy() ? "true" : "false";

    const auto& lobby = m_campaignService.GetLobbyState();
    json += ",\"joinCode\":";
    AppendJsonString(json, lobby ? lobby->JoinCode : m_joinCode);
    json += ",\"canStart\":";
    json += lobby && lobby->CanStart ? "true" : "false";
    json += ",\"members\":[";
    if (lobby)
    {
        for (std::size_t index = 0; index < lobby->Members.size(); ++index)
        {
            if (index > 0)
                json.push_back(',');
            json += "{\"name\":";
            AppendJsonString(json, lobby->Members[index].Name);
            json += ",\"present\":";
            json += lobby->Members[index].Present ? "true" : "false";
            json.push_back('}');
        }
    }
    json += "],\"error\":";
    AppendJsonString(json, m_errorCode);
    json.push_back('}');

    if (!aForce && json == m_lastStateJson)
        return;
    m_lastStateJson = json;

    auto* const pApp =
        m_uiSurfaceService.GetOverlayService().GetOverlayApp();
    if (!pApp)
        return;
    auto arguments = CefListValue::Create();
    arguments->SetString(0, json);
    pApp->ExecuteAsync("campaignBootstrapState", arguments);
}
