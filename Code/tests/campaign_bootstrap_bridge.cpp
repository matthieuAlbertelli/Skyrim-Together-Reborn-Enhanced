#include <CampaignBootstrapBridge.h>
#include <CampaignBootstrapState.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <string_view>

using namespace STRE::Campaign;

TEST_CASE("Campaign bootstrap native CEF binding is registered by its shared manifest", "[campaign.bootstrap][cef]")
{
    REQUIRE(kCampaignBootstrapActionFunction ==
        std::string_view{"campaignBootstrapAction"});
    REQUIRE(std::find(
                kCampaignBootstrapCefFunctions.begin(),
                kCampaignBootstrapCefFunctions.end(),
                kCampaignBootstrapActionFunction) !=
        kCampaignBootstrapCefFunctions.end());
    REQUIRE(kCampaignBootstrapMaximumArgumentCount == 5);
    REQUIRE(kCampaignBootstrapDisplayNameArgumentIndex == 4);
}

TEST_CASE("Campaign bootstrap native action contract reaches entry state transitions", "[campaign.bootstrap][cef]")
{
    struct EntryCase
    {
        std::string_view Action;
        CampaignBootstrapAction Parsed;
        CampaignBootstrapPhase ExpectedPhase;
    };
    constexpr std::array cases{
        EntryCase{"showCreate", CampaignBootstrapAction::ShowCreate,
            CampaignBootstrapPhase::CreateForm},
        EntryCase{"showJoin", CampaignBootstrapAction::ShowJoin,
            CampaignBootstrapPhase::JoinForm}};

    for (const EntryCase& entry : cases)
    {
        CampaignBootstrapState state;
        state.BeginFreshGame();
        const CampaignBootstrapAction parsed =
            ParseCampaignBootstrapAction(entry.Action);
        REQUIRE(parsed == entry.Parsed);
        if (parsed == CampaignBootstrapAction::ShowCreate)
            state.ShowCreateForm();
        else if (parsed == CampaignBootstrapAction::ShowJoin)
            state.ShowJoinForm();
        REQUIRE(state.GetPhase() == entry.ExpectedPhase);
    }

    CampaignBootstrapState solo;
    solo.BeginFreshGame();
    REQUIRE(ParseCampaignBootstrapAction("solo") ==
        CampaignBootstrapAction::Solo);
    REQUIRE(solo.ChooseSolo());
    REQUIRE(solo.GetPhase() == CampaignBootstrapPhase::Authorized);
    REQUIRE_FALSE(solo.ChooseSolo());
    REQUIRE(ParseCampaignBootstrapAction("SOLO") ==
        CampaignBootstrapAction::Unknown);
}
