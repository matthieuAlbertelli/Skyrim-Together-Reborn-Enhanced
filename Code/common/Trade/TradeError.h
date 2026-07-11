#pragma once

#include <cstdint>

namespace Trade
{
enum class Error : std::uint8_t
{
    None,
    InvalidParticipant,
    NotParticipant,
    NotRecipient,
    InvalidState,
    AlreadyTerminal,
    StaleRevision,
    InvalidItem,
    EmptyQuantity,
    DuplicateItem,
    RevisionOverflow,
    SessionExpired,
    InventoryUnavailable,
    ItemUnavailable,
    InsufficientQuantity,
    ItemNotTransferable,
    AmbiguousItem,
    QuantityOverflow
};

struct Result
{
    Error ErrorCode{Error::None};
    bool Changed{};

    [[nodiscard]] constexpr bool Succeeded() const noexcept
    {
        return ErrorCode == Error::None;
    }

    [[nodiscard]] static constexpr Result Success(bool aChanged = true) noexcept
    {
        return Result{Error::None, aChanged};
    }

    [[nodiscard]] static constexpr Result Failure(Error aError, bool aChanged = false) noexcept
    {
        return Result{aError, aChanged};
    }
};
}
