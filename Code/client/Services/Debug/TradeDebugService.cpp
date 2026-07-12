#include <Services/TradeDebugService.h>

#include <Services/ImguiService.h>
#include <Services/TradeService.h>
#include <Services/TransportService.h>

#include <PlayerCharacter.h>
#include <Forms/TESForm.h>
#include <Structs/Inventory.h>
#include <World.h>

#include <imgui.h>

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <limits>
#include <utility>

namespace
{
const char* ToString(Trade::State aState) noexcept
{
    switch (aState)
    {
    case Trade::State::PendingAcceptance:
        return "Pending acceptance";
    case Trade::State::Negotiating:
        return "Negotiating";
    case Trade::State::Locked:
        return "Locked";
    case Trade::State::Applying:
        return "Applying";
    case Trade::State::Completed:
        return "Completed";
    case Trade::State::Cancelled:
        return "Cancelled";
    case Trade::State::Failed:
        return "Failed";
    }

    return "Unknown";
}

const char* ToString(Trade::Error aError) noexcept
{
    switch (aError)
    {
    case Trade::Error::None:
        return "None";
    case Trade::Error::InvalidParticipant:
        return "Invalid participant";
    case Trade::Error::NotParticipant:
        return "Not a participant";
    case Trade::Error::NotRecipient:
        return "Not the recipient";
    case Trade::Error::InvalidState:
        return "Invalid state";
    case Trade::Error::AlreadyTerminal:
        return "Already terminal";
    case Trade::Error::StaleRevision:
        return "Stale revision";
    case Trade::Error::InvalidItem:
        return "Invalid item";
    case Trade::Error::EmptyQuantity:
        return "Empty quantity";
    case Trade::Error::DuplicateItem:
        return "Duplicate item";
    case Trade::Error::RevisionOverflow:
        return "Revision overflow";
    case Trade::Error::SessionExpired:
        return "Session expired";
    case Trade::Error::InventoryUnavailable:
        return "Inventory unavailable";
    case Trade::Error::ItemUnavailable:
        return "Item unavailable";
    case Trade::Error::InsufficientQuantity:
        return "Insufficient quantity";
    case Trade::Error::ItemNotTransferable:
        return "Item not transferable";
    case Trade::Error::AmbiguousItem:
        return "Ambiguous item";
    case Trade::Error::QuantityOverflow:
        return "Quantity overflow";
    case Trade::Error::ApplyResultConflict:
        return "Apply result conflict";
    case Trade::Error::ApplyFailed:
        return "Apply failed";
    case Trade::Error::ApplyTimedOut:
        return "Apply timed out";
    case Trade::Error::ParticipantDisconnected:
        return "Participant disconnected";
    case Trade::Error::ServerCommitFailed:
        return "Server commit failed";
    case Trade::Error::ReconcileResultConflict:
        return "Reconcile result conflict";
    case Trade::Error::ReconciliationUnavailable:
        return "Reconciliation unavailable";
    case Trade::Error::ReconciliationFailed:
        return "Reconciliation failed";
    case Trade::Error::ReconciliationTimedOut:
        return "Reconciliation timed out";
    }

    return "Unknown";
}

bool IsTerminal(Trade::State aState) noexcept
{
    return aState == Trade::State::Completed ||
           aState == Trade::State::Cancelled ||
           aState == Trade::State::Failed;
}

bool IsConfirmedAtCurrentRevision(
    const Trade::Participant& acParticipant,
    Trade::Revision aRevision) noexcept
{
    return acParticipant.Confirmed &&
           acParticipant.ConfirmedRevision == aRevision;
}

bool IsMvpTransferable(
    const Inventory::Entry& acEntry) noexcept
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

std::size_t CountLogicalEntries(
    const Inventory& acInventory,
    const GameId& acItemId) noexcept
{
    return static_cast<std::size_t>(
        std::count_if(
            acInventory.Entries.begin(),
            acInventory.Entries.end(),
            [&acItemId](
                const Inventory::Entry& acEntry) noexcept
            {
                return acEntry.BaseId == acItemId;
            }));
}

const char* ResolveItemName(
    World& aWorld,
    Trade::ItemId aItemId) noexcept
{
    const GameId serverId{
        static_cast<std::uint32_t>(aItemId >> 32),
        static_cast<std::uint32_t>(aItemId)};

    const std::uint32_t gameId =
        aWorld.GetModSystem().GetGameId(serverId);

    if (!gameId)
        return "<unresolved item>";

    const TESForm* const pForm =
        TESForm::GetById(gameId);

    if (!pForm)
        return "<missing form>";

    const char* const pName = pForm->GetName();

    return pName && pName[0] != '\0'
        ? pName
        : "<unnamed item>";
}

void DrawOffer(
    World& aWorld,
    const char* apLabel,
    const Trade::Offer& acOffer)
{
    ImGui::TextUnformatted(apLabel);

    if (acOffer.Items.empty())
    {
        ImGui::BulletText("<empty>");
        return;
    }

    for (const Trade::OfferLine& line :
         acOffer.Items)
    {
        const std::uint32_t modId =
            static_cast<std::uint32_t>(line.Item >> 32);
        const std::uint32_t baseId =
            static_cast<std::uint32_t>(line.Item);

        ImGui::BulletText(
            "%s | %08X:%08X | x %u",
            ResolveItemName(aWorld, line.Item),
            modId,
            baseId,
            line.Quantity);
    }
}
}

TradeDebugService::TradeDebugService(
    World& aWorld,
    TransportService& aTransport,
    TradeService& aTradeService,
    ImguiService& aImguiService) noexcept
    : m_world(aWorld)
    , m_transport(aTransport)
    , m_tradeService(aTradeService)
    , m_drawConnection(
          aImguiService.OnDraw.connect<
              &TradeDebugService::OnDraw>(this))
{
}

void TradeDebugService::OnDraw() noexcept
{
    if (GetAsyncKeyState(VK_F10) & 0x01)
        m_visible = !m_visible;

    const std::optional<Trade::SessionId> pendingInvite =
        m_tradeService.GetPendingInvite();
    const std::optional<Trade::SessionId> activeSession =
        m_tradeService.GetActiveSession();

    if (pendingInvite &&
        pendingInvite != m_lastObservedPendingInvite)
    {
        m_visible = true;
    }

    if (activeSession &&
        activeSession != m_lastObservedActiveSession)
    {
        m_visible = true;
    }

    m_lastObservedPendingInvite = pendingInvite;
    m_lastObservedActiveSession = activeSession;

    if (!m_visible)
        return;

    ImGui::SetNextWindowSize(
        ImVec2(720.0f, 650.0f),
        ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(
            "Player Trade Debug (F10)",
            &m_visible))
    {
        ImGui::End();
        return;
    }

    if (!m_transport.IsConnected())
    {
        ImGui::TextUnformatted(
            "The client is not connected.");
        ImGui::End();
        return;
    }

    DrawInviteSection();

    const auto& sessionState =
        m_tradeService.GetSessionState();

    if (sessionState)
        DrawSession(*sessionState);
    else if (activeSession)
        ImGui::TextUnformatted(
            "Waiting for the authoritative trade snapshot.");
    else
        ImGui::TextUnformatted(
            "No synchronized trade session.");

    ImGui::End();
}

void TradeDebugService::DrawInviteSection() noexcept
{
    ImGui::TextUnformatted("Invitation controls");
    ImGui::Text(
        "Local player id: %u",
        m_transport.GetLocalPlayerId());

    ImGui::SetNextItemWidth(140.0f);
    ImGui::InputInt(
        "Target player id",
        &m_inviteTargetPlayerId);

    if (m_inviteTargetPlayerId > 0 &&
        ImGui::Button("Send invite"))
    {
        m_tradeService.InvitePlayer(
            static_cast<std::uint32_t>(
                m_inviteTargetPlayerId));
    }

    const std::optional<Trade::SessionId> pendingInvite =
        m_tradeService.GetPendingInvite();

    if (pendingInvite)
    {
        ImGui::Separator();
        ImGui::Text(
            "Pending invitation: session %" PRIu64,
            *pendingInvite);

        if (ImGui::Button("Accept invitation"))
            m_tradeService.AcceptInvite(*pendingInvite);

        ImGui::SameLine();

        if (ImGui::Button("Reject invitation"))
            m_tradeService.RejectInvite(*pendingInvite);
    }

    ImGui::Separator();
}

void TradeDebugService::DrawSession(
    const ClientTradeSessionState& acState) noexcept
{
    ImGui::Text(
        "Session: %" PRIu64,
        acState.SessionId);
    ImGui::Text(
        "Revision: %" PRIu64,
        acState.Revision);
    ImGui::Text(
        "State: %s",
        ToString(acState.State));

    if (acState.TerminalError != Trade::Error::None)
    {
        ImGui::Text(
            "Terminal error: %s (%u)",
            ToString(acState.TerminalError),
            static_cast<std::uint32_t>(
                acState.TerminalError));
    }

    const Trade::PlayerId localPlayerId =
        m_transport.GetLocalPlayerId();

    const Trade::Participant* pLocalParticipant{};
    const Trade::Participant* pRemoteParticipant{};

    if (acState.Initiator.Id == localPlayerId)
    {
        pLocalParticipant = &acState.Initiator;
        pRemoteParticipant = &acState.Recipient;
    }
    else if (acState.Recipient.Id == localPlayerId)
    {
        pLocalParticipant = &acState.Recipient;
        pRemoteParticipant = &acState.Initiator;
    }

    if (!pLocalParticipant || !pRemoteParticipant)
    {
        ImGui::Text(
            "Local player %u is not a session participant.",
            localPlayerId);
        return;
    }

    ImGui::Text(
        "Local player: %u",
        pLocalParticipant->Id);
    ImGui::Text(
        "Other player: %u",
        pRemoteParticipant->Id);

    DrawOffers(
        acState,
        *pLocalParticipant,
        *pRemoteParticipant);

    if (acState.State == Trade::State::Negotiating)
    {
        DrawInventory(
            acState,
            *pLocalParticipant);

        const bool localConfirmed =
            IsConfirmedAtCurrentRevision(
                *pLocalParticipant,
                acState.Revision);

        if (!localConfirmed)
        {
            if (ImGui::Button("Confirm current revision"))
                m_tradeService.ConfirmTrade();
        }
        else
        {
            ImGui::TextUnformatted(
                "You confirmed this revision.");
        }
    }
    else if (acState.State == Trade::State::Locked)
    {
        ImGui::TextUnformatted(
            "Both players confirmed. Server validation is running.");
    }
    else if (acState.State == Trade::State::Applying)
    {
        ImGui::TextUnformatted(
            "The transaction is being applied.");
    }

    if (!IsTerminal(acState.State) &&
        m_tradeService.GetActiveSession())
    {
        ImGui::SameLine();

        if (ImGui::Button("Cancel trade"))
            m_tradeService.CancelTrade();
    }
}

void TradeDebugService::DrawOffers(
    const ClientTradeSessionState& acState,
    const Trade::Participant& acLocalParticipant,
    const Trade::Participant& acRemoteParticipant) noexcept
{
    ImGui::Separator();

    DrawOffer(
        m_world,
        "Your offer",
        acLocalParticipant.CurrentOffer);

    ImGui::Text(
        "Your confirmation: %s",
        IsConfirmedAtCurrentRevision(
            acLocalParticipant,
            acState.Revision)
            ? "confirmed"
            : "pending");

    ImGui::Spacing();

    DrawOffer(
        m_world,
        "Other player's offer",
        acRemoteParticipant.CurrentOffer);

    ImGui::Text(
        "Other confirmation: %s",
        IsConfirmedAtCurrentRevision(
            acRemoteParticipant,
            acState.Revision)
            ? "confirmed"
            : "pending");

    ImGui::Separator();
}

void TradeDebugService::DrawInventory(
    const ClientTradeSessionState& acState,
    const Trade::Participant& acLocalParticipant) noexcept
{
    if (m_draftSession != acState.SessionId ||
        m_draftRevision != acState.Revision)
    {
        m_draftSession = acState.SessionId;
        m_draftRevision = acState.Revision;
        m_quantityDrafts.clear();
    }

    PlayerCharacter* const pPlayer =
        PlayerCharacter::Get();

    if (!pPlayer)
    {
        ImGui::TextUnformatted(
            "Local PlayerCharacter is unavailable.");
        return;
    }

    const Inventory inventory =
        pPlayer->GetInventory();

    ImGui::TextUnformatted(
        "Transferable local inventory");
    ImGui::TextDisabled(
        "Only simple, unequipped, non-quest stacks are shown.");

    bool displayedAny{};

    for (const Inventory::Entry& entry :
         inventory.Entries)
    {
        if (!IsMvpTransferable(entry))
            continue;

        displayedAny = true;

        const Trade::ItemId itemId =
            entry.BaseId.LogFormat();
        const bool ambiguous =
            CountLogicalEntries(
                inventory,
                entry.BaseId) > 1;

        const std::uint32_t offeredQuantity =
            FindOfferedQuantity(
                acLocalParticipant.CurrentOffer,
                itemId);

        auto [draftIt, inserted] =
            m_quantityDrafts.try_emplace(
                itemId,
                offeredQuantity > 0
                    ? static_cast<int>(offeredQuantity)
                    : 1);

        TP_UNUSED(inserted);

        ImGui::PushID(
            static_cast<int>(entry.BaseId.ModId));
        ImGui::PushID(
            static_cast<int>(entry.BaseId.BaseId));

        ImGui::Text(
            "%s | %08X:%08X | owned %d | offered %u",
            ResolveItemName(m_world, itemId),
            entry.BaseId.ModId,
            entry.BaseId.BaseId,
            entry.Count,
            offeredQuantity);

        if (ambiguous)
        {
            ImGui::SameLine();
            ImGui::TextDisabled(
                "ambiguous stack");
            ImGui::PopID();
            ImGui::PopID();
            continue;
        }

        ImGui::SetNextItemWidth(90.0f);
        ImGui::InputInt(
            "##quantity",
            &draftIt->second);

        if (draftIt->second < 1)
            draftIt->second = 1;

        if (draftIt->second > entry.Count)
            draftIt->second = entry.Count;

        ImGui::SameLine();

        if (ImGui::Button("Set offer"))
        {
            SetOfferedQuantity(
                itemId,
                static_cast<std::uint32_t>(
                    draftIt->second),
                acLocalParticipant.CurrentOffer);
        }

        if (offeredQuantity > 0)
        {
            ImGui::SameLine();

            if (ImGui::Button("Remove"))
            {
                SetOfferedQuantity(
                    itemId,
                    0,
                    acLocalParticipant.CurrentOffer);
            }
        }

        ImGui::PopID();
        ImGui::PopID();
    }

    if (!displayedAny)
    {
        ImGui::BulletText(
            "<no transferable entries>");
    }

    ImGui::Separator();
}

void TradeDebugService::SetOfferedQuantity(
    Trade::ItemId aItemId,
    std::uint32_t aQuantity,
    const Trade::Offer& acCurrentOffer) noexcept
{
    Trade::Offer updatedOffer =
        acCurrentOffer;

    const auto itemIt = std::find_if(
        updatedOffer.Items.begin(),
        updatedOffer.Items.end(),
        [aItemId](
            const Trade::OfferLine& acLine) noexcept
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
            Trade::OfferLine{
                aItemId,
                aQuantity});
    }

    m_tradeService.UpdateOffer(
        std::move(updatedOffer));
}
