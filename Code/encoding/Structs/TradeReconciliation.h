#pragma once

#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Stl.hpp>

#include <cstdint>

enum class TradeReconcileResultCode : std::uint8_t
{
    Success,
    MalformedPlan,
    SessionMismatch,
    LocalPlayerUnavailable,
    ItemUnavailable,
    ItemNotTransferable,
    AmbiguousItem,
    LocalStateMismatch
};

struct TradeInventoryTargetData
{
    void Serialize(
        TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(
        TiltedPhoques::Buffer::Reader& aReader) noexcept;

    bool operator==(const TradeInventoryTargetData&) const noexcept = default;

    std::uint64_t ItemId{};
    std::uint32_t Quantity{};
    bool Valid{true};
};

struct TradeReconciliationPlanData
{
    static constexpr std::uint64_t MaxTargets = 64;

    void Serialize(
        TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(
        TiltedPhoques::Buffer::Reader& aReader) noexcept;

    bool operator==(const TradeReconciliationPlanData&) const noexcept = default;

    TiltedPhoques::Vector<TradeInventoryTargetData> Targets{};
    bool Valid{true};
};
