#include <CampaignLobbyDirectory.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace STRE::Campaign;

TEST_CASE("Campaign lobby codes use the exact bounded alphabet", "[campaign.lobby][code]")
{
    CampaignLobbyDirectory directory;
    for (std::size_t index = 0; index < 24; ++index)
    {
        const auto code = directory.Allocate(
            "campaign-" + std::to_string(index),
            static_cast<std::uint32_t>(index));
        REQUIRE(code);
        REQUIRE(code->size() == kCampaignJoinCodeLength);
        REQUIRE(std::all_of(code->begin(), code->end(), [](char aCharacter)
        {
            return kCampaignJoinCodeAlphabet.find(aCharacter) !=
                std::string_view::npos;
        }));
    }
}

TEST_CASE("Campaign lobby code normalization rejects ambiguous glyphs", "[campaign.lobby][code]")
{
    TiltedPhoques::String normalized;
    REQUIRE(NormalizeCampaignJoinCode("a7k2", normalized));
    REQUIRE(normalized == "A7K2");

    for (const std::string invalid : {
             "A7K", "A7K22", "A7I2", "A7O2", "A702", "A712", "A7-2"})
    {
        REQUIRE_FALSE(NormalizeCampaignJoinCode(invalid, normalized));
        REQUIRE(normalized.empty());
    }
}

TEST_CASE("Campaign lobby allocation retries collisions without cross-routing", "[campaign.lobby][code]")
{
    std::vector<std::string> generated{"A7K2", "A7K2", "R5WT"};
    std::size_t next{};
    CampaignLobbyDirectory directory(
        [&generated, &next]() { return generated.at(next++); }, 3);

    REQUIRE(directory.Allocate("campaign-a", 11) == "A7K2");
    REQUIRE(directory.Allocate("campaign-b", 22) == "R5WT");
    REQUIRE(directory.Resolve("a7k2"));
    REQUIRE(directory.Resolve("a7k2")->CampaignId == "campaign-a");
    REQUIRE(directory.Resolve("R5WT"));
    REQUIRE(directory.Resolve("R5WT")->CampaignId == "campaign-b");
    REQUIRE(directory.Resolve("A7K2")->PartyId == 11);
    REQUIRE(directory.Resolve("R5WT")->PartyId == 22);
}

TEST_CASE("Campaign lobby allocation fails after its bounded retry budget", "[campaign.lobby][code]")
{
    CampaignLobbyDirectory directory([] { return "9FQ3"; }, 2);
    REQUIRE(directory.Allocate("campaign-a", 1) == "9FQ3");
    REQUIRE_FALSE(directory.Allocate("campaign-b", 2));
    REQUIRE(directory.Resolve("9FQ3")->CampaignId == "campaign-a");
}

TEST_CASE("Campaign lobby aliases and names are ephemeral presentation data", "[campaign.lobby][presentation]")
{
    CampaignLobbyDirectory directory([] { return "R5WT"; });
    REQUIRE(directory.Allocate("campaign-a", 42));

    directory.RememberDisplayName("campaign-a", "player-a", "Matthieu");
    const auto* lobby = directory.FindByCampaign("campaign-a");
    REQUIRE(lobby);
    REQUIRE(lobby->DisplayNames.at("player-a") == "Matthieu");

    directory.RememberDisplayName(
        "campaign-a", "player-unicode",
        "  L\xC3\xA9" "a \xF0\x9F\x90\x89  ");
    REQUIRE(lobby->DisplayNames.at("player-unicode") ==
        "L\xC3\xA9" "a \xF0\x9F\x90\x89");

    directory.RememberDisplayName(
        "campaign-a", "player-b",
        std::string(kCampaignLobbyMaximumDisplayNameLength + 1, 'x'));
    REQUIRE_FALSE(lobby->DisplayNames.contains("player-b"));

    directory.RememberDisplayName(
        "campaign-a", "player-control", "Player\nTwo");
    REQUIRE_FALSE(lobby->DisplayNames.contains("player-control"));

    directory.RememberDisplayName(
        "campaign-a", "player-invalid-utf8",
        std::string{"\xC3\x28", 2});
    REQUIRE_FALSE(lobby->DisplayNames.contains("player-invalid-utf8"));

    directory.ForgetDisplayName("campaign-a", "player-a");
    REQUIRE_FALSE(lobby->DisplayNames.contains("player-a"));
    directory.Invalidate("campaign-a");
    REQUIRE_FALSE(directory.Resolve("R5WT"));
    REQUIRE_FALSE(directory.FindByCampaign("campaign-a"));
}
