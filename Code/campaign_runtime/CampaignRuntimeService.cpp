#include <CampaignRuntimeService.h>

#include <algorithm>
#include <limits>
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
        auto decoded = RuntimeCodec::DecodeCoreState(
            projection.Value.Campaign.Id,
            projection.Value.Campaign.RosterSealed,
            projection.Value.Slots,
            projection.Value.Campaign.CurrentRevision,
            projection.Value.Campaign.CoreStatePayload);
        if (!decoded)
        {
            loaded.Error = CampaignError::IntegrityFailure;
            loaded.PersistenceError = decoded.Error;
            loaded.Message = decoded.Message;
            return loaded;
        }
        loaded.Campaign = std::move(decoded.Value);
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
    Current,
    PossibleReplay
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
    if (aExpected != std::numeric_limits<StateVersion>::max() &&
        aCurrent == aExpected + 1)
    {
        aMode = RevisionMode::PossibleReplay;
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
}

CampaignRuntimeService::CampaignRuntimeService(ICampaignStore& aStore) noexcept
    : m_store(aStore)
{
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
        result.RuntimeState = CampaignStateMachine::DetermineRuntimeState(
            result.Campaign, acPresence);
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
}
