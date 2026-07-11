#include <Structs/TradeApplication.h>

#include <TiltedCore/Serialization.hpp>

#include <algorithm>

void TradeInventoryMutationData::Serialize(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    TiltedPhoques::Serialization::WriteVarInt(aWriter, ItemId);
    TiltedPhoques::Serialization::WriteVarInt(aWriter, Delta);
}

void TradeInventoryMutationData::Deserialize(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ItemId = TiltedPhoques::Serialization::ReadVarInt(aReader);
    Delta = static_cast<std::int32_t>(
        TiltedPhoques::Serialization::ReadVarInt(aReader) & 0xFFFFFFFFu);
}

void TradeMutationPlanData::Serialize(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    TiltedPhoques::Serialization::WriteVarInt(aWriter, Mutations.size());

    for (const TradeInventoryMutationData& mutation : Mutations)
        mutation.Serialize(aWriter);
}

void TradeMutationPlanData::Deserialize(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    Mutations.clear();
    Valid = true;

    const std::uint64_t count =
        TiltedPhoques::Serialization::ReadVarInt(aReader);

    if (count > MaxMutations)
    {
        Valid = false;
        return;
    }

    Mutations.reserve(static_cast<std::size_t>(count));

    for (std::uint64_t i = 0; i < count; ++i)
    {
        TradeInventoryMutationData mutation;
        mutation.Deserialize(aReader);

        if (mutation.ItemId == 0 || mutation.Delta == 0)
            Valid = false;

        const auto duplicate = std::find_if(
            Mutations.begin(),
            Mutations.end(),
            [&mutation](const TradeInventoryMutationData& acExisting) noexcept
            {
                return acExisting.ItemId == mutation.ItemId;
            });

        if (duplicate != Mutations.end())
            Valid = false;

        Mutations.emplace_back(mutation);
    }
}
