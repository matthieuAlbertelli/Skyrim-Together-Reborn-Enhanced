#include <sqlite/SqlitePrimitives.h>

#include <sqlite3.h>

#include <chrono>
#include <utility>

namespace STRE::Campaign::Sqlite
{
std::int64_t NowUnixMs() noexcept
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

StoreResult Failure(StoreError aError, std::string aMessage)
{
    return {aError, std::move(aMessage)};
}

MutationResult MutationFailure(StoreError aError, std::string aMessage)
{
    MutationResult result;
    result.Error = aError;
    result.Message = std::move(aMessage);
    return result;
}

std::string DatabaseMessage(sqlite3* apDatabase, std::string_view acContext)
{
    std::string message(acContext);
    message += ": ";
    message += apDatabase ? sqlite3_errmsg(apDatabase) : "database unavailable";
    return message;
}

StoreResult Execute(sqlite3* apDatabase, const char* apSql)
{
    char* pError = nullptr;
    const int code = sqlite3_exec(apDatabase, apSql, nullptr, nullptr, &pError);
    if (code == SQLITE_OK)
        return {};

    std::string message = pError ? pError : sqlite3_errstr(code);
    sqlite3_free(pError);
    return Failure(StoreError::DatabaseFailure, std::move(message));
}

Statement::Statement(sqlite3* apDatabase, const char* apSql)
    : m_pDatabase(apDatabase)
    , m_code(SQLITE_ERROR)
{
    m_code = sqlite3_prepare_v2(
        m_pDatabase, apSql, -1, &m_pStatement, nullptr);
}

Statement::~Statement()
{
    if (m_pStatement)
        sqlite3_finalize(m_pStatement);
}

bool Statement::Valid() const noexcept
{
    return m_code == SQLITE_OK && m_pStatement;
}

int Statement::Code() const noexcept { return m_code; }

bool Statement::BindText(int aIndex, std::string_view acValue)
{
    return sqlite3_bind_text(
               m_pStatement,
               aIndex,
               acValue.data(),
               static_cast<int>(acValue.size()),
               SQLITE_TRANSIENT) == SQLITE_OK;
}

bool Statement::BindBlob(int aIndex, const Bytes& acValue)
{
    return sqlite3_bind_blob64(
               m_pStatement,
               aIndex,
               acValue.empty() ? nullptr : acValue.data(),
               static_cast<sqlite3_uint64>(acValue.size()),
               SQLITE_TRANSIENT) == SQLITE_OK;
}

bool Statement::BindInt(int aIndex, int aValue)
{
    return sqlite3_bind_int(m_pStatement, aIndex, aValue) == SQLITE_OK;
}

bool Statement::BindInt64(int aIndex, std::int64_t aValue)
{
    return sqlite3_bind_int64(m_pStatement, aIndex, aValue) == SQLITE_OK;
}

bool Statement::BindNull(int aIndex)
{
    return sqlite3_bind_null(m_pStatement, aIndex) == SQLITE_OK;
}

int Statement::Step() { return sqlite3_step(m_pStatement); }

std::int64_t Statement::Int64(int aColumn) const
{
    return sqlite3_column_int64(m_pStatement, aColumn);
}

int Statement::Int(int aColumn) const
{
    return sqlite3_column_int(m_pStatement, aColumn);
}

std::string Statement::Text(int aColumn) const
{
    const auto* pText = sqlite3_column_text(m_pStatement, aColumn);
    const int size = sqlite3_column_bytes(m_pStatement, aColumn);
    if (!pText || size <= 0)
        return {};
    return {
        reinterpret_cast<const char*>(pText),
        static_cast<std::size_t>(size)};
}

Bytes Statement::Blob(int aColumn) const
{
    const auto* pBlob = static_cast<const std::uint8_t*>(
        sqlite3_column_blob(m_pStatement, aColumn));
    const int size = sqlite3_column_bytes(m_pStatement, aColumn);
    if (!pBlob || size <= 0)
        return {};
    return {pBlob, pBlob + size};
}

bool Statement::IsNull(int aColumn) const
{
    return sqlite3_column_type(m_pStatement, aColumn) == SQLITE_NULL;
}

Transaction::Transaction(sqlite3* apDatabase)
    : m_pDatabase(apDatabase)
{
    const StoreResult result = Execute(m_pDatabase, "BEGIN IMMEDIATE;");
    m_active = result.Succeeded();
    m_error = result;
}

Transaction::~Transaction()
{
    if (m_active)
        (void)Execute(m_pDatabase, "ROLLBACK;");
}

bool Transaction::Active() const noexcept { return m_active; }

const StoreResult& Transaction::Error() const noexcept { return m_error; }

StoreResult Transaction::Commit()
{
    if (!m_active)
        return m_error;
    StoreResult result = Execute(m_pDatabase, "COMMIT;");
    if (result)
        m_active = false;
    return result;
}

StoreResult BindFailure(sqlite3* apDatabase, std::string_view acContext)
{
    return Failure(StoreError::DatabaseFailure, DatabaseMessage(apDatabase, acContext));
}

StoreResult StepDone(
    sqlite3* apDatabase,
    Statement& aStatement,
    std::string_view acContext)
{
    if (!aStatement.Valid())
        return BindFailure(apDatabase, acContext);
    const int code = aStatement.Step();
    if (code != SQLITE_DONE)
        return BindFailure(apDatabase, acContext);
    return {};
}
}
