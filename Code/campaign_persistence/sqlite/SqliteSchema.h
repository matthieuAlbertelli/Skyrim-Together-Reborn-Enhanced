#pragma once

#include <CampaignTypes.h>
#include <sqlite/SqliteCampaignStore.h>

#include <cstdint>

struct sqlite3;

namespace STRE::Campaign::Sqlite
{
StoreResult Initialize(
    sqlite3* apDatabase,
    const SqliteCampaignStoreOptions& acOptions) noexcept;
StoreValueResult<std::uint32_t> GetSchemaVersion(sqlite3* apDatabase) noexcept;
StoreResult CheckIntegrity(sqlite3* apDatabase) noexcept;
}
