#pragma once

#include <Structs/GameId.h>
#include <Structs/Inventory.h>

#include <cstdint>

using TiltedPhoques::Buffer;
using TiltedPhoques::String;
using TiltedPhoques::Vector;

enum class CharacterBuildResult : std::uint8_t
{
    Accepted = 0,
    RejectedVersion,
    RejectedNoCharacter,
    RejectedInvalidRace,
    RejectedInvalidBuild,
    RejectedMissingPlugin,
    RejectedAlreadyPending,
    RejectedAlreadyApplied,
    RejectedRevision,
    RejectedInventoryHash,
    RejectedSpellHash
};

enum class CharacterBuildNetworkState : std::uint8_t
{
    Accepted = 1,
    Applied = 2
};

struct CharacterBuildSelectionData
{
    String GroupId{};
    String OptionId{};

    void Serialize(Buffer::Writer& aWriter) const noexcept;
    void Deserialize(Buffer::Reader& aReader) noexcept;

    bool operator==(const CharacterBuildSelectionData& acRhs) const noexcept
    {
        return GroupId == acRhs.GroupId && OptionId == acRhs.OptionId;
    }
};

struct CharacterBuildSnapshotData
{
    std::uint32_t BuildVersion{};
    GameId RaceId{};
    String ClassId{};
    Vector<CharacterBuildSelectionData> Selections{};
    Inventory CanonicalInventory{};
    std::uint64_t InventoryHash{};
    Vector<GameId> CanonicalSpells{};
    std::uint64_t SpellHash{};

    void Serialize(Buffer::Writer& aWriter) const noexcept;
    void Deserialize(Buffer::Reader& aReader) noexcept;

    bool operator==(const CharacterBuildSnapshotData& acRhs) const noexcept
    {
        return BuildVersion == acRhs.BuildVersion &&
            RaceId == acRhs.RaceId &&
            ClassId == acRhs.ClassId &&
            Selections == acRhs.Selections &&
            CanonicalInventory == acRhs.CanonicalInventory &&
            InventoryHash == acRhs.InventoryHash &&
            CanonicalSpells == acRhs.CanonicalSpells &&
            SpellHash == acRhs.SpellHash;
    }
};

[[nodiscard]] std::uint64_t ComputeCharacterBuildInventoryHash(
    const Inventory& acInventory) noexcept;

[[nodiscard]] std::uint64_t ComputeCharacterBuildSpellHash(
    const Vector<GameId>& acSpells) noexcept;
