#include <sqlite/SqliteCampaignState.h>

#include <codec/CampaignCodec.h>
#include <sqlite/SqliteCampaignStore.h>
#include <sqlite/SqliteJournalOutbox.h>
#include <sqlite/SqlitePrimitives.h>
#include <sqlite/SqliteValidation.h>

#include <sqlite3.h>

#include <unordered_set>
#include <utility>

namespace STRE::Campaign::Sqlite
{
StoreResult InsertSlot(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CampaignSlotRecord& acSlot)
{
    Statement statement(
        apDatabase,
        "INSERT INTO campaign_slots(campaign_id, slot_id, player_id, character_binding_id) "
        "VALUES(?1, ?2, ?3, ?4);");
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acSlot.Slot.Value) ||
        !statement.BindText(3, acSlot.Player.Value) ||
        !statement.BindText(4, acSlot.CharacterBinding.Value))
    {
        return BindFailure(apDatabase, "insert campaign slot");
    }
    return StepDone(apDatabase, statement, "insert campaign slot");
}

StoreResult VerifySlotBinding(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CampaignSlotId& acSlot,
    const CharacterBindingId& acBinding)
{
    Statement statement(
        apDatabase,
        "SELECT character_binding_id FROM campaign_slots "
        "WHERE campaign_id=?1 AND slot_id=?2;");
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acSlot.Value))
    {
        return BindFailure(apDatabase, "verify campaign slot binding");
    }
    const int code = statement.Step();
    if (code == SQLITE_DONE)
    {
        return Failure(
            StoreError::InvalidArgument,
            "campaign=" + acCampaign.Value + " slot=" + acSlot.Value +
                " does not exist");
    }
    if (code != SQLITE_ROW)
        return BindFailure(apDatabase, "verify campaign slot binding");
    if (statement.Text(0) != acBinding.Value)
    {
        return Failure(
            StoreError::InvalidArgument,
            "campaign=" + acCampaign.Value + " slot=" + acSlot.Value +
                " CharacterBinding mismatch");
    }
    return {};
}

StoreResult UpsertCharacterBuild(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CharacterBuildState& acBuild,
    StateVersion aRevision)
{
    StoreResult slotResult = VerifySlotBinding(
        apDatabase, acCampaign, acBuild.Slot, acBuild.CharacterBinding);
    if (!slotResult)
        return slotResult;
    Bytes encoded;
    StoreResult encodeResult = Codec::EncodeCharacterBuild(acBuild, encoded);
    if (!encodeResult)
        return encodeResult;

    Statement statement(
        apDatabase,
        "INSERT INTO character_build_state("
        "campaign_id, slot_id, character_binding_id, persistence_codec_version, "
        "build_version, class_id, inventory_hash, spell_hash, state_payload, updated_revision) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10) "
        "ON CONFLICT(campaign_id, slot_id) DO UPDATE SET "
        "character_binding_id=excluded.character_binding_id, "
        "persistence_codec_version=excluded.persistence_codec_version, "
        "build_version=excluded.build_version, class_id=excluded.class_id, "
        "inventory_hash=excluded.inventory_hash, spell_hash=excluded.spell_hash, "
        "state_payload=excluded.state_payload, updated_revision=excluded.updated_revision;");
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acBuild.Slot.Value) ||
        !statement.BindText(3, acBuild.CharacterBinding.Value) ||
        !statement.BindInt(4, static_cast<int>(acBuild.PersistenceCodecVersion)) ||
        !statement.BindInt64(5, static_cast<std::int64_t>(acBuild.BuildVersion)) ||
        !statement.BindText(6, acBuild.ClassId) ||
        !statement.BindText(7, Hex64(acBuild.InventoryHash)) ||
        !statement.BindText(8, Hex64(acBuild.SpellHash)) ||
        !statement.BindBlob(9, encoded) ||
        !statement.BindInt64(10, static_cast<std::int64_t>(aRevision)))
    {
        return BindFailure(apDatabase, "upsert durable character build");
    }
    return StepDone(apDatabase, statement, "upsert durable character build");
}

StoreResult UpsertAdapterState(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const AdapterState& acState,
    StateVersion aRevision)
{
    Statement statement(
        apDatabase,
        "INSERT INTO adapter_state("
        "campaign_id, adapter_id, adapter_version, codec_version, audience, "
        "audience_player_id, state_payload, updated_revision) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8) "
        "ON CONFLICT(campaign_id, adapter_id) DO UPDATE SET "
        "adapter_version=excluded.adapter_version, codec_version=excluded.codec_version, "
        "audience=excluded.audience, audience_player_id=excluded.audience_player_id, "
        "state_payload=excluded.state_payload, updated_revision=excluded.updated_revision;");
    const bool bound = statement.Valid() &&
        statement.BindText(1, acCampaign.Value) &&
        statement.BindText(2, acState.AdapterId) &&
        statement.BindInt64(3, static_cast<std::int64_t>(acState.AdapterVersion)) &&
        statement.BindInt(4, static_cast<int>(acState.CodecVersion)) &&
        statement.BindInt(5, static_cast<int>(acState.Audience)) &&
        (acState.AudiencePlayer
             ? statement.BindText(6, acState.AudiencePlayer->Value)
             : statement.BindNull(6)) &&
        statement.BindBlob(7, acState.Payload) &&
        statement.BindInt64(8, static_cast<std::int64_t>(aRevision));
    if (!bound)
    {
        return BindFailure(apDatabase, "upsert campaign adapter state");
    }
    return StepDone(apDatabase, statement, "upsert campaign adapter state");
}

StoreResult DeleteByTextKey(
    sqlite3* apDatabase,
    const char* apSql,
    const CampaignId& acCampaign,
    std::string_view acKey,
    std::string_view acContext)
{
    Statement statement(apDatabase, apSql);
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acKey))
    {
        return BindFailure(apDatabase, acContext);
    }
    return StepDone(apDatabase, statement, acContext);
}

}

namespace STRE::Campaign
{
using namespace Sqlite;

StoreValueResult<CampaignRecord> SqliteCampaignStore::LoadCampaign(
    const CampaignId& acCampaign) noexcept
{
    StoreValueResult<CampaignRecord> result;
    try
    {
        if (!IsValidId(acCampaign))
        {
            result.Error = StoreError::InvalidArgument;
            result.Message = "invalid CampaignId";
            return result;
        }
        Statement statement(
            m_pDatabase,
            "SELECT persistence_schema_version, current_revision, roster_sealed, "
            "last_committed_checkpoint_id, core_state_codec_version, core_state_payload, "
            "created_at_unix_ms, updated_at_unix_ms "
            "FROM campaigns WHERE campaign_id=?1;");
        if (!statement.Valid() || !statement.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load durable campaign");
            return result;
        }
        const int code = statement.Step();
        if (code == SQLITE_DONE)
        {
            result.Error = StoreError::NotFound;
            result.Message = "campaign=" + acCampaign.Value + " was not found";
            return result;
        }
        if (code != SQLITE_ROW || statement.Int64(0) <= 0 ||
            statement.Int64(1) < 0 || statement.Int(2) < 0 ||
            statement.Int(2) > 1 || statement.Int64(4) <= 0)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign=" + acCampaign.Value + " has malformed durable state";
            return result;
        }
        result.Value.Id = acCampaign;
        result.Value.PersistenceSchemaVersion =
            static_cast<std::uint32_t>(statement.Int64(0));
        result.Value.CurrentRevision =
            static_cast<StateVersion>(statement.Int64(1));
        result.Value.RosterSealed = statement.Int(2) != 0;
        if (!statement.IsNull(3))
            result.Value.LastCommittedCheckpoint = CheckpointId{statement.Text(3)};
        result.Value.CoreStateCodecVersion =
            static_cast<std::uint32_t>(statement.Int64(4));
        result.Value.CoreStatePayload = statement.Blob(5);
        result.Value.CreatedAtUnixMs = statement.Int64(6);
        result.Value.UpdatedAtUnixMs = statement.Int64(7);
        if (result.Value.PersistenceSchemaVersion >
                kCampaignDatabaseSchemaVersion ||
            !IsValidPayload(result.Value.CoreStatePayload))
        {
            result.Error = StoreError::IncompatibleSchema;
            result.Message = "campaign=" + acCampaign.Value +
                " uses unsupported persisted state";
        }
    }
    catch (...)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = "failed to load campaign=" + acCampaign.Value;
    }
    return result;
}


StoreValueResult<CampaignProjection> SqliteCampaignStore::LoadCampaignProjection(
    const CampaignId& acCampaign,
    const ProjectionAudience& acAudience) noexcept
{
    StoreValueResult<CampaignProjection> result;
    try
    {
        auto campaignResult = LoadCampaign(acCampaign);
        if (!campaignResult)
        {
            result.Error = campaignResult.Error;
            result.Message = campaignResult.Message;
            return result;
        }
        result.Value.Campaign = std::move(campaignResult.Value);

        Statement slots(
            m_pDatabase,
            "SELECT slot_id, player_id, character_binding_id FROM campaign_slots "
            "WHERE campaign_id=?1 ORDER BY slot_id;");
        if (!slots.Valid() || !slots.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load durable campaign roster");
            return result;
        }
        int slotCode{};
        for (slotCode = slots.Step(); slotCode == SQLITE_ROW;
             slotCode = slots.Step())
        {
            CampaignSlotRecord slot{
                CampaignSlotId{slots.Text(0)},
                PlayerId{slots.Text(1)},
                CharacterBindingId{slots.Text(2)}};
            if (!IsValidId(slot.Slot) || !IsValidId(slot.Player) ||
                !IsValidId(slot.CharacterBinding))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign=" + acCampaign.Value +
                    " contains malformed roster identity";
                return result;
            }
            result.Value.Slots.push_back(std::move(slot));
        }
        if (slotCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate durable campaign roster");
            return result;
        }

        Statement builds(
            m_pDatabase,
            "SELECT slot_id, character_binding_id, persistence_codec_version, "
            "build_version, class_id, inventory_hash, spell_hash, state_payload, updated_revision "
            "FROM character_build_state WHERE campaign_id=?1 ORDER BY slot_id;");
        if (!builds.Valid() || !builds.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load durable character builds");
            return result;
        }
        int buildCode{};
        for (buildCode = builds.Step(); buildCode == SQLITE_ROW;
             buildCode = builds.Step())
        {
            auto decoded = Codec::DecodeCharacterBuild(builds.Blob(7));
            std::uint64_t inventoryHash{};
            std::uint64_t spellHash{};
            if (!decoded ||
                decoded.Value.Slot.Value != builds.Text(0) ||
                decoded.Value.CharacterBinding.Value != builds.Text(1) ||
                decoded.Value.PersistenceCodecVersion !=
                    static_cast<std::uint32_t>(builds.Int64(2)) ||
                decoded.Value.BuildVersion !=
                    static_cast<std::uint32_t>(builds.Int64(3)) ||
                decoded.Value.ClassId != builds.Text(4) ||
                !ParseHex64(builds.Text(5), inventoryHash) ||
                !ParseHex64(builds.Text(6), spellHash) ||
                decoded.Value.InventoryHash != inventoryHash ||
                decoded.Value.SpellHash != spellHash ||
                builds.Int64(8) < 0)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign=" + acCampaign.Value +
                    " contains malformed character-build state";
                return result;
            }
            decoded.Value.UpdatedRevision =
                static_cast<StateVersion>(builds.Int64(8));
            result.Value.CharacterBuilds.push_back(std::move(decoded.Value));
        }
        if (buildCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate durable character builds");
            return result;
        }

        Statement adapters(
            m_pDatabase,
            "SELECT adapter_id, adapter_version, codec_version, audience, "
            "audience_player_id, state_payload, updated_revision "
            "FROM adapter_state WHERE campaign_id=?1 ORDER BY adapter_id;");
        if (!adapters.Valid() || !adapters.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load campaign adapter state");
            return result;
        }
        int adapterCode{};
        for (adapterCode = adapters.Step(); adapterCode == SQLITE_ROW;
             adapterCode = adapters.Step())
        {
            AdapterState state;
            state.AdapterId = adapters.Text(0);
            if (adapters.Int64(1) < 0 || adapters.Int64(2) <= 0 ||
                adapters.Int(3) < 0 || adapters.Int(3) > 1 ||
                adapters.Int64(6) < 0)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign=" + acCampaign.Value +
                    " contains malformed adapter metadata";
                return result;
            }
            state.AdapterVersion = static_cast<StateVersion>(adapters.Int64(1));
            state.CodecVersion = static_cast<std::uint32_t>(adapters.Int64(2));
            state.Audience = static_cast<StateAudience>(adapters.Int(3));
            if (!adapters.IsNull(4))
                state.AudiencePlayer = PlayerId{adapters.Text(4)};
            state.Payload = adapters.Blob(5);
            state.UpdatedRevision = static_cast<StateVersion>(adapters.Int64(6));
            StoreResult validation = ValidateAdapterState(state);
            if (!validation)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign=" + acCampaign.Value +
                    " contains invalid adapter state";
                return result;
            }

            const bool visible = state.Audience == StateAudience::Public ||
                (acAudience.IncludePrivate &&
                 (!acAudience.Player ||
                  state.AudiencePlayer == acAudience.Player));
            if (visible)
                result.Value.AdapterStates.push_back(std::move(state));
        }
        if (adapterCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate campaign adapter state");
            return result;
        }
    }
    catch (...)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = "failed to load campaign projection=" + acCampaign.Value;
    }
    return result;
}


MutationResult SqliteCampaignStore::CreateCampaign(
    const CreateCampaignRequest& acRequest) noexcept
{
    try
    {
        StoreResult common = ValidateCommonMutation(
            acRequest.Campaign.Id,
            0,
            acRequest.Mutation,
            "CreateCampaign",
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            acRequest.Outbox);
        if (!common)
            return MutationFailure(common.Error, common.Message);
        StoreResult slotValidation = ValidateSlots(acRequest.Slots);
        if (!slotValidation)
            return MutationFailure(slotValidation.Error, slotValidation.Message);
        if (acRequest.Campaign.RosterSealed && acRequest.Slots.empty())
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "a sealed campaign roster cannot be empty");
        }
        if (acRequest.Campaign.PersistenceSchemaVersion !=
                kCampaignDatabaseSchemaVersion ||
            acRequest.Campaign.CoreStateCodecVersion == 0 ||
            !IsValidPayload(acRequest.Campaign.CoreStatePayload))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "campaign persistence version or core state is invalid");
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuilds)
        {
            StoreResult validation = ValidateCharacterBuild(build);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }
        {
            std::unordered_set<std::string> buildSlots;
            for (const CharacterBuildState& build : acRequest.CharacterBuilds)
            {
                if (!buildSlots.insert(build.Slot.Value).second)
                    return MutationFailure(StoreError::InvalidArgument, "duplicate character-build slot");
            }
            std::unordered_set<std::string> adapterIds;
            for (const AdapterState& state : acRequest.AdapterStates)
            {
                if (!adapterIds.insert(state.AdapterId).second)
                    return MutationFailure(StoreError::InvalidArgument, "duplicate adapter-state identity");
            }
        }
        for (const AdapterState& state : acRequest.AdapterStates)
        {
            StoreResult validation = ValidateAdapterState(state);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }

        CampaignProjection digestProjection;
        digestProjection.Campaign = acRequest.Campaign;
        digestProjection.Campaign.CurrentRevision = 1;
        digestProjection.Campaign.CreatedAtUnixMs = 0;
        digestProjection.Campaign.UpdatedAtUnixMs = 0;
        digestProjection.Slots = acRequest.Slots;
        digestProjection.CharacterBuilds = acRequest.CharacterBuilds;
        digestProjection.AdapterStates = acRequest.AdapterStates;
        Bytes digestPayload;
        StoreResult encodeDigest = Codec::EncodeSnapshot(
            digestProjection, digestPayload);
        if (!encodeDigest)
            return MutationFailure(encodeDigest.Error, encodeDigest.Message);
        AppendDigestBlob(digestPayload, acRequest.MutationPayload);
        AppendDigestOutbox(digestPayload, acRequest.Outbox);
        const std::string digest = Codec::MutationDigest(
            "CreateCampaign",
            0,
            acRequest.MutationCodecVersion,
            digestPayload,
            acRequest.Campaign.Id.Value);
        if (digest.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute campaign creation digest");

        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return MutationFailure(transaction.Error().Error, transaction.Error().Message);
        ReplayResult replay = CheckReplay(
            m_pDatabase,
            acRequest.Campaign.Id,
            acRequest.Mutation,
            digest);
        if (replay.Found || !replay.Result)
            return ReplayToMutationResult(replay);

        const std::int64_t now = NowUnixMs();
        const std::int64_t createdAt = acRequest.Campaign.CreatedAtUnixMs != 0
            ? acRequest.Campaign.CreatedAtUnixMs
            : now;
        Statement campaign(
            m_pDatabase,
            "INSERT INTO campaigns("
            "campaign_id, persistence_schema_version, current_revision, roster_sealed, "
            "last_committed_checkpoint_id, core_state_codec_version, core_state_payload, "
            "created_at_unix_ms, updated_at_unix_ms) "
            "VALUES(?1, ?2, 1, ?3, NULL, ?4, ?5, ?6, ?7);");
        if (!campaign.Valid() ||
            !campaign.BindText(1, acRequest.Campaign.Id.Value) ||
            !campaign.BindInt(2, kCampaignDatabaseSchemaVersion) ||
            !campaign.BindInt(3, acRequest.Campaign.RosterSealed ? 1 : 0) ||
            !campaign.BindInt(4, static_cast<int>(acRequest.Campaign.CoreStateCodecVersion)) ||
            !campaign.BindBlob(5, acRequest.Campaign.CoreStatePayload) ||
            !campaign.BindInt64(6, createdAt) ||
            !campaign.BindInt64(7, now))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "insert durable campaign"));
        }
        StoreResult insertCampaign = StepDone(
            m_pDatabase, campaign, "insert durable campaign");
        if (!insertCampaign)
        {
            const StoreError error = sqlite3_extended_errcode(m_pDatabase) ==
                    SQLITE_CONSTRAINT_PRIMARYKEY
                ? StoreError::AlreadyExists
                : insertCampaign.Error;
            return MutationFailure(error, insertCampaign.Message);
        }
        for (const CampaignSlotRecord& slot : acRequest.Slots)
        {
            StoreResult inserted = InsertSlot(
                m_pDatabase, acRequest.Campaign.Id, slot);
            if (!inserted)
                return MutationFailure(inserted.Error, inserted.Message);
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuilds)
        {
            StoreResult upserted = UpsertCharacterBuild(
                m_pDatabase, acRequest.Campaign.Id, build, 1);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }
        for (const AdapterState& state : acRequest.AdapterStates)
        {
            StoreResult upserted = UpsertAdapterState(
                m_pDatabase, acRequest.Campaign.Id, state, 1);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }
        if (ShouldInject(TransactionStage::AfterCurrentState))
            return MutationFailure(StoreError::FaultInjected, "fault injected after current-state write");

        StoreResult journal = AppendJournal(
            m_pDatabase,
            acRequest.Campaign.Id,
            acRequest.Mutation,
            0,
            1,
            "CreateCampaign",
            digest,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            now);
        if (!journal)
            return MutationFailure(journal.Error, journal.Message);
        if (ShouldInject(TransactionStage::AfterJournal))
            return MutationFailure(StoreError::FaultInjected, "fault injected after journal write");
        StoreResult outbox = AppendOutbox(
            m_pDatabase,
            acRequest.Campaign.Id,
            acRequest.Mutation,
            1,
            acRequest.Outbox,
            now);
        if (!outbox)
            return MutationFailure(outbox.Error, outbox.Message);
        if (ShouldInject(TransactionStage::AfterOutbox) ||
            ShouldInject(TransactionStage::BeforeCommit))
        {
            return MutationFailure(StoreError::FaultInjected, "fault injected before campaign commit");
        }
        StoreResult committed = transaction.Commit();
        if (!committed)
            return MutationFailure(committed.Error, committed.Message);

        MutationResult result;
        result.Revision = 1;
        result.Applied = true;
        return result;
    }
    catch (...)
    {
        return MutationFailure(StoreError::DatabaseFailure, "CreateCampaign failed safely");
    }
}


MutationResult SqliteCampaignStore::ApplyMutation(
    const CampaignMutationRequest& acRequest) noexcept
{
    try
    {
        StoreResult common = ValidateCommonMutation(
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            acRequest.Mutation,
            acRequest.Kind,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            acRequest.Outbox);
        if (!common)
            return MutationFailure(common.Error, common.Message);
        if (acRequest.CoreStatePayload &&
            (!acRequest.CoreStateCodecVersion ||
             *acRequest.CoreStateCodecVersion == 0 ||
             !IsValidPayload(*acRequest.CoreStatePayload)))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "core state mutation requires a valid independent codec version and bounded payload");
        }
        if (acRequest.CoreStateCodecVersion && !acRequest.CoreStatePayload)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "core state codec cannot change without a payload");
        }
        if (acRequest.ReplacementRoster)
        {
            StoreResult validation = ValidateSlots(*acRequest.ReplacementRoster);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuildUpserts)
        {
            StoreResult validation = ValidateCharacterBuild(build);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }
        {
            std::unordered_set<std::string> buildSlots;
            for (const CharacterBuildState& build : acRequest.CharacterBuildUpserts)
            {
                if (!buildSlots.insert(build.Slot.Value).second)
                    return MutationFailure(StoreError::InvalidArgument, "duplicate character-build upsert slot");
            }
            std::unordered_set<std::string> adapterIds;
            for (const AdapterState& state : acRequest.AdapterStateUpserts)
            {
                if (!adapterIds.insert(state.AdapterId).second)
                    return MutationFailure(StoreError::InvalidArgument, "duplicate adapter-state upsert identity");
            }
        }
        for (const CampaignSlotId& slot : acRequest.CharacterBuildDeletes)
        {
            if (!IsValidId(slot))
                return MutationFailure(StoreError::InvalidArgument, "invalid character-build delete slot");
        }
        for (const AdapterState& state : acRequest.AdapterStateUpserts)
        {
            StoreResult validation = ValidateAdapterState(state);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }
        for (const std::string& adapter : acRequest.AdapterStateDeletes)
        {
            if (adapter.empty() || adapter.size() > kMaximumAdapterIdLength)
                return MutationFailure(StoreError::InvalidArgument, "invalid adapter-state delete identity");
        }

        Bytes digestPayload = acRequest.MutationPayload;
        if (acRequest.CoreStateCodecVersion)
            AppendDigestScalar(digestPayload, *acRequest.CoreStateCodecVersion);
        if (acRequest.CoreStatePayload)
            AppendDigestBlob(digestPayload, *acRequest.CoreStatePayload);
        if (acRequest.RosterSealed)
            AppendDigestScalar<std::uint8_t>(digestPayload, *acRequest.RosterSealed ? 1 : 0);
        if (acRequest.ReplacementRoster)
        {
            for (const CampaignSlotRecord& slot : *acRequest.ReplacementRoster)
            {
                AppendDigestText(digestPayload, slot.Slot.Value);
                AppendDigestText(digestPayload, slot.Player.Value);
                AppendDigestText(digestPayload, slot.CharacterBinding.Value);
            }
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuildUpserts)
        {
            Bytes encoded;
            StoreResult encodedResult = Codec::EncodeCharacterBuild(build, encoded);
            if (!encodedResult)
                return MutationFailure(encodedResult.Error, encodedResult.Message);
            AppendDigestBlob(digestPayload, encoded);
        }
        for (const CampaignSlotId& slot : acRequest.CharacterBuildDeletes)
            AppendDigestText(digestPayload, slot.Value);
        for (const AdapterState& state : acRequest.AdapterStateUpserts)
        {
            AppendDigestText(digestPayload, state.AdapterId);
            AppendDigestScalar(digestPayload, state.AdapterVersion);
            AppendDigestScalar(digestPayload, state.CodecVersion);
            AppendDigestScalar(digestPayload, static_cast<std::uint8_t>(state.Audience));
            AppendDigestText(
                digestPayload,
                state.AudiencePlayer ? state.AudiencePlayer->Value : std::string_view{});
            AppendDigestBlob(digestPayload, state.Payload);
        }
        for (const std::string& adapter : acRequest.AdapterStateDeletes)
            AppendDigestText(digestPayload, adapter);
        AppendDigestOutbox(digestPayload, acRequest.Outbox);
        const std::string digest = Codec::MutationDigest(
            acRequest.Kind,
            acRequest.ExpectedRevision,
            acRequest.MutationCodecVersion,
            digestPayload);
        if (digest.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute campaign mutation digest");

        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return MutationFailure(transaction.Error().Error, transaction.Error().Message);
        ReplayResult replay = CheckReplay(
            m_pDatabase, acRequest.Campaign, acRequest.Mutation, digest);
        if (replay.Found || !replay.Result)
            return ReplayToMutationResult(replay);
        auto currentRevision = ReadCurrentRevision(m_pDatabase, acRequest.Campaign);
        if (!currentRevision)
            return MutationFailure(currentRevision.Error, currentRevision.Message);
        if (currentRevision.Value != acRequest.ExpectedRevision)
        {
            return MutationFailure(
                StoreError::StaleRevision,
                "campaign=" + acRequest.Campaign.Value +
                    " expected_revision=" + std::to_string(acRequest.ExpectedRevision) +
                    " current_revision=" + std::to_string(currentRevision.Value));
        }
        if (currentRevision.Value >= kMaximumRevision)
            return MutationFailure(StoreError::InvalidArgument, "campaign revision exhausted");
        const StateVersion newRevision = currentRevision.Value + 1;

        bool rosterSealed{};
        {
            Statement statement(
                m_pDatabase,
                "SELECT roster_sealed FROM campaigns WHERE campaign_id=?1;");
            if (!statement.Valid() ||
                !statement.BindText(1, acRequest.Campaign.Value) ||

                statement.Step() != SQLITE_ROW)
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "load roster seal state"));
            }
            rosterSealed = statement.Int(0) != 0;
        }
        if (rosterSealed && acRequest.ReplacementRoster)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "campaign=" + acRequest.Campaign.Value +
                    " roster is sealed and cannot be replaced");
        }
        if (rosterSealed && acRequest.RosterSealed && !*acRequest.RosterSealed)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "campaign=" + acRequest.Campaign.Value + " roster seal is irreversible");
        }
        if (!rosterSealed && acRequest.RosterSealed &&
            *acRequest.RosterSealed)
        {
            bool rosterEmpty{};
            if (acRequest.ReplacementRoster)
            {
                rosterEmpty = acRequest.ReplacementRoster->empty();
            }
            else
            {
                Statement countRoster(
                    m_pDatabase,
                    "SELECT COUNT(*) FROM campaign_slots WHERE campaign_id=?1;");
                if (!countRoster.Valid() ||
                    !countRoster.BindText(1, acRequest.Campaign.Value) ||
                    countRoster.Step() != SQLITE_ROW)
                {
                    return MutationFailure(
                        StoreError::DatabaseFailure,
                        DatabaseMessage(m_pDatabase, "validate roster before seal"));
                }
                rosterEmpty = countRoster.Int64(0) == 0;
            }
            if (rosterEmpty)
            {
                return MutationFailure(
                    StoreError::InvalidArgument,
                    "a sealed campaign roster cannot be empty");
            }
        }

        if (acRequest.ReplacementRoster)
        {
            Statement deleteBuilds(
                m_pDatabase,
                "DELETE FROM character_build_state WHERE campaign_id=?1;");
            if (!deleteBuilds.Valid() ||
                !deleteBuilds.BindText(1, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "clear pre-seal character builds"));
            }
            StoreResult deletedBuilds = StepDone(
                m_pDatabase, deleteBuilds, "clear pre-seal character builds");
            if (!deletedBuilds)
                return MutationFailure(deletedBuilds.Error, deletedBuilds.Message);

            Statement deleteSlots(
                m_pDatabase,
                "DELETE FROM campaign_slots WHERE campaign_id=?1;");
            if (!deleteSlots.Valid() ||
                !deleteSlots.BindText(1, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "replace pre-seal campaign roster"));
            }
            StoreResult deletedSlots = StepDone(
                m_pDatabase, deleteSlots, "replace pre-seal campaign roster");
            if (!deletedSlots)
                return MutationFailure(deletedSlots.Error, deletedSlots.Message);
            for (const CampaignSlotRecord& slot : *acRequest.ReplacementRoster)
            {
                StoreResult inserted = InsertSlot(
                    m_pDatabase, acRequest.Campaign, slot);
                if (!inserted)
                    return MutationFailure(inserted.Error, inserted.Message);
            }
        }

        for (const CampaignSlotId& slot : acRequest.CharacterBuildDeletes)
        {
            StoreResult deleted = DeleteByTextKey(
                m_pDatabase,
                "DELETE FROM character_build_state WHERE campaign_id=?1 AND slot_id=?2;",
                acRequest.Campaign,
                slot.Value,
                "delete durable character build");
            if (!deleted)
                return MutationFailure(deleted.Error, deleted.Message);
        }
        for (const std::string& adapter : acRequest.AdapterStateDeletes)
        {
            StoreResult deleted = DeleteByTextKey(
                m_pDatabase,
                "DELETE FROM adapter_state WHERE campaign_id=?1 AND adapter_id=?2;",
                acRequest.Campaign,
                adapter,
                "delete campaign adapter state");
            if (!deleted)
                return MutationFailure(deleted.Error, deleted.Message);
        }
        if (acRequest.CoreStatePayload)
        {
            Statement statement(
                m_pDatabase,
                "UPDATE campaigns SET core_state_codec_version=?1, core_state_payload=?2 "
                "WHERE campaign_id=?3;");
            if (!statement.Valid() ||
                !statement.BindInt(1, static_cast<int>(*acRequest.CoreStateCodecVersion)) ||
                !statement.BindBlob(2, *acRequest.CoreStatePayload) ||
                !statement.BindText(3, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "update campaign core state"));
            }
            StoreResult updated = StepDone(
                m_pDatabase, statement, "update campaign core state");
            if (!updated)
                return MutationFailure(updated.Error, updated.Message);
        }
        if (acRequest.RosterSealed)
        {
            Statement statement(
                m_pDatabase,
                "UPDATE campaigns SET roster_sealed=?1 WHERE campaign_id=?2;");
            if (!statement.Valid() ||
                !statement.BindInt(1, *acRequest.RosterSealed ? 1 : 0) ||
                !statement.BindText(2, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "update durable roster seal metadata"));
            }
            StoreResult updated = StepDone(
                m_pDatabase, statement, "update durable roster seal metadata");
            if (!updated)
                return MutationFailure(updated.Error, updated.Message);
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuildUpserts)
        {
            StoreResult upserted = UpsertCharacterBuild(
                m_pDatabase, acRequest.Campaign, build, newRevision);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }
        for (const AdapterState& state : acRequest.AdapterStateUpserts)
        {
            StoreResult upserted = UpsertAdapterState(
                m_pDatabase, acRequest.Campaign, state, newRevision);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }

        const std::int64_t now = NowUnixMs();
        StoreResult revision = UpdateCampaignRevision(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            newRevision,
            now);
        if (!revision)
            return MutationFailure(revision.Error, revision.Message);
        if (ShouldInject(TransactionStage::AfterCurrentState))
            return MutationFailure(StoreError::FaultInjected, "fault injected after current-state write");
        StoreResult journal = AppendJournal(
            m_pDatabase,

            acRequest.Campaign,
            acRequest.Mutation,
            acRequest.ExpectedRevision,
            newRevision,
            acRequest.Kind,
            digest,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            now);
        if (!journal)
            return MutationFailure(journal.Error, journal.Message);
        if (ShouldInject(TransactionStage::AfterJournal))
            return MutationFailure(StoreError::FaultInjected, "fault injected after journal write");
        StoreResult outbox = AppendOutbox(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            newRevision,
            acRequest.Outbox,
            now);
        if (!outbox)
            return MutationFailure(outbox.Error, outbox.Message);
        if (ShouldInject(TransactionStage::AfterOutbox) ||
            ShouldInject(TransactionStage::BeforeCommit))
        {
            return MutationFailure(StoreError::FaultInjected, "fault injected before mutation commit");
        }
        StoreResult committed = transaction.Commit();
        if (!committed)
            return MutationFailure(committed.Error, committed.Message);

        MutationResult result;
        result.Revision = newRevision;
        result.Applied = true;
        return result;
    }
    catch (...)
    {
        return MutationFailure(
            StoreError::DatabaseFailure,
            "campaign=" + acRequest.Campaign.Value + " mutation failed safely");
    }
}
}
