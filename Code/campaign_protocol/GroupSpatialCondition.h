#pragma once

#include <TiltedCore/Buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include <Structs/GameId.h>

namespace STRE::Spatial
{
enum class GroupOperator
{
    Any,
    None,
    All
};

enum class EvaluationStatus
{
    Known,
    GateClosed,
    EmptyFootprint,
    IncompleteRoster,
    UnknownPosition
};

struct MemberPosition
{
    bool Present{};
    std::optional<GameId> Cell;
};

struct Evaluation
{
    EvaluationStatus Status{EvaluationStatus::GateClosed};
    bool ConditionMet{};
    std::size_t RelevantCount{};
    std::size_t InsideCount{};
};

[[nodiscard]] Evaluation
EvaluateGroupSpatialCondition(const std::vector<MemberPosition>& acMembers, const std::vector<GameId>& acFootprint, GroupOperator aOperator, bool aGateOpen = true) noexcept;
} // namespace STRE::Spatial
