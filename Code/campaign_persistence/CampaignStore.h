#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace STRE::Campaign
{
inline constexpr std::uint32_t kCampaignDatabaseSchemaVersion = 1;
inline constexpr std::uint32_t kCampaignSnapshotCodecVersion = 1;

using Bytes = std::vector<std::uint8_t>;
using StateVersion = std::uint64_t;

template <class Tag> struct DurableId
{
    std::string Value;

    bool operator==(const DurableId&) const noexcept = default;
};

struct CampaignIdTag;
struct CampaignSlotIdTag;
struct PlayerIdTag;
struct CharacterBindingIdTag;
struct CheckpointIdTag;
struct SnapshotIdTag;
struct MutationIdTag;

using CampaignId = DurableId<CampaignIdTag>;
using CampaignSlotId = DurableId<CampaignSlotIdTag>;
using PlayerId = DurableId<PlayerIdTag>;
using CharacterBindingId = DurableId<CharacterBindingIdTag>;
using CheckpointId = DurableId<CheckpointIdTag>;
using SnapshotId = DurableId<SnapshotIdTag>;
using MutationId = DurableId<MutationIdTag>;

enum class StoreError
{
    None,
    InvalidArgument,
    NotFound,
    AlreadyExists,
    StaleRevision,
    IdempotencyConflict,
    IntegrityFailure,
    IncompatibleSchema,
    MigrationFailure,
    DatabaseFailure,
    FaultInjected
};

struct StoreResult
{
    StoreError Error{StoreError::None};
    std::string Message;

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == StoreError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

template <class T> struct StoreValueResult : StoreResult
{
    T Value{};
};

struct MutationResult : StoreResult
{
    StateVersion Revision{};
    bool Applied{};
    bool IdempotentReplay{};
};

struct FormId
{
    std::uint32_t ModId{};
    std::uint32_t BaseId{};

    bool operator==(const FormId&) const noexcept = default;
};

struct CharacterBuildSelection
{
    std::string GroupId;
    std::string OptionId;

    bool operator==(const CharacterBuildSelection&) const noexcept = default;
};

struct InventoryEffect
{
    float Magnitude{};
    std::int32_t Area{};
    std::int32_t Duration{};
    float RawCost{};
    FormId EffectId{};

    bool operator==(const InventoryEffect&) const noexcept = default;
};

struct InventoryEntry
{
    FormId BaseId{};
    std::int32_t Count{};
    float ExtraCharge{};
    FormId ExtraEnchantId{};
    std::uint16_t ExtraEnchantCharge{};
    bool EnchantmentIsWeapon{};
    std::vector<InventoryEffect> EnchantmentEffects;
    float ExtraHealth{};
    FormId ExtraPoisonId{};
    std::uint32_t ExtraPoisonCount{};
    std::int32_t ExtraSoulLevel{};
    FormId ExtraOwnerId{};
    bool ExtraEnchantRemoveUnequip{};
    bool ExtraWorn{};
    bool ExtraWornLeft{};
    bool IsQuestItem{};

    bool operator==(const InventoryEntry&) const noexcept = default;
};

struct CharacterBuildState
{
    CampaignSlotId Slot;
    CharacterBindingId CharacterBinding;
    std::uint32_t PersistenceCodecVersion{1};
    std::uint32_t BuildVersion{};
    FormId RaceId{};
    std::string ClassId;
    std::vector<CharacterBuildSelection> Selections;
    std::vector<InventoryEntry> CanonicalInventory;
    FormId LeftHandSpell{};
    FormId RightHandSpell{};
    FormId Shout{};
    std::uint64_t InventoryHash{};
    std::vector<FormId> CanonicalSpells;
    std::uint64_t SpellHash{};
    bool Applied{};
    StateVersion UpdatedRevision{};

    bool operator==(const CharacterBuildState&) const noexcept = default;
};

enum class StateAudience : std::uint8_t
{
    Public = 0,
    Private = 1
};

struct AdapterState
{
    std::string AdapterId;
    StateVersion AdapterVersion{};
    std::uint32_t CodecVersion{};
    StateAudience Audience{StateAudience::Public};
    std::optional<PlayerId> AudiencePlayer;
    Bytes Payload;
    StateVersion UpdatedRevision{};

    bool operator==(const AdapterState&) const noexcept = default;
};

struct CampaignSlotRecord
{
    CampaignSlotId Slot;
    PlayerId Player;
    CharacterBindingId CharacterBinding;

    bool operator==(const CampaignSlotRecord&) const noexcept = default;
};

struct CampaignRecord
{
    CampaignId Id;
    std::uint32_t PersistenceSchemaVersion{kCampaignDatabaseSchemaVersion};
    StateVersion CurrentRevision{};
    bool RosterSealed{};
    std::optional<CheckpointId> LastCommittedCheckpoint;
    std::uint32_t CoreStateCodecVersion{1};
    Bytes CoreStatePayload;
    std::int64_t CreatedAtUnixMs{};
    std::int64_t UpdatedAtUnixMs{};

    bool operator==(const CampaignRecord&) const noexcept = default;
};

struct CampaignProjection
{
    CampaignRecord Campaign;
    std::vector<CampaignSlotRecord> Slots;
    std::vector<CharacterBuildState> CharacterBuilds;
    std::vector<AdapterState> AdapterStates;
};

struct ProjectionAudience
{
    bool IncludePrivate{};
    std::optional<PlayerId> Player;

    [[nodiscard]] static ProjectionAudience PublicOnly() noexcept
    {
        return {};
    }

    [[nodiscard]] static ProjectionAudience Server() noexcept
    {
        return {true, std::nullopt};
    }

    [[nodiscard]] static ProjectionAudience ForPlayer(PlayerId aPlayer)
    {
        return {true, std::move(aPlayer)};
    }
};

struct OutboxIntent
{
    std::uint32_t CodecVersion{};
    Bytes Payload;
};

struct CreateCampaignRequest
{
    CampaignRecord Campaign;
    std::vector<CampaignSlotRecord> Slots;
    std::vector<CharacterBuildState> CharacterBuilds;
    std::vector<AdapterState> AdapterStates;
    MutationId Mutation;
    std::uint32_t MutationCodecVersion{1};
    Bytes MutationPayload;
    std::vector<OutboxIntent> Outbox;
};

struct CampaignMutationRequest
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    std::string Kind;
    std::uint32_t MutationCodecVersion{1};
    Bytes MutationPayload;
    std::optional<std::uint32_t> CoreStateCodecVersion;
    std::optional<Bytes> CoreStatePayload;
    std::optional<bool> RosterSealed;
    std::optional<std::vector<CampaignSlotRecord>> ReplacementRoster;
    std::vector<CharacterBuildState> CharacterBuildUpserts;
    std::vector<CampaignSlotId> CharacterBuildDeletes;
    std::vector<AdapterState> AdapterStateUpserts;
    std::vector<std::string> AdapterStateDeletes;
    std::vector<OutboxIntent> Outbox;
};

enum class CheckpointState : std::uint8_t
{
    Candidate = 0,
    Committed = 1
};

struct CheckpointSlotRecord
{
    CampaignSlotId Slot;
    PlayerId Player;
    CharacterBindingId CharacterBinding;
    std::optional<std::string> NativeSaveIdentity;
    std::optional<std::string> FingerprintAlgorithm;
    std::optional<std::uint32_t> FingerprintVersion;
    Bytes Fingerprint;
    std::optional<std::uint32_t> SaveMetadataCodecVersion;
    Bytes SaveMetadata;

    bool operator==(const CheckpointSlotRecord&) const noexcept = default;
};

struct CheckpointRecord
{
    CheckpointId Id;
    CampaignId Campaign;
    CheckpointState State{CheckpointState::Candidate};
    StateVersion SourceRevision{};
    SnapshotId Snapshot;
    std::uint32_t SnapshotCodecVersion{};
    std::string SnapshotChecksum;
    StateVersion CreatedRevision{};
    std::optional<StateVersion> CommittedRevision;
    std::int64_t CreatedAtUnixMs{};
    std::optional<std::int64_t> CommittedAtUnixMs;
    std::vector<CheckpointSlotRecord> Slots;
};

struct CreateCheckpointCandidateRequest
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    CheckpointId Checkpoint;
    SnapshotId Snapshot;
    std::uint32_t MutationCodecVersion{1};
    Bytes MutationPayload;
    std::vector<OutboxIntent> Outbox;
};

struct RecordCheckpointSlotSaveRequest
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    CheckpointId Checkpoint;
    CheckpointSlotRecord Slot;
    std::uint32_t MutationCodecVersion{1};
    Bytes MutationPayload;
    std::vector<OutboxIntent> Outbox;
};

struct CommitCheckpointRequest
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    CheckpointId Checkpoint;
    std::uint32_t MutationCodecVersion{1};
    Bytes MutationPayload;
    std::vector<OutboxIntent> Outbox;
};

struct RestoreCheckpointRequest
{
    CampaignId Campaign;
    StateVersion ExpectedRevision{};
    MutationId Mutation;
    CheckpointId Checkpoint;
    std::uint32_t MutationCodecVersion{1};
    Bytes MutationPayload;
};

struct JournalRecord
{
    std::uint64_t Sequence{};
    CampaignId Campaign;
    MutationId Mutation;
    StateVersion ResultingRevision{};
    std::string Kind;
    std::uint32_t PayloadCodecVersion{};
    Bytes Payload;
    std::optional<CheckpointId> RestoredFromCheckpoint;
    std::optional<StateVersion> RestoredFromRevision;
    std::int64_t CreatedAtUnixMs{};
};

enum class OutboxState : std::uint8_t
{
    Pending = 0,
    Delivered = 1,
    Superseded = 2
};

struct OutboxRecord
{
    std::uint64_t Id{};
    CampaignId Campaign;
    MutationId Mutation;
    std::uint32_t IntentIndex{};
    StateVersion Revision{};
    std::uint32_t CodecVersion{};
    Bytes Payload;
    OutboxState State{OutboxState::Pending};
    std::optional<StateVersion> SupersededByRevision;
};

class ICampaignStore
{
public:
    virtual ~ICampaignStore() = default;

    virtual StoreValueResult<std::uint32_t> GetSchemaVersion() noexcept = 0;
    virtual StoreResult CheckIntegrity() noexcept = 0;

    virtual MutationResult CreateCampaign(
        const CreateCampaignRequest& acRequest) noexcept = 0;
    virtual StoreValueResult<CampaignRecord> LoadCampaign(
        const CampaignId& acCampaign) noexcept = 0;
    virtual StoreValueResult<CampaignProjection> LoadCampaignProjection(
        const CampaignId& acCampaign,
        const ProjectionAudience& acAudience) noexcept = 0;
    virtual MutationResult ApplyMutation(
        const CampaignMutationRequest& acRequest) noexcept = 0;

    virtual MutationResult CreateCheckpointCandidate(
        const CreateCheckpointCandidateRequest& acRequest) noexcept = 0;
    virtual MutationResult RecordCheckpointSlotSave(
        const RecordCheckpointSlotSaveRequest& acRequest) noexcept = 0;
    virtual MutationResult CommitCheckpoint(
        const CommitCheckpointRequest& acRequest) noexcept = 0;
    virtual StoreValueResult<CheckpointRecord> LoadCheckpoint(
        const CampaignId& acCampaign,
        const CheckpointId& acCheckpoint) noexcept = 0;
    virtual StoreValueResult<CheckpointRecord> LoadLastCommittedCheckpoint(
        const CampaignId& acCampaign) noexcept = 0;
    virtual MutationResult RestoreCheckpointSnapshot(
        const RestoreCheckpointRequest& acRequest) noexcept = 0;

    virtual StoreValueResult<std::vector<JournalRecord>> LoadJournal(
        const CampaignId& acCampaign) noexcept = 0;
    virtual StoreValueResult<std::vector<OutboxRecord>> LoadPendingOutbox(
        const CampaignId& acCampaign) noexcept = 0;
    virtual StoreResult MarkOutboxDelivered(std::uint64_t aOutboxId) noexcept = 0;
};
}
