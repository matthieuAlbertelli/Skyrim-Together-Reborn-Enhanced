#pragma once
#include <Structs/Tints.h>
#include <bit>

namespace STRE::CharacterCreation
{
// Diagnostic FNV-1a over ordered wire fields; no StringCache side effects,
// pointers, padding or platform endianness. Not an authentication checksum.
inline uint64_t AppearanceTintHash(const Tints& tints) noexcept
{
    uint64_t hash = 14695981039346656037ull;
    const auto byte = [&](uint8_t value)
    {
        hash = (hash ^ value) * 1099511628211ull;
    };
    const auto word = [&](uint32_t value)
    {
        for (unsigned shift = 0; shift < 32; shift += 8)
            byte(static_cast<uint8_t>(value >> shift));
    };
    word(static_cast<uint32_t>(tints.Entries.size()));
    for (const auto& entry : tints.Entries)
    {
        word(entry.Type);
        word(entry.Color);
        word(std::bit_cast<uint32_t>(entry.Alpha));
        word(static_cast<uint32_t>(entry.Name.size()));
        for (unsigned char value : entry.Name)
            byte(value);
    }
    return hash;
}
} // namespace STRE::CharacterCreation
