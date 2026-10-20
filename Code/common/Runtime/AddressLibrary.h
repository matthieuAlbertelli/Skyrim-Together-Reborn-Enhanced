#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <span>
#include <string>

namespace STRE::Runtime
{
using Version = std::array<std::uint32_t, 4>;
using OffsetMap = std::map<unsigned long long, unsigned long long>;

// Format provenance: TiltedEvolution PR #885 / 6f963497. Parsing is independent
// of Windows and publishes no entries until the entire input has been checked.
class AddressLibrary
{
public:
    static constexpr std::size_t MaxFileBytes = 32 * 1024 * 1024;
    static constexpr std::uint32_t MaxAddresses = 1024 * 1024;

    bool Load(std::span<const std::uint8_t> aBytes, const Version& aExpectedVersion,
              std::uint64_t aImageSize, std::span<const std::uint32_t> aRequiredIds = {});
    void Clear();
    bool FindOffset(unsigned long long aId, unsigned long long& aOffset) const;

    const OffsetMap& Offsets() const { return m_offsets; }
    const std::string& Error() const { return m_error; }
    const std::string& ModuleName() const { return m_moduleName; }
    const Version& LoadedVersion() const { return m_version; }

private:
    OffsetMap m_offsets;
    Version m_version{};
    std::string m_moduleName;
    std::string m_error;
};
} // namespace STRE::Runtime
