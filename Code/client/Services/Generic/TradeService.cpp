#include <Services/TradeService.h>

#include <Services/TransportService.h>

#include <Messages/NotifyTradeCancelled.h>
#include <Messages/NotifyTradeInvite.h>
#include <Messages/NotifyTradeStarted.h>
#include <Messages/NotifyTradeState.h>
#include <Messages/NotifyTradeApply.h>
#include <Messages/TradeCancelRequest.h>
#include <Messages/TradeConfirmRequest.h>
#include <Messages/TradeInviteRequest.h>
#include <Messages/TradeInviteResponseRequest.h>
#include <Messages/TradeOfferUpdateRequest.h>
#include <Messages/TradeApplyResultRequest.h>

#include <Structs/TradeOffer.h>
#include <Structs/TradeApplication.h>

#include <World.h>
#include <PlayerCharacter.h>
#include <Forms/TESBoundObject.h>
#include <Games/Overrides.h>

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

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

GameId ToGameId(std::uint64_t aItemId) noexcept
{
    return GameId{
        static_cast<std::uint32_t>(aItemId >> 32),
        static_cast<std::uint32_t>(aItemId & 0xFFFFFFFFu)};
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

struct LocalItemLookup
{
    const Inventory::Entry* pEntry{};
    std::size_t Matches{};
};

LocalItemLookup FindLocalItem(
    const Inventory& acInventory,
    const GameId& acItemId) noexcept
{
    LocalItemLookup result;

    for (const Inventory::Entry& entry : acInventory.Entries)
    {
        if (entry.BaseId != acItemId)
            continue;

        ++result.Matches;
        result.pEntry = &entry;
    }

    return result;
}

struct ExpectedLocalItem
{
    GameId Id{};
    std::int64_t Count{};
};
}

TradeService::TradeService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept
    : m_world(aWorld)
    , m_transport(aTransportService)
    , m_tradeInviteConnection(aDispatcher.sink<NotifyTradeInvite>().connect<&TradeService::OnNotifyTradeInvite>(this))
    , m_tradeStartedConnection(aDispatcher.sink<NotifyTradeStarted>().connect<&TradeService::OnNotifyTradeStarted>(this))
    , m_tradeCancelledConnection(aDispatcher.sink<NotifyTradeCancelled>().connect<&TradeService::OnNotifyTradeCancelled>(this))
    , m_tradeStateConnection(aDispatcher.sink<NotifyTradeState>().connect<&TradeService::OnNotifyTradeState>(this))
    , m_tradeApplyConnection(aDispatcher.sink<NotifyTradeApply>().connect<&TradeService::OnNotifyTradeApply>(this))
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
        static_cast<std::uint8_t>(Trade::Error::ServerCommitFailed))
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


void TradeService::OnNotifyTradeApply(
    const NotifyTradeApply& acTradeApply) noexcept
{
    const auto existingIt =
        m_applyJournal.find(acTradeApply.ApplyId);

    if (existingIt != m_applyJournal.end())
    {
        const ClientTradeApplyRecord& record =
            existingIt->second;

        if (record.SessionId != acTradeApply.SessionId ||
            record.Revision != acTradeApply.Revision)
        {
            spdlog::error(
                "[TradeService]: apply id collision apply={} incoming_session={} recorded_session={}",
                acTradeApply.ApplyId,
                acTradeApply.SessionId,
                record.SessionId);

            SendApplyResult(
                acTradeApply,
                TradeApplyResultCode::MalformedPlan);
            return;
        }

        const auto result =
            static_cast<TradeApplyResultCode>(
                record.ResultCode);

        spdlog::info(
            "[TradeService]: replaying apply result session={} revision={} apply={} result={}",
            acTradeApply.SessionId,
            acTradeApply.Revision,
            acTradeApply.ApplyId,
            record.ResultCode);

        SendApplyResult(acTradeApply, result);
        return;
    }

    const TradeApplyResultCode result =
        ApplyMutationPlan(acTradeApply);

    RememberApplyResult(acTradeApply, result);
    SendApplyResult(acTradeApply, result);
}

TradeApplyResultCode TradeService::ApplyMutationPlan(
    const NotifyTradeApply& acTradeApply) noexcept
{
    if (!acTradeApply.Plan.Valid)
        return TradeApplyResultCode::MalformedPlan;

    if (!m_sessionState ||
        m_sessionState->SessionId != acTradeApply.SessionId ||
        m_sessionState->Revision != acTradeApply.Revision ||
        m_sessionState->State != Trade::State::Applying)
    {
        return TradeApplyResultCode::SessionMismatch;
    }

    PlayerCharacter* const pPlayer =
        PlayerCharacter::Get();
    if (!pPlayer)
        return TradeApplyResultCode::LocalPlayerUnavailable;

    Inventory currentInventory =
        pPlayer->GetInventory();

    std::vector<ExpectedLocalItem> expectedItems;
    expectedItems.reserve(
        acTradeApply.Plan.Mutations.size());

    auto& modSystem = World::Get().GetModSystem();

    for (const TradeInventoryMutationData& mutation :
         acTradeApply.Plan.Mutations)
    {
        const GameId itemId = ToGameId(mutation.ItemId);
        const std::uint32_t gameId =
            modSystem.GetGameId(itemId);

        TESBoundObject* const pObject =
            Cast<TESBoundObject>(
                TESForm::GetById(gameId));
        if (!pObject)
            return TradeApplyResultCode::ItemUnavailable;

        const LocalItemLookup lookup =
            FindLocalItem(currentInventory, itemId);

        if (lookup.Matches > 1)
            return TradeApplyResultCode::AmbiguousItem;

        if (lookup.pEntry &&
            !IsMvpTransferable(*lookup.pEntry))
        {
            return TradeApplyResultCode::ItemNotTransferable;
        }

        const std::int64_t currentCount =
            lookup.pEntry ? lookup.pEntry->Count : 0;
        const std::int64_t expectedCount =
            currentCount + mutation.Delta;

        if (expectedCount < 0)
            return TradeApplyResultCode::InsufficientQuantity;

        if (expectedCount >
            std::numeric_limits<std::int32_t>::max())
        {
            return TradeApplyResultCode::LocalStateMismatch;
        }

        expectedItems.push_back(
            ExpectedLocalItem{itemId, expectedCount});
    }

    {
        ScopedInventoryOverride inventoryOverride;

        for (int pass = 0; pass < 2; ++pass)
        {
            for (const TradeInventoryMutationData& mutation :
                 acTradeApply.Plan.Mutations)
            {
                const bool isRemoval = mutation.Delta < 0;
                if ((pass == 0) != isRemoval)
                    continue;

                Inventory::Entry entry;
                entry.BaseId = ToGameId(mutation.ItemId);
                entry.Count = mutation.Delta;
                pPlayer->AddOrRemoveItem(entry);
            }
        }
    }

    const Inventory updatedInventory =
        pPlayer->GetInventory();

    for (const ExpectedLocalItem& expected :
         expectedItems)
    {
        const LocalItemLookup lookup =
            FindLocalItem(updatedInventory, expected.Id);

        if (lookup.Matches > 1)
            return TradeApplyResultCode::LocalStateMismatch;

        const std::int64_t actualCount =
            lookup.pEntry ? lookup.pEntry->Count : 0;

        if (actualCount != expected.Count)
            return TradeApplyResultCode::LocalStateMismatch;
    }

    return TradeApplyResultCode::Success;
}

void TradeService::SendApplyResult(
    const NotifyTradeApply& acTradeApply,
    TradeApplyResultCode aResult) const noexcept
{
    TradeApplyResultRequest request;
    request.SessionId = acTradeApply.SessionId;
    request.Revision = acTradeApply.Revision;
    request.ApplyId = acTradeApply.ApplyId;
    request.Result = aResult;

    if (!m_transport.Send(request))
    {
        spdlog::warn(
            "[TradeService]: failed to send apply result session={} revision={} apply={} result={}",
            request.SessionId,
            request.Revision,
            request.ApplyId,
            static_cast<std::uint32_t>(aResult));
    }
}

void TradeService::RememberApplyResult(
    const NotifyTradeApply& acTradeApply,
    TradeApplyResultCode aResult) noexcept
{
    constexpr std::size_t kMaxApplyJournalEntries = 64;

    if (m_applyJournal.size() >=
        kMaxApplyJournalEntries)
    {
        const Trade::ApplyId oldest =
            m_applyJournalOrder.front();
        m_applyJournalOrder.pop_front();
        m_applyJournal.erase(oldest);
    }

    m_applyJournal.insert_or_assign(
        acTradeApply.ApplyId,
        ClientTradeApplyRecord{
            acTradeApply.SessionId,
            acTradeApply.Revision,
            static_cast<std::uint8_t>(aResult)});

    m_applyJournalOrder.push_back(
        acTradeApply.ApplyId);
}
