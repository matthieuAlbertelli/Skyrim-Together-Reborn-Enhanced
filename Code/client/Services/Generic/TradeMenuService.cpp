#include <Services/TradeMenuService.h>

#include <Services/ImguiService.h>
#include <Services/TradeService.h>
#include <Services/TransportService.h>

#include <PlayerCharacter.h>
#include <Forms/TESForm.h>
#include <Structs/Inventory.h>
#include <World.h>

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <utility>

namespace
{
struct ResolvedItem
{
    const TESForm* pForm{};
    const char* pName{"Objet inconnu"};
};

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

    if (!gameId)
        return {};

    const TESForm* const pForm = TESForm::GetById(gameId);
    if (!pForm)
        return {};

    const char* const pName = pForm->GetName();

    return ResolvedItem{
        pForm,
        pName && pName[0] != '\0'
            ? pName
            : "Objet sans nom"};
}

bool MatchesCategory(
    TradeMenuService::Category aCategory,
    const TESForm* apForm) noexcept
{
    if (aCategory == TradeMenuService::Category::All)
        return true;

    if (!apForm)
        return aCategory == TradeMenuService::Category::Misc;

    switch (aCategory)
    {
    case TradeMenuService::Category::Weapons:
        return apForm->formType == FormType::Weapon ||
               apForm->formType == FormType::Ammo;
    case TradeMenuService::Category::Armor:
        return apForm->formType == FormType::Armor;
    case TradeMenuService::Category::Consumables:
        return apForm->formType == FormType::Ingredient ||
               apForm->formType == FormType::Alchemy;
    case TradeMenuService::Category::Books:
        return apForm->formType == FormType::Book;
    case TradeMenuService::Category::Misc:
        return apForm->formType != FormType::Weapon &&
               apForm->formType != FormType::Ammo &&
               apForm->formType != FormType::Armor &&
               apForm->formType != FormType::Ingredient &&
               apForm->formType != FormType::Alchemy &&
               apForm->formType != FormType::Book;
    case TradeMenuService::Category::All:
        return true;
    }

    return true;
}

const char* StateLabel(Trade::State aState) noexcept
{
    switch (aState)
    {
    case Trade::State::PendingAcceptance:
        return "Invitation";
    case Trade::State::Negotiating:
        return "Negociation";
    case Trade::State::Locked:
        return "Validation";
    case Trade::State::Applying:
        return "Transfert";
    case Trade::State::Completed:
        return "Termine";
    case Trade::State::Cancelled:
        return "Annule";
    case Trade::State::Failed:
        return "Echec";
    }

    return "Inconnu";
}

const char* ErrorLabel(Trade::Error aError) noexcept
{
    switch (aError)
    {
    case Trade::Error::None:
        return "Aucune";
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

void PushTradeStyle()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 16.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 7.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.04f, 0.045f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.06f, 0.065f, 0.07f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.48f, 0.43f, 0.31f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.23f, 0.18f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.35f, 0.24f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.52f, 0.43f, 0.26f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.30f, 0.27f, 0.20f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.42f, 0.36f, 0.23f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.52f, 0.43f, 0.26f, 1.0f));
}

void PopTradeStyle()
{
    ImGui::PopStyleColor(9);
    ImGui::PopStyleVar(5);
}

void DrawConfirmationBadge(
    const char* apLabel,
    bool aConfirmed)
{
    ImGui::TextUnformatted(apLabel);
    ImGui::SameLine();

    ImGui::PushStyleColor(
        ImGuiCol_Text,
        aConfirmed
            ? ImVec4(0.45f, 0.90f, 0.50f, 1.0f)
            : ImVec4(0.90f, 0.75f, 0.35f, 1.0f));
    ImGui::TextUnformatted(
        aConfirmed ? "CONFIRME" : "EN ATTENTE");
    ImGui::PopStyleColor();
}
}

TradeMenuService::TradeMenuService(
    World& aWorld,
    TransportService& aTransport,
    TradeService& aTradeService,
    ImguiService& aImguiService) noexcept
    : m_world(aWorld)
    , m_transport(aTransport)
    , m_tradeService(aTradeService)
    , m_drawConnection(
          aImguiService.OnDraw.connect<
              &TradeMenuService::OnDraw>(this))
{
}

void TradeMenuService::OnDraw() noexcept
{
    const std::optional<Trade::SessionId> pendingInvite =
        m_tradeService.GetPendingInvite();
    const std::optional<Trade::SessionId> activeSession =
        m_tradeService.GetActiveSession();
    const auto& sessionState = m_tradeService.GetSessionState();

    if (pendingInvite &&
        pendingInvite != m_lastObservedPendingInvite)
    {
        m_visible = true;
        m_dismissedTerminalSession.reset();
    }

    if (activeSession &&
        activeSession != m_lastObservedActiveSession)
    {
        m_visible = true;
        m_dismissedTerminalSession.reset();
        m_selectionSession = activeSession;
        m_selectedItem.reset();
        m_selectedQuantity = 1;
    }

    m_lastObservedPendingInvite = pendingInvite;
    m_lastObservedActiveSession = activeSession;

    const bool hasDisplayableState =
        pendingInvite.has_value() ||
        activeSession.has_value() ||
        sessionState.has_value();

    if ((GetAsyncKeyState(VK_F9) & 0x01) &&
        hasDisplayableState)
    {
        m_visible = !m_visible;
    }

    if (!pendingInvite &&
        !activeSession &&
        !sessionState)
    {
        m_visible = false;
    }

    if (sessionState &&
        IsTerminal(sessionState->State) &&
        m_dismissedTerminalSession == sessionState->SessionId)
    {
        m_visible = false;
    }

    if (!m_visible)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::GetBackgroundDrawList()->AddRectFilled(
        ImVec2(0.0f, 0.0f),
        io.DisplaySize,
        ImColor(0.0f, 0.0f, 0.0f, 0.60f));

    const float width = std::min(1120.0f, io.DisplaySize.x - 60.0f);
    const float height = std::min(760.0f, io.DisplaySize.y - 60.0f);

    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

    PushTradeStyle();

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("Player Trade", nullptr, flags))
    {
        if (!m_transport.IsConnected())
        {
            ImGui::TextUnformatted("Connexion au serveur perdue.");
        }
        else if (pendingInvite)
        {
            DrawInvite();
        }
        else if (sessionState)
        {
            DrawTrade(*sessionState);
        }
        else
        {
            ImGui::TextUnformatted("Synchronisation de l'echange...");
        }
    }

    ImGui::End();
    PopTradeStyle();
}

void TradeMenuService::DrawInvite() noexcept
{
    const auto& invite = m_tradeService.GetPendingInviteState();
    if (!invite)
        return;

    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float panelWidth = std::min(560.0f, available.x);
    const float panelHeight = 270.0f;

    ImGui::SetCursorPosX(
        ImGui::GetCursorPosX() +
        std::max(0.0f, (available.x - panelWidth) * 0.5f));
    ImGui::SetCursorPosY(
        ImGui::GetCursorPosY() +
        std::max(0.0f, (available.y - panelHeight) * 0.40f));

    ImGui::BeginChild(
        "TradeInvite",
        ImVec2(panelWidth, panelHeight),
        true);

    ImGui::SetWindowFontScale(1.25f);
    ImGui::TextUnformatted("PROPOSITION D'ECHANGE");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text(
        "Le joueur %u souhaite echanger avec vous.",
        invite->InviterId);
    ImGui::Spacing();
    ImGui::TextWrapped(
        "Aucun objet ne sera transfere avant la confirmation des deux joueurs.");
    ImGui::TextDisabled(
        "Aucun or n'est utilise dans cet echange.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const float buttonWidth = (panelWidth - 52.0f) * 0.5f;

    if (ImGui::Button(
            "Accepter",
            ImVec2(buttonWidth, 42.0f)))
    {
        m_tradeService.AcceptInvite(invite->SessionId);
    }

    ImGui::SameLine();

    if (ImGui::Button(
            "Refuser",
            ImVec2(buttonWidth, 42.0f)))
    {
        m_tradeService.RejectInvite(invite->SessionId);
    }

    ImGui::EndChild();
}

void TradeMenuService::DrawTrade(
    const ClientTradeSessionState& acState) noexcept
{
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
        ImGui::TextUnformatted(
            "Le joueur local n'appartient pas a cette session.");
        return;
    }

    ImGui::SetWindowFontScale(1.18f);
    ImGui::Text(
        "ECHANGE AVEC LE JOUEUR %u",
        pRemoteParticipant->Id);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::SameLine();
    const float stateWidth = ImGui::CalcTextSize(StateLabel(acState.State)).x;
    ImGui::SetCursorPosX(
        std::max(
            ImGui::GetCursorPosX(),
            ImGui::GetWindowContentRegionMax().x - stateWidth));
    ImGui::TextDisabled("%s", StateLabel(acState.State));

    ImGui::TextDisabled(
        "Echange direct - aucune piece d'or n'est transferee");
    ImGui::Separator();
    ImGui::Spacing();

    const float footerHeight = 105.0f;
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const float contentHeight =
        std::max(220.0f, available.y - footerHeight);
    const float leftWidth =
        std::max(320.0f, available.x * 0.54f);

    ImGui::BeginChild(
        "TradeInventoryColumn",
        ImVec2(leftWidth, contentHeight),
        true);

    if (acState.State == Trade::State::Negotiating)
    {
        DrawInventoryPane(acState, *pLocalParticipant);
    }
    else
    {
        ImGui::TextUnformatted("VOTRE INVENTAIRE");
        ImGui::Separator();
        ImGui::Spacing();

        if (acState.State == Trade::State::Locked)
        {
            ImGui::TextWrapped(
                "Les deux offres sont verrouillees. Le serveur verifie les inventaires.");
        }
        else if (acState.State == Trade::State::Applying)
        {
            ImGui::TextWrapped(
                "Le transfert est en cours. Ne fermez pas le jeu.");
        }
        else if (acState.State == Trade::State::Completed)
        {
            ImGui::TextWrapped(
                "L'echange a ete applique aux deux inventaires.");
        }
        else if (acState.State == Trade::State::Failed)
        {
            ImGui::TextWrapped(
                "L'echange n'a pas pu etre termine.");
            ImGui::Spacing();
            ImGui::TextWrapped(
                "%s",
                ErrorLabel(acState.TerminalError));
        }
        else
        {
            ImGui::TextWrapped("La session n'est plus modifiable.");
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild(
        "TradeOffersColumn",
        ImVec2(0.0f, contentHeight),
        true);
    DrawOffersPane(
        acState,
        *pLocalParticipant,
        *pRemoteParticipant);
    ImGui::EndChild();

    ImGui::Spacing();
    DrawFooter(
        acState,
        *pLocalParticipant,
        *pRemoteParticipant);
}

void TradeMenuService::DrawInventoryPane(
    const ClientTradeSessionState& acState,
    const Trade::Participant& acLocalParticipant) noexcept
{
    if (m_selectionSession != acState.SessionId)
    {
        m_selectionSession = acState.SessionId;
        m_selectedItem.reset();
        m_selectedQuantity = 1;
    }

    ImGui::TextUnformatted("VOTRE INVENTAIRE");
    ImGui::Separator();

    const struct CategoryButton
    {
        Category Value;
        const char* Label;
    } categories[] = {
        {Category::All, "Tout"},
        {Category::Weapons, "Armes"},
        {Category::Armor, "Armures"},
        {Category::Consumables, "Consommables"},
        {Category::Books, "Livres"},
        {Category::Misc, "Divers"}};

    for (std::size_t index = 0;
         index < (sizeof(categories) / sizeof(categories[0]));
         ++index)
    {
        if (index > 0)
            ImGui::SameLine();

        const bool selected =
            m_category == categories[index].Value;

        if (selected)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Button,
                ImVec4(0.52f, 0.43f, 0.26f, 1.0f));
        }

        if (ImGui::Button(categories[index].Label))
            m_category = categories[index].Value;

        if (selected)
            ImGui::PopStyleColor();
    }

    ImGui::Spacing();

    PlayerCharacter* const pPlayer = PlayerCharacter::Get();
    if (!pPlayer)
    {
        ImGui::TextUnformatted("Inventaire local indisponible.");
        return;
    }

    const Inventory inventory = pPlayer->GetInventory();

    const Inventory::Entry* pSelectedEntry{};
    ResolvedItem selectedResolved{};
    bool displayedAny{};

    ImGui::BeginChild(
        "TradeInventoryList",
        ImVec2(0.0f, -120.0f),
        false);

    for (const Inventory::Entry& entry : inventory.Entries)
    {
        if (!IsMvpTransferable(entry))
            continue;

        if (CountLogicalEntries(inventory, entry.BaseId) != 1)
            continue;

        const Trade::ItemId itemId = entry.BaseId.LogFormat();
        const ResolvedItem resolved = ResolveItem(m_world, itemId);

        if (!MatchesCategory(m_category, resolved.pForm))
            continue;

        displayedAny = true;

        const std::uint32_t offeredQuantity =
            FindOfferedQuantity(
                acLocalParticipant.CurrentOffer,
                itemId);

        char label[768]{};
        if (offeredQuantity > 0)
        {
            std::snprintf(
                label,
                sizeof(label),
                "%s     x%d     [offre x%u]",
                resolved.pName,
                entry.Count,
                offeredQuantity);
        }
        else
        {
            std::snprintf(
                label,
                sizeof(label),
                "%s     x%d",
                resolved.pName,
                entry.Count);
        }

        ImGui::PushID(static_cast<int>(entry.BaseId.ModId));
        ImGui::PushID(static_cast<int>(entry.BaseId.BaseId));

        const bool isSelected =
            m_selectedItem && *m_selectedItem == itemId;

        if (ImGui::Selectable(
                label,
                isSelected,
                ImGuiSelectableFlags_None,
                ImVec2(0.0f, 30.0f)))
        {
            m_selectedItem = itemId;
            m_selectedQuantity = offeredQuantity > 0
                ? static_cast<int>(offeredQuantity)
                : 1;
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(resolved.pName);
            ImGui::Text("Possede : %d", entry.Count);
            ImGui::Text("Dans votre offre : %u", offeredQuantity);
            ImGui::EndTooltip();
        }

        if (isSelected)
        {
            pSelectedEntry = &entry;
            selectedResolved = resolved;
        }

        ImGui::PopID();
        ImGui::PopID();
    }

    if (!displayedAny)
    {
        ImGui::TextDisabled(
            "Aucun objet simple et transferable dans cette categorie.");
    }

    ImGui::EndChild();
    ImGui::Separator();

    if (m_selectedItem && !pSelectedEntry)
    {
        for (const Inventory::Entry& entry : inventory.Entries)
        {
            if (entry.BaseId.LogFormat() != *m_selectedItem)
                continue;

            if (!IsMvpTransferable(entry) ||
                CountLogicalEntries(inventory, entry.BaseId) != 1)
            {
                break;
            }

            pSelectedEntry = &entry;
            selectedResolved = ResolveItem(m_world, *m_selectedItem);
            break;
        }
    }

    if (!m_selectedItem || !pSelectedEntry)
    {
        ImGui::TextDisabled(
            "Selectionnez un objet pour l'ajouter a votre offre.");
        return;
    }

    const std::uint32_t offeredQuantity =
        FindOfferedQuantity(
            acLocalParticipant.CurrentOffer,
            *m_selectedItem);

    ImGui::TextUnformatted(selectedResolved.pName);
    ImGui::SameLine();
    ImGui::TextDisabled(
        "possede %d - offre %u",
        pSelectedEntry->Count,
        offeredQuantity);

    ImGui::SetNextItemWidth(110.0f);
    ImGui::InputInt(
        "Quantite",
        &m_selectedQuantity);

    m_selectedQuantity = std::clamp(
        m_selectedQuantity,
        1,
        pSelectedEntry->Count);

    ImGui::SameLine();

    const char* const actionLabel =
        offeredQuantity > 0
            ? "Mettre a jour l'offre"
            : "Ajouter a l'offre";

    if (ImGui::Button(actionLabel))
    {
        SetOfferedQuantity(
            *m_selectedItem,
            static_cast<std::uint32_t>(m_selectedQuantity),
            acLocalParticipant.CurrentOffer);
    }

    if (offeredQuantity > 0)
    {
        ImGui::SameLine();

        if (ImGui::Button("Retirer"))
        {
            SetOfferedQuantity(
                *m_selectedItem,
                0,
                acLocalParticipant.CurrentOffer);
        }
    }
}

void TradeMenuService::DrawOffersPane(
    const ClientTradeSessionState& acState,
    const Trade::Participant& acLocalParticipant,
    const Trade::Participant& acRemoteParticipant) noexcept
{
    const float halfHeight =
        std::max(
            120.0f,
            (ImGui::GetContentRegionAvail().y - 12.0f) * 0.5f);

    ImGui::BeginChild(
        "LocalOffer",
        ImVec2(0.0f, halfHeight),
        false);
    DrawOfferList(
        "VOUS DONNEZ",
        acLocalParticipant.CurrentOffer,
        acState.State == Trade::State::Negotiating);
    ImGui::EndChild();

    ImGui::Separator();

    ImGui::BeginChild(
        "RemoteOffer",
        ImVec2(0.0f, 0.0f),
        false);
    DrawOfferList(
        "VOUS RECEVEZ",
        acRemoteParticipant.CurrentOffer,
        false);
    ImGui::EndChild();
}

void TradeMenuService::DrawOfferList(
    const char* apTitle,
    const Trade::Offer& acOffer,
    bool aAllowRemoval) noexcept
{
    ImGui::TextUnformatted(apTitle);
    ImGui::Separator();
    ImGui::Spacing();

    if (acOffer.Items.empty())
    {
        ImGui::TextDisabled("Aucun objet");
        return;
    }

    for (const Trade::OfferLine& line : acOffer.Items)
    {
        const ResolvedItem resolved = ResolveItem(m_world, line.Item);

        ImGui::PushID(static_cast<int>(line.Item >> 32));
        ImGui::PushID(static_cast<int>(line.Item));

        ImGui::TextWrapped(
            "%s  x%u",
            resolved.pName,
            line.Quantity);

        if (aAllowRemoval)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Retirer"))
            {
                const auto& sessionState =
                    m_tradeService.GetSessionState();

                if (sessionState)
                {
                    const Trade::PlayerId localPlayerId =
                        m_transport.GetLocalPlayerId();

                    const Trade::Offer* pLocalOffer{};
                    if (sessionState->Initiator.Id == localPlayerId)
                        pLocalOffer = &sessionState->Initiator.CurrentOffer;
                    else if (sessionState->Recipient.Id == localPlayerId)
                        pLocalOffer = &sessionState->Recipient.CurrentOffer;

                    if (pLocalOffer)
                        SetOfferedQuantity(line.Item, 0, *pLocalOffer);
                }
            }
        }

        ImGui::PopID();
        ImGui::PopID();
    }
}

void TradeMenuService::DrawFooter(
    const ClientTradeSessionState& acState,
    const Trade::Participant& acLocalParticipant,
    const Trade::Participant& acRemoteParticipant) noexcept
{
    ImGui::Separator();

    const bool localConfirmed =
        IsConfirmedAtCurrentRevision(
            acLocalParticipant,
            acState.Revision);
    const bool remoteConfirmed =
        IsConfirmedAtCurrentRevision(
            acRemoteParticipant,
            acState.Revision);

    DrawConfirmationBadge("Vous :", localConfirmed);
    ImGui::SameLine(260.0f);
    DrawConfirmationBadge("Autre joueur :", remoteConfirmed);

    ImGui::TextDisabled(
        "Toute modification d'une offre annule les confirmations precedentes.");

    if (acState.State == Trade::State::Negotiating)
    {
        const bool emptyTrade =
            acLocalParticipant.CurrentOffer.Items.empty() &&
            acRemoteParticipant.CurrentOffer.Items.empty();

        if (!localConfirmed && !emptyTrade)
        {
            if (ImGui::Button(
                    "Confirmer l'echange",
                    ImVec2(220.0f, 38.0f)))
            {
                m_tradeService.ConfirmTrade();
            }
        }
        else if (localConfirmed)
        {
            ImGui::TextUnformatted(
                "Votre offre est confirmee. En attente de l'autre joueur...");
        }
        else
        {
            ImGui::TextDisabled(
                "Ajoutez au moins un objet a l'une des offres.");
        }

        ImGui::SameLine();
        if (ImGui::Button(
                "Annuler",
                ImVec2(120.0f, 38.0f)))
        {
            m_tradeService.CancelTrade();
        }
    }
    else if (acState.State == Trade::State::Locked)
    {
        ImGui::TextUnformatted(
            "Verification des objets par le serveur...");
        ImGui::SameLine();
        if (ImGui::Button("Annuler"))
            m_tradeService.CancelTrade();
    }
    else if (acState.State == Trade::State::Applying)
    {
        ImGui::TextUnformatted(
            "Application atomique de l'echange aux deux joueurs...");
    }
    else if (IsTerminal(acState.State))
    {
        if (acState.State == Trade::State::Completed)
        {
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                ImVec4(0.45f, 0.90f, 0.50f, 1.0f));
            ImGui::TextUnformatted("Echange termine avec succes.");
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(
                ImGuiCol_Text,
                ImVec4(0.95f, 0.45f, 0.40f, 1.0f));
            ImGui::TextWrapped(
                "%s",
                ErrorLabel(acState.TerminalError));
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();
        if (ImGui::Button("Fermer", ImVec2(120.0f, 36.0f)))
        {
            m_dismissedTerminalSession = acState.SessionId;
            m_visible = false;
        }
    }
}

void TradeMenuService::SetOfferedQuantity(
    Trade::ItemId aItemId,
    std::uint32_t aQuantity,
    const Trade::Offer& acCurrentOffer) noexcept
{
    Trade::Offer updatedOffer = acCurrentOffer;

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
