#include <CampaignIdentityStore.h>

#include <catch2/catch.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace STRE::Campaign;

namespace
{
class TemporaryIdentityDirectory
{
public:
    TemporaryIdentityDirectory()
    {
        static std::atomic<std::uint64_t> counter{};
        Path = std::filesystem::temp_directory_path() /
            ("stre-campaign-identity-" + std::to_string(
                 std::chrono::high_resolution_clock::now()
                     .time_since_epoch().count()) + "-" +
             std::to_string(counter++));
    }

    ~TemporaryIdentityDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(Path, error);
    }

    std::filesystem::path Path;
};
}

TEST_CASE("Durable STRE PlayerId is generated once and reused", "[campaign.identity]")
{
    TemporaryIdentityDirectory directory;
    CampaignIdentityStore first(directory.Path);
    const auto generated = first.LoadOrCreatePlayerId();
    REQUIRE(generated.Succeeded());
    REQUIRE(CampaignIdentityStore::IsValidPlayerId(generated.Value));
    REQUIRE(generated.Value != "42");

    CampaignIdentityStore restarted(directory.Path);
    const auto loaded = restarted.LoadOrCreatePlayerId();
    REQUIRE(loaded.Succeeded());
    REQUIRE(loaded.Value == generated.Value);
}

TEST_CASE("Malformed durable STRE PlayerId fails closed without replacement", "[campaign.identity][robustness]")
{
    TemporaryIdentityDirectory directory;
    std::filesystem::create_directories(directory.Path);
    const auto path = directory.Path / "stre-player-id-v1.txt";
    {
        std::ofstream stream(path, std::ios::binary);
        stream << "stre-player-id-v1\nnot-a-valid-player-id\n";
    }

    CampaignIdentityStore store(directory.Path);
    const auto loaded = store.LoadOrCreatePlayerId();
    REQUIRE_FALSE(loaded.Succeeded());
    REQUIRE(loaded.Error == LocalIdentityError::Malformed);

    std::ifstream stream(path, std::ios::binary);
    std::string contents(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());
    REQUIRE(contents.find("not-a-valid-player-id") != std::string::npos);
}

TEST_CASE("Oversized durable STRE identity file fails closed", "[campaign.identity][robustness]")
{
    TemporaryIdentityDirectory directory;
    std::filesystem::create_directories(directory.Path);
    const auto path = directory.Path / "stre-player-id-v1.txt";
    {
        std::ofstream stream(path, std::ios::binary);
        stream << std::string(300, 'x');
    }

    CampaignIdentityStore store(directory.Path);
    const auto loaded = store.LoadOrCreatePlayerId();
    REQUIRE_FALSE(loaded.Succeeded());
    REQUIRE(loaded.Error == LocalIdentityError::Malformed);
    REQUIRE(std::filesystem::file_size(path) == 300);
}

TEST_CASE("Campaign binding cache persists canonical assignments only", "[campaign.identity]")
{
    TemporaryIdentityDirectory directory;
    CampaignIdentityStore store(directory.Path);
    const CampaignBindingCacheEntry expected{
        "campaign-1", "slot-02", "binding-2"};
    REQUIRE(store.SaveBinding(expected).Succeeded());

    CampaignIdentityStore restarted(directory.Path);
    const auto loaded = restarted.LoadBinding("campaign-1");
    REQUIRE(loaded.Succeeded());
    REQUIRE(loaded.Value.has_value());
    REQUIRE(*loaded.Value == expected);

    REQUIRE(restarted.RemoveBinding("campaign-1").Succeeded());
    const auto removed = restarted.LoadBinding("campaign-1");
    REQUIRE(removed.Succeeded());
    REQUIRE_FALSE(removed.Value.has_value());
}

TEST_CASE("Malformed campaign binding cache is not overwritten", "[campaign.identity][robustness]")
{
    TemporaryIdentityDirectory directory;
    std::filesystem::create_directories(directory.Path);
    const auto path = directory.Path / "stre-campaign-bindings-v1.txt";
    {
        std::ofstream stream(path, std::ios::binary);
        stream << "wrong-header\ncampaign-1\tslot-1\tbinding-1\n";
    }
    CampaignIdentityStore store(directory.Path);
    const auto result = store.SaveBinding(
        {"campaign-2", "slot-02", "binding-2"});
    REQUIRE_FALSE(result.Succeeded());
    REQUIRE(result.Error == LocalIdentityError::Malformed);
}
