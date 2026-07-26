#include <Structs/CharacterBuild.h>

#include <TiltedCore/Serialization.hpp>

#include <algorithm>
#include <map>
#include <utility>

using TiltedPhoques::Serialization;

void CharacterBuildSelectionData::Serialize(
    Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteString(aWriter, GroupId);
    Serialization::WriteString(aWriter, OptionId);
}

void CharacterBuildSelectionData::Deserialize(
    Buffer::Reader& aReader) noexcept
{
    GroupId = Serialization::ReadString(aReader);
    OptionId = Serialization::ReadString(aReader);
}

void CharacterBuildSnapshotData::Serialize(
    Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, BuildVersion);
    RaceId.Serialize(aWriter);
    Serialization::WriteString(aWriter, ClassId);
    Serialization::WriteVarInt(aWriter, Selections.size());
    for (const CharacterBuildSelectionData& selection : Selections)
        selection.Serialize(aWriter);

    CanonicalInventory.Serialize(aWriter);
    Serialization::WriteVarInt(aWriter, InventoryHash);
}

void CharacterBuildSnapshotData::Deserialize(
    Buffer::Reader& aReader) noexcept
{
    BuildVersion = static_cast<std::uint32_t>(
        Serialization::ReadVarInt(aReader));
    RaceId.Deserialize(aReader);
    ClassId = Serialization::ReadString(aReader);

    Selections.clear();
    const std::uint64_t selectionCount =
        Serialization::ReadVarInt(aReader);
    Selections.reserve(
        static_cast<std::size_t>(
            std::min<std::uint64_t>(selectionCount, 64)));

    for (std::uint64_t i = 0; i < selectionCount; ++i)
    {
        CharacterBuildSelectionData selection;
        selection.Deserialize(aReader);
        if (i < 64)
            Selections.push_back(std::move(selection));
    }

    CanonicalInventory = {};
    CanonicalInventory.Deserialize(aReader);
    InventoryHash = Serialization::ReadVarInt(aReader);
}

std::uint64_t ComputeCharacterBuildInventoryHash(
    const Inventory& acInventory) noexcept
{
    using HashKey = std::pair<std::uint32_t, std::uint32_t>;
    std::map<HashKey, std::uint64_t> counts;

    for (const Inventory::Entry& entry : acInventory.Entries)
    {
        if (entry.Count <= 0)
            continue;

        counts[{entry.BaseId.ModId, entry.BaseId.BaseId}] +=
            static_cast<std::uint32_t>(entry.Count);
    }

    constexpr std::uint64_t kOffset = 1469598103934665603ull;
    std::uint64_t hash = kOffset;

    const auto append = [&hash](std::uint64_t aValue)
    {
        constexpr std::uint64_t kFnvPrime = 1099511628211ull;
        for (std::uint32_t i = 0; i < sizeof(aValue); ++i)
        {
            hash ^= static_cast<std::uint8_t>(aValue & 0xFFu);
            hash *= kFnvPrime;
            aValue >>= 8;
        }
    };

    append(counts.size());
    for (const auto& [key, count] : counts)
    {
        append(key.first);
        append(key.second);
        append(count);
    }

    return hash;
}
