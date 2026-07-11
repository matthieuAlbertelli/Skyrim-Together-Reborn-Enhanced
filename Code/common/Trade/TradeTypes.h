#pragma once

#include <cstdint>
#include <vector>

namespace Trade
{
using SessionId = std::uint64_t;
using Revision = std::uint64_t;
using PlayerId = std::uint32_t;
using ItemId = std::uint64_t;
using Tick = std::uint64_t;

enum class State : std::uint8_t
{
    PendingAcceptance,
    Negotiating,
    Locked,
    Applying,
    Completed,
    Cancelled,
    Failed
};

struct OfferLine
{
    ItemId Item{};
    std::uint32_t Quantity{};

    bool operator==(const OfferLine&) const noexcept = default;
};

struct Offer
{
    std::vector<OfferLine> Items;

    bool operator==(const Offer&) const noexcept = default;
};

struct Participant
{
    PlayerId Id{};
    Offer CurrentOffer{};
    bool Confirmed{};
    Revision ConfirmedRevision{};
};
}
