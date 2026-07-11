#include <Structs/TradeOffer.h>

#include <TiltedCore/Serialization.hpp>

#include <limits>

void TradeOfferLineData::Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    TiltedPhoques::Serialization::WriteVarInt(aWriter, ItemId);
    TiltedPhoques::Serialization::WriteVarInt(aWriter, Quantity);
}

void TradeOfferLineData::Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ItemId = TiltedPhoques::Serialization::ReadVarInt(aReader);

    const std::uint64_t quantity =
        TiltedPhoques::Serialization::ReadVarInt(aReader);

    if (quantity > std::numeric_limits<std::uint32_t>::max())
    {
        Quantity = 0;
        return;
    }

    Quantity = static_cast<std::uint32_t>(quantity);
}

void TradeOfferData::Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    TiltedPhoques::Serialization::WriteVarInt(aWriter, Items.size());

    for (const TradeOfferLineData& line : Items)
        line.Serialize(aWriter);
}

void TradeOfferData::Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    Items.clear();
    Valid = true;

    const std::uint64_t count =
        TiltedPhoques::Serialization::ReadVarInt(aReader);

    if (count > MaxLines)
    {
        Valid = false;
        return;
    }

    Items.reserve(static_cast<std::size_t>(count));

    for (std::uint64_t i = 0; i < count; ++i)
    {
        TradeOfferLineData line;
        line.Deserialize(aReader);

        if (line.Quantity == 0)
            Valid = false;

        Items.emplace_back(line);
    }
}
