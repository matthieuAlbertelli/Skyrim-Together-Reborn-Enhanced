#include <sqlite/SqliteCampaignStore.h>

#include <sqlite/SqlitePrimitives.h>
#include <sqlite/SqliteSchema.h>

#include <sqlite3.h>

#include <limits>
#include <utility>

namespace STRE::Campaign
{
using Sqlite::DatabaseMessage;
using Sqlite::Failure;

SqliteCampaignStore::SqliteCampaignStore(
    sqlite3* apDatabase,
    std::filesystem::path aPath,
    SqliteCampaignStoreOptions aOptions) noexcept
    : m_pDatabase(apDatabase)
    , m_path(std::move(aPath))
    , m_options(std::move(aOptions))
{
}

SqliteCampaignStore::~SqliteCampaignStore()
{
    if (m_pDatabase)
        sqlite3_close_v2(m_pDatabase);
}

std::unique_ptr<SqliteCampaignStore> SqliteCampaignStore::Open(
    const std::filesystem::path& acPath,
    StoreResult& aResult,
    SqliteCampaignStoreOptions aOptions) noexcept
{
    aResult = {};
    try
    {
        if (acPath.empty())
        {
            aResult = Failure(StoreError::InvalidArgument, "campaign database path is empty");
            return nullptr;
        }
        if (aOptions.BusyTimeout.count() < 0 ||
            aOptions.BusyTimeout.count() > std::numeric_limits<int>::max())
        {
            aResult = Failure(StoreError::InvalidArgument, "SQLite busy timeout is outside supported bounds");
            return nullptr;
        }

        const std::filesystem::path parent = acPath.parent_path();
        if (!parent.empty())
        {
            std::error_code error;
            std::filesystem::create_directories(parent, error);
            if (error)
            {
                aResult = Failure(
                    StoreError::DatabaseFailure,
                    "cannot create campaign database directory path=" +
                        parent.string() + " reason=" + error.message());
                return nullptr;
            }
        }

        sqlite3* pDatabase = nullptr;
        const std::u8string utf8 = acPath.u8string();
        const std::string utf8Path = utf8.empty()
            ? std::string{}
            : std::string(
                  reinterpret_cast<const char*>(utf8.data()),
                  utf8.size());
        const int code = sqlite3_open_v2(
            utf8Path.c_str(),
            &pDatabase,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
            nullptr);
        if (code != SQLITE_OK)
        {
            aResult = Failure(
                StoreError::DatabaseFailure,
                DatabaseMessage(pDatabase, "open campaign database path=" + acPath.string()));
            if (pDatabase)
                sqlite3_close_v2(pDatabase);
            return nullptr;
        }

        auto store = std::unique_ptr<SqliteCampaignStore>(
            new SqliteCampaignStore(pDatabase, acPath, std::move(aOptions)));
        aResult = Sqlite::Initialize(store->m_pDatabase, store->m_options);
        if (!aResult)
            return nullptr;
        return store;
    }
    catch (const std::exception& exception)
    {
        aResult = Failure(
            StoreError::DatabaseFailure,
            "campaign database initialization exception path=" +
                acPath.string() + " reason=" + exception.what());
        return nullptr;
    }
    catch (...)
    {
        aResult = Failure(
            StoreError::DatabaseFailure,
            "campaign database initialization failed path=" + acPath.string());
        return nullptr;
    }
}


bool SqliteCampaignStore::ShouldInject(TransactionStage aStage) const
{
    return m_options.FaultInjector && m_options.FaultInjector(aStage);
}

StoreValueResult<std::uint32_t> SqliteCampaignStore::GetSchemaVersion() noexcept
{
    return Sqlite::GetSchemaVersion(m_pDatabase);
}

StoreResult SqliteCampaignStore::CheckIntegrity() noexcept
{
    return Sqlite::CheckIntegrity(m_pDatabase);
}
}
