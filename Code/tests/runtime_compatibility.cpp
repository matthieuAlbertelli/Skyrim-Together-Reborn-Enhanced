#include <catch2/catch.hpp>
#include <Runtime/AddressLibrary.h>
#include <Runtime/Compatibility.h>
#include <Runtime/Executable.h>

#include <algorithm>
#include <limits>
#include <vector>
#ifdef _WIN32
#include "../client/VersionDb.h"
#endif

using namespace STRE::Runtime;

namespace
{
using Bytes = std::vector<std::uint8_t>;

void Put(Bytes& aBytes, std::size_t aOffset, std::uint64_t aValue, std::size_t aSize = 4)
{
    REQUIRE(aOffset + aSize <= aBytes.size());
    for (std::size_t i = 0; i < aSize; ++i)
        aBytes[aOffset + i] = static_cast<std::uint8_t>(aValue >> (8 * i));
}

void Append(Bytes& aBytes, std::uint64_t aValue, std::size_t aSize)
{
    const auto offset = aBytes.size();
    aBytes.resize(offset + aSize);
    Put(aBytes, offset, aValue, aSize);
}

Bytes Library(std::uint32_t aFormat)
{
    Bytes bytes;
    Append(bytes, aFormat, 4);
    for (const auto part : TargetVersion)
        Append(bytes, part, 4);
    const std::string name = "SkyrimSE.exe";
    if (aFormat == 2)
        Append(bytes, name.size(), 4);
    bytes.insert(bytes.end(), name.begin(), name.end());
    if (aFormat == 5)
        bytes.resize(84);
    Append(bytes, 8, aFormat == 5 ? 8 : 4);
    Append(bytes, aFormat == 5 ? 3 : 2, 4);
    if (aFormat == 5)
    {
        Append(bytes, 0, 4);
        Append(bytes, 0x100, 4);
        Append(bytes, 0x101, 4);
    }
    else
    {
        Append(bytes, 0, 1);
        Append(bytes, 1, 8);
        Append(bytes, 0x100, 8);
        Append(bytes, 0x11, 1);
    }
    return bytes;
}

Bytes Pe(std::string_view aImport = "steam_api64.dll")
{
    Bytes bytes(0x600);
    Put(bytes, 0, 0x5A4D, 2);
    Put(bytes, 0x3C, 0x80);
    Put(bytes, 0x80, 0x4550);
    Put(bytes, 0x84, 0x8664, 2);
    Put(bytes, 0x86, 2, 2);
    Put(bytes, 0x94, 240, 2);
    Put(bytes, 0x98, 0x20B, 2);
    Put(bytes, 0x98 + 16, 0x3100);
    Put(bytes, 0x98 + 56, 0x4000);
    Put(bytes, 0x98 + 108, 16);
    Put(bytes, 0x98 + 120, 0x2000);
    Put(bytes, 0x98 + 124, 40);
    const std::string names[2]{".rdata", ".bind"};
    for (std::size_t i = 0; i < 2; ++i)
    {
        const auto section = 0x188 + 40 * i;
        std::copy(names[i].begin(), names[i].end(), bytes.begin() + section);
        Put(bytes, section + 8, 0x200);
        Put(bytes, section + 12, 0x2000 + 0x1000 * i);
        Put(bytes, section + 16, 0x200);
        Put(bytes, section + 20, 0x200 + 0x200 * i);
    }
    Put(bytes, 0x20C, 0x2040);
    std::copy(aImport.begin(), aImport.end(), bytes.begin() + 0x240);
    return bytes;
}
} // namespace

TEST_CASE("Address Library v2 and sparse v5 resolve without admitting another runtime", "[runtime-port][address-library]")
{
    for (const auto format : {2, 5})
    {
        AddressLibrary db;
        const std::array<std::uint32_t, 2> required{1, 2};
        REQUIRE(db.Load(Library(format), TargetVersion, 0x10000, required));
        REQUIRE(db.LoadedVersion() == TargetVersion);
        REQUIRE(db.ModuleName() == "SkyrimSE.exe");
        REQUIRE(db.Offsets().size() == 2);
        unsigned long long offset{};
        REQUIRE(db.FindOffset(1, offset));
        REQUIRE(offset == 0x100);
        REQUIRE(db.FindOffset(2, offset));
        REQUIRE(offset == 0x101);
        REQUIRE_FALSE(db.FindOffset(0, offset));
        REQUIRE(offset == 0);
        REQUIRE_FALSE(db.FindOffset(42, offset));
    }
}

TEST_CASE("Every truncated Address Library fails transactionally, including reload", "[runtime-port][address-library]")
{
    for (const auto format : {2, 5})
    {
        const auto bytes = Library(format);
        AddressLibrary db;
        for (std::size_t size = 0; size < bytes.size(); ++size)
        {
            REQUIRE(db.Load(bytes, TargetVersion, 0x10000));
            INFO("format=" << format << " prefix=" << size);
            REQUIRE_FALSE(db.Load(std::span(bytes).first(size), TargetVersion, 0x10000));
            REQUIRE(db.Offsets().empty());
            REQUIRE(db.ModuleName().empty());
            REQUIRE(db.LoadedVersion() == Version{});
            REQUIRE_FALSE(db.Error().empty());
        }
    }
}

TEST_CASE("Address Library rejects incoherent headers, sizes, counts and missing required IDs", "[runtime-port][address-library]")
{
    for (const auto format : {2, 5})
    {
        const auto valid = Library(format);
        const auto pointerAt = format == 5 ? 84 : 36;
        const auto countAt = format == 5 ? 92 : 40;
        for (const auto field : std::vector<std::pair<std::size_t, std::uint32_t>>{
                 {0, 3}, {8, 6}, {pointerAt, 0}, {pointerAt, 4}, {countAt, 0},
                 {countAt, UINT32_MAX}, {countAt, AddressLibrary::MaxAddresses + 1}})
        {
            auto bytes = valid;
            Put(bytes, field.first, field.second);
            AddressLibrary db;
            REQUIRE_FALSE(db.Load(bytes, TargetVersion, 0x10000));
            REQUIRE(db.Offsets().empty());
        }
        auto bytes = valid;
        bytes[format == 5 ? 20 : 24] = 'X';
        AddressLibrary db;
        REQUIRE_FALSE(db.Load(bytes, TargetVersion, 0x10000));
        bytes = valid;
        bytes.push_back(0);
        REQUIRE_FALSE(db.Load(bytes, TargetVersion, 0x10000));
        REQUIRE_FALSE(db.Load(valid, TargetVersion, 0));
        REQUIRE_FALSE(db.Load(valid, TargetVersion, 0x100));
        const std::array<std::uint32_t, 2> missing{1, 9};
        REQUIRE_FALSE(db.Load(valid, TargetVersion, 0x10000, missing));
        REQUIRE(db.Error() == "required relocation missing: 9");
        REQUIRE(db.Offsets().empty());
    }
    auto bytes = Library(2);
    Put(bytes, 20, UINT32_MAX);
    AddressLibrary db;
    REQUIRE_FALSE(db.Load(bytes, TargetVersion, 0x10000));
    bytes = Library(5);
    bytes[40] = 'X'; // Padding after NUL must not disguise another module.
    REQUIRE_FALSE(db.Load(bytes, TargetVersion, 0x10000));
    bytes = Library(5);
    std::fill(bytes.begin() + 96, bytes.end(), 0);
    REQUIRE_FALSE(db.Load(bytes, TargetVersion, 0x10000));
}

TEST_CASE("Address Library v2 validates compressed arithmetic and duplicate IDs", "[runtime-port][address-library]")
{
    // First entry id=1 offset=0x100; replace second compressed record.
    for (const auto kind : {0, 1, 2, 3, 4, 5, 6, 7})
    {
        auto bytes = Library(2);
        bytes.resize(bytes.size() - 1);
        Append(bytes, (0x8 | kind) << 4 | 1, 1);
        const std::size_t width = kind == 0 ? 8 : kind == 1 ? 0 : kind <= 3 ? 1 : kind <= 6 ? 2 : 4;
        if (width)
            Append(bytes, kind == 0 || kind >= 6 ? 33 : 1, width);
        AddressLibrary db;
        REQUIRE(db.Load(bytes, TargetVersion, 0x10000));
        unsigned long long offset{};
        REQUIRE(db.FindOffset(2, offset));
        REQUIRE(offset == (kind == 3 || kind == 5 ? 31 * 8 : 33 * 8));
    }
    for (const auto code : {0x18, 0x13, 0x80})
    {
        auto bytes = Library(2);
        bytes.resize(bytes.size() - 1);
        Append(bytes, code, 1);
        if (code == 0x13)
            Append(bytes, 2, 1); // ID underflow.
        if (code == 0x80)
        {
            Append(bytes, 2, 8);
            Append(bytes, UINT64_MAX, 8); // Scaled offset overflow.
        }
        AddressLibrary db;
        REQUIRE_FALSE(db.Load(bytes, TargetVersion, 0x10000));
        REQUIRE(db.Offsets().empty());
    }
    auto bytes = Library(2);
    bytes.back() = 0x12;
    Append(bytes, 0, 1); // Duplicate id=1.
    AddressLibrary db;
    REQUIRE_FALSE(db.Load(bytes, TargetVersion, 0x10000));
}

TEST_CASE("Runtime admission is exact and Steam-specific", "[runtime-port]")
{
    REQUIRE(Supports("1.7.104.0", Distribution::Steam));
    for (const auto version : {"", "1.6.1170.0", "1.7.99.0", "1.7.104", "1.7.104.1", "1.7.105.0", "2.0.0.0", "1.7.104.0extra"})
        REQUIRE_FALSE(Supports(version, Distribution::Steam));
    REQUIRE_FALSE(Supports("1.7.104.0", Distribution::Other));
    REQUIRE_FALSE(Supports("1.7.104.0", Distribution::Unknown));
}

TEST_CASE("PE preflight requires Steam evidence independent of the version", "[runtime-port]")
{
    Executable result;
    REQUIRE(InspectExecutable(Pe(), result));
    REQUIRE(result.ImageSize == 0x4000);
    REQUIRE(Supports("1.7.104.0", result.Store));
    REQUIRE(InspectExecutable(Pe("galaxy64.dll"), result));
    REQUIRE_FALSE(Supports("1.7.104.0", result.Store));
    REQUIRE(InspectExecutable(Pe("kernel32.dll"), result));
    REQUIRE_FALSE(Supports("1.7.104.0", result.Store));
    auto bytes = Pe();
    bytes[0x1B0] = 'X'; // No protected Steam entry section.
    REQUIRE(InspectExecutable(bytes, result));
    REQUIRE_FALSE(Supports("1.7.104.0", result.Store));
    for (const auto field : std::vector<std::pair<std::size_t, std::uint32_t>>{
             {0x3C, UINT32_MAX}, {0x98 + 56, 0}, {0x98 + 16, 0x4000},
             {0x98 + 124, UINT32_MAX}, {0x188 + 20, UINT32_MAX}, {0x20C, UINT32_MAX}})
    {
        bytes = Pe();
        Put(bytes, field.first, field.second);
        REQUIRE_FALSE(InspectExecutable(bytes, result));
        REQUIRE(result.Store == Distribution::Unknown);
        REQUIRE(result.ImageSize == 0);
    }
    const auto valid = Pe();
    for (std::size_t size = 0; size < valid.size(); ++size)
        REQUIRE_FALSE(InspectExecutable(std::span(valid).first(size), result));
}

#ifdef _WIN32
TEST_CASE("VersionDb clears exposed addresses and version after a failed reload", "[runtime-port][address-library]")
{
    const auto root = std::filesystem::temp_directory_path() /
        ("stre-runtime-fixture-" + std::to_string(GetCurrentProcessId()));
    // This disposable fixture owns only its one synthetic database.
    REQUIRE_FALSE(std::filesystem::exists(root));
    std::filesystem::create_directories(root / "Data/SKSE/Plugins");
    struct Cleanup
    {
        std::filesystem::path Root;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(Root, error); }
    } cleanup{root};
    auto bytes = Library(5);
    // Bootstrap IDs required by the real VersionDb adapter.
    bytes.resize(96 + 53927 * 4, 0);
    Put(bytes, 92, 53927);
    for (const auto id : {36544, 36564, 53926, 40412})
        Put(bytes, 96 + id * 4, 0x100);
    const auto file = root / "Data/SKSE/Plugins/versionlib-1-7-104-0.bin";
    {
        std::ofstream stream(file, std::ios::binary);
        stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    auto& db = VersionDb::Get();
    VersionDbPtr<void> cached(36544);
    REQUIRE(db.Load(root, "1.7.104.0", 0x10000));
    REQUIRE(db.FindAddressById(36544) != nullptr);
    REQUIRE(cached.GetPtr() != nullptr);
    REQUIRE(db.GetLoadedVersionString() == "1.7.104.0");
    REQUIRE(db.DumpToTextFile((root / "addresses.txt").string()));
    {
        std::ifstream dump(root / "addresses.txt");
        std::string line;
        REQUIRE(static_cast<bool>(std::getline(dump, line)));
        REQUIRE(line.find('\t') != std::string::npos);
        REQUIRE(line.ends_with("140000100"));
    }
    REQUIRE_FALSE(db.Load(root, "1.7.104.0trailing", 0x10000));
    REQUIRE(db.GetOffsetMap().empty());
    REQUIRE(db.GetLoadedVersionString().empty());
    REQUIRE(db.FindAddressById(36544) == nullptr);
    REQUIRE(cached.GetPtr() == nullptr);
    REQUIRE(db.Load(root, "1.7.104.0", 0x10000));
    REQUIRE(std::filesystem::remove(file));
    REQUIRE_FALSE(db.Load(root, "1.7.104.0", 0x10000));
    REQUIRE(db.GetError().find("missing/unreadable") != std::string::npos);
    REQUIRE(db.GetLoadedVersionString().empty());
    REQUIRE(db.FindAddressById(36544) == nullptr);
}
#endif
