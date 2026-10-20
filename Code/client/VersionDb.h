#pragma once

#include <Runtime/AddressLibrary.h>
#include <Windows.h>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string_view>
#include <vector>

class VersionDb
{
public:
    static VersionDb& Get();

    const std::string& GetModuleName() const { return m_library.ModuleName(); }
    const std::string& GetLoadedVersionString() const { return m_versionString; }
    const std::string& GetError() const { return m_error; }
    std::uint64_t Generation() const { return m_generation; }
    const STRE::Runtime::OffsetMap& GetOffsetMap() const { return m_library.Offsets(); }

    void* FindAddressById(unsigned long long aId) const
    {
        unsigned long long offset{};
        if (!m_base || !FindOffsetById(aId, offset) || offset > std::numeric_limits<std::uintptr_t>::max() - m_base)
            return nullptr;
        return reinterpret_cast<void*>(m_base + offset);
    }

    bool FindOffsetById(unsigned long long aId, unsigned long long& aResult) const
    {
        return m_library.FindOffset(aId, aResult);
    }

    bool FindIdByAddress(void* apAddress, unsigned long long& aResult) const
    {
        const auto address = reinterpret_cast<std::uintptr_t>(apAddress);
        aResult = 0;
        return m_base && address >= m_base && FindIdByOffset(address - m_base, aResult);
    }

    bool FindIdByOffset(unsigned long long aOffset, unsigned long long& aResult) const
    {
        aResult = 0;
        const auto it = m_reverse.find(aOffset);
        if (it == m_reverse.end())
            return false;
        aResult = it->second;
        return true;
    }

    void GetLoadedVersion(int& aMajor, int& aMinor, int& aRevision, int& aBuild) const
    {
        const auto& version = m_library.LoadedVersion();
        aMajor = version[0];
        aMinor = version[1];
        aRevision = version[2];
        aBuild = version[3];
    }

    void Clear()
    {
        ++m_generation;
        m_library.Clear();
        m_reverse.clear();
        m_versionString.clear();
        m_error.clear();
        m_base = 0;
    }

    // aImageSize comes from the checked on-disk executable, before mapping.
    bool Load(const std::filesystem::path& aGamePath, std::string_view aExeVersion, std::uint32_t aImageSize)
    {
        Clear();
        STRE::Runtime::Version version{};
        auto remaining = aExeVersion;
        for (std::size_t i = 0; i < version.size(); ++i)
        {
            const auto [end, error] = std::from_chars(remaining.data(), remaining.data() + remaining.size(), version[i]);
            if (error != std::errc{} || (i < 3 && (end == remaining.data() + remaining.size() || *end != '.')) ||
                (i == 3 && end != remaining.data() + remaining.size()))
            {
                m_error = "invalid executable version";
                return false;
            }
            remaining.remove_prefix(static_cast<std::size_t>(end - remaining.data()) + (i < 3 ? 1 : 0));
        }
        std::string fileName = "versionlib";
        for (const auto part : version)
            fileName += "-" + std::to_string(part);
        std::ifstream file(aGamePath / "Data" / "SKSE" / "Plugins" / (fileName + ".bin"), std::ios::binary | std::ios::ate);
        if (!file)
        {
            m_error = "missing/unreadable Address Library: " + fileName + ".bin";
            return false;
        }
        const std::streamoff size = file.tellg();
        if (size <= 0 || size > static_cast<std::streamoff>(STRE::Runtime::AddressLibrary::MaxFileBytes))
        {
            m_error = "invalid Address Library file size";
            return false;
        }
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        file.seekg(0);
        if (!file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
        {
            m_error = "truncated Address Library read";
            return false;
        }
        // Minimum bootstrap dependencies. This is not an ABI/signature attestation.
        constexpr std::array<std::uint32_t, 4> requiredIds{36544, 36564, 53926, 40412};
        if (!m_library.Load(bytes, version, aImageSize, requiredIds))
        {
            m_error = m_library.Error();
            return false;
        }
        for (const auto& [id, offset] : m_library.Offsets())
            m_reverse[offset] = id;
        m_versionString = aExeVersion;
        m_base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
        return true;
    }

    bool DumpToTextFile(const std::string& aPath)
    {
        std::ofstream file(aPath);
        for (const auto& [id, offset] : m_library.Offsets())
            file << std::dec << id << '\t' << std::hex << offset + 0x140000000 << '\n';
        return file.good();
    }

private:
    STRE::Runtime::AddressLibrary m_library;
    STRE::Runtime::OffsetMap m_reverse;
    std::string m_versionString;
    std::string m_error;
    std::uintptr_t m_base{};
    std::uint64_t m_generation{};
};

template <class T> struct VersionDbPtr
{
    VersionDbPtr(const uint32_t aId) noexcept
        : m_pPtr{nullptr}
        , m_id{aId}
    {
    }

    VersionDbPtr() = delete;
    VersionDbPtr(VersionDbPtr&) = delete;
    VersionDbPtr& operator=(VersionDbPtr&) = delete;

    operator T*() const noexcept { return Get(); }

    T* operator->() const noexcept { return Get(); }

    T* Get() const noexcept { return static_cast<T*>(GetPtr()); }

    void* GetPtr() const noexcept
    {
        const auto& database = VersionDb::Get();
        if (m_generation != database.Generation() || m_pPtr == nullptr)
        {
            m_pPtr = database.FindAddressById(m_id);
            m_generation = database.Generation();
        }

        return m_pPtr;
    }

private:
    mutable void* m_pPtr;
    mutable std::uint64_t m_generation{};
    uint32_t m_id;
};
