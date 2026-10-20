#include "AddressLibrary.h"

#include <algorithm>
#include <limits>

namespace STRE::Runtime
{
namespace
{
class Reader
{
public:
    explicit Reader(std::span<const std::uint8_t> aBytes) : m_bytes(aBytes) {}

    bool Read(std::size_t aSize, std::uint64_t& aValue)
    {
        aValue = 0;
        if (aSize > 8 || aSize > Remaining())
            return false;
        for (std::size_t i = 0; i < aSize; ++i)
            aValue |= std::uint64_t{m_bytes[m_position++]} << (i * 8);
        return true;
    }

    bool Name(std::size_t aSize, std::string& aName, bool aPadded)
    {
        if (aSize == 0 || aSize > 64 || aSize > Remaining())
            return false;
        auto name = m_bytes.subspan(m_position, aSize);
        m_position += aSize;
        auto end = std::find(name.begin(), name.end(), 0);
        if (aPadded && (end == name.end() || !std::all_of(end, name.end(), [](auto c) { return c == 0; })))
            return false;
        if (!aPadded && end != name.end())
            return false;
        aName.assign(name.begin(), end);
        return aName == "SkyrimSE.exe";
    }

    std::size_t Remaining() const { return m_bytes.size() - m_position; }

private:
    std::span<const std::uint8_t> m_bytes;
    std::size_t m_position{};
};

bool Decode(Reader& aReader, std::uint8_t aKind, std::uint64_t aPrevious, std::uint64_t& aValue)
{
    if (aKind > 7)
        return false;
    if (aKind == 0)
        return aReader.Read(8, aValue);
    if (aKind == 6 || aKind == 7)
        return aReader.Read(aKind == 6 ? 2 : 4, aValue);
    std::uint64_t delta = 1;
    if (aKind != 1 && !aReader.Read(aKind <= 3 ? 1 : 2, delta))
        return false;
    if (aKind == 3 || aKind == 5)
    {
        if (delta > aPrevious)
            return false;
        aValue = aPrevious - delta;
    }
    else
    {
        if (delta > std::numeric_limits<std::uint64_t>::max() - aPrevious)
            return false;
        aValue = aPrevious + delta;
    }
    return true;
}
} // namespace

void AddressLibrary::Clear()
{
    m_offsets.clear();
    m_version = {};
    m_moduleName.clear();
    m_error.clear();
}

bool AddressLibrary::FindOffset(unsigned long long aId, unsigned long long& aOffset) const
{
    aOffset = 0;
    const auto it = m_offsets.find(aId);
    if (it == m_offsets.end())
        return false;
    aOffset = it->second;
    return true;
}

bool AddressLibrary::Load(std::span<const std::uint8_t> aBytes, const Version& aExpectedVersion,
                          std::uint64_t aImageSize, std::span<const std::uint32_t> aRequiredIds)
{
    Clear();
    const auto fail = [this](const char* aReason) { m_error = aReason; return false; };
    if (aBytes.size() > MaxFileBytes || aImageSize == 0 || aImageSize > 0x40000000)
        return fail("invalid file or executable image size");
    Reader reader(aBytes);
    std::uint64_t format{};
    if (!reader.Read(4, format))
        return fail("truncated format");
    if (format != 2 && format != 5)
        return fail("unsupported Address Library format");
    Version version{};
    for (auto& part : version)
    {
        std::uint64_t value{};
        if (!reader.Read(4, value))
            return fail("truncated version");
        part = static_cast<std::uint32_t>(value);
    }
    if (version != aExpectedVersion)
        return fail("Address Library version differs from executable");
    std::uint64_t nameSize = 64;
    if (format == 2 && !reader.Read(4, nameSize))
        return fail("truncated module name size");
    std::string moduleName;
    if (!reader.Name(static_cast<std::size_t>(nameSize), moduleName, format == 5))
        return fail("invalid module name");
    std::uint64_t pointerSize{}, count{};
    if (!reader.Read(format == 5 ? 8 : 4, pointerSize) || pointerSize != 8)
        return fail("invalid pointer size (expected 8)");
    if (!reader.Read(4, count) || count == 0 || count > MaxAddresses ||
        count > reader.Remaining() / (format == 5 ? 4 : 1))
        return fail("invalid or truncated address count");

    OffsetMap candidate;
    std::uint64_t previousId{}, previousOffset{};
    for (std::uint64_t index = 0; index < count; ++index)
    {
        std::uint64_t id = index, offset{};
        if (format == 5)
        {
            if (!reader.Read(4, offset))
                return fail("truncated offset");
            if (offset == 0) // Sparse v5 ID, not an address at the module base.
                continue;
        }
        else
        {
            std::uint64_t type{};
            if (!reader.Read(1, type) || !Decode(reader, type & 0xF, previousId, id))
                return fail("invalid or truncated ID encoding");
            const auto high = static_cast<std::uint8_t>(type >> 4);
            const bool scaled = (high & 8) != 0;
            if (!Decode(reader, high & 7, scaled ? previousOffset / pointerSize : previousOffset, offset))
                return fail("invalid or truncated offset encoding");
            if (scaled)
            {
                if (offset > std::numeric_limits<std::uint64_t>::max() / pointerSize)
                    return fail("scaled offset overflow");
                offset *= pointerSize;
            }
            previousId = id;
            previousOffset = offset;
        }
        if (offset == 0 || offset >= aImageSize || !candidate.emplace(id, offset).second)
            return fail("zero/out-of-image offset or duplicate ID");
    }
    if (reader.Remaining() != 0 || candidate.empty())
        return fail("trailing bytes or empty address database");
    for (const auto id : aRequiredIds)
    {
        if (!candidate.contains(id))
        {
            m_error = "required relocation missing: " + std::to_string(id);
            return false;
        }
    }
    m_offsets.swap(candidate);
    m_version = version;
    m_moduleName = std::move(moduleName);
    return true;
}
} // namespace STRE::Runtime
