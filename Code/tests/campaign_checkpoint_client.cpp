#include <CampaignCheckpointClient.h>

#include <catch2/catch.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>

using namespace STRE::Campaign;

namespace
{
class TemporaryCheckpointDirectory
{
public:
    TemporaryCheckpointDirectory()
    {
        static std::atomic<std::uint64_t> counter{};
        Path = std::filesystem::temp_directory_path() /
            ("stre-checkpoint-client-" +
             std::to_string(
                 std::chrono::high_resolution_clock::now()
                     .time_since_epoch().count()) +
             "-" + std::to_string(counter++));
    }

    ~TemporaryCheckpointDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(Path, error);
    }

    std::filesystem::path ArtifactPath() const
    {
        for (const auto& entry : std::filesystem::directory_iterator(Path))
        {
            if (entry.path().filename().string().starts_with(
                    "stre-checkpoint-"))
            {
                return entry.path();
            }
        }
        return {};
    }

    std::filesystem::path Path;
};

NativeSaveBundleArtifact ClientArtifact(
    const std::string& acIdentity,
    std::uint8_t aMarker)
{
    std::vector<NativeSaveBundleMember> members(2);
    members[0].Role = NativeSaveMemberRole::Ess;
    members[0].Size = 100 + aMarker;
    members[0].Sha256.fill(aMarker);
    members[1].Role = NativeSaveMemberRole::Skse;
    members[1].Size = 200 + aMarker;
    members[1].Sha256.fill(static_cast<std::uint8_t>(aMarker + 1));
    const auto result = BuildNativeSaveBundleArtifact(
        acIdentity, std::move(members));
    REQUIRE(result.Succeeded());
    return result.Value;
}

CampaignCheckpointClientRequest Request(std::string aCheckpoint = "cp-42")
{
    return {
        "campaign-1",
        aCheckpoint,
        7,
        "stre-" + aCheckpoint};
}

std::optional<CampaignClientAdmission> Admission(
    std::string aCampaign = "campaign-1")
{
    return CampaignClientAdmission{
        std::move(aCampaign), "slot-1", "binding-1"};
}
}

TEST_CASE(
    "Checkpoint client accepts only its admitted campaign and handles processing deterministically",
    "[campaign.checkpoint][client]")
{
    TemporaryCheckpointDirectory directory;
    CampaignIdentityStore store(directory.Path);
    CampaignCheckpointClient client(store);

    REQUIRE(client.HandleRequest(Request(), std::nullopt).Kind ==
        CampaignCheckpointClientActionKind::SendFailure);
    REQUIRE(client.HandleRequest(Request(), Admission("campaign-other")).Kind ==
        CampaignCheckpointClientActionKind::SendFailure);

    const auto started = client.HandleRequest(Request(), Admission());
    REQUIRE(started.Kind ==
        CampaignCheckpointClientActionKind::StartNativeSave);
    REQUIRE(client.HandleRequest(Request(), Admission()).Kind ==
        CampaignCheckpointClientActionKind::Wait);
    REQUIRE(client.HandleRequest(Request("cp-43"), Admission()).Kind ==
        CampaignCheckpointClientActionKind::SendFailure);

    const auto completed = client.CompleteNativeSave(
        ClientArtifact("stre-cp-42", 1));
    REQUIRE(completed);
    REQUIRE(completed->Succeeded);
    REQUIRE(completed->Artifact);
    REQUIRE_FALSE(client.GetActiveRequest());
}

TEST_CASE(
    "Checkpoint client cache survives restart and rejects conflicting replay",
    "[campaign.checkpoint][client][replay][persistence]")
{
    TemporaryCheckpointDirectory directory;
    const auto expected = ClientArtifact("stre-cp-42", 1);
    {
        CampaignIdentityStore store(directory.Path);
        CampaignCheckpointClient client(store);
        REQUIRE(client.HandleRequest(Request(), Admission()).Kind ==
            CampaignCheckpointClientActionKind::StartNativeSave);
        REQUIRE(client.CompleteNativeSave(expected)->Succeeded);
        const auto path = directory.ArtifactPath();
        REQUIRE_FALSE(path.empty());
        const auto preservedTime =
            std::filesystem::file_time_type::clock::now() -
            std::chrono::hours(1);
        std::filesystem::last_write_time(path, preservedTime);
        const auto recordedTime = std::filesystem::last_write_time(path);
        REQUIRE(store.SaveCheckpointArtifact(
            "campaign-1", "cp-42", expected));
        REQUIRE(std::filesystem::last_write_time(path) == recordedTime);
    }

    CampaignIdentityStore restartedStore(directory.Path);
    CampaignCheckpointClient restarted(restartedStore);
    const auto replay = restarted.HandleRequest(Request(), Admission());
    REQUIRE(replay.Kind ==
        CampaignCheckpointClientActionKind::ValidateExisting);
    REQUIRE(replay.ExpectedArtifact == expected);
    const auto same = restarted.CompleteNativeSave(expected);
    REQUIRE(same);
    REQUIRE(same->Succeeded);

    const auto next = restarted.HandleRequest(Request(), Admission());
    REQUIRE(next.Kind ==
        CampaignCheckpointClientActionKind::ValidateExisting);
    const auto conflict = restarted.CompleteNativeSave(
        ClientArtifact("stre-cp-42", 2));
    REQUIRE(conflict);
    REQUIRE_FALSE(conflict->Succeeded);

    const auto different = restarted.HandleRequest(
        Request("cp-43"), Admission());
    REQUIRE(different.Kind ==
        CampaignCheckpointClientActionKind::StartNativeSave);
    const auto failed = restarted.FailNativeSave();
    REQUIRE(failed);
    REQUIRE_FALSE(failed->Succeeded);
}

TEST_CASE(
    "Malformed and unsupported local checkpoint artifacts fail closed without replacement",
    "[campaign.checkpoint][client][persistence][robustness]")
{
    SECTION("malformed metadata")
    {
        TemporaryCheckpointDirectory directory;
        CampaignIdentityStore store(directory.Path);
        REQUIRE(store.SaveCheckpointArtifact(
            "campaign-1", "cp-42", ClientArtifact("stre-cp-42", 1)));
        const auto path = directory.ArtifactPath();
        REQUIRE_FALSE(path.empty());
        {
            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            stream << "stre-campaign-checkpoint-artifact-v1\n"
                   << "campaign-1\ncp-42\nstre-cp-42\n"
                   << std::string(64, '0') << "\nzz\n";
        }
        const auto malformed = store.LoadCheckpointArtifact(
            "campaign-1", "cp-42");
        REQUIRE_FALSE(malformed.Succeeded());
        REQUIRE(malformed.Error == LocalIdentityError::Malformed);
        REQUIRE(std::filesystem::exists(path));
    }

    SECTION("unsupported cache codec")
    {
        TemporaryCheckpointDirectory directory;
        CampaignIdentityStore store(directory.Path);
        REQUIRE(store.SaveCheckpointArtifact(
            "campaign-1", "cp-42", ClientArtifact("stre-cp-42", 1)));
        const auto path = directory.ArtifactPath();
        std::ifstream input(path, std::ios::binary);
        std::string contents(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());
        input.close();
        const auto newline = contents.find('\n');
        REQUIRE(newline != std::string::npos);
        contents.replace(
            0, newline,
            "stre-campaign-checkpoint-artifact-v2");
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << contents;
        }
        const auto unsupported = store.LoadCheckpointArtifact(
            "campaign-1", "cp-42");
        REQUIRE_FALSE(unsupported.Succeeded());
        REQUIRE(unsupported.Error == LocalIdentityError::Malformed);
        REQUIRE(std::filesystem::exists(path));
    }
}
