#pragma once

#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Stl.hpp>

#include <cstdint>

struct TradeOfferLineData
{
    void Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept;

    bool operator==(const TradeOfferLineData&) const noexcept = default;

    std::uint64_t ItemId{};
    std::uint32_t Quantity{};
};

struct TradeOfferData
{
    static constexpr std::uint64_t MaxLines = 64;

    void Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept;

    bool operator==(const TradeOfferData&) const noexcept = default;

    TiltedPhoques::Vector<TradeOfferLineData> Items{};
    bool Valid{true};
};
