#include <Structs/TradeReconciliation.h>

#include <TiltedCore/Serialization.hpp>

#include <algorithm>
#include <limits>

void TradeInventoryTargetData::Serialize(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    TiltedPhoques::Serialization::WriteVarInt(
        aWriter,
        ItemId);
    TiltedPhoques::Serialization::WriteVarInt(
        aWriter,
        Quantity);
}

void TradeInventoryTargetData::Deserialize(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ItemId =
        TiltedPhoques::Serialization::ReadVarInt(aReader);

    const std::uint64_t quantity =
        TiltedPhoques::Serialization::ReadVarInt(aReader);

    Valid =
        quantity <= std::numeric_limits<std::uint32_t>::max();

    Quantity = Valid
        ? static_cast<std::uint32_t>(quantity)
        : 0;
}

void TradeReconciliationPlanData::Serialize(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    TiltedPhoques::Serialization::WriteVarInt(
        aWriter,
        Targets.size());

    for (const TradeInventoryTargetData& target :
         Targets)
    {
        target.Serialize(aWriter);
    }
}

void TradeReconciliationPlanData::Deserialize(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    Targets.clear();
    Valid = true;

    const std::uint64_t count =
        TiltedPhoques::Serialization::ReadVarInt(aReader);

    if (count > MaxTargets)
    {
        Valid = false;
        return;
    }

    Targets.reserve(static_cast<std::size_t>(count));

    for (std::uint64_t i = 0; i < count; ++i)
    {
        TradeInventoryTargetData target;
        target.Deserialize(aReader);

        if (!target.Valid || target.ItemId == 0)
            Valid = false;

        const auto duplicate = std::find_if(
            Targets.begin(),
            Targets.end(),
            [&target](
                const TradeInventoryTargetData& acExisting) noexcept
            {
                return acExisting.ItemId ==
                       target.ItemId;
            });

        if (duplicate != Targets.end())
            Valid = false;

        Targets.emplace_back(target);
    }
}
