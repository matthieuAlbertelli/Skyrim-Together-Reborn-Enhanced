#include <GroupSpatialCondition.h>

#include <algorithm>

namespace STRE::Spatial
{
Evaluation EvaluateGroupSpatialCondition(const std::vector<MemberPosition>& acMembers, const std::vector<GameId>& acFootprint, GroupOperator aOperator, bool aGateOpen) noexcept
{
    Evaluation result;
    result.RelevantCount = acMembers.size();

    if (!aGateOpen)
        return result;

    if (acFootprint.empty())
    {
        result.Status = EvaluationStatus::EmptyFootprint;
        return result;
    }

    if (acMembers.empty() || std::any_of(acMembers.begin(), acMembers.end(), [](const MemberPosition& acMember) { return !acMember.Present; }))
    {
        result.Status = EvaluationStatus::IncompleteRoster;
        return result;
    }

    if (std::any_of(acMembers.begin(), acMembers.end(), [](const MemberPosition& acMember) { return !acMember.Cell || !*acMember.Cell; }))
    {
        result.Status = EvaluationStatus::UnknownPosition;
        return result;
    }

    for (const MemberPosition& member : acMembers)
    {
        if (std::find(acFootprint.begin(), acFootprint.end(), *member.Cell) != acFootprint.end())
        {
            ++result.InsideCount;
        }
    }

    result.Status = EvaluationStatus::Known;
    switch (aOperator)
    {
    case GroupOperator::Any: result.ConditionMet = result.InsideCount > 0; break;
    case GroupOperator::None: result.ConditionMet = result.InsideCount == 0; break;
    case GroupOperator::All: result.ConditionMet = result.InsideCount == result.RelevantCount; break;
    }
    return result;
}
} // namespace STRE::Spatial
