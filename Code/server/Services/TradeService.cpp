#include <Services/TradeService.h>

#include <GameServer.h>
#include <World.h>

#include <Events/PlayerLeaveEvent.h>
#include <Events/UpdateEvent.h>

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

#include <vector>

namespace
{
constexpr Trade::Tick kInviteLifetimeMs = 30000;
constexpr Trade::Tick kExpirySweepIntervalMs = 1000;

Trade::Offer ToDomainOffer(const TradeOfferData& acOffer)
{
    Trade::Offer result;
    result.Items.reserve(acOffer.Items.size());

    for (const TradeOfferLineData& line : acOffer.Items)
        result.Items.push_back({line.ItemId, line.Quantity});

    return result;
}

TradeOfferData ToNetworkOffer(const Trade::Offer& acOffer)
{
    TradeOfferData result;
    result.Items.reserve(acOffer.Items.size());

    for (const Trade::OfferLine& line : acOffer.Items)
        result.Items.push_back({line.Item, line.Quantity});

    return result;
}
}

TradeService::TradeService(World& aWorld, entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_updateConnection(aDispatcher.sink<UpdateEvent>().connect<&TradeService::OnUpdate>(this))
    , m_playerLeaveConnection(aDispatcher.sink<PlayerLeaveEvent>().connect<&TradeService::OnPlayerLeave>(this))
    , m_tradeInviteConnection(aDispatcher.sink<PacketEvent<TradeInviteRequest>>().connect<&TradeService::OnTradeInviteRequest>(this))
    , m_tradeInviteResponseConnection(aDispatcher.sink<PacketEvent<TradeInviteResponseRequest>>().connect<&TradeService::OnTradeInviteResponseRequest>(this))
    , m_tradeOfferUpdateConnection(aDispatcher.sink<PacketEvent<TradeOfferUpdateRequest>>().connect<&TradeService::OnTradeOfferUpdateRequest>(this))
    , m_tradeConfirmConnection(aDispatcher.sink<PacketEvent<TradeConfirmRequest>>().connect<&TradeService::OnTradeConfirmRequest>(this))
    , m_tradeCancelConnection(aDispatcher.sink<PacketEvent<TradeCancelRequest>>().connect<&TradeService::OnTradeCancelRequest>(this))
{
}

void TradeService::OnTradeInviteRequest(const PacketEvent<TradeInviteRequest>& acPacket) noexcept
{
    Player* const pInviter = acPacket.pPlayer;
    if (!pInviter)
        return;

    const Trade::PlayerId inviterId = pInviter->GetId();
    const Trade::PlayerId inviteeId = acPacket.Packet.TargetPlayerId;
    Player* const pInvitee = m_world.GetPlayerManager().GetById(inviteeId);

    spdlog::info("[trade][player={}] invite_requested target={}", inviterId, inviteeId);

    if (!pInvitee)
    {
        spdlog::warn("[trade][player={}] invite_rejected reason=target_not_found target={}", inviterId, inviteeId);
        return;
    }

    if (pInvitee == pInviter)
    {
        spdlog::warn("[trade][player={}] invite_rejected reason=self_trade", inviterId);
        return;
    }

    const auto existingInviterSession = m_playerSessions.find(inviterId);
    if (existingInviterSession != m_playerSessions.end())
    {
        const Trade::Session* const pSession = FindSession(existingInviterSession->second);
        if (pSession &&
            pSession->GetState() == Trade::State::PendingAcceptance &&
            pSession->GetInitiator().Id == inviterId &&
            pSession->GetRecipient().Id == inviteeId)
        {
            SendInvite(*pInvitee, *pSession);
            spdlog::info(
                "[trade={}][revision={}] invite_retransmitted initiator={} recipient={}",
                pSession->GetId(),
                pSession->GetRevision(),
                inviterId,
                inviteeId);
        }
        else
        {
            spdlog::warn("[trade][player={}] invite_rejected reason=initiator_busy", inviterId);
        }

        return;
    }

    if (IsPlayerInSession(inviteeId))
    {
        spdlog::warn(
            "[trade][player={}] invite_rejected reason=recipient_busy target={}",
            inviterId,
            inviteeId);
        return;
    }

    const Trade::Tick currentTick = GameServer::Get()->GetTick();
    const Trade::Tick expiryTick = currentTick + kInviteLifetimeMs;
    const Trade::SessionId sessionId = AllocateSessionId();

    auto [sessionIt, inserted] = m_sessions.try_emplace(
        sessionId,
        sessionId,
        inviterId,
        inviteeId,
        currentTick,
        expiryTick);

    if (!inserted)
    {
        spdlog::error("[trade={}] session_creation_failed reason=id_collision", sessionId);
        return;
    }

    const auto [inviterIndexIt, inviterIndexed] = m_playerSessions.emplace(inviterId, sessionId);
    const auto [inviteeIndexIt, inviteeIndexed] = m_playerSessions.emplace(inviteeId, sessionId);

    if (!inviterIndexed || !inviteeIndexed)
    {
        if (inviterIndexed)
            m_playerSessions.erase(inviterIndexIt);

        if (inviteeIndexed)
            m_playerSessions.erase(inviteeIndexIt);

        m_sessions.erase(sessionIt);
        spdlog::error("[trade={}] session_creation_failed reason=player_index_conflict", sessionId);
        return;
    }

    SendInvite(*pInvitee, sessionIt->second);

    spdlog::info(
        "[trade={}][revision={}] invite_sent initiator={} recipient={} expiry_tick={}",
        sessionId,
        sessionIt->second.GetRevision(),
        inviterId,
        inviteeId,
        expiryTick);
}

void TradeService::OnTradeInviteResponseRequest(const PacketEvent<TradeInviteResponseRequest>& acPacket) noexcept
{
    Player* const pResponder = acPacket.pPlayer;
    if (!pResponder)
        return;

    const TradeInviteResponseRequest& message = acPacket.Packet;
    const Trade::PlayerId responderId = pResponder->GetId();

    Trade::Session* const pSession = FindSession(message.SessionId);
    if (!pSession)
    {
        spdlog::warn(
            "[trade={}][player={}] invite_response_rejected reason=session_not_found",
            message.SessionId,
            responderId);
        return;
    }

    if (!IsIndexedParticipant(responderId, message.SessionId))
    {
        spdlog::warn(
            "[trade={}][player={}] invite_response_rejected reason=not_indexed_participant",
            message.SessionId,
            responderId);
        return;
    }

    Trade::Session& session = *pSession;
    const Trade::Tick currentTick = GameServer::Get()->GetTick();

    if (session.GetState() == Trade::State::PendingAcceptance &&
        currentTick >= session.GetExpiryTick())
    {
        session.Expire(currentTick);
        SendCancelled(session, TradeCancelReason::Expired, 0);

        spdlog::info(
            "[trade={}][revision={}] cancelled reason=expired",
            session.GetId(),
            session.GetRevision());

        RemoveSession(session.GetId());
        return;
    }

    if (!message.Accepted)
    {
        const Trade::Result result = session.Reject(responderId, currentTick);
        if (!result.Succeeded())
        {
            spdlog::warn(
                "[trade={}][revision={}][player={}] invite_reject_failed error={}",
                session.GetId(),
                session.GetRevision(),
                responderId,
                static_cast<std::uint32_t>(result.ErrorCode));
            return;
        }

        SendCancelled(session, TradeCancelReason::Rejected, responderId);
        spdlog::info(
            "[trade={}][revision={}][player={}] cancelled reason=rejected",
            session.GetId(),
            session.GetRevision(),
            responderId);
        RemoveSession(session.GetId());
        return;
    }

    const Trade::Result result = session.Accept(responderId, currentTick);
    if (!result.Succeeded())
    {
        spdlog::warn(
            "[trade={}][revision={}][player={}] invite_accept_failed error={}",
            session.GetId(),
            session.GetRevision(),
            responderId,
            static_cast<std::uint32_t>(result.ErrorCode));
        return;
    }

    Player* const pInitiator = m_world.GetPlayerManager().GetById(session.GetInitiator().Id);
    Player* const pRecipient = m_world.GetPlayerManager().GetById(session.GetRecipient().Id);

    if (!pInitiator || !pRecipient)
    {
        session.Cancel(responderId, currentTick);
        SendCancelled(session, TradeCancelReason::ParticipantUnavailable, responderId);
        spdlog::warn(
            "[trade={}][revision={}] cancelled reason=participant_unavailable",
            session.GetId(),
            session.GetRevision());
        RemoveSession(session.GetId());
        return;
    }

    SendStarted(session);
    SendState(session);

    spdlog::info(
        "[trade={}][revision={}] started initiator={} recipient={} retransmission={}",
        session.GetId(),
        session.GetRevision(),
        session.GetInitiator().Id,
        session.GetRecipient().Id,
        result.Changed ? 0 : 1);
}

void TradeService::OnTradeOfferUpdateRequest(const PacketEvent<TradeOfferUpdateRequest>& acPacket) noexcept
{
    Player* const pPlayer = acPacket.pPlayer;
    if (!pPlayer)
        return;

    const TradeOfferUpdateRequest& message = acPacket.Packet;
    const Trade::PlayerId playerId = pPlayer->GetId();
    Trade::Session* const pSession = FindSession(message.SessionId);

    if (!pSession || !IsIndexedParticipant(playerId, message.SessionId))
    {
        spdlog::warn(
            "[trade={}][revision={}][player={}] offer_update_rejected reason=session_or_participant_invalid",
            message.SessionId,
            message.ExpectedRevision,
            playerId);
        return;
    }

    if (!message.Offer.Valid)
    {
        spdlog::warn(
            "[trade={}][revision={}][player={}] offer_update_rejected reason=malformed_offer",
            message.SessionId,
            message.ExpectedRevision,
            playerId);
        SendState(*pSession, pPlayer);
        return;
    }

    Trade::Session& session = *pSession;
    const Trade::Result result = session.UpdateOffer(
        playerId,
        message.ExpectedRevision,
        ToDomainOffer(message.Offer),
        GameServer::Get()->GetTick());

    if (!result.Succeeded())
    {
        spdlog::warn(
            "[trade={}][revision={}][player={}] offer_update_rejected error={}",
            session.GetId(),
            session.GetRevision(),
            playerId,
            static_cast<std::uint32_t>(result.ErrorCode));
        SendState(session, pPlayer);
        return;
    }

    spdlog::info(
        "[trade={}][revision={}][player={}] offer_updated lines={} changed={}",
        session.GetId(),
        session.GetRevision(),
        playerId,
        message.Offer.Items.size(),
        result.Changed ? 1 : 0);

    SendState(session, result.Changed ? nullptr : pPlayer);
}

void TradeService::OnTradeConfirmRequest(const PacketEvent<TradeConfirmRequest>& acPacket) noexcept
{
    Player* const pPlayer = acPacket.pPlayer;
    if (!pPlayer)
        return;

    const TradeConfirmRequest& message = acPacket.Packet;
    const Trade::PlayerId playerId = pPlayer->GetId();
    Trade::Session* const pSession = FindSession(message.SessionId);

    if (!pSession || !IsIndexedParticipant(playerId, message.SessionId))
    {
        spdlog::warn(
            "[trade={}][revision={}][player={}] confirm_rejected reason=session_or_participant_invalid",
            message.SessionId,
            message.Revision,
            playerId);
        return;
    }

    Trade::Session& session = *pSession;
    const Trade::Result result = session.Confirm(
        playerId,
        message.Revision,
        GameServer::Get()->GetTick());

    if (!result.Succeeded())
    {
        spdlog::warn(
            "[trade={}][revision={}][player={}] confirm_rejected error={}",
            session.GetId(),
            session.GetRevision(),
            playerId,
            static_cast<std::uint32_t>(result.ErrorCode));
        SendState(session, pPlayer);
        return;
    }

    spdlog::info(
        "[trade={}][revision={}][player={}] confirmed state={} changed={}",
        session.GetId(),
        session.GetRevision(),
        playerId,
        static_cast<std::uint32_t>(session.GetState()),
        result.Changed ? 1 : 0);

    SendState(session, result.Changed ? nullptr : pPlayer);
}

void TradeService::OnTradeCancelRequest(const PacketEvent<TradeCancelRequest>& acPacket) noexcept
{
    Player* const pPlayer = acPacket.pPlayer;
    if (!pPlayer)
        return;

    const TradeCancelRequest& message = acPacket.Packet;
    const Trade::PlayerId playerId = pPlayer->GetId();
    Trade::Session* const pSession = FindSession(message.SessionId);

    if (!pSession || !IsIndexedParticipant(playerId, message.SessionId))
    {
        spdlog::warn(
            "[trade={}][player={}] cancel_rejected reason=session_or_participant_invalid",
            message.SessionId,
            playerId);
        return;
    }

    Trade::Session& session = *pSession;
    const Trade::Result result = session.Cancel(playerId, GameServer::Get()->GetTick());

    if (!result.Succeeded())
    {
        spdlog::warn(
            "[trade={}][revision={}][player={}] cancel_rejected error={}",
            session.GetId(),
            session.GetRevision(),
            playerId,
            static_cast<std::uint32_t>(result.ErrorCode));
        SendState(session, pPlayer);
        return;
    }

    SendCancelled(session, TradeCancelReason::CancelledByParticipant, playerId);

    spdlog::info(
        "[trade={}][revision={}][player={}] cancelled reason=participant_request changed={}",
        session.GetId(),
        session.GetRevision(),
        playerId,
        result.Changed ? 1 : 0);

    RemoveSession(session.GetId());
}

void TradeService::OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept
{
    Player* const pPlayer = acEvent.pPlayer;
    if (!pPlayer)
        return;

    const Trade::PlayerId playerId = pPlayer->GetId();
    const auto playerSessionIt = m_playerSessions.find(playerId);
    if (playerSessionIt == m_playerSessions.end())
        return;

    const Trade::SessionId sessionId = playerSessionIt->second;
    Trade::Session* const pSession = FindSession(sessionId);
    if (!pSession)
    {
        m_playerSessions.erase(playerSessionIt);
        return;
    }

    const Trade::Tick currentTick = GameServer::Get()->GetTick();
    pSession->Cancel(playerId, currentTick);
    SendCancelled(*pSession, TradeCancelReason::PlayerDisconnected, playerId, pPlayer);

    spdlog::info(
        "[trade={}][revision={}][player={}] cancelled reason=player_disconnected",
        pSession->GetId(),
        pSession->GetRevision(),
        playerId);

    RemoveSession(sessionId);
}

void TradeService::OnUpdate(const UpdateEvent& acEvent) noexcept
{
    TP_UNUSED(acEvent);

    const Trade::Tick currentTick = GameServer::Get()->GetTick();
    if (currentTick < m_nextExpirySweepTick)
        return;

    m_nextExpirySweepTick = currentTick + kExpirySweepIntervalMs;

    std::vector<Trade::SessionId> expiredSessions;
    expiredSessions.reserve(m_sessions.size());

    for (const auto& [sessionId, session] : m_sessions)
    {
        if (session.GetState() == Trade::State::PendingAcceptance &&
            currentTick >= session.GetExpiryTick())
        {
            expiredSessions.push_back(sessionId);
        }
    }

    for (const Trade::SessionId sessionId : expiredSessions)
    {
        Trade::Session* const pSession = FindSession(sessionId);
        if (!pSession)
            continue;

        const Trade::Result result = pSession->Expire(currentTick);
        if (result.Changed)
        {
            SendCancelled(*pSession, TradeCancelReason::Expired, 0);
            spdlog::info(
                "[trade={}][revision={}] cancelled reason=expired",
                pSession->GetId(),
                pSession->GetRevision());
        }

        RemoveSession(sessionId);
    }
}

Trade::SessionId TradeService::AllocateSessionId() noexcept
{
    for (;;)
    {
        const Trade::SessionId candidate = m_nextSessionId++;

        if (m_nextSessionId == 0)
            m_nextSessionId = 1;

        if (candidate != 0 && m_sessions.find(candidate) == m_sessions.end())
            return candidate;
    }
}

Trade::Session* TradeService::FindSession(Trade::SessionId aSessionId) noexcept
{
    const auto sessionIt = m_sessions.find(aSessionId);
    return sessionIt != m_sessions.end() ? &sessionIt->second : nullptr;
}

const Trade::Session* TradeService::FindSession(Trade::SessionId aSessionId) const noexcept
{
    const auto sessionIt = m_sessions.find(aSessionId);
    return sessionIt != m_sessions.end() ? &sessionIt->second : nullptr;
}

bool TradeService::IsPlayerInSession(Trade::PlayerId aPlayerId) const noexcept
{
    return m_playerSessions.find(aPlayerId) != m_playerSessions.end();
}

bool TradeService::IsIndexedParticipant(
    Trade::PlayerId aPlayerId,
    Trade::SessionId aSessionId) const noexcept
{
    const auto playerSessionIt = m_playerSessions.find(aPlayerId);
    return playerSessionIt != m_playerSessions.end() &&
           playerSessionIt->second == aSessionId;
}

void TradeService::SendInvite(Player& aInvitee, const Trade::Session& acSession) const noexcept
{
    NotifyTradeInvite notify;
    notify.SessionId = acSession.GetId();
    notify.InviterId = acSession.GetInitiator().Id;
    notify.ExpiryTick = acSession.GetExpiryTick();
    aInvitee.Send(notify);
}

void TradeService::SendStarted(const Trade::Session& acSession) const noexcept
{
    NotifyTradeStarted notify;
    notify.SessionId = acSession.GetId();
    notify.InitiatorId = acSession.GetInitiator().Id;
    notify.RecipientId = acSession.GetRecipient().Id;
    notify.Revision = acSession.GetRevision();

    Player* const pInitiator = m_world.GetPlayerManager().GetById(notify.InitiatorId);
    Player* const pRecipient = m_world.GetPlayerManager().GetById(notify.RecipientId);

    if (pInitiator)
        pInitiator->Send(notify);

    if (pRecipient)
        pRecipient->Send(notify);
}

void TradeService::SendState(const Trade::Session& acSession, Player* apRecipient) const noexcept
{
    NotifyTradeState notify;
    notify.SessionId = acSession.GetId();
    notify.Revision = acSession.GetRevision();
    notify.State = static_cast<std::uint8_t>(acSession.GetState());

    notify.InitiatorId = acSession.GetInitiator().Id;
    notify.RecipientId = acSession.GetRecipient().Id;

    notify.InitiatorOffer = ToNetworkOffer(acSession.GetInitiator().CurrentOffer);
    notify.RecipientOffer = ToNetworkOffer(acSession.GetRecipient().CurrentOffer);

    notify.InitiatorConfirmed = acSession.GetInitiator().Confirmed;
    notify.RecipientConfirmed = acSession.GetRecipient().Confirmed;
    notify.InitiatorConfirmedRevision = acSession.GetInitiator().ConfirmedRevision;
    notify.RecipientConfirmedRevision = acSession.GetRecipient().ConfirmedRevision;

    if (apRecipient)
    {
        apRecipient->Send(notify);
        return;
    }

    Player* const pInitiator = m_world.GetPlayerManager().GetById(notify.InitiatorId);
    Player* const pRecipient = m_world.GetPlayerManager().GetById(notify.RecipientId);

    if (pInitiator)
        pInitiator->Send(notify);

    if (pRecipient)
        pRecipient->Send(notify);
}

void TradeService::SendCancelled(
    const Trade::Session& acSession,
    TradeCancelReason aReason,
    Trade::PlayerId aActorId,
    Player* apIgnoredPlayer) const noexcept
{
    NotifyTradeCancelled notify;
    notify.SessionId = acSession.GetId();
    notify.ActorId = aActorId;
    notify.Reason = aReason;

    Player* const pInitiator = m_world.GetPlayerManager().GetById(acSession.GetInitiator().Id);
    Player* const pRecipient = m_world.GetPlayerManager().GetById(acSession.GetRecipient().Id);

    if (pInitiator && pInitiator != apIgnoredPlayer)
        pInitiator->Send(notify);

    if (pRecipient && pRecipient != apIgnoredPlayer)
        pRecipient->Send(notify);
}

void TradeService::RemoveSession(Trade::SessionId aSessionId) noexcept
{
    const auto sessionIt = m_sessions.find(aSessionId);
    if (sessionIt == m_sessions.end())
        return;

    m_playerSessions.erase(sessionIt->second.GetInitiator().Id);
    m_playerSessions.erase(sessionIt->second.GetRecipient().Id);
    m_sessions.erase(sessionIt);
}
