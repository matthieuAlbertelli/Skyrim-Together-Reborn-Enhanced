#include <CampaignIdentityStore.h>
#include <CampaignClientAdmissionState.h>

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

TEST_CASE(
    "Cold-session campaign binding enumeration is deterministic and removable",
    "[campaign.identity][campaign.resume][reconnect]")
{
    TemporaryIdentityDirectory directory;
    CampaignIdentityStore empty(directory.Path);
    const auto zero = empty.ListBindings();
    REQUIRE(zero.Succeeded());
    REQUIRE(zero.Value.empty());

    REQUIRE(empty.SaveBinding(
        {"campaign-z", "slot-z", "binding-z"}).Succeeded());
    REQUIRE(empty.SaveBinding(
        {"campaign-a", "slot-a", "binding-a"}).Succeeded());

    CampaignIdentityStore restarted(directory.Path);
    const auto visible = restarted.ListBindings();
    REQUIRE(visible.Succeeded());
    REQUIRE(visible.Value.size() == 2);
    REQUIRE(visible.Value[0] == CampaignBindingCacheEntry{
        "campaign-a", "slot-a", "binding-a"});
    REQUIRE(visible.Value[1] == CampaignBindingCacheEntry{
        "campaign-z", "slot-z", "binding-z"});

    REQUIRE(restarted.RemoveBinding("campaign-a").Succeeded());
    const auto afterLeave = restarted.ListBindings();
    REQUIRE(afterLeave.Succeeded());
    REQUIRE(afterLeave.Value == std::vector<CampaignBindingCacheEntry>{
        {"campaign-z", "slot-z", "binding-z"}});
}

TEST_CASE(
    "Ending a loaded runtime keeps the durable campaign binding candidate",
    "[campaign.identity][campaign.resume][main-menu][lifecycle]")
{
    TemporaryIdentityDirectory directory;
    const CampaignBindingCacheEntry binding{
        "campaign-active", "slot-active", "binding-active"};
    CampaignIdentityStore store(directory.Path);
    REQUIRE(store.SaveBinding(binding).Succeeded());

    CampaignClientAdmissionState admission;
    admission.Accept({
        binding.CampaignId,
        binding.CampaignSlotId,
        binding.CharacterBindingId});
    REQUIRE(admission.EndRuntimeSession() == binding.CampaignId);
    REQUIRE_FALSE(admission.GetAdmission());
    REQUIRE_FALSE(admission.BeginResume());

    CampaignIdentityStore restarted(directory.Path);
    const auto candidates = restarted.ListBindings();
    REQUIRE(candidates.Succeeded());
    REQUIRE(candidates.Value ==
        std::vector<CampaignBindingCacheEntry>{binding});
}

TEST_CASE(
    "Corrupted local binding cache cannot yield resume candidates",
    "[campaign.identity][campaign.resume][robustness]")
{
    TemporaryIdentityDirectory directory;
    std::filesystem::create_directories(directory.Path);
    {
        std::ofstream stream(
            directory.Path / "stre-campaign-bindings-v1.txt",
            std::ios::binary);
        stream << "stre-campaign-bindings-v1\n"
               << "campaign-a\tslot-a\tbinding-a\textra\n";
    }

    CampaignIdentityStore restarted(directory.Path);
    const auto candidates = restarted.ListBindings();
    REQUIRE_FALSE(candidates.Succeeded());
    REQUIRE(candidates.Error == LocalIdentityError::Malformed);
    REQUIRE(candidates.Value.empty());
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

TEST_CASE(
    "Versioned campaign save marker survives restart and validates exact identity",
    "[campaign.identity][campaign.load][metadata]")
{
    TemporaryIdentityDirectory directory;
    const CampaignSaveMarker expected{
        "campaign-a", "slot-a", "binding-a", "checkpoint-a",
        "stre-checkpoint-a"};
    CampaignIdentityStore first(directory.Path);
    REQUIRE(first.SaveCampaignSaveMarker(expected).Succeeded());

    CampaignIdentityStore restarted(directory.Path);
    const auto loaded = restarted.LoadCampaignSaveMarker(
        "stre-checkpoint-a");
    REQUIRE(loaded.Succeeded());
    REQUIRE(loaded.Value == expected);
    const auto unrelated = restarted.LoadCampaignSaveMarker(
        "stre-checkpoint-b");
    REQUIRE(unrelated.Succeeded());
    REQUIRE_FALSE(unrelated.Value);
}

TEST_CASE(
    "Marked save lookup resolves zero or one exact local binding",
    "[campaign.identity][campaign.load][campaign.resume][security]")
{
    TemporaryIdentityDirectory directory;
    CampaignIdentityStore store(directory.Path);
    REQUIRE(store.SaveBinding(
        {"campaign-a", "slot-a", "binding-a"}).Succeeded());
    REQUIRE(store.SaveBinding(
        {"campaign-b", "slot-b", "binding-b"}).Succeeded());

    const CampaignSaveMarker marker{
        "campaign-a", "slot-a", "binding-a", "checkpoint-a",
        "stre-checkpoint-a"};
    const auto exact = store.LoadBinding(marker.CampaignId);
    REQUIRE(exact.Succeeded());
    REQUIRE(exact.Value == CampaignBindingCacheEntry{
        marker.CampaignId,
        marker.CampaignSlotId,
        marker.CharacterBindingId});

    const auto missing = store.LoadBinding("campaign-missing");
    REQUIRE(missing.Succeeded());
    REQUIRE_FALSE(missing.Value);

    CampaignSaveMarker mismatched = marker;
    mismatched.CharacterBindingId = "binding-other";
    REQUIRE(exact.Value->CharacterBindingId !=
        mismatched.CharacterBindingId);
}

TEST_CASE(
    "Corrupted or mismatched campaign save markers fail closed",
    "[campaign.identity][campaign.load][metadata][robustness]")
{
    TemporaryIdentityDirectory directory;
    CampaignIdentityStore store(directory.Path);
    REQUIRE(store.SaveCampaignSaveMarker({
        "campaign-a", "slot-a", "binding-a", "checkpoint-a",
        "stre-checkpoint-a"}).Succeeded());

    std::filesystem::path markerPath;
    for (const auto& entry : std::filesystem::directory_iterator(directory.Path))
    {
        if (entry.path().filename().string().starts_with("stre-save-"))
            markerPath = entry.path();
    }
    REQUIRE_FALSE(markerPath.empty());
    {
        std::ofstream stream(markerPath, std::ios::binary | std::ios::trunc);
        stream << "stre-campaign-save-v99\ncampaign-a\n";
    }
    const auto corrupted = store.LoadCampaignSaveMarker(
        "stre-checkpoint-a");
    REQUIRE_FALSE(corrupted.Succeeded());
    REQUIRE(corrupted.Error == LocalIdentityError::Malformed);

    REQUIRE_FALSE(store.SaveCampaignSaveMarker({
        "campaign-a", "slot-a", "binding-a", "checkpoint-a",
        "stre-checkpoint-other"}).Succeeded());
}
