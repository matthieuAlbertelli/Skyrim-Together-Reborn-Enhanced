#include <Services/TradeService.h>

#include <GameServer.h>
#include <World.h>

#include <Components.h>
#include <Game/Player.h>

#include <Events/PlayerLeaveEvent.h>
#include <Events/UpdateEvent.h>

#include <Messages/NotifyTradeCancelled.h>
#include <Messages/NotifyTradeInvite.h>
#include <Messages/NotifyTradeStarted.h>
#include <Messages/NotifyTradeState.h>
#include <Messages/NotifyTradeApply.h>
#include <Messages/NotifyTradeReconcile.h>
#include <Messages/TradeCancelRequest.h>
#include <Messages/TradeConfirmRequest.h>
#include <Messages/TradeInviteRequest.h>
#include <Messages/TradeInviteResponseRequest.h>
#include <Messages/TradeOfferUpdateRequest.h>
#include <Messages/TradeApplyResultRequest.h>
#include <Messages/TradeReconcileResultRequest.h>

#include <Structs/TradeOffer.h>
#include <Structs/TradeApplication.h>
#include <Structs/TradeReconciliation.h>

#include <algorithm>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
constexpr Trade::Tick kInviteLifetimeMs = 30000;
constexpr Trade::Tick kExpirySweepIntervalMs = 1000;
constexpr Trade::Tick kApplyTimeoutMs = 15000;
constexpr Trade::Tick kTerminalRetentionMs = 5000;
constexpr Trade::Tick kReconciliationTimeoutMs = 30000;

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

Trade::ItemId ToTradeItemId(const GameId& acGameId) noexcept
{
    return (static_cast<Trade::ItemId>(acGameId.ModId) << 32) |
           static_cast<Trade::ItemId>(acGameId.BaseId);
}

bool IsMvpTransferable(const Inventory::Entry& acEntry) noexcept
{
    return acEntry.Count > 0 &&
           !acEntry.IsQuestItem &&
           !acEntry.IsWorn() &&
           acEntry.ExtraCharge == 0.0f &&
           !acEntry.ExtraEnchantId &&
           acEntry.ExtraEnchantCharge == 0 &&
           acEntry.EnchantData.Effects.empty() &&
           !acEntry.EnchantData.IsWeapon &&
           acEntry.ExtraHealth == 0.0f &&
           !acEntry.ExtraPoisonId &&
           acEntry.ExtraPoisonCount == 0 &&
           acEntry.ExtraSoulLevel == 0 &&
           !acEntry.ExtraEnchantRemoveUnequip;
}

GameId ToGameId(Trade::ItemId aItemId) noexcept
{
    return GameId{
        static_cast<std::uint32_t>(aItemId >> 32),
        static_cast<std::uint32_t>(aItemId & 0xFFFFFFFFu)};
}

Trade::ApplyOutcome ToApplyOutcome(TradeApplyResultCode aResult) noexcept
{
    return static_cast<Trade::ApplyOutcome>(
        static_cast<std::uint8_t>(aResult));
}

TradeMutationPlanData ToNetworkPlan(
    const Trade::PlayerMutationPlan& acPlan)
{
    TradeMutationPlanData result;
    result.Mutations.reserve(acPlan.Mutations.size());

    for (const Trade::InventoryMutation& mutation : acPlan.Mutations)
        result.Mutations.push_back({mutation.Item, mutation.Delta});

    return result;
}

Trade::ReconcileOutcome ToReconcileOutcome(
    TradeReconcileResultCode aResult) noexcept
{
    return static_cast<Trade::ReconcileOutcome>(
        static_cast<std::uint8_t>(aResult));
}

TradeReconciliationPlanData ToNetworkPlan(
    const Trade::PlayerReconciliationPlan& acPlan)
{
    TradeReconciliationPlanData result;
    result.Targets.reserve(acPlan.Targets.size());

    for (const Trade::InventoryTarget& target :
         acPlan.Targets)
    {
        result.Targets.push_back(
            {target.Item, target.Quantity});
    }

    return result;
}

struct InventoryTarget
{
    InventoryComponent* pComponent{};
};

bool TryGetInventoryTarget(
    World& aWorld,
    Trade::PlayerId aPlayerId,
    InventoryTarget& aTarget) noexcept
{
    Player* const pPlayer = aWorld.GetPlayerManager().GetById(aPlayerId);
    if (!pPlayer)
        return false;

    const std::optional<entt::entity> character = pPlayer->GetCharacter();
    if (!character)
        return false;

    auto inventoryView = aWorld.view<InventoryComponent>();
    const auto inventoryIt = inventoryView.find(*character);
    if (inventoryIt == inventoryView.end())
        return false;

    aTarget.pComponent = &inventoryView.get<InventoryComponent>(*inventoryIt);
    return true;
}

bool ApplyMutationsToInventory(
    Inventory& aInventory,
    const Trade::PlayerMutationPlan& acPlan) noexcept
{
    for (const Trade::InventoryMutation& mutation : acPlan.Mutations)
    {
        if (mutation.Item == 0 || mutation.Delta == 0)
            return false;

        const GameId itemId = ToGameId(mutation.Item);

        Inventory::Entry* pMatchingEntry = nullptr;
        std::size_t matches = 0;

        for (Inventory::Entry& entry : aInventory.Entries)
        {
            if (entry.BaseId != itemId)
                continue;

            ++matches;
            pMatchingEntry = &entry;
        }

        if (matches > 1)
            return false;

        if (pMatchingEntry && !IsMvpTransferable(*pMatchingEntry))
            return false;

        const std::int64_t currentCount =
            pMatchingEntry ? pMatchingEntry->Count : 0;
        const std::int64_t resultingCount =
            currentCount + mutation.Delta;

        if (resultingCount < 0 ||
            resultingCount > std::numeric_limits<std::int32_t>::max())
        {
            return false;
        }

        if (mutation.Delta < 0 && !pMatchingEntry)
            return false;

        Inventory::Entry deltaEntry;
        if (pMatchingEntry)
            deltaEntry = *pMatchingEntry;

        deltaEntry.BaseId = itemId;
        deltaEntry.Count = mutation.Delta;
        aInventory.AddOrRemoveEntry(deltaEntry);
    }

    return true;
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
    , m_tradeApplyResultConnection(aDispatcher.sink<PacketEvent<TradeApplyResultRequest>>().connect<&TradeService::OnTradeApplyResultRequest>(this))
    , m_tradeReconcileResultConnection(aDispatcher.sink<PacketEvent<TradeReconcileResultRequest>>().connect<&TradeService::OnTradeReconcileResultRequest>(this))
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
    const Trade::State previousState = session.GetState();
    const Trade::Tick currentTick = GameServer::Get()->GetTick();

    const Trade::Result result = session.Confirm(
        playerId,
        message.Revision,
        currentTick);

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

    if (previousState != Trade::State::Locked &&
        session.GetState() == Trade::State::Locked)
    {
        Trade::MutationPlanResult planResult =
            ValidateAndBuildMutationPlan(session);

        if (!planResult.Succeeded())
        {
            session.Fail(planResult.ErrorCode, currentTick);

            spdlog::warn(
                "[trade={}][revision={}] validation_failed error={}",
                session.GetId(),
                session.GetRevision(),
                static_cast<std::uint32_t>(planResult.ErrorCode));

            SendState(session);
            RemoveSession(session.GetId());
            return;
        }

        BeginApplication(
            session,
            std::move(planResult.Plan),
            currentTick);
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

void TradeService::OnTradeApplyResultRequest(
    const PacketEvent<TradeApplyResultRequest>& acPacket) noexcept
{
    Player* const pPlayer = acPacket.pPlayer;
    if (!pPlayer)
        return;

    const TradeApplyResultRequest& message = acPacket.Packet;
    const Trade::PlayerId playerId = pPlayer->GetId();

    Trade::Session* const pSession = FindSession(message.SessionId);
    const auto applicationIt = m_applications.find(message.SessionId);

    if (!pSession || applicationIt == m_applications.end())
    {
        spdlog::warn(
            "[trade={}][apply={}][player={}] apply_result_rejected reason=session_not_found",
            message.SessionId,
            message.ApplyId,
            playerId);
        return;
    }

    Trade::Session& session = *pSession;
    Trade::Application& application = applicationIt->second;

    const auto reconciliationIt =
        m_reconciliations.find(message.SessionId);

    if (reconciliationIt != m_reconciliations.end())
    {
        if (reconciliationIt->second.IsParticipant(playerId))
        {
            SendReconciliation(
                reconciliationIt->second,
                pPlayer);
        }

        return;
    }

    if (!session.IsParticipant(playerId) ||
        application.GetId() != message.ApplyId ||
        application.GetRevision() != message.Revision)
    {
        spdlog::warn(
            "[trade={}][revision={}][apply={}][player={}] apply_result_rejected reason=identity_mismatch",
            message.SessionId,
            message.Revision,
            message.ApplyId,
            playerId);
        SendState(session, pPlayer);
        return;
    }

    const TradeApplyResultCode resultCode =
        message.Valid
            ? message.Result
            : TradeApplyResultCode::MalformedPlan;

    const Trade::Result recordResult =
        application.RecordResult(
            playerId,
            ToApplyOutcome(resultCode),
            GameServer::Get()->GetTick());

    if (!recordResult.Succeeded())
    {
        spdlog::warn(
            "[trade={}][revision={}][apply={}][player={}] apply_result_rejected error={}",
            session.GetId(),
            session.GetRevision(),
            application.GetId(),
            playerId,
            static_cast<std::uint32_t>(
                recordResult.ErrorCode));
        SendState(session, pPlayer);
        return;
    }

    spdlog::info(
        "[trade={}][revision={}][apply={}][player={}] apply_result={} changed={}",
        session.GetId(),
        session.GetRevision(),
        application.GetId(),
        playerId,
        static_cast<std::uint32_t>(resultCode),
        recordResult.Changed ? 1 : 0);

    const Trade::Tick currentTick =
        GameServer::Get()->GetTick();

    if (application.HasFailed())
    {
        if (!session.IsTerminal())
        {
            session.Fail(
                Trade::Error::ApplyFailed,
                currentTick);
        }

        SendState(session);
        BeginReconciliation(
            session,
            application,
            currentTick);
        return;
    }

    if (!application.AllSucceeded())
        return;

    if (!CommitMutationPlan(application.GetPlan()))
    {
        session.Fail(
            Trade::Error::ServerCommitFailed,
            currentTick);

        spdlog::error(
            "[trade={}][revision={}][apply={}] server_commit_failed",
            session.GetId(),
            session.GetRevision(),
            application.GetId());

        SendState(session);
        BeginReconciliation(
            session,
            application,
            currentTick);
        return;
    }

    session.Complete(currentTick);

    spdlog::info(
        "[trade={}][revision={}][apply={}] completed",
        session.GetId(),
        session.GetRevision(),
        application.GetId());

    SendState(session);
    ScheduleTerminalCleanup(
        session.GetId(),
        currentTick);
}

void TradeService::OnTradeReconcileResultRequest(
    const PacketEvent<TradeReconcileResultRequest>& acPacket) noexcept
{
    Player* const pPlayer = acPacket.pPlayer;
    if (!pPlayer)
        return;

    const TradeReconcileResultRequest& message =
        acPacket.Packet;
    const Trade::PlayerId playerId =
        pPlayer->GetId();

    Trade::Session* const pSession =
        FindSession(message.SessionId);
    const auto reconciliationIt =
        m_reconciliations.find(message.SessionId);

    if (!pSession ||
        reconciliationIt == m_reconciliations.end())
    {
        spdlog::warn(
            "[trade={}][reconcile={}][player={}] reconcile_result_rejected reason=session_not_found",
            message.SessionId,
            message.ReconcileId,
            playerId);
        return;
    }

    Trade::Reconciliation& reconciliation =
        reconciliationIt->second;

    if (!reconciliation.IsParticipant(playerId) ||
        reconciliation.GetId() != message.ReconcileId ||
        reconciliation.GetApplyId() != message.ApplyId ||
        reconciliation.GetRevision() != message.Revision)
    {
        spdlog::warn(
            "[trade={}][revision={}][apply={}][reconcile={}][player={}] reconcile_result_rejected reason=identity_mismatch",
            message.SessionId,
            message.Revision,
            message.ApplyId,
            message.ReconcileId,
            playerId);
        SendState(*pSession, pPlayer);
        return;
    }

    const TradeReconcileResultCode resultCode =
        message.Valid
            ? message.Result
            : TradeReconcileResultCode::MalformedPlan;

    const Trade::Result result =
        reconciliation.RecordResult(
            playerId,
            ToReconcileOutcome(resultCode),
            GameServer::Get()->GetTick());

    if (!result.Succeeded())
    {
        spdlog::warn(
            "[trade={}][revision={}][apply={}][reconcile={}][player={}] reconcile_result_rejected error={}",
            reconciliation.GetSessionId(),
            reconciliation.GetRevision(),
            reconciliation.GetApplyId(),
            reconciliation.GetId(),
            playerId,
            static_cast<std::uint32_t>(
                result.ErrorCode));
        return;
    }

    spdlog::info(
        "[trade={}][revision={}][apply={}][reconcile={}][player={}] reconcile_result={} changed={}",
        reconciliation.GetSessionId(),
        reconciliation.GetRevision(),
        reconciliation.GetApplyId(),
        reconciliation.GetId(),
        playerId,
        static_cast<std::uint32_t>(resultCode),
        result.Changed ? 1 : 0);

    if (!reconciliation.IsSettled())
        return;

    if (reconciliation.AllSucceeded())
    {
        spdlog::info(
            "[trade={}][revision={}][apply={}][reconcile={}] reconciliation_completed",
            reconciliation.GetSessionId(),
            reconciliation.GetRevision(),
            reconciliation.GetApplyId(),
            reconciliation.GetId());
    }
    else
    {
        spdlog::error(
            "[trade={}][revision={}][apply={}][reconcile={}] reconciliation_unresolved",
            reconciliation.GetSessionId(),
            reconciliation.GetRevision(),
            reconciliation.GetApplyId(),
            reconciliation.GetId());
    }

    ScheduleTerminalCleanup(
        reconciliation.GetSessionId(),
        GameServer::Get()->GetTick());
}

void TradeService::OnPlayerLeave(
    const PlayerLeaveEvent& acEvent) noexcept
{
    Player* const pPlayer = acEvent.pPlayer;
    if (!pPlayer)
        return;

    const Trade::PlayerId playerId =
        pPlayer->GetId();
    const auto playerSessionIt =
        m_playerSessions.find(playerId);

    if (playerSessionIt == m_playerSessions.end())
        return;

    const Trade::SessionId sessionId =
        playerSessionIt->second;
    Trade::Session* const pSession =
        FindSession(sessionId);

    if (!pSession)
    {
        m_playerSessions.erase(playerSessionIt);
        return;
    }

    const Trade::Tick currentTick =
        GameServer::Get()->GetTick();

    const auto reconciliationIt =
        m_reconciliations.find(sessionId);

    if (reconciliationIt != m_reconciliations.end())
    {
        Trade::Reconciliation& reconciliation =
            reconciliationIt->second;

        const Trade::Result unavailableResult =
            reconciliation.MarkUnavailable(
                playerId,
                currentTick);

        if (!unavailableResult.Succeeded())
        {
            spdlog::warn(
                "[trade={}][reconcile={}][player={}] unavailable_mark_failed error={}",
                sessionId,
                reconciliation.GetId(),
                playerId,
                static_cast<std::uint32_t>(
                    unavailableResult.ErrorCode));
        }

        if (reconciliation.IsSettled())
        {
            ScheduleTerminalCleanup(
                sessionId,
                currentTick);
        }

        return;
    }

    if (pSession->GetState() == Trade::State::Applying)
    {
        const auto applicationIt =
            m_applications.find(sessionId);

        if (applicationIt ==
            m_applications.end())
        {
            pSession->Fail(
                Trade::Error::ParticipantDisconnected,
                currentTick);
            SendState(*pSession);
            ScheduleTerminalCleanup(
                sessionId,
                currentTick);
            return;
        }

        pSession->Fail(
            Trade::Error::ParticipantDisconnected,
            currentTick);
        SendState(*pSession);

        spdlog::warn(
            "[trade={}][revision={}][player={}] failed reason=participant_disconnected",
            pSession->GetId(),
            pSession->GetRevision(),
            playerId);

        BeginReconciliation(
            *pSession,
            applicationIt->second,
            currentTick,
            playerId,
            pPlayer);
        return;
    }

    const Trade::Result result =
        pSession->Cancel(playerId, currentTick);

    if (!result.Succeeded())
    {
        spdlog::warn(
            "[trade={}][revision={}][player={}] disconnect_cancel_failed error={}",
            pSession->GetId(),
            pSession->GetRevision(),
            playerId,
            static_cast<std::uint32_t>(
                result.ErrorCode));
        return;
    }

    SendCancelled(
        *pSession,
        TradeCancelReason::PlayerDisconnected,
        playerId,
        pPlayer);

    spdlog::info(
        "[trade={}][revision={}][player={}] cancelled reason=player_disconnected",
        pSession->GetId(),
        pSession->GetRevision(),
        playerId);

    RemoveSession(sessionId);
}

void TradeService::OnUpdate(
    const UpdateEvent& acEvent) noexcept
{
    TP_UNUSED(acEvent);

    const Trade::Tick currentTick =
        GameServer::Get()->GetTick();

    if (currentTick < m_nextExpirySweepTick)
        return;

    m_nextExpirySweepTick =
        currentTick + kExpirySweepIntervalMs;

    std::vector<Trade::SessionId> expiredInvitations;
    std::vector<Trade::SessionId> timedOutApplications;
    std::vector<Trade::SessionId> timedOutReconciliations;
    std::vector<Trade::SessionId> cleanupSessions;

    expiredInvitations.reserve(m_sessions.size());
    timedOutApplications.reserve(m_applications.size());
    timedOutReconciliations.reserve(
        m_reconciliations.size());
    cleanupSessions.reserve(
        m_terminalCleanupTicks.size());

    for (const auto& [sessionId, session] :
         m_sessions)
    {
        if (session.GetState() ==
                Trade::State::PendingAcceptance &&
            currentTick >= session.GetExpiryTick())
        {
            expiredInvitations.push_back(sessionId);
        }
    }

    for (const auto& [sessionId, application] :
         m_applications)
    {
        if (m_reconciliations.find(sessionId) !=
            m_reconciliations.end())
        {
            continue;
        }

        if (application.IsTimedOut(currentTick))
            timedOutApplications.push_back(sessionId);
    }

    for (const auto& [sessionId, reconciliation] :
         m_reconciliations)
    {
        if (reconciliation.IsTimedOut(currentTick))
        {
            timedOutReconciliations.push_back(
                sessionId);
        }
    }

    for (const auto& [sessionId, cleanupTick] :
         m_terminalCleanupTicks)
    {
        if (currentTick >= cleanupTick)
            cleanupSessions.push_back(sessionId);
    }

    for (const Trade::SessionId sessionId :
         expiredInvitations)
    {
        Trade::Session* const pSession =
            FindSession(sessionId);

        if (!pSession)
            continue;

        const Trade::Result result =
            pSession->Expire(currentTick);

        if (result.Changed)
        {
            SendCancelled(
                *pSession,
                TradeCancelReason::Expired,
                0);

            spdlog::info(
                "[trade={}][revision={}] cancelled reason=expired",
                pSession->GetId(),
                pSession->GetRevision());
        }

        RemoveSession(sessionId);
    }

    for (const Trade::SessionId sessionId :
         timedOutApplications)
    {
        Trade::Session* const pSession =
            FindSession(sessionId);
        const auto applicationIt =
            m_applications.find(sessionId);

        if (!pSession ||
            applicationIt == m_applications.end())
        {
            continue;
        }

        if (!pSession->IsTerminal())
        {
            pSession->Fail(
                Trade::Error::ApplyTimedOut,
                currentTick);
            SendState(*pSession);

            spdlog::warn(
                "[trade={}][revision={}] failed reason=apply_timeout",
                pSession->GetId(),
                pSession->GetRevision());
        }

        BeginReconciliation(
            *pSession,
            applicationIt->second,
            currentTick);
    }

    for (const Trade::SessionId sessionId :
         timedOutReconciliations)
    {
        const auto reconciliationIt =
            m_reconciliations.find(sessionId);

        if (reconciliationIt ==
            m_reconciliations.end())
        {
            continue;
        }

        spdlog::error(
            "[trade={}][revision={}][apply={}][reconcile={}] reconciliation_timeout",
            reconciliationIt->second.GetSessionId(),
            reconciliationIt->second.GetRevision(),
            reconciliationIt->second.GetApplyId(),
            reconciliationIt->second.GetId());

        ScheduleTerminalCleanup(
            sessionId,
            currentTick);
    }

    for (const Trade::SessionId sessionId :
         cleanupSessions)
    {
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

Trade::ApplyId TradeService::AllocateApplyId() noexcept
{
    for (;;)
    {
        const Trade::ApplyId candidate = m_nextApplyId++;

        if (m_nextApplyId == 0)
            m_nextApplyId = 1;

        bool alreadyUsed = false;
        for (const auto& [sessionId, application] : m_applications)
        {
            TP_UNUSED(sessionId);

            if (application.GetId() == candidate)
            {
                alreadyUsed = true;
                break;
            }
        }

        if (candidate != 0 && !alreadyUsed)
            return candidate;
    }
}

Trade::ReconcileId TradeService::AllocateReconcileId() noexcept
{
    for (;;)
    {
        const Trade::ReconcileId candidate =
            m_nextReconcileId++;

        if (m_nextReconcileId == 0)
            m_nextReconcileId = 1;

        bool alreadyUsed = false;

        for (const auto& [sessionId, reconciliation] :
             m_reconciliations)
        {
            TP_UNUSED(sessionId);

            if (reconciliation.GetId() == candidate)
            {
                alreadyUsed = true;
                break;
            }
        }

        if (candidate != 0 && !alreadyUsed)
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

bool TradeService::TryBuildInventorySnapshot(
    Trade::PlayerId aPlayerId,
    Trade::InventorySnapshot& aSnapshot) const noexcept
{
    Player* const pPlayer = m_world.GetPlayerManager().GetById(aPlayerId);
    if (!pPlayer)
        return false;

    const std::optional<entt::entity> character = pPlayer->GetCharacter();
    if (!character)
        return false;

    auto inventoryView = m_world.view<InventoryComponent>();
    const auto inventoryIt = inventoryView.find(*character);
    if (inventoryIt == inventoryView.end())
        return false;

    const Inventory& inventory =
        inventoryView.get<InventoryComponent>(*inventoryIt).Content;

    aSnapshot = {};
    aSnapshot.Owner = aPlayerId;
    aSnapshot.Items.reserve(inventory.Entries.size());

    for (const Inventory::Entry& entry : inventory.Entries)
    {
        if (!entry.BaseId || entry.Count <= 0)
            continue;

        const Trade::ItemId itemId = ToTradeItemId(entry.BaseId);

        auto snapshotIt = std::find_if(
            aSnapshot.Items.begin(),
            aSnapshot.Items.end(),
            [itemId](const Trade::InventoryItemState& acItem) noexcept
            {
                return acItem.Item == itemId;
            });

        if (snapshotIt == aSnapshot.Items.end())
        {
            aSnapshot.Items.push_back(
                Trade::InventoryItemState{
                    itemId,
                    static_cast<std::uint64_t>(entry.Count),
                    IsMvpTransferable(entry),
                    false});
            continue;
        }

        snapshotIt->Ambiguous = true;
        snapshotIt->Transferable = false;

        const std::uint64_t count = static_cast<std::uint64_t>(entry.Count);
        if (snapshotIt->Quantity > std::numeric_limits<std::uint64_t>::max() - count)
            snapshotIt->Quantity = std::numeric_limits<std::uint64_t>::max();
        else
            snapshotIt->Quantity += count;
    }

    return true;
}

Trade::MutationPlanResult TradeService::ValidateAndBuildMutationPlan(
    const Trade::Session& acSession) const noexcept
{
    Trade::InventorySnapshot initiatorInventory;
    Trade::InventorySnapshot recipientInventory;

    if (!TryBuildInventorySnapshot(
            acSession.GetInitiator().Id,
            initiatorInventory) ||
        !TryBuildInventorySnapshot(
            acSession.GetRecipient().Id,
            recipientInventory))
    {
        return Trade::MutationPlanResult::Failure(
            Trade::Error::InventoryUnavailable);
    }

    return Trade::BuildMutationPlan(
        acSession,
        initiatorInventory,
        recipientInventory);
}

Trade::ReconciliationPlanResult TradeService::BuildCurrentReconciliationPlan(
    const Trade::Application& acApplication) const noexcept
{
    Trade::InventorySnapshot initiatorInventory;
    Trade::InventorySnapshot recipientInventory;

    if (!TryBuildInventorySnapshot(
            acApplication.GetPlan().Initiator.Player,
            initiatorInventory) ||
        !TryBuildInventorySnapshot(
            acApplication.GetPlan().Recipient.Player,
            recipientInventory))
    {
        return Trade::ReconciliationPlanResult::Failure(
            Trade::Error::InventoryUnavailable);
    }

    return Trade::BuildReconciliationPlan(
        acApplication,
        initiatorInventory,
        recipientInventory);
}

bool TradeService::CommitMutationPlan(
    const Trade::MutationPlan& acPlan) noexcept
{
    InventoryTarget initiatorTarget;
    InventoryTarget recipientTarget;

    if (!TryGetInventoryTarget(
            m_world,
            acPlan.Initiator.Player,
            initiatorTarget) ||
        !TryGetInventoryTarget(
            m_world,
            acPlan.Recipient.Player,
            recipientTarget))
    {
        return false;
    }

    Inventory initiatorInventory =
        initiatorTarget.pComponent->Content;
    Inventory recipientInventory =
        recipientTarget.pComponent->Content;

    if (!ApplyMutationsToInventory(
            initiatorInventory,
            acPlan.Initiator) ||
        !ApplyMutationsToInventory(
            recipientInventory,
            acPlan.Recipient))
    {
        return false;
    }

    initiatorTarget.pComponent->Content =
        std::move(initiatorInventory);
    recipientTarget.pComponent->Content =
        std::move(recipientInventory);

    return true;
}

void TradeService::BeginApplication(
    Trade::Session& aSession,
    Trade::MutationPlan aPlan,
    Trade::Tick aCurrentTick) noexcept
{
    const Trade::ApplyId applyId = AllocateApplyId();

    const Trade::Result startResult =
        aSession.StartApplying(aCurrentTick);

    if (!startResult.Succeeded())
    {
        aSession.Fail(
            Trade::Error::ApplyFailed,
            aCurrentTick);
        SendState(aSession);
        RemoveSession(aSession.GetId());
        return;
    }

    const auto [applicationIt, inserted] =
        m_applications.try_emplace(
            aSession.GetId(),
            applyId,
            std::move(aPlan),
            aCurrentTick,
            aCurrentTick + kApplyTimeoutMs);

    if (!inserted)
    {
        aSession.Fail(
            Trade::Error::ApplyFailed,
            aCurrentTick);
        SendState(aSession);
        RemoveSession(aSession.GetId());
        return;
    }

    const Trade::Application& application =
        applicationIt->second;

    Trade::ReconciliationPlanResult baselineResult =
        BuildCurrentReconciliationPlan(application);

    if (!baselineResult.Succeeded())
    {
        aSession.Fail(
            Trade::Error::ReconciliationUnavailable,
            aCurrentTick);

        spdlog::error(
            "[trade={}][revision={}][apply={}] reconciliation_baseline_failed error={}",
            aSession.GetId(),
            aSession.GetRevision(),
            application.GetId(),
            static_cast<std::uint32_t>(
                baselineResult.ErrorCode));

        SendState(aSession);
        RemoveSession(aSession.GetId());
        return;
    }

    m_reconciliationBaselines.insert_or_assign(
        aSession.GetId(),
        std::move(baselineResult.Plan));

    spdlog::info(
        "[trade={}][revision={}][apply={}] applying initiator_mutations={} recipient_mutations={}",
        aSession.GetId(),
        aSession.GetRevision(),
        application.GetId(),
        application.GetPlan().Initiator.Mutations.size(),
        application.GetPlan().Recipient.Mutations.size());

    SendState(aSession);
    SendApply(aSession, application);
}

void TradeService::BeginReconciliation(
    Trade::Session& aSession,
    Trade::Application& aApplication,
    Trade::Tick aCurrentTick,
    Trade::PlayerId aUnavailablePlayerId,
    Player* apIgnoredPlayer) noexcept
{
    const auto existingIt =
        m_reconciliations.find(aSession.GetId());

    if (existingIt != m_reconciliations.end())
    {
        SendReconciliation(
            existingIt->second,
            nullptr,
            apIgnoredPlayer);
        return;
    }

    Trade::ReconciliationPlan plan;

    Trade::ReconciliationPlanResult currentPlanResult =
        BuildCurrentReconciliationPlan(aApplication);

    if (currentPlanResult.Succeeded())
    {
        plan = std::move(currentPlanResult.Plan);
    }
    else
    {
        const auto baselineIt =
            m_reconciliationBaselines.find(
                aSession.GetId());

        const bool canUseBaseline =
            currentPlanResult.ErrorCode ==
                Trade::Error::InventoryUnavailable &&
            baselineIt !=
                m_reconciliationBaselines.end();

        if (!canUseBaseline)
        {
            spdlog::error(
                "[trade={}][revision={}][apply={}] reconciliation_plan_failed error={}",
                aSession.GetId(),
                aSession.GetRevision(),
                aApplication.GetId(),
                static_cast<std::uint32_t>(
                    currentPlanResult.ErrorCode));

            ScheduleTerminalCleanup(
                aSession.GetId(),
                aCurrentTick);
            return;
        }

        plan = baselineIt->second;

        spdlog::warn(
            "[trade={}][revision={}][apply={}] reconciliation_using_application_baseline current_error={}",
            aSession.GetId(),
            aSession.GetRevision(),
            aApplication.GetId(),
            static_cast<std::uint32_t>(
                currentPlanResult.ErrorCode));
    }

    const Trade::ReconcileId reconcileId =
        AllocateReconcileId();

    const auto [reconciliationIt, inserted] =
        m_reconciliations.try_emplace(
            aSession.GetId(),
            reconcileId,
            std::move(plan),
            aCurrentTick,
            aCurrentTick + kReconciliationTimeoutMs);

    if (!inserted)
    {
        ScheduleTerminalCleanup(
            aSession.GetId(),
            aCurrentTick);
        return;
    }

    Trade::Reconciliation& reconciliation =
        reconciliationIt->second;

    m_reconciliationBaselines.erase(
        aSession.GetId());

    if (aUnavailablePlayerId != 0)
    {
        reconciliation.MarkUnavailable(
            aUnavailablePlayerId,
            aCurrentTick);
    }

    m_terminalCleanupTicks.erase(
        aSession.GetId());

    spdlog::warn(
        "[trade={}][revision={}][apply={}][reconcile={}] reconciliation_started initiator_targets={} recipient_targets={}",
        reconciliation.GetSessionId(),
        reconciliation.GetRevision(),
        reconciliation.GetApplyId(),
        reconciliation.GetId(),
        reconciliation.GetPlan().Initiator.Targets.size(),
        reconciliation.GetPlan().Recipient.Targets.size());

    SendReconciliation(
        reconciliation,
        nullptr,
        apIgnoredPlayer);

    if (reconciliation.IsSettled())
    {
        ScheduleTerminalCleanup(
            aSession.GetId(),
            aCurrentTick);
    }
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
    notify.TerminalError = static_cast<std::uint8_t>(acSession.GetTerminalError());

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

void TradeService::SendApply(
    const Trade::Session& acSession,
    const Trade::Application& acApplication) const noexcept
{
    NotifyTradeApply initiatorNotify;
    initiatorNotify.SessionId = acSession.GetId();
    initiatorNotify.Revision = acSession.GetRevision();
    initiatorNotify.ApplyId = acApplication.GetId();
    initiatorNotify.Plan =
        ToNetworkPlan(acApplication.GetPlan().Initiator);

    NotifyTradeApply recipientNotify;
    recipientNotify.SessionId = acSession.GetId();
    recipientNotify.Revision = acSession.GetRevision();
    recipientNotify.ApplyId = acApplication.GetId();
    recipientNotify.Plan =
        ToNetworkPlan(acApplication.GetPlan().Recipient);

    Player* const pInitiator =
        m_world.GetPlayerManager().GetById(
            acSession.GetInitiator().Id);
    Player* const pRecipient =
        m_world.GetPlayerManager().GetById(
            acSession.GetRecipient().Id);

    if (pInitiator)
        pInitiator->Send(initiatorNotify);

    if (pRecipient)
        pRecipient->Send(recipientNotify);
}

void TradeService::SendReconciliation(
    const Trade::Reconciliation& acReconciliation,
    Player* apRecipient,
    Player* apIgnoredPlayer) const noexcept
{
    const Trade::ReconciliationPlan& plan =
        acReconciliation.GetPlan();

    auto sendPlan =
        [&](Player* apPlayer,
            const Trade::PlayerReconciliationPlan& acPlan) noexcept
        {
            if (!apPlayer ||
                apPlayer == apIgnoredPlayer)
            {
                return;
            }

            NotifyTradeReconcile notify;
            notify.SessionId =
                acReconciliation.GetSessionId();
            notify.Revision =
                acReconciliation.GetRevision();
            notify.ApplyId =
                acReconciliation.GetApplyId();
            notify.ReconcileId =
                acReconciliation.GetId();
            notify.Plan = ToNetworkPlan(acPlan);
            apPlayer->Send(notify);
        };

    if (apRecipient)
    {
        if (apRecipient->GetId() ==
            plan.Initiator.Player)
        {
            sendPlan(
                apRecipient,
                plan.Initiator);
        }
        else if (apRecipient->GetId() ==
                 plan.Recipient.Player)
        {
            sendPlan(
                apRecipient,
                plan.Recipient);
        }

        return;
    }

    sendPlan(
        m_world.GetPlayerManager().GetById(
            plan.Initiator.Player),
        plan.Initiator);

    sendPlan(
        m_world.GetPlayerManager().GetById(
            plan.Recipient.Player),
        plan.Recipient);
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

void TradeService::ReleaseSessionParticipants(
    Trade::SessionId aSessionId) noexcept
{
    const auto sessionIt = m_sessions.find(aSessionId);
    if (sessionIt == m_sessions.end())
        return;

    const Trade::PlayerId initiatorId =
        sessionIt->second.GetInitiator().Id;
    const Trade::PlayerId recipientId =
        sessionIt->second.GetRecipient().Id;

    const auto initiatorIndexIt =
        m_playerSessions.find(initiatorId);
    if (initiatorIndexIt != m_playerSessions.end() &&
        initiatorIndexIt->second == aSessionId)
    {
        m_playerSessions.erase(initiatorIndexIt);
    }

    const auto recipientIndexIt =
        m_playerSessions.find(recipientId);
    if (recipientIndexIt != m_playerSessions.end() &&
        recipientIndexIt->second == aSessionId)
    {
        m_playerSessions.erase(recipientIndexIt);
    }
}

void TradeService::ScheduleTerminalCleanup(
    Trade::SessionId aSessionId,
    Trade::Tick aCurrentTick) noexcept
{
    ReleaseSessionParticipants(aSessionId);
    m_terminalCleanupTicks.insert_or_assign(
        aSessionId,
        aCurrentTick + kTerminalRetentionMs);
}

void TradeService::RemoveSession(
    Trade::SessionId aSessionId) noexcept
{
    ReleaseSessionParticipants(aSessionId);
    m_applications.erase(aSessionId);
    m_reconciliationBaselines.erase(aSessionId);
    m_reconciliations.erase(aSessionId);
    m_terminalCleanupTicks.erase(aSessionId);
    m_sessions.erase(aSessionId);
}
