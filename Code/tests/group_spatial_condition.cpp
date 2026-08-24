#include <GroupSpatialCondition.h>
#include <CampaignHelgenStateCache.h>

#include <catch2/catch.hpp>

using namespace STRE::Spatial;

namespace
{
const GameId kHelgenExterior{0, 0x000097ED};
const GameId kHelgenKeep{0, 0x0005DE24};
const GameId kOutsideHelgen{0, 0x0000A000};
const std::vector<GameId> kHelgenFootprint{kHelgenExterior, kHelgenKeep};

MemberPosition At(GameId aCell)
{
    return {true, aCell};
}
} // namespace

TEST_CASE("Group spatial NONE handles one and two member exit orders", "[campaign.spatial]")
{
    auto evaluate = [](std::vector<MemberPosition> aMembers)
    {
        return EvaluateGroupSpatialCondition(aMembers, kHelgenFootprint, GroupOperator::None);
    };

    REQUIRE_FALSE(evaluate({At(kHelgenKeep)}).ConditionMet);
    REQUIRE(evaluate({At(kOutsideHelgen)}).ConditionMet);

    REQUIRE_FALSE(evaluate({At(kHelgenExterior), At(kHelgenKeep)}).ConditionMet);
    REQUIRE_FALSE(evaluate({At(kHelgenExterior), At(kOutsideHelgen)}).ConditionMet);
    REQUIRE_FALSE(evaluate({At(kOutsideHelgen), At(kHelgenKeep)}).ConditionMet);
    REQUIRE(evaluate({At(kOutsideHelgen), At(kOutsideHelgen)}).ConditionMet);

    std::vector<MemberPosition> members{At(kOutsideHelgen), At(kHelgenKeep), At(kOutsideHelgen)};
    REQUIRE_FALSE(evaluate(members).ConditionMet);
    members[1] = At(kOutsideHelgen);
    const Evaluation lastPlayerLeft = evaluate(members);
    REQUIRE(lastPlayerLeft.Status == EvaluationStatus::Known);
    REQUIRE(lastPlayerLeft.ConditionMet);
    REQUIRE(lastPlayerLeft.InsideCount == 0);
    REQUIRE(lastPlayerLeft.RelevantCount == 3);
}

TEST_CASE("Group spatial operators recognize interior and exterior footprint cells", "[campaign.spatial]")
{
    const std::vector<MemberPosition> members{At(kHelgenExterior), At(kHelgenKeep), At(kOutsideHelgen)};

    const Evaluation any = EvaluateGroupSpatialCondition(members, kHelgenFootprint, GroupOperator::Any);
    const Evaluation all = EvaluateGroupSpatialCondition(members, kHelgenFootprint, GroupOperator::All);
    const Evaluation none = EvaluateGroupSpatialCondition(members, kHelgenFootprint, GroupOperator::None);

    REQUIRE(any.ConditionMet);
    REQUIRE_FALSE(all.ConditionMet);
    REQUIRE_FALSE(none.ConditionMet);
    REQUIRE(any.InsideCount == 2);
}

TEST_CASE("Group spatial evaluation fails closed", "[campaign.spatial]")
{
    Evaluation missingMember = EvaluateGroupSpatialCondition({At(kOutsideHelgen), {false, std::nullopt}}, kHelgenFootprint, GroupOperator::None);
    REQUIRE(missingMember.Status == EvaluationStatus::IncompleteRoster);
    REQUIRE_FALSE(missingMember.ConditionMet);

    Evaluation unknownPosition = EvaluateGroupSpatialCondition({At(kOutsideHelgen), {true, std::nullopt}}, kHelgenFootprint, GroupOperator::None);
    REQUIRE(unknownPosition.Status == EvaluationStatus::UnknownPosition);
    REQUIRE_FALSE(unknownPosition.ConditionMet);

    Evaluation inactiveCampaign = EvaluateGroupSpatialCondition({At(kOutsideHelgen), At(kOutsideHelgen)}, kHelgenFootprint, GroupOperator::None, false);
    REQUIRE(inactiveCampaign.Status == EvaluationStatus::GateClosed);
    REQUIRE_FALSE(inactiveCampaign.ConditionMet);

    Evaluation emptyFootprint = EvaluateGroupSpatialCondition({At(kOutsideHelgen)}, {}, GroupOperator::None);
    REQUIRE(emptyFootprint.Status == EvaluationStatus::EmptyFootprint);
    REQUIRE_FALSE(emptyFootprint.ConditionMet);
}

TEST_CASE("Helgen client cache latches start and fails spatial state closed", "[campaign.spatial][campaign.cache]")
{
    CampaignHelgenStateCache cache;
    REQUIRE_FALSE(cache.IsInvestigationStartAuthorized());
    REQUIRE_FALSE(cache.AreAllRequiredPlayersOutside());

    cache.Apply(true, true, true);
    REQUIRE(cache.IsInvestigationStartAuthorized());
    REQUIRE(cache.AreAllRequiredPlayersOutside());

    cache.Apply(false, false, true);
    REQUIRE(cache.IsInvestigationStartAuthorized());
    REQUIRE_FALSE(cache.AreAllRequiredPlayersOutside());

    cache.Apply(false, true, false);
    REQUIRE_FALSE(cache.AreAllRequiredPlayersOutside());

    cache.Reset();
    REQUIRE_FALSE(cache.IsInvestigationStartAuthorized());
    REQUIRE_FALSE(cache.AreAllRequiredPlayersOutside());
}
