#include <Services/TradeService.h>

#include <Services/TransportService.h>

#include <Messages/NotifyTradeCancelled.h>
#include <Messages/NotifyTradeInvite.h>
#include <Messages/NotifyTradeStarted.h>
#include <Messages/NotifyTradeState.h>
#include <Messages/TradeCancelRequest.h>
#include <Messages/TradeConfirmRequest.h>
#include <Messages/TradeInviteRequest.h>
#include <Messages/TradeInviteResponseRequest.h>
#include <Messages/TradeOfferUpdateRequest.h>

#include <Structs/TradeOffer.h>

#include <World.h>

#include <utility>

namespace
{
TradeOfferData ToNetworkOffer(const Trade::Offer& acOffer)
{
    TradeOfferData result;
    result.Items.reserve(acOffer.Items.size());

    for (const Trade::OfferLine& line : acOffer.Items)
        result.Items.push_back({line.Item, line.Quantity});

    return result;
}

Trade::Offer ToDomainOffer(const TradeOfferData& acOffer)
{
    Trade::Offer result;
    result.Items.reserve(acOffer.Items.size());

    for (const TradeOfferLineData& line : acOffer.Items)
        result.Items.push_back({line.ItemId, line.Quantity});

    return result;
}
}

TradeService::TradeService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept
    : m_world(aWorld)
    , m_transport(aTransportService)
    , m_tradeInviteConnection(aDispatcher.sink<NotifyTradeInvite>().connect<&TradeService::OnNotifyTradeInvite>(this))
    , m_tradeStartedConnection(aDispatcher.sink<NotifyTradeStarted>().connect<&TradeService::OnNotifyTradeStarted>(this))
    , m_tradeCancelledConnection(aDispatcher.sink<NotifyTradeCancelled>().connect<&TradeService::OnNotifyTradeCancelled>(this))
    , m_tradeStateConnection(aDispatcher.sink<NotifyTradeState>().connect<&TradeService::OnNotifyTradeState>(this))
{
}

void TradeService::InvitePlayer(std::uint32_t aPlayerId) const noexcept
{
    if (!m_transport.IsConnected())
    {
        spdlog::warn("[TradeService]: cannot send trade invite, transport is not connected");
        return;
    }

    TradeInviteRequest request;
    request.TargetPlayerId = aPlayerId;

    if (m_transport.Send(request))
        spdlog::info("[TradeService]: sent trade invite request to player {}", aPlayerId);
    else
        spdlog::warn("[TradeService]: failed to send trade invite request to player {}", aPlayerId);
}

void TradeService::AcceptInvite(std::uint64_t aSessionId) const noexcept
{
    RespondToInvite(aSessionId, true);
}

void TradeService::RejectInvite(std::uint64_t aSessionId) const noexcept
{
    RespondToInvite(aSessionId, false);
}

void TradeService::UpdateOffer(Trade::Offer aOffer) const noexcept
{
    if (!m_transport.IsConnected() || !m_sessionState)
    {
        spdlog::warn("[TradeService]: cannot update offer without an active synchronized trade");
        return;
    }

    if (m_sessionState->State != Trade::State::Negotiating)
    {
        spdlog::warn(
            "[TradeService]: cannot update offer for session {} in state {}",
            m_sessionState->SessionId,
            static_cast<std::uint32_t>(m_sessionState->State));
        return;
    }

    TradeOfferUpdateRequest request;
    request.SessionId = m_sessionState->SessionId;
    request.ExpectedRevision = m_sessionState->Revision;
    request.Offer = ToNetworkOffer(aOffer);

    if (!m_transport.Send(request))
    {
        spdlog::warn(
            "[TradeService]: failed to send offer update for session {} revision {}",
            request.SessionId,
            request.ExpectedRevision);
    }
}

void TradeService::ConfirmTrade() const noexcept
{
    if (!m_transport.IsConnected() || !m_sessionState)
    {
        spdlog::warn("[TradeService]: cannot confirm without an active synchronized trade");
        return;
    }

    TradeConfirmRequest request;
    request.SessionId = m_sessionState->SessionId;
    request.Revision = m_sessionState->Revision;

    if (!m_transport.Send(request))
    {
        spdlog::warn(
            "[TradeService]: failed to confirm session {} revision {}",
            request.SessionId,
            request.Revision);
    }
}

void TradeService::CancelTrade() const noexcept
{
    if (!m_transport.IsConnected() || !m_activeSession)
    {
        spdlog::warn("[TradeService]: cannot cancel without an active trade");
        return;
    }

    TradeCancelRequest request;
    request.SessionId = *m_activeSession;

    if (!m_transport.Send(request))
    {
        spdlog::warn(
            "[TradeService]: failed to cancel session {}",
            request.SessionId);
    }
}

void TradeService::RespondToInvite(std::uint64_t aSessionId, bool aAccepted) const noexcept
{
    if (!m_transport.IsConnected())
    {
        spdlog::warn("[TradeService]: cannot respond to trade invite, transport is not connected");
        return;
    }

    if (!m_pendingInvite || *m_pendingInvite != aSessionId)
    {
        spdlog::warn(
            "[TradeService]: cannot respond to unknown trade invite session {}",
            aSessionId);
        return;
    }

    TradeInviteResponseRequest request;
    request.SessionId = aSessionId;
    request.Accepted = aAccepted;

    if (m_transport.Send(request))
    {
        spdlog::info(
            "[TradeService]: sent trade invite response session {} accepted={}",
            aSessionId,
            aAccepted);
    }
    else
    {
        spdlog::warn(
            "[TradeService]: failed to send trade invite response session {} accepted={}",
            aSessionId,
            aAccepted);
    }
}

void TradeService::OnNotifyTradeInvite(const NotifyTradeInvite& acTradeInvite) noexcept
{
    m_pendingInvite = acTradeInvite.SessionId;

    spdlog::info(
        "[TradeService]: received trade invite session {} from player {}, expires at tick {}",
        acTradeInvite.SessionId,
        acTradeInvite.InviterId,
        acTradeInvite.ExpiryTick);
}

void TradeService::OnNotifyTradeStarted(const NotifyTradeStarted& acTradeStarted) noexcept
{
    if (m_pendingInvite && *m_pendingInvite == acTradeStarted.SessionId)
        m_pendingInvite.reset();

    m_activeSession = acTradeStarted.SessionId;

    spdlog::info(
        "[TradeService]: trade session {} started between players {} and {} at revision {}",
        acTradeStarted.SessionId,
        acTradeStarted.InitiatorId,
        acTradeStarted.RecipientId,
        acTradeStarted.Revision);
}

void TradeService::OnNotifyTradeCancelled(const NotifyTradeCancelled& acTradeCancelled) noexcept
{
    if (m_pendingInvite && *m_pendingInvite == acTradeCancelled.SessionId)
        m_pendingInvite.reset();

    if (m_activeSession && *m_activeSession == acTradeCancelled.SessionId)
        m_activeSession.reset();

    if (m_sessionState && m_sessionState->SessionId == acTradeCancelled.SessionId)
        m_sessionState.reset();

    spdlog::info(
        "[TradeService]: trade session {} cancelled reason={} actor={}",
        acTradeCancelled.SessionId,
        static_cast<std::uint32_t>(acTradeCancelled.Reason),
        acTradeCancelled.ActorId);
}

void TradeService::OnNotifyTradeState(const NotifyTradeState& acTradeState) noexcept
{
    if (!acTradeState.InitiatorOffer.Valid || !acTradeState.RecipientOffer.Valid)
    {
        spdlog::error(
            "[TradeService]: rejected malformed state snapshot for session {}",
            acTradeState.SessionId);
        return;
    }

    if (acTradeState.State > static_cast<std::uint8_t>(Trade::State::Failed))
    {
        spdlog::error(
            "[TradeService]: rejected state snapshot with invalid state {} for session {}",
            acTradeState.State,
            acTradeState.SessionId);
        return;
    }

    ClientTradeSessionState state;
    if (acTradeState.TerminalError >
        static_cast<std::uint8_t>(Trade::Error::QuantityOverflow))
    {
        spdlog::error(
            "[TradeService]: rejected state snapshot with invalid terminal error {} for session {}",
            acTradeState.TerminalError,
            acTradeState.SessionId);
        return;
    }

    state.SessionId = acTradeState.SessionId;
    state.Revision = acTradeState.Revision;
    state.State = static_cast<Trade::State>(acTradeState.State);
    state.TerminalError = static_cast<Trade::Error>(acTradeState.TerminalError);

    state.Initiator.Id = acTradeState.InitiatorId;
    state.Initiator.CurrentOffer = ToDomainOffer(acTradeState.InitiatorOffer);
    state.Initiator.Confirmed = acTradeState.InitiatorConfirmed;
    state.Initiator.ConfirmedRevision = acTradeState.InitiatorConfirmedRevision;

    state.Recipient.Id = acTradeState.RecipientId;
    state.Recipient.CurrentOffer = ToDomainOffer(acTradeState.RecipientOffer);
    state.Recipient.Confirmed = acTradeState.RecipientConfirmed;
    state.Recipient.ConfirmedRevision = acTradeState.RecipientConfirmedRevision;

    const bool terminal =
        state.State == Trade::State::Completed ||
        state.State == Trade::State::Cancelled ||
        state.State == Trade::State::Failed;

    if (terminal)
        m_activeSession.reset();
    else
        m_activeSession = state.SessionId;

    m_sessionState = std::move(state);

    spdlog::info(
        "[TradeService]: synchronized session {} revision {} state={} terminal_error={} initiator_confirmed={} recipient_confirmed={}",
        acTradeState.SessionId,
        acTradeState.Revision,
        acTradeState.State,
        acTradeState.TerminalError,
        acTradeState.InitiatorConfirmed,
        acTradeState.RecipientConfirmed);
}
