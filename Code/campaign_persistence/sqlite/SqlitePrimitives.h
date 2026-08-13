#pragma once

#include <CampaignTypes.h>

#include <cstdint>
#include <string>
#include <string_view>

struct sqlite3;
struct sqlite3_stmt;

namespace STRE::Campaign::Sqlite
{
std::int64_t NowUnixMs() noexcept;
StoreResult Failure(StoreError aError, std::string aMessage);
MutationResult MutationFailure(StoreError aError, std::string aMessage);
std::string DatabaseMessage(sqlite3* apDatabase, std::string_view acContext);
StoreResult Execute(sqlite3* apDatabase, const char* apSql);

class Statement
{
public:
    Statement(sqlite3* apDatabase, const char* apSql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    [[nodiscard]] bool Valid() const noexcept;
    [[nodiscard]] int Code() const noexcept;

    bool BindText(int aIndex, std::string_view acValue);
    bool BindBlob(int aIndex, const Bytes& acValue);
    bool BindInt(int aIndex, int aValue);
    bool BindInt64(int aIndex, std::int64_t aValue);
    bool BindNull(int aIndex);

    int Step();
    [[nodiscard]] std::int64_t Int64(int aColumn) const;
    [[nodiscard]] int Int(int aColumn) const;
    [[nodiscard]] std::string Text(int aColumn) const;
    [[nodiscard]] Bytes Blob(int aColumn) const;
    [[nodiscard]] bool IsNull(int aColumn) const;

private:
    sqlite3* m_pDatabase{};
    sqlite3_stmt* m_pStatement{};
    int m_code{};
};

class Transaction
{
public:
    explicit Transaction(sqlite3* apDatabase);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    [[nodiscard]] bool Active() const noexcept;
    [[nodiscard]] const StoreResult& Error() const noexcept;
    StoreResult Commit();

private:
    sqlite3* m_pDatabase{};
    bool m_active{};
    StoreResult m_error;
};

StoreResult BindFailure(sqlite3* apDatabase, std::string_view acContext);
StoreResult StepDone(
    sqlite3* apDatabase,
    Statement& aStatement,
    std::string_view acContext);
}
