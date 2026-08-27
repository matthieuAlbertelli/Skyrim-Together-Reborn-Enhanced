#include <CampaignRuntimeService.h>

#include <Structs/NativeSaveBundle.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <optional>
#include <string_view>
#include <utility>

namespace STRE::Campaign
{
namespace
{
struct LoadedCampaign
{
    CampaignError Error{CampaignError::None};
    StoreError PersistenceError{StoreError::None};
    std::string Message;
    CampaignAggregate Campaign;
};

CampaignCommandResult DomainFailure(const CampaignDomainResult& acResult)
{
    CampaignCommandResult result;
    result.Error = acResult.Error;
    result.Message = acResult.Message;
    return result;
}

CampaignError TranslateStoreError(StoreError aError)
{
    switch (aError)
    {
    case StoreError::None: return CampaignError::None;
    case StoreError::StaleRevision: return CampaignError::StaleRevision;
    case StoreError::IntegrityFailure:
    case StoreError::IncompatibleSchema:
        return CampaignError::IntegrityFailure;
    default: return CampaignError::PersistenceFailure;
    }
}

CampaignCommandResult StoreFailure(const StoreResult& acResult)
{
    CampaignCommandResult result;
    result.Error = TranslateStoreError(acResult.Error);
    result.PersistenceError = acResult.Error;
    result.Message = acResult.Message;
    return result;
}

CampaignCommandResult FromMutationResult(const MutationResult& acResult)
{
    if (!acResult)
        return StoreFailure(acResult);
    CampaignCommandResult result;
    result.Version = acResult.Revision;
    result.Applied = acResult.Applied;
    result.IdempotentReplay = acResult.IdempotentReplay;
    return result;
}

CampaignSlotState ToSlotState(const CampaignSlotRecord& acSlot)
{
    return {
        acSlot.Slot,
        acSlot.Player,
        acSlot.CharacterBinding,
        false};
}

std::vector<CampaignSlotRecord> ToRosterRecords(
    const CampaignAggregate& acCampaign)
{
    std::vector<CampaignSlotRecord> result;
    result.reserve(acCampaign.Roster.size());
    for (const CampaignSlotState& slot : acCampaign.Roster)
    {
        result.push_back(
            {slot.Slot, slot.Player, slot.CharacterBinding});
    }
    return result;
}

void AppendScalar(Bytes& aPayload, std::uint32_t aValue)
{
    for (std::size_t index = 0; index < sizeof(aValue); ++index)
    {
        aPayload.push_back(static_cast<std::uint8_t>(
            (aValue >> (index * 8)) & 0xFF));
    }
}

void AppendField(Bytes& aPayload, std::string_view acValue)
{
    AppendScalar(aPayload, static_cast<std::uint32_t>(acValue.size()));
    aPayload.insert(aPayload.end(), acValue.begin(), acValue.end());
}

void AppendBlob(Bytes& aPayload, const Bytes& acValue)
{
    AppendScalar(aPayload, static_cast<std::uint32_t>(acValue.size()));
    aPayload.insert(aPayload.end(), acValue.begin(), acValue.end());
}

Bytes BeginCommandPayload(std::string_view acKind)
{
    Bytes payload{'S', 'T', 'R', 'E', 'C', 'M', '0', '1'};
    AppendField(payload, acKind);
    return payload;
}

void AppendSlot(Bytes& aPayload, const CampaignSlotRecord& acSlot)
{
    AppendField(aPayload, acSlot.Slot.Value);
    AppendField(aPayload, acSlot.Player.Value);
    AppendField(aPayload, acSlot.CharacterBinding.Value);
}

bool ReadScalar(
    const Bytes& acPayload,
    std::size_t& aOffset,
    std::uint32_t& aValue) noexcept
{
    if (aOffset > acPayload.size() ||
        acPayload.size() - aOffset < sizeof(aValue))
    {
        return false;
    }
    aValue = 0;
    for (std::size_t index = 0; index < sizeof(aValue); ++index)
    {
        aValue |= static_cast<std::uint32_t>(acPayload[aOffset++]) <<
            (index * 8);
    }
    return true;
}

bool ReadField(
    const Bytes& acPayload,
    std::size_t& aOffset,
    std::string& aValue) noexcept
{
    std::uint32_t size{};
    if (!ReadScalar(acPayload, aOffset, size) || size > 128 ||
        aOffset > acPayload.size() || size > acPayload.size() - aOffset)
    {
        return false;
    }
    aValue.assign(
        reinterpret_cast<const char*>(acPayload.data() + aOffset), size);
    aOffset += size;
    return true;
}

bool DecodeCreateCampaignPayload(
    const Bytes& acPayload,
    std::vector<CampaignSlotRecord>& aRoster)
{
    static constexpr std::array<std::uint8_t, 8> cMagic{
        'S', 'T', 'R', 'E', 'C', 'M', '0', '1'};
    if (acPayload.size() < cMagic.size() ||
        !std::equal(cMagic.begin(), cMagic.end(), acPayload.begin()))
    {
        return false;
    }

    std::size_t offset = cMagic.size();
    std::string kind;
    if (!ReadField(acPayload, offset, kind) || kind != "CreateCampaign")
        return false;
    while (offset < acPayload.size())
    {
        if (aRoster.size() >= kMaximumCampaignRosterSize)
            return false;
        CampaignSlotRecord slot;
        if (!ReadField(acPayload, offset, slot.Slot.Value) ||
            !ReadField(acPayload, offset, slot.Player.Value) ||
            !ReadField(acPayload, offset, slot.CharacterBinding.Value))
        {
            return false;
        }
        aRoster.push_back(std::move(slot));
    }

    std::vector<CampaignSlotState> roster;
    roster.reserve(aRoster.size());
    for (const CampaignSlotRecord& slot : aRoster)
        roster.push_back(ToSlotState(slot));
    return CampaignStateMachine::ValidateRoster(roster, true).Succeeded();
}

bool ProducesCoreState(const JournalRecord& acRecord) noexcept
{
    return acRecord.ResultingRevision > acRecord.ExpectedRevision &&
        acRecord.Kind != "CreateCheckpointCandidate" &&
        acRecord.Kind != "RecordCheckpointSlotSave" &&
        acRecord.Kind != "CommitCheckpoint" &&
        acRecord.Kind != "BeginRecovery" &&
        acRecord.Kind != "CompleteRecovery";
}

StoreValueResult<CampaignAggregate> DecodeCanonicalCoreState(
    const CampaignId& acCampaign,
    bool aRosterSealed,
    const std::vector<CampaignSlotRecord>& acRoster,
    StateVersion aPreferredRevision,
    const Bytes& acPayload,
    const std::vector<JournalRecord>& acJournal,
    std::optional<StateVersion> aAlternateRevision = std::nullopt) noexcept
{
    auto decode = [&](StateVersion aRevision)
    {
        return RuntimeCodec::DecodeCoreState(
            acCampaign,
            aRosterSealed,
            acRoster,
            aRevision,
            acPayload);
    };

    auto decoded = decode(aPreferredRevision);
    if (decoded)
        return decoded;

    StateVersion revision = aPreferredRevision;
    if (aAlternateRevision && *aAlternateRevision != revision)
    {
        revision = *aAlternateRevision;
        auto alternate = decode(revision);
        if (alternate)
            return alternate;
    }

    // Compatibility for snapshots written before restore revisions rebased the
    // opaque runtime core payload. The journal provides an exact, bounded
    // restore lineage; no unrelated revision or current-state fallback is used.
    for (std::size_t depth = 0; depth < acJournal.size(); ++depth)
    {
        const auto restored = std::find_if(
            acJournal.rbegin(),
            acJournal.rend(),
            [&](const JournalRecord& acRecord)
            {
                return acRecord.ResultingRevision == revision &&
                    acRecord.Kind == "RestoreCheckpoint" &&
                    acRecord.RestoredFromRevision.has_value();
            });
        if (restored == acJournal.rend() ||
            *restored->RestoredFromRevision == revision)
        {
            break;
        }
        revision = *restored->RestoredFromRevision;
        auto ancestor = decode(revision);
        if (ancestor)
            return ancestor;
    }
    return decoded;
}

LoadedCampaign LoadAggregate(
    ICampaignStore& aStore,
    const CampaignId& acCampaign) noexcept
{
    LoadedCampaign loaded;
    try
    {
        auto projection = aStore.LoadCampaignProjection(
            acCampaign, ProjectionAudience::Server());
        if (!projection)
        {
            loaded.Error = TranslateStoreError(projection.Error);
            loaded.PersistenceError = projection.Error;
            loaded.Message = projection.Message;
            return loaded;
        }
        if (projection.Value.Campaign.CoreStateCodecVersion !=
            RuntimeCodec::kCampaignCoreCodecVersion)
        {
            loaded.Error = CampaignError::IntegrityFailure;
            loaded.PersistenceError = StoreError::IncompatibleSchema;
            loaded.Message = "campaign uses an unsupported core-state codec";
            return loaded;
        }
        StateVersion coreStateRevision =
            projection.Value.Campaign.CurrentRevision;
        auto journal = aStore.LoadJournal(acCampaign);
        if (!journal)
        {
            loaded.Error = TranslateStoreError(journal.Error);
            loaded.PersistenceError = journal.Error;
            loaded.Message = journal.Message;
            return loaded;
        }
        const auto coreMutation = std::find_if(
            journal.Value.rbegin(), journal.Value.rend(),
            [&](const JournalRecord& acRecord)
            {
                return ProducesCoreState(acRecord);
            });
        if (coreMutation == journal.Value.rend())
        {
            loaded.Error = CampaignError::IntegrityFailure;
            loaded.PersistenceError = StoreError::IntegrityFailure;
            loaded.Message =
                "campaign journal has no core-state-producing mutation";
            return loaded;
        }
        coreStateRevision = coreMutation->ResultingRevision;
        auto decoded = DecodeCanonicalCoreState(
            projection.Value.Campaign.Id,
            projection.Value.Campaign.RosterSealed,
            projection.Value.Slots,
            coreStateRevision,
            projection.Value.Campaign.CoreStatePayload,
            journal.Value);
        if (!decoded)
        {
            loaded.Error = CampaignError::IntegrityFailure;
            loaded.PersistenceError = decoded.Error;
            loaded.Message = decoded.Message;
            return loaded;
        }
        loaded.Campaign = std::move(decoded.Value);
        loaded.Campaign.Version =
            projection.Value.Campaign.CurrentRevision;
    }
    catch (...)
    {
        loaded.Error = CampaignError::PersistenceFailure;
        loaded.PersistenceError = StoreError::DatabaseFailure;
        loaded.Message = "campaign aggregate load failed safely";
    }
    return loaded;
}

CampaignCommandResult StaleRevision(
    StateVersion aExpected,
    StateVersion aCurrent)
{
    CampaignCommandResult result;
    result.Error = CampaignError::StaleRevision;
    result.PersistenceError = StoreError::StaleRevision;
    result.Message = "expected campaign revision=" +
        std::to_string(aExpected) + " current revision=" +
        std::to_string(aCurrent);
    result.Version = aCurrent;
    return result;
}

enum class RevisionMode
{
    Current
};

bool ResolveRevisionMode(
    StateVersion aExpected,
    StateVersion aCurrent,
    RevisionMode& aMode)
{
    if (aCurrent == aExpected)
    {
        aMode = RevisionMode::Current;
        return true;
    }
    return false;
}

CampaignCommandResult PersistMutation(
    ICampaignStore& aStore,
    const CampaignAggregate& acCampaign,
    StateVersion aExpectedRevision,
    const MutationId& acMutation,
    std::string aKind,
    Bytes aMutationPayload,
    bool aReplaceRoster,
    std::optional<bool> aRosterSealed)
{
    Bytes corePayload;
    StoreResult encodedCore = RuntimeCodec::EncodeCoreState(
        acCampaign, corePayload);
    if (!encodedCore)
        return StoreFailure(encodedCore);
    Bytes snapshotPayload;
    StoreResult encodedSnapshot = RuntimeCodec::EncodeSnapshotIntent(
        acCampaign, snapshotPayload);
    if (!encodedSnapshot)
        return StoreFailure(encodedSnapshot);

    CampaignMutationRequest request;
    request.Campaign = acCampaign.Id;
    request.ExpectedRevision = aExpectedRevision;
    request.Mutation = acMutation;
    request.Kind = std::move(aKind);
    request.MutationCodecVersion = 1;
    request.MutationPayload = std::move(aMutationPayload);
    request.CoreStateCodecVersion = RuntimeCodec::kCampaignCoreCodecVersion;
    request.CoreStatePayload = std::move(corePayload);
    request.RosterSealed = aRosterSealed;
    if (aReplaceRoster)
        request.ReplacementRoster = ToRosterRecords(acCampaign);
    request.Outbox.push_back(
        {RuntimeCodec::kCampaignOutboxCodecVersion,
         std::move(snapshotPayload)});
    return FromMutationResult(aStore.ApplyMutation(request));
}

CampaignCommandResult PersistAcceptedNoOp(
    ICampaignStore& aStore,
    const CampaignId& acCampaign,
    StateVersion aExpectedRevision,
    const MutationId& acMutation,
    std::string aKind,
    Bytes aMutationPayload)
{
    CampaignMutationRequest request;
    request.Campaign = acCampaign;
    request.ExpectedRevision = aExpectedRevision;
    request.Mutation = acMutation;
    request.Kind = std::move(aKind);
    request.AdvancesStateVersion = false;
    request.MutationCodecVersion = 1;
    request.MutationPayload = std::move(aMutationPayload);
    return FromMutationResult(aStore.ApplyMutation(request));
}

std::optional<CampaignCommandResult> FindMutationReplay(
    ICampaignStore& aStore,
    const CampaignId& acCampaign,
    StateVersion aExpectedRevision,
    const MutationId& acMutation,
    std::string_view acKind,
    const Bytes& acPayload)
{
    auto journal = aStore.LoadJournal(acCampaign);
    if (!journal)
    {
        if (journal.Error == StoreError::NotFound)
            return std::nullopt;
        return StoreFailure(journal);
    }
    const auto record = std::find_if(
        journal.Value.begin(),
        journal.Value.end(),
        [&acMutation](const JournalRecord& acRecord)
        {
            return acRecord.Mutation == acMutation;
        });
    if (record == journal.Value.end())
        return std::nullopt;
    if (record->ExpectedRevision != aExpectedRevision ||
        record->Kind != acKind ||
        record->PayloadCodecVersion != 1 || record->Payload != acPayload)
    {
        CampaignCommandResult conflict;
        conflict.Error = CampaignError::PersistenceFailure;
        conflict.PersistenceError = StoreError::IdempotencyConflict;
        conflict.Message = "MutationId was already used for different command content";
        conflict.Version = record->ResultingRevision;
        return conflict;
    }
    CampaignCommandResult replay;
    replay.Version = record->ResultingRevision;
    replay.IdempotentReplay = true;
    return replay;
}

CampaignCommandResult CheckpointFailure(
    CampaignError aError,
    std::string aMessage,
    StoreError aPersistenceError = StoreError::None)
{
    CampaignCommandResult result;
    result.Error = aError;
    result.PersistenceError = aPersistenceError;
    result.Message = std::move(aMessage);
    return result;
}

std::string CheckpointMutationId(
    std::string_view acOperation,
    const CheckpointId& acCheckpoint,
    const CampaignSlotId* apSlot = nullptr)
{
    std::string result("checkpoint-");
    result.append(acOperation);
    result.push_back('-');
    result.append(acCheckpoint.Value);
    if (apSlot)
    {
        result.push_back('-');
        result.append(apSlot->Value);
    }
    return result;
}

bool IsExactCheckpointArtifact(
    const CheckpointSlotRecord& acSlot,
    std::string_view acExpectedIdentity) noexcept
{
    if (!acSlot.NativeSaveIdentity ||
        *acSlot.NativeSaveIdentity != acExpectedIdentity ||
        !acSlot.FingerprintAlgorithm ||
        *acSlot.FingerprintAlgorithm != kNativeSaveFingerprintAlgorithm ||
        acSlot.FingerprintVersion != kNativeSaveFingerprintVersion ||
        acSlot.Fingerprint.size() != kNativeSaveSha256Size ||
        acSlot.SaveMetadataCodecVersion != kNativeSaveMetadataCodecVersion ||
        acSlot.SaveMetadata.empty() ||
        acSlot.SaveMetadata.size() > kMaximumNativeSaveMetadataSize)
    {
        return false;
    }
    return ParseNativeSaveBundleArtifact(
        acExpectedIdentity, acSlot.Fingerprint, acSlot.SaveMetadata)
        .Succeeded();
}

bool IsExactCheckpointMember(
    const CheckpointSlotRecord& acSlot,
    const CampaignMemberIdentity& acActor) noexcept
{
    return acSlot.Slot == acActor.Slot && acSlot.Player == acActor.Player &&
        acSlot.CharacterBinding == acActor.CharacterBinding;
}

CampaignCommandResult RecoveryFailure(
    CampaignError aError,
    std::string aMessage,
    StoreError aPersistenceError = StoreError::None)
{
    CampaignCommandResult result;
    result.Error = aError;
    result.PersistenceError = aPersistenceError;
    result.Message = std::move(aMessage);
    return result;
}

std::string StableIdHash(std::string_view acValue)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const unsigned char value : acValue)
    {
        hash ^= value;
        hash *= 1099511628211ull;
    }
    char encoded[17]{};
    std::snprintf(
        encoded,
        sizeof(encoded),
        "%016llx",
        static_cast<unsigned long long>(hash));
    return encoded;
}

std::string StableCampaignHash(const CampaignId& acCampaign)
{
    return StableIdHash(acCampaign.Value);
}

RestoreAttemptId MakeRestoreAttempt(
    const CampaignId& acCampaign,
    StateVersion aEntryRevision)
{
    return RestoreAttemptId{
        "restore-" + StableCampaignHash(acCampaign) + "-r" +
        std::to_string(aEntryRevision)};
}

MutationId RecoveryBeginMutationId(
    const CampaignId& acCampaign,
    StateVersion aExpectedRevision)
{
    return MutationId{
        "recovery-begin-" + StableCampaignHash(acCampaign) + "-r" +
        std::to_string(aExpectedRevision)};
}

MutationId RecoveryMutationId(
    std::string_view acOperation,
    const RestoreAttemptId& acAttempt)
{
    return MutationId{
        "recovery-" + std::string(acOperation) + "-" + acAttempt.Value};
}

Bytes CompleteRecoveryPayload(
    const RestoreAttemptId& acAttempt,
    const CheckpointId& acCheckpoint,
    StateVersion aRestoreRevision)
{
    Bytes payload = BeginCommandPayload("CompleteRecovery");
    AppendField(payload, acAttempt.Value);
    AppendField(payload, acCheckpoint.Value);
    AppendField(payload, std::to_string(aRestoreRevision));
    return payload;
}

Bytes RestoreRecoveryPayload(
    const RestoreAttemptId& acAttempt,
    const CheckpointId& acCheckpoint)
{
    Bytes payload = BeginCommandPayload("RestoreCheckpoint");
    AppendField(payload, acAttempt.Value);
    AppendField(payload, acCheckpoint.Value);
    return payload;
}

MutationId RecoverySlotMutationId(
    std::string_view acOperation,
    const RestoreAttemptId& acAttempt,
    const CampaignSlotId& acSlot)
{
    return RecoveryMutationId(
        std::string(acOperation) + "-" + StableIdHash(acSlot.Value),
        acAttempt);
}

Bytes RecoveryLoadedPayload(
    const RestoreAttemptId& acAttempt,
    const CheckpointRecord& acCheckpoint,
    const CheckpointSlotRecord& acSlot)
{
    Bytes payload = BeginCommandPayload("RecoveryLoaded");
    AppendField(payload, acAttempt.Value);
    AppendField(payload, acCheckpoint.Id.Value);
    AppendField(payload, acSlot.Slot.Value);
    AppendField(payload, acSlot.Player.Value);
    AppendField(payload, acSlot.CharacterBinding.Value);
    AppendField(payload, acSlot.NativeSaveIdentity.value_or(std::string{}));
    AppendField(payload, acSlot.FingerprintAlgorithm.value_or(std::string{}));
    AppendScalar(payload, acSlot.FingerprintVersion.value_or(0));
    AppendBlob(payload, acSlot.Fingerprint);
    AppendScalar(payload, acSlot.SaveMetadataCodecVersion.value_or(0));
    AppendBlob(payload, acSlot.SaveMetadata);
    return payload;
}

Bytes RecoverySnapshotAppliedPayload(
    const RestoreAttemptId& acAttempt,
    const CheckpointRecord& acCheckpoint,
    StateVersion aRestoreRevision,
    const CheckpointSlotRecord& acSlot)
{
    Bytes payload = BeginCommandPayload("RecoverySnapshotApplied");
    AppendField(payload, acAttempt.Value);
    AppendField(payload, acCheckpoint.Id.Value);
    AppendField(payload, std::to_string(aRestoreRevision));
    AppendField(payload, acSlot.Slot.Value);
    AppendField(payload, acSlot.Player.Value);
    AppendField(payload, acSlot.CharacterBinding.Value);
    return payload;
}

bool MatchesRecoveryProof(
    const CheckpointSlotRecord& acSlot,
    const RecordCampaignRecoveryLoadedCommand& acCommand) noexcept
{
    return acSlot.NativeSaveIdentity == acCommand.NativeSaveIdentity &&
        acSlot.FingerprintAlgorithm == acCommand.FingerprintAlgorithm &&
        acSlot.FingerprintVersion == acCommand.FingerprintVersion &&
        acSlot.Fingerprint == acCommand.Fingerprint &&
        acSlot.SaveMetadataCodecVersion ==
            acCommand.SaveMetadataCodecVersion &&
        acSlot.SaveMetadata == acCommand.SaveMetadata &&
        IsExactCheckpointArtifact(acSlot, acCommand.NativeSaveIdentity);
}
}

CampaignRuntimeService::CampaignRuntimeService(ICampaignStore& aStore) noexcept
    : m_store(aStore)
{
}

std::optional<CampaignCommandResult>
CampaignRuntimeService::ReconstructRecovery(
    const CampaignId& acCampaign) noexcept
{
    try
    {
        if (m_recoveries.contains(acCampaign.Value))
            return std::nullopt;

        auto journal = m_store.LoadJournal(acCampaign);
        if (!journal)
        {
            if (journal.Error == StoreError::NotFound)
                return std::nullopt;
            return StoreFailure(journal);
        }

        std::optional<CampaignRecoveryActivity> open;
        std::vector<JournalRecord> durableLoadedAcks;
        std::vector<JournalRecord> durableAppliedAcks;
        for (const JournalRecord& record : journal.Value)
        {
            if (record.Kind == "BeginRecovery")
            {
                if (open)
                {
                    return RecoveryFailure(
                        CampaignError::IntegrityFailure,
                        "campaign journal contains overlapping recovery attempts",
                        StoreError::IntegrityFailure);
                }
                if (record.Mutation != RecoveryBeginMutationId(
                        acCampaign, record.ExpectedRevision) ||
                    record.ResultingRevision != record.ExpectedRevision + 1)
                {
                    return RecoveryFailure(
                        CampaignError::IntegrityFailure,
                        "recovery begin journal correlation is invalid",
                        StoreError::IntegrityFailure);
                }
                CampaignRecoveryActivity activity;
                activity.Campaign = acCampaign;
                activity.EntryRevision = record.ResultingRevision;
                activity.Attempt = MakeRestoreAttempt(
                    acCampaign, record.ResultingRevision);
                open = std::move(activity);
                durableLoadedAcks.clear();
                durableAppliedAcks.clear();
                continue;
            }
            if (!open)
                continue;

            if (record.Kind == "RecoveryLoaded")
            {
                durableLoadedAcks.push_back(record);
                continue;
            }
            if (record.Kind == "RecoverySnapshotApplied")
            {
                durableAppliedAcks.push_back(record);
                continue;
            }

            if (record.Kind == "RestoreCheckpoint" &&
                record.Mutation ==
                    RecoveryMutationId("restore", open->Attempt))
            {
                if (!record.RestoredFromCheckpoint ||
                    !record.RestoredFromRevision ||
                    record.Payload != RestoreRecoveryPayload(
                        open->Attempt, *record.RestoredFromCheckpoint) ||
                    record.ResultingRevision != record.ExpectedRevision + 1)
                {
                    return RecoveryFailure(
                        CampaignError::IntegrityFailure,
                        "recovery restore journal provenance is incomplete",
                        StoreError::IntegrityFailure);
                }
                open->Checkpoint = *record.RestoredFromCheckpoint;
                open->SourceRevision = *record.RestoredFromRevision;
                open->RestoreRevision = record.ResultingRevision;
                // The current-session barrier counts are deliberately
                // volatile. After a server restart every member must re-prove
                // its native checkpoint load before the already-durable
                // restored snapshot can be replayed. Durable slot receipts
                // retain idempotency, and RecordRecoveryLoaded() reuses this
                // RestoreCheckpoint mutation.
                open->Stage = CampaignRecoveryStage::RecoveryLock;
                open->Reason = CampaignRecoveryReason::None;
                continue;
            }
            if (record.Kind == "CompleteRecovery" &&
                record.Mutation ==
                    RecoveryMutationId("complete", open->Attempt))
            {
                if (!open->Checkpoint || !open->RestoreRevision ||
                    record.ResultingRevision != *open->RestoreRevision ||
                    record.Payload != CompleteRecoveryPayload(
                        open->Attempt,
                        *open->Checkpoint,
                        *open->RestoreRevision))
                {
                    return RecoveryFailure(
                        CampaignError::IntegrityFailure,
                        "recovery completion journal correlation is invalid",
                        StoreError::IntegrityFailure);
                }
                open.reset();
                durableLoadedAcks.clear();
                durableAppliedAcks.clear();
            }
        }

        if (open)
        {
            std::optional<CheckpointRecord> checkpoint;
            if (!durableLoadedAcks.empty() || !durableAppliedAcks.empty())
            {
                auto loadedCheckpoint = open->Checkpoint
                    ? m_store.LoadCheckpoint(acCampaign, *open->Checkpoint)
                    : m_store.LoadLastCommittedCheckpoint(acCampaign);
                if (!loadedCheckpoint)
                    return StoreFailure(loadedCheckpoint);
                checkpoint = std::move(loadedCheckpoint.Value);
                if (!open->Checkpoint)
                {
                    open->Checkpoint = checkpoint->Id;
                    open->SourceRevision = checkpoint->SourceRevision;
                }
            }

            const auto recoverDurableSlots = [&]<typename TPayload>(
                const std::vector<JournalRecord>& acRecords,
                std::string_view acKind,
                std::string_view acMutationOperation,
                std::unordered_set<std::string>& aSlots,
                TPayload&& aPayload) -> bool
            {
                if (acRecords.empty())
                    return true;
                if (!checkpoint)
                    return false;
                for (const JournalRecord& record : acRecords)
                {
                    const auto slot = std::find_if(
                        checkpoint->Slots.begin(), checkpoint->Slots.end(),
                        [&](const CheckpointSlotRecord& acSlot)
                        {
                            return record.Mutation == RecoverySlotMutationId(
                                acMutationOperation,
                                open->Attempt,
                                acSlot.Slot);
                        });
                    if (slot == checkpoint->Slots.end() ||
                        record.Kind != acKind ||
                        record.ExpectedRevision != record.ResultingRevision ||
                        record.Payload != aPayload(*slot) ||
                        !aSlots.insert(slot->Slot.Value).second)
                    {
                        return false;
                    }
                }
                return true;
            };
            if (!recoverDurableSlots(
                    durableLoadedAcks,
                    "RecoveryLoaded",
                    "loaded",
                    open->DurableLoadedSlots,
                    [&](const CheckpointSlotRecord& acSlot)
                    {
                        return RecoveryLoadedPayload(
                            open->Attempt, *checkpoint, acSlot);
                    }) ||
                (!durableAppliedAcks.empty() && !open->RestoreRevision) ||
                !recoverDurableSlots(
                    durableAppliedAcks,
                    "RecoverySnapshotApplied",
                    "applied",
                    open->DurableSnapshotAppliedSlots,
                    [&](const CheckpointSlotRecord& acSlot)
                    {
                        return RecoverySnapshotAppliedPayload(
                            open->Attempt,
                            *checkpoint,
                            open->RestoreRevision.value_or(0),
                            acSlot);
                    }))
            {
                return RecoveryFailure(
                    CampaignError::IntegrityFailure,
                    "recovery acknowledgement journal correlation is invalid",
                    StoreError::IntegrityFailure);
            }
            m_recoveries.emplace(acCampaign.Value, std::move(*open));
        }
        return std::nullopt;
    }
    catch (...)
    {
        return RecoveryFailure(
            CampaignError::PersistenceFailure,
            "recovery journal reconstruction failed safely",
            StoreError::DatabaseFailure);
    }
}

std::optional<CampaignCommandResult>
CampaignRuntimeService::CheckMutationFence(
    const CampaignId& acCampaign) noexcept
{
    if (m_activeCheckpoints.contains(acCampaign.Value))
    {
        return CheckpointFailure(
            CampaignError::CheckpointInProgress,
            "persistent campaign mutation rejected while checkpoint is active");
    }
    if (auto failure = ReconstructRecovery(acCampaign))
        return failure;
    if (m_recoveries.contains(acCampaign.Value))
    {
        return RecoveryFailure(
            CampaignError::RecoveryInProgress,
            "persistent campaign mutation rejected while recovery is active");
    }
    return std::nullopt;
}

CampaignCommandResult CampaignRuntimeService::CreateLobbyCampaign(
    const CreateLobbyCampaignCommand& acCommand) noexcept
{
    try
    {
        CampaignAggregate campaign;
        campaign.Id = acCommand.Campaign;
        campaign.Version = 1;
        campaign.Phase = CampaignPhase::Lobby;
        campaign.Roster.reserve(acCommand.InitialRoster.size());
        for (const CampaignSlotRecord& slot : acCommand.InitialRoster)
            campaign.Roster.push_back(ToSlotState(slot));
        CampaignStateMachine::SortRoster(campaign.Roster);
        CampaignDomainResult roster = CampaignStateMachine::ValidateRoster(
            campaign.Roster, true);
        if (!roster)
            return DomainFailure(roster);

        Bytes corePayload;
        StoreResult encodedCore = RuntimeCodec::EncodeCoreState(
            campaign, corePayload);
        if (!encodedCore)
            return StoreFailure(encodedCore);
        Bytes snapshotPayload;
        StoreResult encodedSnapshot = RuntimeCodec::EncodeSnapshotIntent(
            campaign, snapshotPayload);
        if (!encodedSnapshot)
            return StoreFailure(encodedSnapshot);

        Bytes commandPayload = BeginCommandPayload("CreateCampaign");
        for (const CampaignSlotRecord& slot : ToRosterRecords(campaign))
            AppendSlot(commandPayload, slot);

        CreateCampaignRequest request;
        request.Campaign.Id = campaign.Id;
        request.Campaign.PersistenceSchemaVersion =
            kCampaignDatabaseSchemaVersion;
        request.Campaign.RosterSealed = false;
        request.Campaign.CoreStateCodecVersion =
            RuntimeCodec::kCampaignCoreCodecVersion;
        request.Campaign.CoreStatePayload = std::move(corePayload);
        request.Slots = ToRosterRecords(campaign);
        request.Mutation = acCommand.Mutation;
        request.MutationCodecVersion = 1;
        request.MutationPayload = std::move(commandPayload);
        request.Outbox.push_back(
            {RuntimeCodec::kCampaignOutboxCodecVersion,
             std::move(snapshotPayload)});
        return FromMutationResult(m_store.CreateCampaign(request));
    }
    catch (...)
    {
        return StoreFailure(
            {StoreError::DatabaseFailure,
             "CreateLobbyCampaign failed safely"});
    }
}

CampaignCommandResult CampaignRuntimeService::AddRosterSlot(
    const AddRosterSlotCommand& acCommand) noexcept try
{
    if (auto fenced = CheckMutationFence(acCommand.Campaign))
        return *fenced;
    Bytes commandPayload = BeginCommandPayload("AddRosterSlot");
    AppendSlot(commandPayload, acCommand.Slot);
    if (auto replay = FindMutationReplay(
            m_store,
            acCommand.Campaign,
            acCommand.ExpectedRevision,
            acCommand.Mutation,
            "AddRosterSlot",
            commandPayload))
    {
        return *replay;
    }
    LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
    if (loaded.Error != CampaignError::None)
        return {loaded.Error, loaded.PersistenceError, loaded.Message};
    RevisionMode mode;
    if (!ResolveRevisionMode(
            acCommand.ExpectedRevision, loaded.Campaign.Version, mode))
    {
        return StaleRevision(
            acCommand.ExpectedRevision, loaded.Campaign.Version);
    }

    if (mode == RevisionMode::Current)
    {
        CampaignDomainResult changed = CampaignStateMachine::AddSlot(
            loaded.Campaign, ToSlotState(acCommand.Slot));
        if (!changed)
            return DomainFailure(changed);
    }
    return PersistMutation(
        m_store,
        loaded.Campaign,
        acCommand.ExpectedRevision,
        acCommand.Mutation,
        "AddRosterSlot",
        std::move(commandPayload),
        true,
        std::nullopt);
}
catch (...)
{
    return StoreFailure(
        {StoreError::DatabaseFailure, "AddRosterSlot failed safely"});
}

CampaignCommandResult CampaignRuntimeService::RemoveRosterSlot(
    const RemoveRosterSlotCommand& acCommand) noexcept try
{
    if (auto fenced = CheckMutationFence(acCommand.Campaign))
        return *fenced;
    Bytes commandPayload = BeginCommandPayload("RemoveRosterSlot");
    AppendField(commandPayload, acCommand.Slot.Value);
    if (auto replay = FindMutationReplay(
            m_store,
            acCommand.Campaign,
            acCommand.ExpectedRevision,
            acCommand.Mutation,
            "RemoveRosterSlot",
            commandPayload))
    {
        return *replay;
    }
    LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
    if (loaded.Error != CampaignError::None)
        return {loaded.Error, loaded.PersistenceError, loaded.Message};
    RevisionMode mode;
    if (!ResolveRevisionMode(
            acCommand.ExpectedRevision, loaded.Campaign.Version, mode))
    {
        return StaleRevision(
            acCommand.ExpectedRevision, loaded.Campaign.Version);
    }

    if (mode == RevisionMode::Current)
    {
        CampaignDomainResult changed = CampaignStateMachine::RemoveSlot(
            loaded.Campaign, acCommand.Slot);
        if (!changed)
            return DomainFailure(changed);
    }
    return PersistMutation(
        m_store,
        loaded.Campaign,
        acCommand.ExpectedRevision,
        acCommand.Mutation,
        "RemoveRosterSlot",
        std::move(commandPayload),
        true,
        std::nullopt);
}
catch (...)
{
    return StoreFailure(
        {StoreError::DatabaseFailure, "RemoveRosterSlot failed safely"});
}

CampaignCommandResult CampaignRuntimeService::ReplaceRosterSlot(
    const ReplaceRosterSlotCommand& acCommand) noexcept try
{
    if (auto fenced = CheckMutationFence(acCommand.Campaign))
        return *fenced;
    Bytes commandPayload = BeginCommandPayload("ReplaceRosterSlot");
    AppendSlot(commandPayload, acCommand.Slot);
    if (auto replay = FindMutationReplay(
            m_store,
            acCommand.Campaign,
            acCommand.ExpectedRevision,
            acCommand.Mutation,
            "ReplaceRosterSlot",
            commandPayload))
    {
        return *replay;
    }
    LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
    if (loaded.Error != CampaignError::None)
        return {loaded.Error, loaded.PersistenceError, loaded.Message};
    RevisionMode mode;
    if (!ResolveRevisionMode(
            acCommand.ExpectedRevision, loaded.Campaign.Version, mode))
    {
        return StaleRevision(
            acCommand.ExpectedRevision, loaded.Campaign.Version);
    }

    if (mode == RevisionMode::Current)
    {
        CampaignDomainResult changed = CampaignStateMachine::ReplaceSlot(
            loaded.Campaign, ToSlotState(acCommand.Slot));
        if (!changed)
            return DomainFailure(changed);
        if (!changed.Changed)
        {
            return PersistAcceptedNoOp(
                m_store,
                acCommand.Campaign,
                acCommand.ExpectedRevision,
                acCommand.Mutation,
                "ReplaceRosterSlot",
                std::move(commandPayload));
        }
    }
    return PersistMutation(
        m_store,
        loaded.Campaign,
        acCommand.ExpectedRevision,
        acCommand.Mutation,
        "ReplaceRosterSlot",
        std::move(commandPayload),
        true,
        std::nullopt);
}
catch (...)
{
    return StoreFailure(
        {StoreError::DatabaseFailure, "ReplaceRosterSlot failed safely"});
}

CampaignCommandResult CampaignRuntimeService::CommitCampaignStart(
    const CommitCampaignStartCommand& acCommand) noexcept try
{
    if (auto fenced = CheckMutationFence(acCommand.Campaign))
        return *fenced;
    Bytes commandPayload = BeginCommandPayload("CommitCampaignStart");
    AppendField(commandPayload, acCommand.SessionManager.Value);
    if (auto replay = FindMutationReplay(
            m_store,
            acCommand.Campaign,
            acCommand.ExpectedRevision,
            acCommand.Mutation,
            "CommitCampaignStart",
            commandPayload))
    {
        return *replay;
    }
    LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
    if (loaded.Error != CampaignError::None)
        return {loaded.Error, loaded.PersistenceError, loaded.Message};
    RevisionMode mode;
    if (!ResolveRevisionMode(
            acCommand.ExpectedRevision, loaded.Campaign.Version, mode))
    {
        return StaleRevision(
            acCommand.ExpectedRevision, loaded.Campaign.Version);
    }

    if (mode == RevisionMode::Current)
    {
        CampaignDomainResult changed = CampaignStateMachine::CommitCampaignStart(
            loaded.Campaign,
            CampaignActor::Server(),
            acCommand.SessionManager);
        if (!changed)
            return DomainFailure(changed);
    }
    return PersistMutation(
        m_store,
        loaded.Campaign,
        acCommand.ExpectedRevision,
        acCommand.Mutation,
        "CommitCampaignStart",
        std::move(commandPayload),
        true,
        true);
}
catch (...)
{
    return StoreFailure(
        {StoreError::DatabaseFailure, "CommitCampaignStart failed safely"});
}

CampaignCommandResult CampaignRuntimeService::TransferSessionManager(
    const TransferSessionManagerCommand& acCommand) noexcept try
{
    if (auto fenced = CheckMutationFence(acCommand.Campaign))
        return *fenced;
    Bytes commandPayload = BeginCommandPayload("TransferSessionManager");
    AppendField(commandPayload, acCommand.Actor.Value);
    AppendField(commandPayload, acCommand.NewManager.Value);
    if (auto replay = FindMutationReplay(
            m_store,
            acCommand.Campaign,
            acCommand.ExpectedRevision,
            acCommand.Mutation,
            "TransferSessionManager",
            commandPayload))
    {
        return *replay;
    }
    LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
    if (loaded.Error != CampaignError::None)
        return {loaded.Error, loaded.PersistenceError, loaded.Message};
    RevisionMode mode;
    if (!ResolveRevisionMode(
            acCommand.ExpectedRevision, loaded.Campaign.Version, mode))
    {
        return StaleRevision(
            acCommand.ExpectedRevision, loaded.Campaign.Version);
    }

    if (mode == RevisionMode::Current)
    {
        CampaignDomainResult changed =
            CampaignStateMachine::TransferSessionManager(
                loaded.Campaign, acCommand.Actor, acCommand.NewManager);
        if (!changed)
            return DomainFailure(changed);
        if (!changed.Changed)
        {
            return PersistAcceptedNoOp(
                m_store,
                acCommand.Campaign,
                acCommand.ExpectedRevision,
                acCommand.Mutation,
                "TransferSessionManager",
                std::move(commandPayload));
        }
    }
    return PersistMutation(
        m_store,
        loaded.Campaign,
        acCommand.ExpectedRevision,
        acCommand.Mutation,
        "TransferSessionManager",
        std::move(commandPayload),
        false,
        std::nullopt);
}
catch (...)
{
    return StoreFailure(
        {StoreError::DatabaseFailure, "TransferSessionManager failed safely"});
}

CampaignCommandResult CampaignRuntimeService::SetReady(
    const SetCampaignReadyCommand& acCommand) noexcept try
{
    if (auto fenced = CheckMutationFence(acCommand.Campaign))
        return *fenced;
    Bytes commandPayload = BeginCommandPayload("SetReady");
    AppendField(commandPayload, acCommand.Actor.Campaign.Value);
    AppendField(commandPayload, acCommand.Actor.Slot.Value);
    AppendField(commandPayload, acCommand.Actor.Player.Value);
    AppendField(commandPayload, acCommand.Actor.CharacterBinding.Value);
    commandPayload.push_back(acCommand.Ready ? 1 : 0);
    if (auto replay = FindMutationReplay(
            m_store,
            acCommand.Campaign,
            acCommand.ExpectedRevision,
            acCommand.Mutation,
            "SetReady",
            commandPayload))
    {
        return *replay;
    }
    LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
    if (loaded.Error != CampaignError::None)
        return {loaded.Error, loaded.PersistenceError, loaded.Message};
    RevisionMode mode;
    if (!ResolveRevisionMode(
            acCommand.ExpectedRevision, loaded.Campaign.Version, mode))
    {
        return StaleRevision(
            acCommand.ExpectedRevision, loaded.Campaign.Version);
    }

    if (mode == RevisionMode::Current)
    {
        CampaignDomainResult changed = CampaignStateMachine::SetReady(
            loaded.Campaign, acCommand.Actor, acCommand.Ready);
        if (!changed)
            return DomainFailure(changed);
        if (!changed.Changed)
        {
            return PersistAcceptedNoOp(
                m_store,
                acCommand.Campaign,
                acCommand.ExpectedRevision,
                acCommand.Mutation,
                "SetReady",
                std::move(commandPayload));
        }
    }
    return PersistMutation(
        m_store,
        loaded.Campaign,
        acCommand.ExpectedRevision,
        acCommand.Mutation,
        "SetReady",
        std::move(commandPayload),
        false,
        std::nullopt);
}
catch (...)
{
    return StoreFailure(
        {StoreError::DatabaseFailure, "SetReady failed safely"});
}

CampaignCheckpointCommandResult CampaignRuntimeService::BeginCheckpoint(
    const BeginCampaignCheckpointCommand& acCommand) noexcept
{
    CampaignCheckpointCommandResult result;
    try
    {
        if (m_activeCheckpoints.contains(acCommand.Campaign.Value))
        {
            result.Command = CheckpointFailure(
                CampaignError::CheckpointInProgress,
                "campaign already has an active checkpoint");
            result.Activity = m_activeCheckpoints.at(acCommand.Campaign.Value);
            return result;
        }
        if (auto fenced = CheckMutationFence(acCommand.Campaign))
        {
            result.Command = *fenced;
            return result;
        }
        if (acCommand.Checkpoint.Value.empty() ||
            acCommand.NativeSaveIdentity !=
                "stre-" + acCommand.Checkpoint.Value)
        {
            result.Command = CheckpointFailure(
                CampaignError::InvalidIdentity,
                "checkpoint and native save identities do not match");
            return result;
        }

        LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
        if (loaded.Error != CampaignError::None)
        {
            result.Command = {
                loaded.Error, loaded.PersistenceError, loaded.Message};
            return result;
        }
        const FullRosterEvaluation fullRoster =
            CampaignStateMachine::EvaluateFullRoster(
                loaded.Campaign, acCommand.Presence);
        if (!fullRoster.Eligible())
        {
            result.Command = CheckpointFailure(
                loaded.Campaign.RosterSealed
                    ? CampaignError::RosterIncomplete
                    : CampaignError::RosterNotSealed,
                fullRoster.Message);
            return result;
        }

        CreateCheckpointCandidateRequest request;
        request.Campaign = acCommand.Campaign;
        request.ExpectedRevision = loaded.Campaign.Version;
        request.Mutation = MutationId{CheckpointMutationId(
            "create", acCommand.Checkpoint)};
        request.Checkpoint = acCommand.Checkpoint;
        request.Snapshot = SnapshotId{"snapshot-" + acCommand.Checkpoint.Value};
        request.SnapshotCoreStateCodecVersion =
            RuntimeCodec::kCampaignCoreCodecVersion;
        StoreResult encodedCheckpointCore = RuntimeCodec::EncodeCoreState(
            loaded.Campaign, request.SnapshotCoreStatePayload);
        if (!encodedCheckpointCore)
        {
            result.Command = StoreFailure(encodedCheckpointCore);
            return result;
        }
        request.MutationPayload = BeginCommandPayload(
            "CreateCheckpointCandidate");
        AppendField(request.MutationPayload, acCommand.Checkpoint.Value);
        AppendField(request.MutationPayload, acCommand.NativeSaveIdentity);

        const MutationResult persisted =
            m_store.CreateCheckpointCandidate(request);
        result.Command = FromMutationResult(persisted);
        if (!result.Command)
            return result;

        CampaignCheckpointActivity activity{
            acCommand.Campaign,
            acCommand.Checkpoint,
            loaded.Campaign.Version,
            acCommand.NativeSaveIdentity};
        m_activeCheckpoints.emplace(
            acCommand.Campaign.Value, activity);
        result.Activity = std::move(activity);
        return result;
    }
    catch (...)
    {
        result.Command = CheckpointFailure(
            CampaignError::PersistenceFailure,
            "BeginCheckpoint failed safely",
            StoreError::DatabaseFailure);
        return result;
    }
}

CampaignCheckpointCommandResult
CampaignRuntimeService::RecordCheckpointSave(
    const RecordCampaignCheckpointSaveCommand& acCommand) noexcept
{
    CampaignCheckpointCommandResult result;
    try
    {
        if (auto recovery = ReconstructRecovery(acCommand.Campaign))
        {
            result.Command = *recovery;
            return result;
        }
        if (m_recoveries.contains(acCommand.Campaign.Value))
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryInProgress,
                "checkpoint acknowledgement rejected while recovery is active");
            return result;
        }
        const auto active = m_activeCheckpoints.find(acCommand.Campaign.Value);
        if (active == m_activeCheckpoints.end())
        {
            result.Command = CheckpointFailure(
                CampaignError::CheckpointNotActive,
                "campaign has no active checkpoint");
            return result;
        }
        result.Activity = active->second;
        if (active->second.Checkpoint != acCommand.Checkpoint ||
            active->second.NativeSaveIdentity != acCommand.NativeSaveIdentity ||
            acCommand.Actor.Campaign != acCommand.Campaign)
        {
            result.Command = CheckpointFailure(
                CampaignError::CheckpointMismatch,
                "checkpoint result does not match the active operation");
            return result;
        }

        auto checkpoint = m_store.LoadCheckpoint(
            acCommand.Campaign, acCommand.Checkpoint);
        if (!checkpoint)
        {
            result.Command = StoreFailure(checkpoint);
            return result;
        }
        const auto slot = std::find_if(
            checkpoint.Value.Slots.begin(), checkpoint.Value.Slots.end(),
            [&](const CheckpointSlotRecord& acSlot)
            {
                return IsExactCheckpointMember(acSlot, acCommand.Actor);
            });
        if (slot == checkpoint.Value.Slots.end())
        {
            result.Command = CheckpointFailure(
                CampaignError::NotCampaignMember,
                "admitted identity is not an exact checkpoint slot");
            return result;
        }

        CheckpointSlotRecord canonical{
            slot->Slot,
            slot->Player,
            slot->CharacterBinding,
            acCommand.NativeSaveIdentity,
            acCommand.FingerprintAlgorithm,
            acCommand.FingerprintVersion,
            acCommand.Fingerprint,
            acCommand.SaveMetadataCodecVersion,
            acCommand.SaveMetadata};
        if (!IsExactCheckpointArtifact(
                canonical, active->second.NativeSaveIdentity))
        {
            result.Command = CheckpointFailure(
                CampaignError::InvalidCheckpointArtifact,
                "checkpoint native save metadata is incomplete or invalid");
            return result;
        }

        const MutationId mutation{CheckpointMutationId(
            "ack", acCommand.Checkpoint, &slot->Slot)};
        StateVersion expectedRevision{};
        auto journal = m_store.LoadJournal(acCommand.Campaign);
        if (!journal)
        {
            result.Command = StoreFailure(journal);
            return result;
        }
        const auto replay = std::find_if(
            journal.Value.begin(), journal.Value.end(),
            [&](const JournalRecord& acRecord)
            {
                return acRecord.Mutation == mutation;
            });
        if (replay != journal.Value.end())
        {
            expectedRevision = replay->ExpectedRevision;
        }
        else
        {
            auto campaign = m_store.LoadCampaign(acCommand.Campaign);
            if (!campaign)
            {
                result.Command = StoreFailure(campaign);
                return result;
            }
            expectedRevision = campaign.Value.CurrentRevision;
        }

        Bytes payload = BeginCommandPayload("RecordCheckpointSlotSave");
        AppendField(payload, acCommand.Checkpoint.Value);
        AppendField(payload, slot->Slot.Value);
        RecordCheckpointSlotSaveRequest record;
        record.Campaign = acCommand.Campaign;
        record.ExpectedRevision = expectedRevision;
        record.Mutation = mutation;
        record.Checkpoint = acCommand.Checkpoint;
        record.Slot = std::move(canonical);
        record.MutationPayload = std::move(payload);
        const MutationResult recorded =
            m_store.RecordCheckpointSlotSave(record);
        result.Command = FromMutationResult(recorded);
        if (!result.Command)
            return result;

        checkpoint = m_store.LoadCheckpoint(
            acCommand.Campaign, acCommand.Checkpoint);
        if (!checkpoint)
        {
            result.Command = StoreFailure(checkpoint);
            return result;
        }
        const bool complete = !checkpoint.Value.Slots.empty() &&
            std::all_of(
                checkpoint.Value.Slots.begin(), checkpoint.Value.Slots.end(),
                [&](const CheckpointSlotRecord& acSlot)
                {
                    return IsExactCheckpointArtifact(
                        acSlot, active->second.NativeSaveIdentity);
                });
        if (!complete)
            return result;

        auto campaign = m_store.LoadCampaign(acCommand.Campaign);
        if (!campaign)
        {
            result.Command = StoreFailure(campaign);
            return result;
        }
        CommitCheckpointRequest commit;
        commit.Campaign = acCommand.Campaign;
        commit.ExpectedRevision = campaign.Value.CurrentRevision;
        commit.Mutation = MutationId{CheckpointMutationId(
            "commit", acCommand.Checkpoint)};
        commit.Checkpoint = acCommand.Checkpoint;
        commit.MutationPayload = BeginCommandPayload("CommitCheckpoint");
        AppendField(commit.MutationPayload, acCommand.Checkpoint.Value);
        const MutationResult committed = m_store.CommitCheckpoint(commit);
        result.Command = FromMutationResult(committed);
        if (!result.Command)
            return result;

        result.Committed = true;
        m_activeCheckpoints.erase(active);
        return result;
    }
    catch (...)
    {
        result.Command = CheckpointFailure(
            CampaignError::PersistenceFailure,
            "RecordCheckpointSave failed safely",
            StoreError::DatabaseFailure);
        return result;
    }
}

CampaignCheckpointCommandResult CampaignRuntimeService::FailCheckpoint(
    const FailCampaignCheckpointCommand& acCommand) noexcept
{
    CampaignCheckpointCommandResult result;
    try
    {
        if (auto recovery = ReconstructRecovery(acCommand.Campaign))
        {
            result.Command = *recovery;
            return result;
        }
        if (m_recoveries.contains(acCommand.Campaign.Value))
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryInProgress,
                "checkpoint failure rejected while recovery is active");
            return result;
        }
        const auto active = m_activeCheckpoints.find(acCommand.Campaign.Value);
        if (active == m_activeCheckpoints.end())
        {
            result.Command = CheckpointFailure(
                CampaignError::CheckpointNotActive,
                "campaign has no active checkpoint");
            return result;
        }
        result.Activity = active->second;
        if (active->second.Checkpoint != acCommand.Checkpoint ||
            active->second.NativeSaveIdentity != acCommand.NativeSaveIdentity ||
            acCommand.Actor.Campaign != acCommand.Campaign)
        {
            result.Command = CheckpointFailure(
                CampaignError::CheckpointMismatch,
                "checkpoint failure does not match the active operation");
            return result;
        }
        auto checkpoint = m_store.LoadCheckpoint(
            acCommand.Campaign, acCommand.Checkpoint);
        if (!checkpoint)
        {
            result.Command = StoreFailure(checkpoint);
            return result;
        }
        const bool exactMember = std::any_of(
            checkpoint.Value.Slots.begin(), checkpoint.Value.Slots.end(),
            [&](const CheckpointSlotRecord& acSlot)
            {
                return IsExactCheckpointMember(acSlot, acCommand.Actor);
            });
        if (!exactMember)
        {
            result.Command = CheckpointFailure(
                CampaignError::NotCampaignMember,
                "admitted identity is not an exact checkpoint slot");
            return result;
        }
        auto campaign = m_store.LoadCampaign(acCommand.Campaign);
        if (!campaign)
        {
            result.Command = StoreFailure(campaign);
            return result;
        }
        result.Command.Version = campaign.Value.CurrentRevision;
        m_activeCheckpoints.erase(active);
        return result;
    }
    catch (...)
    {
        result.Command = CheckpointFailure(
            CampaignError::PersistenceFailure,
            "FailCheckpoint failed safely",
            StoreError::DatabaseFailure);
        return result;
    }
}

void CampaignRuntimeService::AbandonCheckpoint(
    const CampaignId& acCampaign) noexcept
{
    m_activeCheckpoints.erase(acCampaign.Value);
}

std::optional<CampaignCheckpointActivity>
CampaignRuntimeService::GetActiveCheckpoint(
    const CampaignId& acCampaign) const noexcept
{
    try
    {
        const auto active = m_activeCheckpoints.find(acCampaign.Value);
        return active == m_activeCheckpoints.end()
            ? std::nullopt
            : std::optional<CampaignCheckpointActivity>(active->second);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

CampaignRecoveryCommandResult CampaignRuntimeService::BeginRecovery(
    const BeginCampaignRecoveryCommand& acCommand) noexcept
{
    CampaignRecoveryCommandResult result;
    try
    {
        if (acCommand.DisconnectedMember.Campaign != acCommand.Campaign)
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryMismatch,
                "disconnected member does not belong to the recovery campaign");
            return result;
        }
        if (auto failure = ReconstructRecovery(acCommand.Campaign))
        {
            result.Command = *failure;
            return result;
        }

        auto existing = m_recoveries.find(acCommand.Campaign.Value);
        if (existing != m_recoveries.end())
        {
            if (existing->second.Stage !=
                CampaignRecoveryStage::RecoveryLock)
            {
                existing->second.Stage = CampaignRecoveryStage::RecoveryLock;
                existing->second.LoadedSlots.clear();
                existing->second.SnapshotAppliedSlots.clear();
            }
            existing->second.Reason = acCommand.CampaignLoadRequested
                ? CampaignRecoveryReason::CampaignLoadRequested
                : CampaignRecoveryReason::RosterIncomplete;
            auto campaign = m_store.LoadCampaign(acCommand.Campaign);
            if (!campaign)
            {
                result.Command = StoreFailure(campaign);
                return result;
            }
            result.Command.Version = campaign.Value.CurrentRevision;
            result.Command.IdempotentReplay = true;
            result.Activity = existing->second;
            return result;
        }

        if (acCommand.RuntimeStateBeforeDisconnect !=
                CampaignRuntimeState::ACTIVE &&
            acCommand.RuntimeStateBeforeDisconnect !=
                CampaignRuntimeState::CHECKPOINTING)
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryNotActive,
                "a new recovery can begin only from ACTIVE or CHECKPOINTING");
            return result;
        }

        LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
        if (loaded.Error != CampaignError::None)
        {
            result.Command = {
                loaded.Error, loaded.PersistenceError, loaded.Message};
            return result;
        }
        if (!loaded.Campaign.RosterSealed)
        {
            result.Command = RecoveryFailure(
                CampaignError::RosterNotSealed,
                "unsealed campaign disconnect does not enter collective recovery");
            return result;
        }
        const auto member = std::find_if(
            loaded.Campaign.Roster.begin(),
            loaded.Campaign.Roster.end(),
            [&](const CampaignSlotState& acSlot)
            {
                return acSlot.Slot == acCommand.DisconnectedMember.Slot &&
                    acSlot.Player == acCommand.DisconnectedMember.Player &&
                    acSlot.CharacterBinding ==
                        acCommand.DisconnectedMember.CharacterBinding;
            });
        if (member == loaded.Campaign.Roster.end())
        {
            result.Command = RecoveryFailure(
                CampaignError::NotCampaignMember,
                "disconnect is not an exact sealed-roster member");
            return result;
        }
        const bool fullRoster = CampaignStateMachine::EvaluateFullRoster(
            loaded.Campaign, acCommand.Presence).Eligible();
        if (!acCommand.CampaignLoadRequested && fullRoster)
        {
            result.Command = RecoveryFailure(
                CampaignError::InvalidRoster,
                "collective recovery requires an observed roster loss");
            return result;
        }
        if (acCommand.CampaignLoadRequested && !fullRoster)
        {
            result.Command = RecoveryFailure(
                CampaignError::RosterIncomplete,
                "campaign load recovery waits for the exact full roster");
            return result;
        }

        m_activeCheckpoints.erase(acCommand.Campaign.Value);
        CampaignMutationRequest request;
        request.Campaign = acCommand.Campaign;
        request.ExpectedRevision = loaded.Campaign.Version;
        request.Mutation = RecoveryBeginMutationId(
            acCommand.Campaign, loaded.Campaign.Version);
        request.Kind = "BeginRecovery";
        request.MutationPayload = BeginCommandPayload("BeginRecovery");
        AppendField(
            request.MutationPayload,
            acCommand.DisconnectedMember.Slot.Value);
        AppendField(
            request.MutationPayload,
            acCommand.DisconnectedMember.Player.Value);
        AppendField(
            request.MutationPayload,
            acCommand.DisconnectedMember.CharacterBinding.Value);
        AppendField(
            request.MutationPayload,
            acCommand.CampaignLoadRequested
                ? "campaign-load" : "roster-loss");

        const MutationResult persisted = m_store.ApplyMutation(request);
        result.Command = FromMutationResult(persisted);
        if (!result.Command)
            return result;

        CampaignRecoveryActivity activity;
        activity.Campaign = acCommand.Campaign;
        activity.EntryRevision = persisted.Revision;
        activity.Attempt = MakeRestoreAttempt(
            acCommand.Campaign, persisted.Revision);
        activity.Reason = acCommand.CampaignLoadRequested
            ? CampaignRecoveryReason::CampaignLoadRequested
            : CampaignRecoveryReason::RosterIncomplete;
        const auto [inserted, wasInserted] = m_recoveries.emplace(
            acCommand.Campaign.Value, std::move(activity));
        (void)wasInserted;
        result.Activity = inserted->second;
        return result;
    }
    catch (...)
    {
        result.Command = RecoveryFailure(
            CampaignError::PersistenceFailure,
            "BeginRecovery failed safely",
            StoreError::DatabaseFailure);
        return result;
    }
}

CampaignRecoveryCommandResult CampaignRuntimeService::PrepareRecovery(
    const PrepareCampaignRecoveryCommand& acCommand) noexcept
{
    CampaignRecoveryCommandResult result;
    try
    {
        if (auto failure = ReconstructRecovery(acCommand.Campaign))
        {
            result.Command = *failure;
            return result;
        }
        auto active = m_recoveries.find(acCommand.Campaign.Value);
        if (active == m_recoveries.end())
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryNotActive,
                "campaign has no open recovery attempt");
            return result;
        }

        LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
        if (loaded.Error != CampaignError::None)
        {
            result.Command = {
                loaded.Error, loaded.PersistenceError, loaded.Message};
            return result;
        }
        result.Command.Version = loaded.Campaign.Version;
        const FullRosterEvaluation fullRoster =
            CampaignStateMachine::EvaluateFullRoster(
                loaded.Campaign, acCommand.Presence);
        if (!fullRoster.Eligible())
        {
            active->second.Reason = CampaignRecoveryReason::RosterIncomplete;
            result.Command = RecoveryFailure(
                CampaignError::RosterIncomplete, fullRoster.Message);
            result.Command.Version = loaded.Campaign.Version;
            result.Activity = active->second;
            return result;
        }

        if (active->second.Stage ==
            CampaignRecoveryStage::ApplyingSnapshot)
        {
            if (!active->second.Checkpoint ||
                !active->second.RestoreRevision)
            {
                result.Command = RecoveryFailure(
                    CampaignError::IntegrityFailure,
                    "durable recovery restore provenance is incomplete",
                    StoreError::IntegrityFailure);
                return result;
            }
            auto checkpoint = m_store.LoadCheckpoint(
                acCommand.Campaign, *active->second.Checkpoint);
            if (!checkpoint)
            {
                result.Command = StoreFailure(checkpoint);
                return result;
            }
            active->second.Reason = CampaignRecoveryReason::None;
            result.Activity = active->second;
            result.Checkpoint = std::move(checkpoint.Value);
            result.Dispatch = CampaignRecoveryDispatch::RestoredSnapshot;
            return result;
        }

        auto checkpoint = active->second.Checkpoint
            ? m_store.LoadCheckpoint(
                  acCommand.Campaign, *active->second.Checkpoint)
            : m_store.LoadLastCommittedCheckpoint(acCommand.Campaign);
        if (!checkpoint)
        {
            if (checkpoint.Error == StoreError::NotFound)
            {
                active->second.Reason =
                    CampaignRecoveryReason::NoCommittedCheckpoint;
                result.Command = RecoveryFailure(
                    CampaignError::NoCommittedCheckpoint,
                    "campaign recovery is locked because no committed checkpoint exists");
                result.Command.Version = loaded.Campaign.Version;
                result.Activity = active->second;
                return result;
            }
            result.Command = StoreFailure(checkpoint);
            return result;
        }
        if (checkpoint.Value.State != CheckpointState::Committed ||
            checkpoint.Value.Slots.size() != loaded.Campaign.Roster.size() ||
            std::any_of(
                checkpoint.Value.Slots.begin(),
                checkpoint.Value.Slots.end(),
                [](const CheckpointSlotRecord& acSlot)
                {
                    return !acSlot.NativeSaveIdentity ||
                        !IsExactCheckpointArtifact(
                            acSlot, *acSlot.NativeSaveIdentity);
                }))
        {
            result.Command = RecoveryFailure(
                CampaignError::IntegrityFailure,
                "last committed checkpoint is not a complete recoverable bundle",
                StoreError::IntegrityFailure);
            return result;
        }

        if (active->second.Stage == CampaignRecoveryStage::RecoveryLock)
        {
            active->second.LoadedSlots.clear();
            active->second.SnapshotAppliedSlots.clear();
        }
        active->second.Stage = CampaignRecoveryStage::LoadingNativeSaves;
        active->second.Reason = CampaignRecoveryReason::None;
        active->second.Checkpoint = checkpoint.Value.Id;
        active->second.SourceRevision = checkpoint.Value.SourceRevision;
        result.Activity = active->second;
        result.Checkpoint = std::move(checkpoint.Value);
        result.Dispatch = CampaignRecoveryDispatch::NativeLoad;
        return result;
    }
    catch (...)
    {
        result.Command = RecoveryFailure(
            CampaignError::PersistenceFailure,
            "PrepareRecovery failed safely",
            StoreError::DatabaseFailure);
        return result;
    }
}

CampaignRecoveryCommandResult CampaignRuntimeService::RecordRecoveryLoaded(
    const RecordCampaignRecoveryLoadedCommand& acCommand) noexcept
{
    CampaignRecoveryCommandResult result;
    try
    {
        if (auto failure = ReconstructRecovery(acCommand.Campaign))
        {
            result.Command = *failure;
            return result;
        }
        auto active = m_recoveries.find(acCommand.Campaign.Value);
        if (active == m_recoveries.end())
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryNotActive,
                "campaign has no open recovery attempt");
            return result;
        }
        if (active->second.Attempt != acCommand.Attempt ||
            active->second.Checkpoint != acCommand.Checkpoint ||
            acCommand.Actor.Campaign != acCommand.Campaign)
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryMismatch,
                "native-load acknowledgement does not match the open recovery attempt");
            return result;
        }

        LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
        if (loaded.Error != CampaignError::None)
        {
            result.Command = {
                loaded.Error, loaded.PersistenceError, loaded.Message};
            return result;
        }
        result.Command.Version = loaded.Campaign.Version;
        const FullRosterEvaluation fullRoster =
            CampaignStateMachine::EvaluateFullRoster(
                loaded.Campaign, acCommand.Presence);
        if (!fullRoster.Eligible())
        {
            result.Command = RecoveryFailure(
                CampaignError::RosterIncomplete, fullRoster.Message);
            result.Command.Version = loaded.Campaign.Version;
            return result;
        }

        auto checkpoint = m_store.LoadCheckpoint(
            acCommand.Campaign, acCommand.Checkpoint);
        if (!checkpoint)
        {
            result.Command = StoreFailure(checkpoint);
            return result;
        }
        const auto slot = std::find_if(
            checkpoint.Value.Slots.begin(),
            checkpoint.Value.Slots.end(),
            [&](const CheckpointSlotRecord& acSlot)
            {
                return IsExactCheckpointMember(acSlot, acCommand.Actor);
            });
        if (slot == checkpoint.Value.Slots.end())
        {
            result.Command = RecoveryFailure(
                CampaignError::NotCampaignMember,
                "native-load acknowledgement is not an exact checkpoint slot");
            return result;
        }
        if (!acCommand.Succeeded)
        {
            active->second.Reason = CampaignRecoveryReason::ClientLoadFailed;
            result.Command = RecoveryFailure(
                CampaignError::InvalidCheckpointArtifact,
                "client native checkpoint load failed; recovery remains locked");
            result.Command.Version = loaded.Campaign.Version;
            result.Activity = active->second;
            return result;
        }
        if (!MatchesRecoveryProof(*slot, acCommand))
        {
            result.Command = RecoveryFailure(
                CampaignError::InvalidCheckpointArtifact,
                "native-load acknowledgement proof differs from the committed checkpoint slot");
            result.Command.Version = loaded.Campaign.Version;
            return result;
        }
        if (active->second.Stage !=
                CampaignRecoveryStage::LoadingNativeSaves &&
            active->second.Stage !=
                CampaignRecoveryStage::ApplyingSnapshot)
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryMismatch,
                "native-load acknowledgement arrived outside the load barrier");
            result.Command.Version = loaded.Campaign.Version;
            return result;
        }

        const bool durableReplay =
            active->second.DurableLoadedSlots.contains(slot->Slot.Value);
        if (!durableReplay)
        {
            CampaignMutationRequest acknowledgement;
            acknowledgement.Campaign = acCommand.Campaign;
            acknowledgement.ExpectedRevision = loaded.Campaign.Version;
            acknowledgement.Mutation = RecoverySlotMutationId(
                "loaded", active->second.Attempt, slot->Slot);
            acknowledgement.Kind = "RecoveryLoaded";
            acknowledgement.AdvancesStateVersion = false;
            acknowledgement.MutationPayload = RecoveryLoadedPayload(
                active->second.Attempt, checkpoint.Value, *slot);
            const MutationResult persisted =
                m_store.ApplyMutation(acknowledgement);
            result.Command = FromMutationResult(persisted);
            if (!result.Command)
                return result;
            active->second.DurableLoadedSlots.insert(slot->Slot.Value);
        }

        if (active->second.Stage ==
            CampaignRecoveryStage::ApplyingSnapshot)
        {
            result.Command.IdempotentReplay = true;
            result.Activity = active->second;
            result.Checkpoint = std::move(checkpoint.Value);
            result.Dispatch = CampaignRecoveryDispatch::RestoredSnapshot;
            result.FirstBarrierCompleted = true;
            return result;
        }
        const auto [ack, inserted] = active->second.LoadedSlots.insert(
            slot->Slot.Value);
        (void)ack;
        result.Command.IdempotentReplay = durableReplay || !inserted;
        result.Activity = active->second;
        result.Checkpoint = checkpoint.Value;
        if (active->second.LoadedSlots.size() !=
            checkpoint.Value.Slots.size())
        {
            return result;
        }

        const MutationId restoreMutation = RecoveryMutationId(
            "restore", active->second.Attempt);
        auto journal = m_store.LoadJournal(acCommand.Campaign);
        if (!journal)
        {
            result.Command = StoreFailure(journal);
            return result;
        }
        const auto existingRestore = std::find_if(
            journal.Value.begin(), journal.Value.end(),
            [&](const JournalRecord& acRecord)
            {
                return acRecord.Mutation == restoreMutation;
            });
        if (existingRestore != journal.Value.end())
        {
            if (existingRestore->Kind != "RestoreCheckpoint" ||
                existingRestore->RestoredFromCheckpoint !=
                    acCommand.Checkpoint ||
                existingRestore->RestoredFromRevision !=
                    checkpoint.Value.SourceRevision)
            {
                result.Command = RecoveryFailure(
                    CampaignError::IntegrityFailure,
                    "recovery restore replay provenance conflicts with the open attempt",
                    StoreError::IntegrityFailure);
                return result;
            }
            result.Command.Version = existingRestore->ResultingRevision;
            result.Command.IdempotentReplay = true;
        }
        else
        {
            auto campaign = m_store.LoadCampaign(acCommand.Campaign);
            if (!campaign)
            {
                result.Command = StoreFailure(campaign);
                return result;
            }
            std::vector<CampaignSlotRecord> checkpointRoster;
            checkpointRoster.reserve(checkpoint.Value.Slots.size());
            for (const CheckpointSlotRecord& checkpointSlot :
                 checkpoint.Value.Slots)
            {
                checkpointRoster.push_back(
                    {checkpointSlot.Slot,
                     checkpointSlot.Player,
                     checkpointSlot.CharacterBinding});
            }
            const auto coreMutation = std::find_if(
                journal.Value.rbegin(),
                journal.Value.rend(),
                [&](const JournalRecord& acRecord)
                {
                    return acRecord.ResultingRevision <=
                            checkpoint.Value.SourceRevision &&
                        ProducesCoreState(acRecord);
                });
            const std::optional<StateVersion> historicalCoreRevision =
                coreMutation == journal.Value.rend()
                ? std::nullopt
                : std::optional<StateVersion>(
                      coreMutation->ResultingRevision);
            auto checkpointCore = DecodeCanonicalCoreState(
                acCommand.Campaign,
                true,
                checkpointRoster,
                checkpoint.Value.SourceRevision,
                checkpoint.Value.SnapshotCoreStatePayload,
                journal.Value,
                historicalCoreRevision);
            if (!checkpointCore)
            {
                result.Command = RecoveryFailure(
                    CampaignError::IntegrityFailure,
                    "checkpoint canonical core state is unavailable",
                    checkpointCore.Error);
                return result;
            }
            checkpointCore.Value.Version =
                campaign.Value.CurrentRevision + 1;
            RestoreCheckpointRequest restore;
            restore.Campaign = acCommand.Campaign;
            restore.ExpectedRevision = campaign.Value.CurrentRevision;
            restore.Mutation = restoreMutation;
            restore.Checkpoint = acCommand.Checkpoint;
            restore.RestoredCoreStateCodecVersion =
                RuntimeCodec::kCampaignCoreCodecVersion;
            const StoreResult encodedRestoredCore =
                RuntimeCodec::EncodeCoreState(
                    checkpointCore.Value,
                    restore.RestoredCoreStatePayload);
            if (!encodedRestoredCore)
            {
                result.Command = StoreFailure(encodedRestoredCore);
                return result;
            }
            restore.MutationPayload = RestoreRecoveryPayload(
                active->second.Attempt, acCommand.Checkpoint);
            const MutationResult restored =
                m_store.RestoreCheckpointSnapshot(restore);
            result.Command = FromMutationResult(restored);
            if (!result.Command)
                return result;
        }

        active->second.Stage = CampaignRecoveryStage::ApplyingSnapshot;
        active->second.Reason = CampaignRecoveryReason::None;
        active->second.RestoreRevision = result.Command.Version;
        active->second.SnapshotAppliedSlots.clear();
        result.Activity = active->second;
        result.Checkpoint = std::move(checkpoint.Value);
        result.Dispatch = CampaignRecoveryDispatch::RestoredSnapshot;
        result.FirstBarrierCompleted = true;
        return result;
    }
    catch (...)
    {
        result.Command = RecoveryFailure(
            CampaignError::PersistenceFailure,
            "RecordRecoveryLoaded failed safely",
            StoreError::DatabaseFailure);
        return result;
    }
}

CampaignRecoveryCommandResult
CampaignRuntimeService::RecordRecoverySnapshotApplied(
    const RecordCampaignRecoverySnapshotAppliedCommand& acCommand) noexcept
{
    CampaignRecoveryCommandResult result;
    try
    {
        if (auto failure = ReconstructRecovery(acCommand.Campaign))
        {
            result.Command = *failure;
            return result;
        }
        auto active = m_recoveries.find(acCommand.Campaign.Value);
        if (active == m_recoveries.end())
        {
            auto checkpoint = m_store.LoadCheckpoint(
                acCommand.Campaign, acCommand.Checkpoint);
            if (!checkpoint)
            {
                result.Command = StoreFailure(checkpoint);
                return result;
            }
            const bool exactMember = std::any_of(
                checkpoint.Value.Slots.begin(),
                checkpoint.Value.Slots.end(),
                [&](const CheckpointSlotRecord& acSlot)
                {
                    return IsExactCheckpointMember(acSlot, acCommand.Actor);
                });
            if (!exactMember)
            {
                result.Command = RecoveryFailure(
                    CampaignError::NotCampaignMember,
                    "snapshot acknowledgement is not an exact checkpoint slot");
                return result;
            }
            const Bytes payload = CompleteRecoveryPayload(
                acCommand.Attempt,
                acCommand.Checkpoint,
                acCommand.RestoreRevision);
            auto journal = m_store.LoadJournal(acCommand.Campaign);
            if (!journal)
            {
                result.Command = StoreFailure(journal);
                return result;
            }
            const auto completed = std::find_if(
                journal.Value.begin(), journal.Value.end(),
                [&](const JournalRecord& acRecord)
                {
                    return acRecord.Mutation == RecoveryMutationId(
                               "complete", acCommand.Attempt) &&
                        acRecord.Kind == "CompleteRecovery" &&
                        acRecord.ResultingRevision ==
                            acCommand.RestoreRevision &&
                        acRecord.Payload == payload;
                });
            if (completed == journal.Value.end())
            {
                result.Command = RecoveryFailure(
                    CampaignError::RecoveryNotActive,
                    "campaign has no matching open or completed recovery attempt");
                return result;
            }
            result.Command.Version = completed->ResultingRevision;
            result.Command.IdempotentReplay = true;
            result.RecoveryCompleted = true;
            return result;
        }

        if (active->second.Stage !=
                CampaignRecoveryStage::ApplyingSnapshot ||
            active->second.Attempt != acCommand.Attempt ||
            active->second.Checkpoint != acCommand.Checkpoint ||
            active->second.RestoreRevision != acCommand.RestoreRevision ||
            acCommand.Actor.Campaign != acCommand.Campaign)
        {
            result.Command = RecoveryFailure(
                CampaignError::RecoveryMismatch,
                "snapshot acknowledgement does not match the restored recovery attempt");
            return result;
        }

        LoadedCampaign loaded = LoadAggregate(m_store, acCommand.Campaign);
        if (loaded.Error != CampaignError::None)
        {
            result.Command = {
                loaded.Error, loaded.PersistenceError, loaded.Message};
            return result;
        }
        result.Command.Version = loaded.Campaign.Version;
        const FullRosterEvaluation fullRoster =
            CampaignStateMachine::EvaluateFullRoster(
                loaded.Campaign, acCommand.Presence);
        if (!fullRoster.Eligible())
        {
            result.Command = RecoveryFailure(
                CampaignError::RosterIncomplete, fullRoster.Message);
            result.Command.Version = loaded.Campaign.Version;
            return result;
        }
        auto checkpoint = m_store.LoadCheckpoint(
            acCommand.Campaign, acCommand.Checkpoint);
        if (!checkpoint)
        {
            result.Command = StoreFailure(checkpoint);
            return result;
        }
        const auto slot = std::find_if(
            checkpoint.Value.Slots.begin(),
            checkpoint.Value.Slots.end(),
            [&](const CheckpointSlotRecord& acSlot)
            {
                return IsExactCheckpointMember(acSlot, acCommand.Actor);
            });
        if (slot == checkpoint.Value.Slots.end())
        {
            result.Command = RecoveryFailure(
                CampaignError::NotCampaignMember,
                "snapshot acknowledgement is not an exact checkpoint slot");
            return result;
        }

        const bool durableReplay =
            active->second.DurableSnapshotAppliedSlots.contains(
                slot->Slot.Value);
        if (!durableReplay)
        {
            CampaignMutationRequest acknowledgement;
            acknowledgement.Campaign = acCommand.Campaign;
            acknowledgement.ExpectedRevision = loaded.Campaign.Version;
            acknowledgement.Mutation = RecoverySlotMutationId(
                "applied", active->second.Attempt, slot->Slot);
            acknowledgement.Kind = "RecoverySnapshotApplied";
            acknowledgement.AdvancesStateVersion = false;
            acknowledgement.MutationPayload =
                RecoverySnapshotAppliedPayload(
                    active->second.Attempt,
                    checkpoint.Value,
                    acCommand.RestoreRevision,
                    *slot);
            const MutationResult persisted =
                m_store.ApplyMutation(acknowledgement);
            result.Command = FromMutationResult(persisted);
            if (!result.Command)
                return result;
            active->second.DurableSnapshotAppliedSlots.insert(
                slot->Slot.Value);
        }

        const auto [ack, inserted] =
            active->second.SnapshotAppliedSlots.insert(slot->Slot.Value);
        (void)ack;
        result.Command.IdempotentReplay = durableReplay || !inserted;
        result.Activity = active->second;
        result.Checkpoint = checkpoint.Value;
        if (active->second.SnapshotAppliedSlots.size() !=
            checkpoint.Value.Slots.size())
        {
            return result;
        }

        CampaignMutationRequest complete;
        complete.Campaign = acCommand.Campaign;
        complete.ExpectedRevision = acCommand.RestoreRevision;
        complete.Mutation = RecoveryMutationId(
            "complete", acCommand.Attempt);
        complete.Kind = "CompleteRecovery";
        complete.AdvancesStateVersion = false;
        complete.MutationPayload = CompleteRecoveryPayload(
            acCommand.Attempt,
            acCommand.Checkpoint,
            acCommand.RestoreRevision);
        const MutationResult completed = m_store.ApplyMutation(complete);
        result.Command = FromMutationResult(completed);
        if (!result.Command)
            return result;

        result.Activity = active->second;
        result.Checkpoint = std::move(checkpoint.Value);
        result.RecoveryCompleted = true;
        m_recoveries.erase(active);
        return result;
    }
    catch (...)
    {
        result.Command = RecoveryFailure(
            CampaignError::PersistenceFailure,
            "RecordRecoverySnapshotApplied failed safely",
            StoreError::DatabaseFailure);
        return result;
    }
}

std::optional<CampaignRecoveryActivity>
CampaignRuntimeService::GetRecoveryActivity(
    const CampaignId& acCampaign) noexcept
{
    if (ReconstructRecovery(acCampaign))
        return std::nullopt;
    const auto active = m_recoveries.find(acCampaign.Value);
    return active == m_recoveries.end()
        ? std::nullopt
        : std::optional<CampaignRecoveryActivity>(active->second);
}

CampaignLoadResult CampaignRuntimeService::LoadCampaign(
    const CampaignId& acCampaign,
    const std::vector<CampaignMemberPresence>& acPresence) noexcept try
{
    LoadedCampaign loaded = LoadAggregate(m_store, acCampaign);
    CampaignLoadResult result;
    result.Error = loaded.Error;
    result.PersistenceError = loaded.PersistenceError;
    result.Message = std::move(loaded.Message);
    result.Campaign = std::move(loaded.Campaign);
    if (result.Succeeded())
    {
        if (auto recoveryFailure = ReconstructRecovery(acCampaign))
        {
            result.Error = recoveryFailure->Error;
            result.PersistenceError = recoveryFailure->PersistenceError;
            result.Message = recoveryFailure->Message;
        }
        else if (const auto recovery = m_recoveries.find(acCampaign.Value);
                 recovery != m_recoveries.end())
        {
            result.RuntimeState = recovery->second.Stage ==
                    CampaignRecoveryStage::RecoveryLock
                ? CampaignRuntimeState::RECOVERY_LOCK
                : CampaignRuntimeState::RESTORING_CHECKPOINT;
        }
        else
        {
            result.RuntimeState = m_activeCheckpoints.contains(acCampaign.Value)
                ? CampaignRuntimeState::CHECKPOINTING
                : CampaignStateMachine::DetermineRuntimeState(
                      result.Campaign, acPresence);
        }
    }
    return result;
}
catch (...)
{
    CampaignLoadResult result;
    result.Error = CampaignError::PersistenceFailure;
    result.PersistenceError = StoreError::DatabaseFailure;
    result.Message = "LoadCampaign failed safely";
    return result;
}

CampaignCreationLookupResult CampaignRuntimeService::FindCampaignCreation(
    const PlayerId& acPlayer,
    const MutationId& acMutation) noexcept try
{
    CampaignCreationLookupResult result;
    if (acPlayer.Value.empty() || acMutation.Value.empty())
    {
        result.Error = CampaignError::InvalidIdentity;
        result.PersistenceError = StoreError::InvalidArgument;
        result.Message = "campaign creation lookup requires durable identities";
        return result;
    }

    auto entries = m_store.LoadJournalByMutation(
        acMutation, "CreateCampaign");
    if (!entries)
    {
        result.Error = TranslateStoreError(entries.Error);
        result.PersistenceError = entries.Error;
        result.Message = std::move(entries.Message);
        return result;
    }
    for (const JournalRecord& entry : entries.Value)
    {
        if (entry.ExpectedRevision != 0 || entry.ResultingRevision != 1 ||
            entry.PayloadCodecVersion != 1 ||
            entry.RestoredFromCheckpoint || entry.RestoredFromRevision)
        {
            result.Error = CampaignError::IntegrityFailure;
            result.PersistenceError = StoreError::IntegrityFailure;
            result.Message =
                "campaign creation journal metadata is inconsistent";
            return result;
        }
        std::vector<CampaignSlotRecord> initialRoster;
        if (!DecodeCreateCampaignPayload(entry.Payload, initialRoster))
        {
            result.Error = CampaignError::IntegrityFailure;
            result.PersistenceError = StoreError::IntegrityFailure;
            result.Message =
                "campaign creation journal payload is malformed";
            return result;
        }
        const auto slot = std::find_if(
            initialRoster.begin(), initialRoster.end(),
            [&](const CampaignSlotRecord& acSlot)
            {
                return acSlot.Player == acPlayer;
            });
        if (slot == initialRoster.end())
            continue;
        if (result.Identity)
        {
            result.Error = CampaignError::IntegrityFailure;
            result.PersistenceError = StoreError::IntegrityFailure;
            result.Message =
                "campaign creation mutation resolves to multiple campaigns";
            return result;
        }
        result.Identity = CampaignMemberIdentity{
            entry.Campaign,
            slot->Slot,
            slot->Player,
            slot->CharacterBinding};
        result.Version = entry.ResultingRevision;
    }
    return result;
}
catch (...)
{
    CampaignCreationLookupResult result;
    result.Error = CampaignError::PersistenceFailure;
    result.PersistenceError = StoreError::DatabaseFailure;
    result.Message = "campaign creation lookup failed safely";
    return result;
}
}
