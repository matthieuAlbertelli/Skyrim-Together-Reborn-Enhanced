#pragma once

#include <CampaignTypes.h>

#include <optional>
#include <string_view>
#include <vector>

struct sqlite3;

namespace STRE::Campaign::Sqlite
{
struct ReplayResult
{
    StoreResult Result;
    bool Found{};
    bool Conflict{};
    StateVersion Revision{};
};

ReplayResult CheckReplay(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const MutationId& acMutation,
    std::string_view acDigest);
StoreValueResult<StateVersion> ReadCurrentRevision(
    sqlite3* apDatabase,
    const CampaignId& acCampaign);
StoreResult AppendJournal(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const MutationId& acMutation,
    StateVersion aExpectedRevision,
    StateVersion aResultingRevision,
    std::string_view acKind,
    std::string_view acDigest,
    std::uint32_t aCodecVersion,
    const Bytes& acPayload,
    std::int64_t aCreatedAt,
    const std::optional<CheckpointId>& acRestoredCheckpoint = std::nullopt,
    const std::optional<StateVersion>& acRestoredRevision = std::nullopt);
StoreResult AppendOutbox(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const MutationId& acMutation,
    StateVersion aRevision,
    const std::vector<OutboxIntent>& acIntents,
    std::int64_t aCreatedAt);
StoreResult UpdateCampaignRevision(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    StateVersion aExpectedRevision,
    StateVersion aNewRevision,
    std::int64_t aUpdatedAt);
MutationResult ReplayToMutationResult(const ReplayResult& acReplay);
}
