#pragma once

#include <CampaignTypes.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace STRE::Campaign
{
inline constexpr std::size_t kMaximumCampaignRosterSize = 10;

enum class CampaignPhase : std::uint8_t
{
    Lobby = 0,
    CharacterCreation,
    Arrival,
    Gathering,
    ValenIntroduction,
    ClassSelection,
    ReadyCheck,
    Departure,
    OpenWorld
};

enum class CampaignRuntimeState : std::uint8_t
{
    WAITING_FOR_ROSTER = 0,
    ACTIVE,
    CHECKPOINTING,
    RECOVERY_LOCK,
    RESTORING_CHECKPOINT
};

enum class CampaignError
{
    None,
    InvalidIdentity,
    InvalidRoster,
    RosterLimitExceeded,
    DuplicateSlot,
    DuplicatePlayer,
    DuplicateCharacterBinding,
    SlotNotFound,
    RosterSealed,
    RosterNotSealed,
    InvalidPhase,
    InvalidSessionManager,
    UnauthorizedActor,
    NotCampaignMember,
    RosterIncomplete,
    TransitionNotImplemented,
    CheckpointInProgress,
    CheckpointNotActive,
    CheckpointMismatch,
    InvalidCheckpointArtifact,
    StaleRevision,
    PersistenceFailure,
    IntegrityFailure
};

struct CampaignDomainResult
{
    CampaignError Error{CampaignError::None};
    std::string Message;
    bool Changed{};

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == CampaignError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

struct CampaignSlotState
{
    CampaignSlotId Slot;
    PlayerId Player;
    CharacterBindingId CharacterBinding;
    bool Ready{};

    bool operator==(const CampaignSlotState&) const noexcept = default;
};

struct CampaignAggregate
{
    CampaignId Id;
    StateVersion Version{};
    CampaignPhase Phase{CampaignPhase::Lobby};
    bool RosterSealed{};
    std::optional<PlayerId> SessionManager;
    std::vector<CampaignSlotState> Roster;

    bool operator==(const CampaignAggregate&) const noexcept = default;
};

struct CampaignMemberIdentity
{
    CampaignId Campaign;
    CampaignSlotId Slot;
    PlayerId Player;
    CharacterBindingId CharacterBinding;

    bool operator==(const CampaignMemberIdentity&) const noexcept = default;
};

struct CampaignMemberPresence
{
    CampaignMemberIdentity Identity;
    bool TransportConnected{};
    bool CampaignAdmitted{};
};

enum class FullRosterFailure
{
    None,
    CampaignNotSealed,
    EmptyRoster,
    MissingMember,
    ExtraMember,
    WrongCampaign,
    WrongSlot,
    WrongPlayer,
    WrongCharacterBinding,
    DuplicateActiveIdentity,
    AdmittedWithoutTransport
};

struct FullRosterEvaluation
{
    FullRosterFailure Failure{FullRosterFailure::None};
    std::string Message;

    [[nodiscard]] bool Eligible() const noexcept
    {
        return Failure == FullRosterFailure::None;
    }
};

enum class CampaignActorKind : std::uint8_t
{
    Server,
    Player
};

struct CampaignActor
{
    CampaignActorKind Kind{CampaignActorKind::Server};
    std::optional<PlayerId> Player;

    [[nodiscard]] static CampaignActor Server() noexcept { return {}; }
    [[nodiscard]] static CampaignActor ForPlayer(PlayerId aPlayer)
    {
        return {CampaignActorKind::Player, std::move(aPlayer)};
    }
};

enum class CampaignTransition : std::uint8_t
{
    CommitCampaignStart,
    CompleteCharacterCreation,
    CompleteArrival,
    BeginValenIntroduction,
    CompleteValenIntroduction,
    BeginReadyCheck,
    AuthorizeDeparture,
    EnterOpenWorld
};

enum class CampaignTransitionAuthority : std::uint8_t
{
    Server,
    SessionManager
};

enum class CampaignTransitionIntent : std::uint8_t
{
    CampaignStarted,
    PhaseAdvanced,
    DepartureAuthorized,
    OpenWorldEntered
};

struct CampaignTransitionPolicy
{
    CampaignTransition Transition;
    CampaignPhase Source;
    CampaignPhase Target;
    CampaignTransitionAuthority Authority;
    CampaignTransitionIntent ResultingIntent;
    bool RequiresSealedRoster{};
    bool RequiresFullRoster{};
    bool RequiresAllReady{};
    bool Implemented{};
};

class CampaignStateMachine final
{
public:
    static CampaignDomainResult ValidateRoster(
        const std::vector<CampaignSlotState>& acRoster,
        bool aAllowEmpty);
    static void SortRoster(std::vector<CampaignSlotState>& aRoster) noexcept;

    static CampaignDomainResult ReplaceRoster(
        CampaignAggregate& aCampaign,
        std::vector<CampaignSlotState> aRoster);
    static CampaignDomainResult AddSlot(
        CampaignAggregate& aCampaign,
        CampaignSlotState aSlot);
    static CampaignDomainResult RemoveSlot(
        CampaignAggregate& aCampaign,
        const CampaignSlotId& acSlot);
    static CampaignDomainResult ReplaceSlot(
        CampaignAggregate& aCampaign,
        CampaignSlotState aSlot);
    static CampaignDomainResult CommitCampaignStart(
        CampaignAggregate& aCampaign,
        const CampaignActor& acActor,
        const PlayerId& acSessionManager);
    static CampaignDomainResult TransferSessionManager(
        CampaignAggregate& aCampaign,
        const PlayerId& acActor,
        const PlayerId& acNewManager);
    static CampaignDomainResult SetReady(
        CampaignAggregate& aCampaign,
        const CampaignMemberIdentity& acActor,
        bool aReady);

    static FullRosterEvaluation EvaluateFullRoster(
        const CampaignAggregate& acCampaign,
        const std::vector<CampaignMemberPresence>& acPresence) noexcept;
    static CampaignRuntimeState DetermineRuntimeState(
        const CampaignAggregate& acCampaign,
        const std::vector<CampaignMemberPresence>& acPresence) noexcept;

    static const CampaignTransitionPolicy& GetTransitionPolicy(
        CampaignTransition aTransition) noexcept;
    static CampaignDomainResult EvaluateTransition(
        const CampaignAggregate& acCampaign,
        CampaignTransition aTransition,
        const CampaignActor& acActor,
        const std::vector<CampaignMemberPresence>& acPresence);
};
}
