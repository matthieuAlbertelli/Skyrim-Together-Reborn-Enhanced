#pragma once

#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Stl.hpp>

#include <cstdint>

enum class TradeApplyResultCode : std::uint8_t
{
    Success,
    MalformedPlan,
    SessionMismatch,
    LocalPlayerUnavailable,
    ItemUnavailable,
    InsufficientQuantity,
    ItemNotTransferable,
    AmbiguousItem,
    LocalStateMismatch
};

struct TradeInventoryMutationData
{
    void Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept;

    bool operator==(const TradeInventoryMutationData&) const noexcept = default;

    std::uint64_t ItemId{};
    std::int32_t Delta{};
};

struct TradeMutationPlanData
{
    static constexpr std::uint64_t MaxMutations = 64;

    void Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept;

    bool operator==(const TradeMutationPlanData&) const noexcept = default;

    TiltedPhoques::Vector<TradeInventoryMutationData> Mutations{};
    bool Valid{true};
};
