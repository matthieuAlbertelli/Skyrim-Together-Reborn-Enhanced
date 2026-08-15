#include <CampaignState.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <unordered_set>
#include <utility>

namespace STRE::Campaign
{
namespace
{
CampaignDomainResult Failure(CampaignError aError, std::string aMessage)
{
    return {aError, std::move(aMessage), false};
}

bool IsValidIdentity(const std::string& acValue)
{
    if (acValue.empty() || acValue.size() > 128)
        return false;
    return std::all_of(
        acValue.begin(), acValue.end(), [](unsigned char aCharacter)
        {
            return aCharacter >= 0x21 && aCharacter <= 0x7E;
        });
}

CampaignDomainResult RequireMutableLobby(const CampaignAggregate& acCampaign)
{
    if (acCampaign.RosterSealed)
        return Failure(CampaignError::RosterSealed, "campaign roster is sealed");
    if (acCampaign.Phase != CampaignPhase::Lobby)
    {
        return Failure(
            CampaignError::InvalidPhase,
            "campaign roster can change only during Lobby");
    }
    return {};
}

CampaignDomainResult AdvanceVersion(CampaignAggregate& aCampaign)
{
    if (aCampaign.Version == std::numeric_limits<StateVersion>::max())
    {
        return Failure(
            CampaignError::IntegrityFailure,
            "campaign state version is exhausted");
    }
    ++aCampaign.Version;
    return {CampaignError::None, {}, true};
}

const CampaignSlotState* FindByPlayer(
    const CampaignAggregate& acCampaign,
    const PlayerId& acPlayer)
{
    const auto it = std::find_if(
        acCampaign.Roster.begin(),
        acCampaign.Roster.end(),
        [&acPlayer](const CampaignSlotState& acSlot)
        {
            return acSlot.Player == acPlayer;
        });
    return it == acCampaign.Roster.end() ? nullptr : &*it;
}

const CampaignSlotState* FindBySlot(
    const CampaignAggregate& acCampaign,
    const CampaignSlotId& acSlot)
{
    const auto it = std::find_if(
        acCampaign.Roster.begin(),
        acCampaign.Roster.end(),
        [&acSlot](const CampaignSlotState& acExpected)
        {
            return acExpected.Slot == acSlot;
        });
    return it == acCampaign.Roster.end() ? nullptr : &*it;
}

bool IsRosterMember(
    const CampaignAggregate& acCampaign,
    const PlayerId& acPlayer)
{
    return FindByPlayer(acCampaign, acPlayer) != nullptr;
}

FullRosterEvaluation RosterFailure(
    FullRosterFailure aFailure,
    std::string aMessage)
{
    return {aFailure, std::move(aMessage)};
}
}

CampaignDomainResult CampaignStateMachine::ValidateRoster(
    const std::vector<CampaignSlotState>& acRoster,
    bool aAllowEmpty)
{
    try
    {
        if (acRoster.empty() && !aAllowEmpty)
        {
            return Failure(
                CampaignError::InvalidRoster,
                "campaign roster must contain at least one slot");
        }
        if (acRoster.size() > kMaximumCampaignRosterSize)
        {
            return Failure(
                CampaignError::RosterLimitExceeded,
                "campaign roster exceeds the v1 ten-slot limit");
        }

        std::unordered_set<std::string> slots;
        std::unordered_set<std::string> players;
        std::unordered_set<std::string> bindings;
        for (const CampaignSlotState& slot : acRoster)
        {
            if (!IsValidIdentity(slot.Slot.Value) ||
                !IsValidIdentity(slot.Player.Value) ||
                !IsValidIdentity(slot.CharacterBinding.Value))
            {
                return Failure(
                    CampaignError::InvalidIdentity,
                    "campaign roster contains an empty or invalid durable identity");
            }
            if (!slots.insert(slot.Slot.Value).second)
            {
                return Failure(
                    CampaignError::DuplicateSlot,
                    "campaign roster contains a duplicate CampaignSlotId");
            }
            if (!players.insert(slot.Player.Value).second)
            {
                return Failure(
                    CampaignError::DuplicatePlayer,
                    "campaign roster contains a duplicate PlayerId");
            }
            if (!bindings.insert(slot.CharacterBinding.Value).second)
            {
                return Failure(
                    CampaignError::DuplicateCharacterBinding,
                    "campaign roster contains a duplicate CharacterBindingId");
            }
        }
        return {};
    }
    catch (...)
    {
        return Failure(
            CampaignError::IntegrityFailure,
            "campaign roster validation failed safely");
    }
}

void CampaignStateMachine::SortRoster(
    std::vector<CampaignSlotState>& aRoster) noexcept
{
    std::sort(
        aRoster.begin(),
        aRoster.end(),
        [](const CampaignSlotState& acLeft, const CampaignSlotState& acRight)
        {
            return acLeft.Slot.Value < acRight.Slot.Value;
        });
}

CampaignDomainResult CampaignStateMachine::ReplaceRoster(
    CampaignAggregate& aCampaign,
    std::vector<CampaignSlotState> aRoster)
{
    CampaignDomainResult mutableLobby = RequireMutableLobby(aCampaign);
    if (!mutableLobby)
        return mutableLobby;
    CampaignDomainResult validation = ValidateRoster(aRoster, true);
    if (!validation)
        return validation;

    SortRoster(aRoster);
    for (CampaignSlotState& slot : aRoster)
        slot.Ready = false;
    if (aCampaign.Roster == aRoster)
        return {};

    const std::vector<CampaignSlotState> previous = aCampaign.Roster;
    aCampaign.Roster = std::move(aRoster);
    CampaignDomainResult advanced = AdvanceVersion(aCampaign);
    if (!advanced)
        aCampaign.Roster = previous;
    return advanced;
}

CampaignDomainResult CampaignStateMachine::AddSlot(
    CampaignAggregate& aCampaign,
    CampaignSlotState aSlot)
{
    CampaignDomainResult mutableLobby = RequireMutableLobby(aCampaign);
    if (!mutableLobby)
        return mutableLobby;
    if (aCampaign.Roster.size() >= kMaximumCampaignRosterSize)
    {
        return Failure(
            CampaignError::RosterLimitExceeded,
            "campaign roster exceeds the v1 ten-slot limit");
    }
    for (const CampaignSlotState& existing : aCampaign.Roster)
    {
        if (existing.Slot == aSlot.Slot)
            return Failure(CampaignError::DuplicateSlot, "CampaignSlotId already exists");
        if (existing.Player == aSlot.Player)
            return Failure(CampaignError::DuplicatePlayer, "PlayerId already owns a slot");
        if (existing.CharacterBinding == aSlot.CharacterBinding)
        {
            return Failure(
                CampaignError::DuplicateCharacterBinding,
                "CharacterBindingId already belongs to a slot");
        }
    }
    aSlot.Ready = false;
    std::vector<CampaignSlotState> roster = aCampaign.Roster;
    roster.push_back(std::move(aSlot));
    return ReplaceRoster(aCampaign, std::move(roster));
}

CampaignDomainResult CampaignStateMachine::RemoveSlot(
    CampaignAggregate& aCampaign,
    const CampaignSlotId& acSlot)
{
    CampaignDomainResult mutableLobby = RequireMutableLobby(aCampaign);
    if (!mutableLobby)
        return mutableLobby;
    const auto it = std::find_if(
        aCampaign.Roster.begin(),
        aCampaign.Roster.end(),
        [&acSlot](const CampaignSlotState& acExisting)
        {
            return acExisting.Slot == acSlot;
        });
    if (it == aCampaign.Roster.end())
        return Failure(CampaignError::SlotNotFound, "campaign slot was not found");
    std::vector<CampaignSlotState> roster = aCampaign.Roster;
    roster.erase(roster.begin() + std::distance(aCampaign.Roster.begin(), it));
    return ReplaceRoster(aCampaign, std::move(roster));
}

CampaignDomainResult CampaignStateMachine::ReplaceSlot(
    CampaignAggregate& aCampaign,
    CampaignSlotState aSlot)
{
    CampaignDomainResult mutableLobby = RequireMutableLobby(aCampaign);
    if (!mutableLobby)
        return mutableLobby;
    auto it = std::find_if(
        aCampaign.Roster.begin(),
        aCampaign.Roster.end(),
        [&aSlot](const CampaignSlotState& acExisting)
        {
            return acExisting.Slot == aSlot.Slot;
        });
    if (it == aCampaign.Roster.end())
        return Failure(CampaignError::SlotNotFound, "campaign slot was not found");
    for (const CampaignSlotState& existing : aCampaign.Roster)
    {
        if (existing.Slot == aSlot.Slot)
            continue;
        if (existing.Player == aSlot.Player)
            return Failure(CampaignError::DuplicatePlayer, "PlayerId already owns a slot");
        if (existing.CharacterBinding == aSlot.CharacterBinding)
        {
            return Failure(
                CampaignError::DuplicateCharacterBinding,
                "CharacterBindingId already belongs to a slot");
        }
    }
    aSlot.Ready = false;
    if (*it == aSlot)
        return {};
    std::vector<CampaignSlotState> roster = aCampaign.Roster;
    roster[static_cast<std::size_t>(std::distance(aCampaign.Roster.begin(), it))] =
        std::move(aSlot);
    return ReplaceRoster(aCampaign, std::move(roster));
}

CampaignDomainResult CampaignStateMachine::CommitCampaignStart(
    CampaignAggregate& aCampaign,
    const CampaignActor& acActor,
    const PlayerId& acSessionManager)
{
    if (aCampaign.RosterSealed)
        return Failure(CampaignError::RosterSealed, "campaign roster is sealed");
    CampaignDomainResult transition = EvaluateTransition(
        aCampaign,
        CampaignTransition::CommitCampaignStart,
        acActor,
        {});
    if (!transition)
        return transition;
    CampaignDomainResult roster = ValidateRoster(aCampaign.Roster, false);
    if (!roster)
        return roster;
    if (!IsValidIdentity(acSessionManager.Value) ||
        !IsRosterMember(aCampaign, acSessionManager))
    {
        return Failure(
            CampaignError::InvalidSessionManager,
            "Session Manager must be an expected roster member");
    }

    aCampaign.RosterSealed = true;
    aCampaign.SessionManager = acSessionManager;
    aCampaign.Phase = CampaignPhase::CharacterCreation;
    CampaignDomainResult advanced = AdvanceVersion(aCampaign);
    if (!advanced)
    {
        aCampaign.RosterSealed = false;
        aCampaign.SessionManager.reset();
        aCampaign.Phase = CampaignPhase::Lobby;
    }
    return advanced;
}

CampaignDomainResult CampaignStateMachine::TransferSessionManager(
    CampaignAggregate& aCampaign,
    const PlayerId& acActor,
    const PlayerId& acNewManager)
{
    if (!aCampaign.SessionManager || *aCampaign.SessionManager != acActor)
    {
        return Failure(
            CampaignError::UnauthorizedActor,
            "only the current Session Manager may transfer the role");
    }
    if (!IsRosterMember(aCampaign, acNewManager))
    {
        return Failure(
            CampaignError::InvalidSessionManager,
            "new Session Manager must be an expected roster member");
    }
    if (*aCampaign.SessionManager == acNewManager)
        return {};
    aCampaign.SessionManager = acNewManager;
    return AdvanceVersion(aCampaign);
}

CampaignDomainResult CampaignStateMachine::SetReady(
    CampaignAggregate& aCampaign,
    const CampaignMemberIdentity& acActor,
    bool aReady)
{
    if (!aCampaign.RosterSealed)
    {
        return Failure(
            CampaignError::RosterNotSealed,
            "readiness is available only to sealed campaign members");
    }
    if (acActor.Campaign != aCampaign.Id)
    {
        return Failure(
            CampaignError::NotCampaignMember,
            "ready actor named a different CampaignId");
    }
    auto it = std::find_if(
        aCampaign.Roster.begin(),
        aCampaign.Roster.end(),
        [&acActor](const CampaignSlotState& acSlot)
        {
            return acSlot.Slot == acActor.Slot &&
                acSlot.Player == acActor.Player &&
                acSlot.CharacterBinding == acActor.CharacterBinding;
        });
    if (it == aCampaign.Roster.end())
    {
        return Failure(
            CampaignError::NotCampaignMember,
            "only the exact expected slot owner may change readiness");
    }
    if (it->Ready == aReady)
        return {};
    it->Ready = aReady;
    return AdvanceVersion(aCampaign);
}

FullRosterEvaluation CampaignStateMachine::EvaluateFullRoster(
    const CampaignAggregate& acCampaign,
    const std::vector<CampaignMemberPresence>& acPresence) noexcept
{
    try
    {
        if (!acCampaign.RosterSealed)
        {
            return RosterFailure(
                FullRosterFailure::CampaignNotSealed,
                "campaign roster is not sealed");
        }
        if (acCampaign.Roster.empty())
            return RosterFailure(FullRosterFailure::EmptyRoster, "campaign roster is empty");

        std::unordered_set<std::string> activeSlots;
        std::unordered_set<std::string> activePlayers;
        std::unordered_set<std::string> activeBindings;
        for (const CampaignMemberPresence& presence : acPresence)
        {
            if (!presence.CampaignAdmitted)
                continue;
            if (!presence.TransportConnected)
            {
                return RosterFailure(
                    FullRosterFailure::AdmittedWithoutTransport,
                    "campaign member is admitted without an active transport connection");
            }
            if (presence.Identity.Campaign != acCampaign.Id)
            {
                return RosterFailure(
                    FullRosterFailure::WrongCampaign,
                    "admitted member named a different CampaignId");
            }

            const CampaignSlotState* const pExpectedSlot =
                FindBySlot(acCampaign, presence.Identity.Slot);
            const CampaignSlotState* const pExpectedPlayer =
                FindByPlayer(acCampaign, presence.Identity.Player);
            if (pExpectedSlot && pExpectedSlot->Player != presence.Identity.Player)
            {
                return RosterFailure(
                    FullRosterFailure::WrongPlayer,
                    "admitted member presented the wrong PlayerId for its slot");
            }
            if (!pExpectedSlot && pExpectedPlayer)
            {
                return RosterFailure(
                    FullRosterFailure::WrongSlot,
                    "admitted member presented the wrong CampaignSlotId");
            }
            if (!pExpectedSlot)
            {
                return RosterFailure(
                    FullRosterFailure::ExtraMember,
                    "an extra participant was admitted as a campaign member");
            }
            if (pExpectedSlot->CharacterBinding !=
                presence.Identity.CharacterBinding)
            {
                return RosterFailure(
                    FullRosterFailure::WrongCharacterBinding,
                    "admitted member presented the wrong CharacterBindingId");
            }
            if (!activeSlots.insert(presence.Identity.Slot.Value).second ||
                !activePlayers.insert(presence.Identity.Player.Value).second ||
                !activeBindings.insert(
                    presence.Identity.CharacterBinding.Value).second)
            {
                return RosterFailure(
                    FullRosterFailure::DuplicateActiveIdentity,
                    "an active campaign identity is duplicated");
            }
        }
        if (activeSlots.size() != acCampaign.Roster.size())
        {
            return RosterFailure(
                FullRosterFailure::MissingMember,
                "one or more expected campaign members are missing");
        }
        return {};
    }
    catch (...)
    {
        return RosterFailure(
            FullRosterFailure::DuplicateActiveIdentity,
            "full-roster evaluation failed safely");
    }
}

CampaignRuntimeState CampaignStateMachine::DetermineRuntimeState(
    const CampaignAggregate& acCampaign,
    const std::vector<CampaignMemberPresence>& acPresence) noexcept
{
    return EvaluateFullRoster(acCampaign, acPresence).Eligible()
        ? CampaignRuntimeState::ACTIVE
        : CampaignRuntimeState::WAITING_FOR_ROSTER;
}

const CampaignTransitionPolicy& CampaignStateMachine::GetTransitionPolicy(
    CampaignTransition aTransition) noexcept
{
    static constexpr std::array<CampaignTransitionPolicy, 8> cPolicies{{
        {CampaignTransition::CommitCampaignStart,
         CampaignPhase::Lobby,
         CampaignPhase::CharacterCreation,
         CampaignTransitionAuthority::Server,
         CampaignTransitionIntent::CampaignStarted,
         false,
         false,
         false,
         true},
        {CampaignTransition::CompleteCharacterCreation,
         CampaignPhase::CharacterCreation,
         CampaignPhase::Arrival,
         CampaignTransitionAuthority::Server,
         CampaignTransitionIntent::PhaseAdvanced,
         true,
         true,
         false,
         false},
        {CampaignTransition::CompleteArrival,
         CampaignPhase::Arrival,
         CampaignPhase::Gathering,
         CampaignTransitionAuthority::Server,
         CampaignTransitionIntent::PhaseAdvanced,
         true,
         true,
         false,
         false},
        {CampaignTransition::BeginValenIntroduction,
         CampaignPhase::Gathering,
         CampaignPhase::ValenIntroduction,
         CampaignTransitionAuthority::SessionManager,
         CampaignTransitionIntent::PhaseAdvanced,
         true,
         true,
         false,
         false},
        {CampaignTransition::CompleteValenIntroduction,
         CampaignPhase::ValenIntroduction,
         CampaignPhase::ClassSelection,
         CampaignTransitionAuthority::Server,
         CampaignTransitionIntent::PhaseAdvanced,
         true,
         true,
         false,
         false},
        {CampaignTransition::BeginReadyCheck,
         CampaignPhase::ClassSelection,
         CampaignPhase::ReadyCheck,
         CampaignTransitionAuthority::SessionManager,
         CampaignTransitionIntent::PhaseAdvanced,
         true,
         true,
         false,
         false},
        {CampaignTransition::AuthorizeDeparture,
         CampaignPhase::ReadyCheck,
         CampaignPhase::Departure,
         CampaignTransitionAuthority::SessionManager,
         CampaignTransitionIntent::DepartureAuthorized,
         true,
         true,
         true,
         false},
        {CampaignTransition::EnterOpenWorld,
         CampaignPhase::Departure,
         CampaignPhase::OpenWorld,
         CampaignTransitionAuthority::Server,
         CampaignTransitionIntent::OpenWorldEntered,
         true,
         true,
         false,
         false}}};
    return cPolicies[static_cast<std::size_t>(aTransition)];
}

CampaignDomainResult CampaignStateMachine::EvaluateTransition(
    const CampaignAggregate& acCampaign,
    CampaignTransition aTransition,
    const CampaignActor& acActor,
    const std::vector<CampaignMemberPresence>& acPresence)
{
    const CampaignTransitionPolicy& policy = GetTransitionPolicy(aTransition);
    if (acCampaign.Phase != policy.Source)
        return Failure(CampaignError::InvalidPhase, "transition source phase is invalid");
    if (policy.RequiresSealedRoster && !acCampaign.RosterSealed)
        return Failure(CampaignError::RosterNotSealed, "transition requires a sealed roster");

    if (policy.Authority == CampaignTransitionAuthority::Server)
    {
        if (acActor.Kind != CampaignActorKind::Server)
            return Failure(CampaignError::UnauthorizedActor, "transition requires server authority");
    }
    else
    {
        if (acActor.Kind != CampaignActorKind::Player || !acActor.Player)
            return Failure(CampaignError::UnauthorizedActor, "transition requires a player actor");
        if (!acCampaign.SessionManager ||
            *acCampaign.SessionManager != *acActor.Player)
        {
            return Failure(
                CampaignError::UnauthorizedActor,
                "transition requires the current Session Manager");
        }
    }

    if (policy.RequiresFullRoster &&
        !EvaluateFullRoster(acCampaign, acPresence).Eligible())
    {
        return Failure(
            CampaignError::RosterIncomplete,
            "normal campaign progression requires the exact full roster");
    }
    if (policy.RequiresAllReady &&
        !std::all_of(
            acCampaign.Roster.begin(),
            acCampaign.Roster.end(),
            [](const CampaignSlotState& acSlot) { return acSlot.Ready; }))
    {
        return Failure(
            CampaignError::RosterIncomplete,
            "transition requires every expected slot to be ready");
    }
    if (!policy.Implemented)
    {
        return Failure(
            CampaignError::TransitionNotImplemented,
            "transition policy is reserved for later feature wiring");
    }
    return {};
}
}
