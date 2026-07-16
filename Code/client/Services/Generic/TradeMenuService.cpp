#include <TiltedOnlinePCH.h>

#include <Services/TradeMenuService.h>

#include <Services/OverlayService.h>
#include <Services/UiSurfaceService.h>
#include <Services/TradeService.h>
#include <Services/TradeItemPreviewService.h>
#include <Services/TransportService.h>

#include <Events/UpdateEvent.h>

#include <PlayerCharacter.h>
#include <Forms/TESForm.h>
#include <Structs/Inventory.h>
#include <World.h>

#include <OverlayApp.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
struct ResolvedItem
{
    const TESForm* pForm{};
    std::string Name{"Objet inconnu"};
    const char* Category{"misc"};
    const char* Icon{"misc"};
    const char* TypeLabel{"Divers"};
};

struct UiInventoryItem
{
    Trade::ItemId Id{};
    std::string Name;
    const char* Category{};
    const char* Icon{};
    const char* TypeLabel{};
    std::uint32_t Quantity{};
    std::uint32_t Offered{};
};

bool IsTerminal(Trade::State aState) noexcept
{
    return aState == Trade::State::Completed ||
           aState == Trade::State::Cancelled ||
           aState == Trade::State::Failed;
}

bool IsConfirmedAtRevision(
    const Trade::Participant& acParticipant,
    Trade::Revision aRevision) noexcept
{
    return acParticipant.Confirmed &&
           acParticipant.ConfirmedRevision == aRevision;
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

std::size_t CountLogicalEntries(
    const Inventory& acInventory,
    const GameId& acItemId) noexcept
{
    return static_cast<std::size_t>(
        std::count_if(
            acInventory.Entries.begin(),
            acInventory.Entries.end(),
            [&acItemId](const Inventory::Entry& acEntry) noexcept
            {
                return acEntry.BaseId == acItemId;
            }));
}

std::uint32_t FindOfferedQuantity(
    const Trade::Offer& acOffer,
    Trade::ItemId aItemId) noexcept
{
    const auto itemIt = std::find_if(
        acOffer.Items.begin(),
        acOffer.Items.end(),
        [aItemId](const Trade::OfferLine& acLine) noexcept
        {
            return acLine.Item == aItemId;
        });

    return itemIt != acOffer.Items.end()
        ? itemIt->Quantity
        : 0;
}

ResolvedItem ResolveItem(
    World& aWorld,
    Trade::ItemId aItemId) noexcept
{
    const GameId serverId{
        static_cast<std::uint32_t>(aItemId >> 32),
        static_cast<std::uint32_t>(aItemId)};

    const std::uint32_t gameId =
        aWorld.GetModSystem().GetGameId(serverId);

    const TESForm* const pForm = gameId
        ? TESForm::GetById(gameId)
        : nullptr;

    ResolvedItem result;
    result.pForm = pForm;

    if (pForm)
    {
        const char* const pName = pForm->GetName();
        if (pName && pName[0] != '\0')
            result.Name = pName;

        switch (pForm->formType)
        {
        case FormType::Weapon:
            result.Category = "weapons";
            result.Icon = "weapon";
            result.TypeLabel = "Arme";
            break;
        case FormType::Ammo:
            result.Category = "weapons";
            result.Icon = "ammo";
            result.TypeLabel = "Munitions";
            break;
        case FormType::Armor:
            result.Category = "armor";
            result.Icon = "armor";
            result.TypeLabel = "Armure";
            break;
        case FormType::Ingredient:
            result.Category = "consumables";
            result.Icon = "ingredient";
            result.TypeLabel = "Ingredient";
            break;
        case FormType::Alchemy:
            result.Category = "consumables";
            result.Icon = "potion";
            result.TypeLabel = "Potion";
            break;
        case FormType::Book:
            result.Category = "books";
            result.Icon = "book";
            result.TypeLabel = "Livre";
            break;
        default:
            break;
        }
    }

    return result;
}

const char* StateName(Trade::State aState) noexcept
{
    switch (aState)
    {
    case Trade::State::PendingAcceptance:
        return "pending";
    case Trade::State::Negotiating:
        return "negotiating";
    case Trade::State::Locked:
        return "locked";
    case Trade::State::Applying:
        return "applying";
    case Trade::State::Completed:
        return "completed";
    case Trade::State::Cancelled:
        return "cancelled";
    case Trade::State::Failed:
        return "failed";
    }

    return "unknown";
}

const char* ErrorLabel(Trade::Error aError) noexcept
{
    switch (aError)
    {
    case Trade::Error::None:
        return "";
    case Trade::Error::InvalidParticipant:
    case Trade::Error::NotParticipant:
    case Trade::Error::NotRecipient:
        return "Participant invalide";
    case Trade::Error::InvalidState:
    case Trade::Error::AlreadyTerminal:
        return "Etat de session invalide";
    case Trade::Error::StaleRevision:
        return "L'offre a ete modifiee";
    case Trade::Error::InvalidItem:
    case Trade::Error::ItemUnavailable:
        return "Objet indisponible";
    case Trade::Error::EmptyQuantity:
    case Trade::Error::InsufficientQuantity:
    case Trade::Error::QuantityOverflow:
        return "Quantite invalide";
    case Trade::Error::DuplicateItem:
    case Trade::Error::AmbiguousItem:
        return "Pile d'objet ambigue";
    case Trade::Error::ItemNotTransferable:
        return "Objet non transferable";
    case Trade::Error::InventoryUnavailable:
        return "Inventaire indisponible";
    case Trade::Error::SessionExpired:
        return "Invitation expiree";
    case Trade::Error::ParticipantDisconnected:
        return "L'autre joueur s'est deconnecte";
    case Trade::Error::ApplyFailed:
    case Trade::Error::ApplyTimedOut:
    case Trade::Error::ServerCommitFailed:
        return "Le transfert n'a pas pu etre applique";
    case Trade::Error::ReconciliationUnavailable:
    case Trade::Error::ReconciliationFailed:
    case Trade::Error::ReconciliationTimedOut:
        return "La verification des inventaires a echoue";
    default:
        return "Erreur technique";
    }
}

void AppendJsonString(
    std::string& aOutput,
    std::string_view aValue)
{
    aOutput.push_back('"');

    for (const unsigned char character : aValue)
    {
        switch (character)
        {
        case '"':
            aOutput += "\\\"";
            break;
        case '\\':
            aOutput += "\\\\";
            break;
        case '\b':
            aOutput += "\\b";
            break;
        case '\f':
            aOutput += "\\f";
            break;
        case '\n':
            aOutput += "\\n";
            break;
        case '\r':
            aOutput += "\\r";
            break;
        case '\t':
            aOutput += "\\t";
            break;
        default:
            if (character < 0x20)
                aOutput += fmt::format("\\u{:04x}", character);
            else
                aOutput.push_back(static_cast<char>(character));
            break;
        }
    }

    aOutput.push_back('"');
}

void AppendBoolean(
    std::string& aOutput,
    bool aValue)
{
    aOutput += aValue ? "true" : "false";
}

void AppendUiItem(
    std::string& aOutput,
    const UiInventoryItem& acItem)
{
    aOutput += "{\"id\":";
    AppendJsonString(aOutput, fmt::format("{}", acItem.Id));
    aOutput += ",\"name\":";
    AppendJsonString(aOutput, acItem.Name);
    aOutput += ",\"category\":";
    AppendJsonString(aOutput, acItem.Category);
    aOutput += ",\"icon\":";
    AppendJsonString(aOutput, acItem.Icon);
    aOutput += ",\"typeLabel\":";
    AppendJsonString(aOutput, acItem.TypeLabel);
    aOutput += fmt::format(
        ",\"quantity\":{},\"offered\":{}",
        acItem.Quantity,
        acItem.Offered);
    aOutput += "}";
}

}

TradeMenuService::TradeMenuService(
    World& aWorld,
    TransportService& aTransport,
    TradeService& aTradeService,
    UiSurfaceService& aUiSurfaceService,
    entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_transport(aTransport)
    , m_tradeService(aTradeService)
    , m_uiSurfaceService(aUiSurfaceService)
    , m_updateConnection(
          aDispatcher.sink<UpdateEvent>().connect<
              &TradeMenuService::OnUpdate>(this))
{
}

void TradeMenuService::AcceptInvite(
    Trade::SessionId aSessionId) noexcept
{
    const auto& invite = m_tradeService.GetPendingInviteState();
    if (!invite || invite->SessionId != aSessionId)
        return;

    m_tradeService.AcceptInvite(aSessionId);
}

void TradeMenuService::RejectInvite(
    Trade::SessionId aSessionId) noexcept
{
    const auto& invite = m_tradeService.GetPendingInviteState();
    if (!invite || invite->SessionId != aSessionId)
        return;

    m_tradeService.RejectInvite(aSessionId);
}

void TradeMenuService::SetOfferedQuantity(
    Trade::ItemId aItemId,
    std::uint32_t aQuantity) noexcept
{
    const auto& state = m_tradeService.GetSessionState();
    if (!state || state->State != Trade::State::Negotiating)
        return;

    const Trade::Participant* const pLocal =
        GetLocalParticipant(*state);
    if (!pLocal)
        return;

    if (aQuantity > 0)
    {
        PlayerCharacter* const pPlayer = PlayerCharacter::Get();
        if (!pPlayer)
            return;

        const Inventory inventory = pPlayer->GetInventory();
        const auto entryIt = std::find_if(
            inventory.Entries.begin(),
            inventory.Entries.end(),
            [aItemId](const Inventory::Entry& acEntry) noexcept
            {
                return acEntry.BaseId.LogFormat() == aItemId;
            });

        if (entryIt == inventory.Entries.end() ||
            !IsMvpTransferable(*entryIt) ||
            CountLogicalEntries(inventory, entryIt->BaseId) != 1 ||
            aQuantity > static_cast<std::uint32_t>(entryIt->Count))
        {
            return;
        }
    }

    Trade::Offer updatedOffer = pLocal->CurrentOffer;
    const auto itemIt = std::find_if(
        updatedOffer.Items.begin(),
        updatedOffer.Items.end(),
        [aItemId](const Trade::OfferLine& acLine) noexcept
        {
            return acLine.Item == aItemId;
        });

    if (aQuantity == 0)
    {
        if (itemIt != updatedOffer.Items.end())
            updatedOffer.Items.erase(itemIt);
    }
    else if (itemIt != updatedOffer.Items.end())
    {
        itemIt->Quantity = aQuantity;
    }
    else
    {
        updatedOffer.Items.push_back(
            Trade::OfferLine{aItemId, aQuantity});
    }

    m_tradeService.UpdateOffer(std::move(updatedOffer));
}

void TradeMenuService::ConfirmTrade() noexcept
{
    m_tradeService.ConfirmTrade();
}

void TradeMenuService::CancelTrade() noexcept
{
    m_tradeService.CancelTrade();
}

void TradeMenuService::DismissTerminalState() noexcept
{
    const auto& state = m_tradeService.GetSessionState();
    if (!state || !IsTerminal(state->State))
        return;

    m_dismissedTerminalSession = state->SessionId;
    SetVisible(false);
    PushState(true);
}

void TradeMenuService::DismissOutgoingInvite() noexcept
{
    if (!m_tradeService.GetOutgoingInviteTarget())
        return;

    m_tradeService.DismissOutgoingInvite();
    SetVisible(false);
    PushState(true);
}

void TradeMenuService::OnUpdate(
    const UpdateEvent& acEvent) noexcept
{
    m_refreshAccumulator += acEvent.Delta;

    const auto outgoingInviteTarget =
        m_tradeService.GetOutgoingInviteTarget();
    const auto pendingInvite = m_tradeService.GetPendingInvite();
    const auto activeSession = m_tradeService.GetActiveSession();
    const auto& state = m_tradeService.GetSessionState();

    bool forcePush{};

    if (outgoingInviteTarget &&
        outgoingInviteTarget != m_lastOutgoingInviteTarget)
    {
        m_dismissedTerminalSession.reset();
        SetVisible(true);
        forcePush = true;
    }

    if (pendingInvite && pendingInvite != m_lastPendingInvite)
    {
        m_dismissedTerminalSession.reset();
        SetVisible(true);
        forcePush = true;
    }

    if (activeSession && activeSession != m_lastActiveSession)
    {
        m_dismissedTerminalSession.reset();
        SetVisible(true);
        forcePush = true;
    }

    if (state &&
        IsTerminal(state->State) &&
        m_dismissedTerminalSession != state->SessionId)
    {
        SetVisible(true);
    }

    const bool hasDisplayableState =
        outgoingInviteTarget.has_value() ||
        pendingInvite.has_value() ||
        activeSession.has_value() ||
        (state.has_value() &&
         m_dismissedTerminalSession != state->SessionId);

    if (!hasDisplayableState)
        SetVisible(false);

    m_lastOutgoingInviteTarget = outgoingInviteTarget;
    m_lastPendingInvite = pendingInvite;
    m_lastActiveSession = activeSession;

    if (forcePush || m_refreshAccumulator >= 0.10)
    {
        m_refreshAccumulator = 0.0;
        PushState(forcePush);
    }
}

void TradeMenuService::SetVisible(bool aVisible) noexcept
{
    if (m_visible == aVisible)
        return;

    m_visible = aVisible;

    if (aVisible)
    {
        m_previousSurface = m_uiSurfaceService.GetSurface();
        m_uiSurfaceService.SetSurface(UiSurface::Trade);
        return;
    }

    m_world.ctx().at<TradeItemPreviewService>().Clear();

    if (m_uiSurfaceService.GetSurface() == UiSurface::Trade)
    {
        const UiSurface restoreSurface =
            m_previousSurface == UiSurface::SkyrimTogether
                ? UiSurface::SkyrimTogether
                : UiSurface::None;

        m_uiSurfaceService.SetSurface(restoreSurface);
    }

    m_previousSurface = UiSurface::None;
}

void TradeMenuService::PushState(bool aForce) noexcept
{
    const auto pOverlay =
        m_uiSurfaceService.GetOverlayService().GetOverlayApp();
    if (!pOverlay)
        return;

    const std::string stateJson = BuildStateJson();
    if (!aForce && stateJson == m_lastStateJson)
        return;

    m_lastStateJson = stateJson;

    // Use a dedicated UI event. Reusing dummyData competes with the stock
    // ClientService listener and can leave TradeUiService without the state.
    auto arguments = CefListValue::Create();
    arguments->SetString(0, stateJson);
    pOverlay->ExecuteAsync("tradeState", arguments);
}

std::string TradeMenuService::BuildStateJson() const
{
    std::string output;
    output.reserve(8192);

    output += "{\"visible\":";
    AppendBoolean(output, m_visible);

    if (!m_visible)
    {
        output += "}";
        return output;
    }

    const auto& invite = m_tradeService.GetPendingInviteState();
    if (invite)
    {
        output += ",\"mode\":\"invite\",\"sessionId\":";
        AppendJsonString(output, fmt::format("{}", invite->SessionId));
        output += fmt::format(
            ",\"inviterId\":{}",
            invite->InviterId);
        output += "}";
        return output;
    }

    const auto outgoingInviteTarget =
        m_tradeService.GetOutgoingInviteTarget();
    if (outgoingInviteTarget)
    {
        output += fmt::format(
            ",\"mode\":\"outgoing\",\"targetPlayerId\":{}",
            *outgoingInviteTarget);
        output += "}";
        return output;
    }

    const auto activeSession = m_tradeService.GetActiveSession();
    const auto& state = m_tradeService.GetSessionState();
    if (!state ||
        (activeSession && state->SessionId != *activeSession))
    {
        output += ",\"mode\":\"syncing\"}";
        return output;
    }

    const Trade::Participant* const pLocal =
        GetLocalParticipant(*state);
    const Trade::Participant* const pRemote =
        GetRemoteParticipant(*state);

    if (!pLocal || !pRemote)
    {
        output += ",\"mode\":\"error\",\"error\":\"Session locale invalide\"}";
        return output;
    }

    output += ",\"mode\":\"session\",\"phase\":";
    AppendJsonString(output, StateName(state->State));
    output += ",\"sessionId\":";
    AppendJsonString(output, fmt::format("{}", state->SessionId));
    output += fmt::format(
        ",\"localPlayerId\":{},\"remotePlayerId\":{}",
        pLocal->Id,
        pRemote->Id);

    output += ",\"localConfirmed\":";
    AppendBoolean(
        output,
        IsConfirmedAtRevision(*pLocal, state->Revision));
    output += ",\"remoteConfirmed\":";
    AppendBoolean(
        output,
        IsConfirmedAtRevision(*pRemote, state->Revision));
    output += ",\"mutable\":";
    AppendBoolean(output, state->State == Trade::State::Negotiating);
    output += ",\"error\":";
    AppendJsonString(output, ErrorLabel(state->TerminalError));

    std::vector<UiInventoryItem> inventoryItems;

    if (PlayerCharacter* const pPlayer = PlayerCharacter::Get())
    {
        const Inventory inventory = pPlayer->GetInventory();
        inventoryItems.reserve(inventory.Entries.size());

        for (const Inventory::Entry& entry : inventory.Entries)
        {
            if (!IsMvpTransferable(entry) ||
                CountLogicalEntries(inventory, entry.BaseId) != 1)
            {
                continue;
            }

            const Trade::ItemId itemId = entry.BaseId.LogFormat();
            const ResolvedItem resolved = ResolveItem(m_world, itemId);

            inventoryItems.push_back(
                UiInventoryItem{
                    itemId,
                    resolved.Name,
                    resolved.Category,
                    resolved.Icon,
                    resolved.TypeLabel,
                    static_cast<std::uint32_t>(entry.Count),
                    FindOfferedQuantity(
                        pLocal->CurrentOffer,
                        itemId)});
        }

        std::sort(
            inventoryItems.begin(),
            inventoryItems.end(),
            [](const UiInventoryItem& acLeft,
               const UiInventoryItem& acRight)
            {
                return acLeft.Name < acRight.Name;
            });
    }

    output += ",\"inventory\":[";
    for (std::size_t index = 0;
         index < inventoryItems.size();
         ++index)
    {
        if (index > 0)
            output.push_back(',');

        AppendUiItem(output, inventoryItems[index]);
    }
    output += "]";

    const auto appendOffer =
        [this, &output](
            const char* apProperty,
            const Trade::Offer& acOffer)
        {
            output += ",\"";
            output += apProperty;
            output += "\":[";

            for (std::size_t index = 0;
                 index < acOffer.Items.size();
                 ++index)
            {
                if (index > 0)
                    output.push_back(',');

                const Trade::OfferLine& line = acOffer.Items[index];
                const ResolvedItem resolved =
                    ResolveItem(m_world, line.Item);

                AppendUiItem(
                    output,
                    UiInventoryItem{
                        line.Item,
                        resolved.Name,
                        resolved.Category,
                        resolved.Icon,
                        resolved.TypeLabel,
                        line.Quantity,
                        line.Quantity});
            }

            output += "]";
        };

    appendOffer("localOffer", pLocal->CurrentOffer);
    appendOffer("remoteOffer", pRemote->CurrentOffer);

    output += "}";
    return output;
}

const Trade::Participant* TradeMenuService::GetLocalParticipant(
    const ClientTradeSessionState& acState) const noexcept
{
    const Trade::PlayerId localPlayerId =
        m_transport.GetLocalPlayerId();

    if (acState.Initiator.Id == localPlayerId)
        return &acState.Initiator;

    if (acState.Recipient.Id == localPlayerId)
        return &acState.Recipient;

    return nullptr;
}

const Trade::Participant* TradeMenuService::GetRemoteParticipant(
    const ClientTradeSessionState& acState) const noexcept
{
    const Trade::PlayerId localPlayerId =
        m_transport.GetLocalPlayerId();

    if (acState.Initiator.Id == localPlayerId)
        return &acState.Recipient;

    if (acState.Recipient.Id == localPlayerId)
        return &acState.Initiator;

    return nullptr;
}
