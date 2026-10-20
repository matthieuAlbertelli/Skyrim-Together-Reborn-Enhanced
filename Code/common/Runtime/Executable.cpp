#include "Executable.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace STRE::Runtime
{
bool InspectExecutable(std::span<const std::uint8_t> aBytes, Executable& aResult)
{
    aResult = {};
    const auto contains = [&](std::size_t aOffset, std::size_t aSize) {
        return aOffset <= aBytes.size() && aSize <= aBytes.size() - aOffset;
    };
    const auto read = [&](std::size_t aOffset, std::size_t aSize) -> std::uint32_t {
        std::uint32_t value{};
        if (contains(aOffset, aSize))
            for (std::size_t i = 0; i < aSize; ++i)
                value |= std::uint32_t{aBytes[aOffset + i]} << (8 * i);
        return value;
    };
    if (!contains(0, 64) || read(0, 2) != 0x5A4D)
        return false;
    const std::size_t nt = read(0x3C, 4);
    if (!contains(nt, 24) || read(nt, 4) != 0x4550 || read(nt + 4, 2) != 0x8664)
        return false;
    const std::size_t opt = nt + 24, optSize = read(nt + 20, 2), sectionCount = read(nt + 6, 2);
    if (optSize < 240 || !contains(opt, optSize) || read(opt, 2) != 0x20B ||
        sectionCount == 0 || sectionCount > 96 || !contains(opt + optSize, sectionCount * 40))
        return false;
    const auto imageSize = read(opt + 56, 4), entry = read(opt + 16, 4);
    if (imageSize == 0 || imageSize > 0x40000000 || entry >= imageSize || read(opt + 108, 4) < 2)
        return false;
    struct Section { std::uint32_t Rva, Raw, Size, VirtualSize; };
    std::vector<Section> sections;
    bool bindEntry = false;
    for (std::size_t i = 0; i < sectionCount; ++i)
    {
        const auto at = opt + optSize + i * 40;
        Section section{read(at + 12, 4), read(at + 20, 4), read(at + 16, 4), read(at + 8, 4)};
        const auto extent = std::max(section.VirtualSize, section.Size);
        if (!contains(section.Raw, section.Size) || section.Rva >= imageSize || extent > imageSize - section.Rva)
            return false;
        const std::string name(reinterpret_cast<const char*>(aBytes.data() + at), 8);
        if (name == std::string(".bind\0\0\0", 8) && entry >= section.Rva && entry - section.Rva < section.Size)
            bindEntry = true;
        sections.push_back(section);
    }
    const auto offset = [&](std::uint32_t aRva, std::size_t aSize) -> std::optional<std::size_t> {
        for (const auto& section : sections)
            if (aRva >= section.Rva && aRva - section.Rva <= section.Size && aSize <= section.Size - (aRva - section.Rva))
                return std::size_t{section.Raw} + aRva - section.Rva;
        return std::nullopt;
    };
    const auto importRva = read(opt + 120, 4), importSize = read(opt + 124, 4);
    const auto imports = offset(importRva, importSize);
    if (!imports || importSize < 20 || importSize > 1024 * 1024)
        return false;
    bool steam = false, other = false, terminated = false;
    for (std::size_t i = 0; i + 20 <= importSize; i += 20)
    {
        const auto at = *imports + i;
        if (std::all_of(aBytes.begin() + at, aBytes.begin() + at + 20, [](auto aByte) { return aByte == 0; }))
        {
            terminated = true;
            break;
        }
        const auto nameRva = read(at + 12, 4);
        std::string name;
        bool nameTerminated = false;
        for (std::uint32_t j = 0; j < 128; ++j)
        {
            if (nameRva > UINT32_MAX - j)
                return false;
            const auto pos = offset(nameRva + j, 1);
            if (!pos)
                return false;
            auto c = aBytes[*pos];
            if (c == 0)
            {
                nameTerminated = true;
                break;
            }
            if (c >= 'A' && c <= 'Z')
                c += 'a' - 'A';
            name += static_cast<char>(c);
        }
        if (!nameTerminated || name.empty())
            return false;
        steam |= name == "steam_api64.dll";
        other |= name == "galaxy64.dll" || name.starts_with("eossdk");
    }
    if (!terminated)
        return false;
    aResult.ImageSize = imageSize;
    aResult.Store = other ? Distribution::Other : (steam && bindEntry ? Distribution::Steam : Distribution::Unknown);
    return true;
}
} // namespace STRE::Runtime
