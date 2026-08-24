#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Serialization.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <Messages/AuthenticationRequest.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>
#include <Messages/ClientMessageFactory.h>
#include <Messages/ServerMessageFactory.h>
#include <Structs/NativeSaveBundle.h>

#include <catch2/catch.hpp>

using namespace TiltedPhoques;

static_assert(kCharacterBuildRequest == 59);
static_assert(kCharacterBuildAppliedRequest == 60);
static_assert(kRequestWorldEntityManipulation == 61);
static_assert(kCampaignCreateRequest == 62);
static_assert(kCampaignJoinRequest == 63);
static_assert(kCampaignResumeRequest == 64);
static_assert(kCampaignStartRequest == 65);
static_assert(kCampaignSetReadyRequest == 66);
static_assert(kCampaignLeaveRequest == 67);
static_assert(kCampaignJoinByCodeRequest == 68);
static_assert(kCampaignHelgenInvestigationReadyRequest == 69);
static_assert(kCampaignCheckpointSaveResult == 70);
static_assert(kNotifyCharacterBuildState == 63);
static_assert(kNotifyWorldEntityManipulation == 64);
static_assert(kCampaignCommandResponse == 65);
static_assert(kNotifyCampaignSnapshot == 66);
static_assert(kNotifyCampaignLobbyState == 67);
static_assert(kNotifyCampaignHelgenState == 68);
static_assert(kCampaignCheckpointSaveRequest == 69);

namespace
{
template <class T> UniquePtr<T> RoundTripClient(const T& acMessage)
{
    Buffer buffer(4096);
    Buffer::Writer writer(&buffer);
    acMessage.Serialize(writer);
    Buffer::Reader reader(&buffer);
    auto decoded = ClientMessageFactory{}.Extract(reader);
    REQUIRE(decoded);
    REQUIRE(decoded->GetOpcode() == T::Opcode);
    return CastUnique<T>(std::move(decoded));
}

template <class T> UniquePtr<T> RoundTripServer(const T& acMessage)
{
    Buffer buffer(4096);
    Buffer::Writer writer(&buffer);
    acMessage.Serialize(writer);
    Buffer::Reader reader(&buffer);
    auto decoded = ServerMessageFactory{}.Extract(reader);
    REQUIRE(decoded);
    REQUIRE(decoded->GetOpcode() == T::Opcode);
    return CastUnique<T>(std::move(decoded));
}
} // namespace

TEST_CASE("Authentication carries durable STRE PlayerId metadata", "[campaign.protocol]")
{
    AuthenticationRequest request;
    request.StrePlayerId = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    const auto decoded = RoundTripClient(request);
    REQUIRE(decoded->StrePlayerId == request.StrePlayerId);
}

TEST_CASE("Every campaign client command round-trips through the factory", "[campaign.protocol]")
{
    CampaignCreateRequest create;
    create.MutationId = "mutation-create";
    create.DisplayName = "Matthieu";
    const auto created = RoundTripClient(create);
    REQUIRE(created->MutationId == create.MutationId);
    REQUIRE(created->DisplayName == create.DisplayName);
    REQUIRE(created->IsValid());

    CampaignJoinRequest join;
    join.CampaignId = "campaign-1";
    join.MutationId = "mutation-join";
    join.ExpectedRevision = 7;
    const auto joined = RoundTripClient(join);
    REQUIRE(joined->CampaignId == join.CampaignId);
    REQUIRE(joined->MutationId == join.MutationId);
    REQUIRE(joined->ExpectedRevision == 7);

    CampaignResumeRequest resume;
    resume.CampaignId = "campaign-1";
    resume.CharacterBindingId = "binding-2";
    const auto resumed = RoundTripClient(resume);
    REQUIRE(resumed->CampaignId == resume.CampaignId);
    REQUIRE(resumed->CharacterBindingId == resume.CharacterBindingId);

    CampaignStartRequest start;
    start.CampaignId = "campaign-1";
    start.MutationId = "mutation-start";
    start.ExpectedRevision = 8;
    const auto started = RoundTripClient(start);
    REQUIRE(started->CampaignId == start.CampaignId);
    REQUIRE(started->MutationId == start.MutationId);
    REQUIRE(started->ExpectedRevision == 8);

    CampaignSetReadyRequest ready;
    ready.CampaignId = "campaign-1";
    ready.MutationId = "mutation-ready";
    ready.ExpectedRevision = 9;
    ready.Ready = true;
    const auto readied = RoundTripClient(ready);
    REQUIRE(readied->CampaignId == ready.CampaignId);
    REQUIRE(readied->MutationId == ready.MutationId);
    REQUIRE(readied->ExpectedRevision == 9);
    REQUIRE(readied->Ready);

    CampaignLeaveRequest leave;
    leave.CampaignId = "campaign-1";
    leave.MutationId = "mutation-leave";
    leave.ExpectedRevision = 10;
    const auto left = RoundTripClient(leave);
    REQUIRE(left->CampaignId == leave.CampaignId);
    REQUIRE(left->MutationId == leave.MutationId);
    REQUIRE(left->ExpectedRevision == 10);

    CampaignJoinByCodeRequest joinByCode;
    joinByCode.JoinCode = "A7K2";
    joinByCode.MutationId = "mutation-code";
    joinByCode.DisplayName = "\xC3\x89" "owyn";
    const auto joinedByCode = RoundTripClient(joinByCode);
    REQUIRE(joinedByCode->JoinCode == "A7K2");
    REQUIRE(joinedByCode->MutationId == "mutation-code");
    REQUIRE(joinedByCode->DisplayName == joinByCode.DisplayName);
    REQUIRE(joinedByCode->IsValid());

    CampaignHelgenInvestigationReadyRequest helgenReady;
    REQUIRE(RoundTripClient(helgenReady));
}

TEST_CASE("Campaign response and public snapshot round-trip through the server factory", "[campaign.protocol]")
{
    CampaignCommandResponse response;
    response.Operation = CampaignProtocolOperation::Join;
    response.Result = CampaignProtocolResult::Applied;
    response.MutationId = "mutation-join";
    response.CampaignId = "campaign-1";
    response.StateVersion = 4;
    response.CampaignSlotId = "slot-02";
    response.CharacterBindingId = "binding-2";
    const auto decodedResponse = RoundTripServer(response);
    REQUIRE(decodedResponse->Operation == response.Operation);
    REQUIRE(decodedResponse->Result == response.Result);
    REQUIRE(decodedResponse->MutationId == response.MutationId);
    REQUIRE(decodedResponse->CampaignId == response.CampaignId);
    REQUIRE(decodedResponse->StateVersion == 4);
    REQUIRE(decodedResponse->CampaignSlotId == response.CampaignSlotId);
    REQUIRE(decodedResponse->CharacterBindingId == response.CharacterBindingId);
    REQUIRE(decodedResponse->IsValid());

    CampaignCommandResponse existingMember;
    existingMember.Operation = CampaignProtocolOperation::Join;
    existingMember.Result = CampaignProtocolResult::ExistingMembershipRequiresResume;
    existingMember.MutationId = "mutation-join-new";
    existingMember.CampaignId = "campaign-1";
    existingMember.StateVersion = 4;
    const auto decodedExistingMember = RoundTripServer(existingMember);
    REQUIRE(decodedExistingMember->Result == existingMember.Result);
    REQUIRE(decodedExistingMember->IsValid());

    NotifyCampaignSnapshot notification;
    notification.Snapshot.CampaignId = "campaign-1";
    notification.Snapshot.StateVersion = 5;
    notification.Snapshot.Phase = 1;
    notification.Snapshot.RuntimeState = 1;
    notification.Snapshot.RosterSealed = true;
    notification.Snapshot.SessionManagerPlayerId = "player-1";
    notification.Snapshot.Roster.push_back({"slot-01", "player-1", true, true});
    notification.Snapshot.Roster.push_back({"slot-02", "player-2", false, true});
    const auto decodedSnapshot = RoundTripServer(notification);
    REQUIRE(decodedSnapshot->Snapshot == notification.Snapshot);
    REQUIRE(decodedSnapshot->IsValid());

    NotifyCampaignLobbyState lobby;
    lobby.JoinCode = "R5WT";
    lobby.CampaignId = "campaign-1";
    lobby.StateVersion = 4;
    lobby.Members.push_back({"Matthieu", true});
    lobby.Members.push_back({"\xC3\x89" "owyn", false});
    lobby.CanStart = false;
    const auto decodedLobby = RoundTripServer(lobby);
    REQUIRE(decodedLobby->JoinCode == "R5WT");
    REQUIRE(decodedLobby->CampaignId == "campaign-1");
    REQUIRE(decodedLobby->StateVersion == 4);
    REQUIRE(decodedLobby->Members.size() == 2);
    REQUIRE(decodedLobby->Members[0].Name == "Matthieu");
    REQUIRE(decodedLobby->Members[0].Present);
    REQUIRE(decodedLobby->Members[1].Name == "\xC3\x89" "owyn");
    REQUIRE_FALSE(decodedLobby->Members[1].Present);
    REQUIRE_FALSE(decodedLobby->CanStart);
    REQUIRE(decodedLobby->IsValid());

    NotifyCampaignHelgenState helgen;
    helgen.InvestigationStartAuthorized = true;
    helgen.SpatialStatus = CampaignHelgenSpatialStatus::Known;
    helgen.AllRequiredPlayersOutside = true;
    const auto decodedHelgen = RoundTripServer(helgen);
    REQUIRE(decodedHelgen->InvestigationStartAuthorized);
    REQUIRE(decodedHelgen->SpatialStatus == CampaignHelgenSpatialStatus::Known);
    REQUIRE(decodedHelgen->AllRequiredPlayersOutside);
    REQUIRE(decodedHelgen->IsValid());
}

TEST_CASE(
    "Campaign checkpoint request and result round-trip through their factories",
    "[campaign.protocol][checkpoint]")
{
    CampaignCheckpointSaveRequest request;
    request.CampaignId = "campaign-1";
    request.CheckpointId = "cp-42";
    request.SourceRevision = 9;
    request.NativeSaveIdentity = "stre-cp-42";
    const auto decodedRequest = RoundTripServer(request);
    REQUIRE(decodedRequest->CampaignId == request.CampaignId);
    REQUIRE(decodedRequest->CheckpointId == request.CheckpointId);
    REQUIRE(decodedRequest->SourceRevision == 9);
    REQUIRE(decodedRequest->NativeSaveIdentity == request.NativeSaveIdentity);
    REQUIRE(decodedRequest->IsValid());

    std::vector<STRE::Campaign::NativeSaveBundleMember> members(2);
    members[0].Role = STRE::Campaign::NativeSaveMemberRole::Ess;
    members[0].Size = 100;
    members[0].Sha256.fill(1);
    members[1].Role = STRE::Campaign::NativeSaveMemberRole::Skse;
    members[1].Size = 20;
    members[1].Sha256.fill(2);
    const auto artifact = STRE::Campaign::BuildNativeSaveBundleArtifact(
        "stre-cp-42", std::move(members));
    REQUIRE(artifact.Succeeded());

    CampaignCheckpointSaveResult result;
    result.CampaignId = "campaign-1";
    result.CheckpointId = "cp-42";
    result.NativeSaveIdentity = "stre-cp-42";
    result.Result = CampaignCheckpointSaveResultCode::Success;
    result.FingerprintAlgorithm = "SHA-256";
    result.FingerprintVersion = 1;
    result.Fingerprint.assign(
        artifact.Value.Fingerprint.begin(),
        artifact.Value.Fingerprint.end());
    result.SaveMetadataCodecVersion = 1;
    result.SaveMetadata.assign(
        artifact.Value.Metadata.begin(), artifact.Value.Metadata.end());
    const auto decodedResult = RoundTripClient(result);
    REQUIRE(decodedResult->CampaignId == result.CampaignId);
    REQUIRE(decodedResult->CheckpointId == result.CheckpointId);
    REQUIRE(decodedResult->NativeSaveIdentity == result.NativeSaveIdentity);
    REQUIRE(decodedResult->Fingerprint == result.Fingerprint);
    REQUIRE(decodedResult->SaveMetadata == result.SaveMetadata);
    REQUIRE(decodedResult->IsValid());

    CampaignCheckpointSaveResult failure;
    failure.CampaignId = "campaign-1";
    failure.CheckpointId = "cp-42";
    failure.NativeSaveIdentity = "stre-cp-42";
    failure.Result = CampaignCheckpointSaveResultCode::Failure;
    const auto decodedFailure = RoundTripClient(failure);
    REQUIRE(decodedFailure->IsValid());
    REQUIRE(decodedFailure->Result ==
        CampaignCheckpointSaveResultCode::Failure);
}

TEST_CASE(
    "Malformed campaign checkpoint packets fail strict validation",
    "[campaign.protocol][checkpoint][robustness]")
{
    CampaignCheckpointSaveRequest invalidRequest;
    invalidRequest.CampaignId = "campaign-1";
    invalidRequest.CheckpointId = "cp-42";
    invalidRequest.SourceRevision = 0;
    invalidRequest.NativeSaveIdentity = "stre-cp-42";
    REQUIRE_FALSE(RoundTripServer(invalidRequest)->IsValid());
    invalidRequest.SourceRevision = 1;
    invalidRequest.NativeSaveIdentity = "stre-cp-other";
    REQUIRE_FALSE(RoundTripServer(invalidRequest)->IsValid());

    Buffer truncatedRequest(8);
    Buffer::Writer requestWriter(&truncatedRequest);
    requestWriter.WriteBits(kCampaignCheckpointSaveRequest, 8);
    Buffer::Reader requestReader(&truncatedRequest);
    auto requestMessage = ServerMessageFactory{}.Extract(requestReader);
    REQUIRE(requestMessage);
    REQUIRE_FALSE(CastUnique<CampaignCheckpointSaveRequest>(
        std::move(requestMessage))->IsValid());

    Buffer truncatedResult(8);
    Buffer::Writer resultWriter(&truncatedResult);
    resultWriter.WriteBits(kCampaignCheckpointSaveResult, 8);
    Buffer::Reader resultReader(&truncatedResult);
    auto resultMessage = ClientMessageFactory{}.Extract(resultReader);
    REQUIRE(resultMessage);
    REQUIRE_FALSE(CastUnique<CampaignCheckpointSaveResult>(
        std::move(resultMessage))->IsValid());

    CampaignCheckpointSaveResult invalid;
    invalid.CampaignId = "campaign-1";
    invalid.CheckpointId = "cp-42";
    invalid.NativeSaveIdentity = "stre-cp-42";
    invalid.Result = static_cast<CampaignCheckpointSaveResultCode>(9);
    REQUIRE_FALSE(RoundTripClient(invalid)->IsValid());

    invalid.Result = CampaignCheckpointSaveResultCode::Success;
    REQUIRE_FALSE(RoundTripClient(invalid)->IsValid());

    invalid.Result = CampaignCheckpointSaveResultCode::Failure;
    invalid.FingerprintAlgorithm = "SHA-256";
    REQUIRE_FALSE(RoundTripClient(invalid)->IsValid());

    invalid.Result = CampaignCheckpointSaveResultCode::Success;
    invalid.Fingerprint.assign(
        STRE::Campaign::kNativeSaveSha256Size + 1, 1);
    invalid.SaveMetadata.assign(1, 1);
    invalid.FingerprintVersion = 1;
    invalid.SaveMetadataCodecVersion = 1;
    REQUIRE_FALSE(RoundTripClient(invalid)->IsValid());

    invalid.Fingerprint.assign(
        STRE::Campaign::kNativeSaveSha256Size, 1);
    invalid.SaveMetadata.assign(
        STRE::Campaign::kMaximumNativeSaveMetadataSize + 1, 1);
    REQUIRE_FALSE(RoundTripClient(invalid)->IsValid());

    invalid.CampaignId = String(kCampaignWireMaximumIdLength + 1, 'x');
    invalid.SaveMetadata.assign(1, 1);
    REQUIRE_FALSE(RoundTripClient(invalid)->IsValid());
}

TEST_CASE("Malformed and truncated campaign packets fail validation safely", "[campaign.protocol][robustness]")
{
    Buffer oversizedIdentifier(256);
    Buffer::Writer oversizedIdentifierWriter(&oversizedIdentifier);
    Serialization::WriteVarInt(oversizedIdentifierWriter, kCampaignWireMaximumIdLength + 1);
    Buffer::Reader oversizedIdentifierReader(&oversizedIdentifier);
    String decodedIdentifier = "must-be-cleared";
    REQUIRE_FALSE(ReadCampaignWireId(oversizedIdentifierReader, decodedIdentifier));
    REQUIRE(decodedIdentifier.empty());

    CampaignJoinRequest oversizedJoin;
    oversizedJoin.CampaignId = String(kCampaignWireMaximumIdLength + 1, 'x');
    oversizedJoin.MutationId = "mutation-join";
    REQUIRE_FALSE(RoundTripClient(oversizedJoin)->IsValid());

    Buffer truncatedClient(8);
    Buffer::Writer clientWriter(&truncatedClient);
    clientWriter.WriteBits(kCampaignSetReadyRequest, 8);
    Buffer::Reader clientReader(&truncatedClient);
    auto clientMessage = ClientMessageFactory{}.Extract(clientReader);
    REQUIRE(clientMessage);
    const auto ready = CastUnique<CampaignSetReadyRequest>(std::move(clientMessage));
    REQUIRE_FALSE(ready->IsValid());

    Buffer truncatedServer(8);
    Buffer::Writer serverWriter(&truncatedServer);
    serverWriter.WriteBits(kNotifyCampaignSnapshot, 8);
    Buffer::Reader serverReader(&truncatedServer);
    auto serverMessage = ServerMessageFactory{}.Extract(serverReader);
    REQUIRE(serverMessage);
    const auto snapshot = CastUnique<NotifyCampaignSnapshot>(std::move(serverMessage));
    REQUIRE_FALSE(snapshot->IsValid());

    Buffer invalidOpcode(8);
    Buffer::Writer invalidWriter(&invalidOpcode);
    invalidWriter.WriteBits(255, 8);
    Buffer::Reader invalidReader(&invalidOpcode);
    REQUIRE_FALSE(ClientMessageFactory{}.Extract(invalidReader));

    NotifyCampaignSnapshot oversized;
    oversized.Snapshot.CampaignId = "campaign-1";
    oversized.Snapshot.RosterSealed = false;
    for (std::size_t index = 0; index < 11; ++index)
    {
        oversized.Snapshot.Roster.push_back({"slot-x", "player-x", false, false});
    }
    REQUIRE_FALSE(RoundTripServer(oversized)->IsValid());

    for (const char* invalidCode : {
             "A7K", "A7K22", "A7I2", "A7O2", "A702", "A712"})
    {
        CampaignJoinByCodeRequest invalidJoinCode;
        invalidJoinCode.JoinCode = invalidCode;
        invalidJoinCode.MutationId = "mutation-code";
        invalidJoinCode.DisplayName = "Player";
        REQUIRE_FALSE(RoundTripClient(invalidJoinCode)->IsValid());
    }

    const std::vector<std::string> invalidDisplayNames{
        "",
        "   ",
        std::string(kCampaignLobbyMaximumDisplayNameLength + 1, 'x'),
        std::string{"Player\nTwo"},
        std::string{"Player\xC2\x80"},
        std::string{"\xC3\x28", 2},
        std::string(kCampaignLobbyMaximumDisplayNameBytes + 1, 'x')};
    for (const std::string& invalidDisplayName : invalidDisplayNames)
    {
        CampaignCreateRequest invalidCreate;
        invalidCreate.MutationId = "mutation-create";
        invalidCreate.DisplayName = invalidDisplayName.c_str();
        REQUIRE_FALSE(RoundTripClient(invalidCreate)->IsValid());

        CampaignJoinByCodeRequest invalidJoin;
        invalidJoin.JoinCode = "A7K2";
        invalidJoin.MutationId = "mutation-code";
        invalidJoin.DisplayName = invalidDisplayName.c_str();
        REQUIRE_FALSE(RoundTripClient(invalidJoin)->IsValid());
    }

    NotifyCampaignLobbyState oversizedLobby;
    oversizedLobby.JoinCode = "A7K2";
    oversizedLobby.CampaignId = "campaign-1";
    for (std::size_t index = 0; index < 11; ++index)
        oversizedLobby.Members.push_back({"Player", true});
    REQUIRE_FALSE(RoundTripServer(oversizedLobby)->IsValid());

    NotifyCampaignLobbyState oversizedName;
    oversizedName.JoinCode = "A7K2";
    oversizedName.CampaignId = "campaign-1";
    oversizedName.Members.push_back(
        {String(kCampaignLobbyMaximumDisplayNameLength + 1, 'x'), true});
    REQUIRE_FALSE(RoundTripServer(oversizedName)->IsValid());
}

TEST_CASE("Campaign lobby display names normalize bounded Unicode safely", "[campaign.protocol][presentation]")
{
    TiltedPhoques::String normalized;
    const std::string unicodeName =
        "  L\xC3\xA9" "a \xF0\x9F\x90\x89  ";
    REQUIRE(NormalizeCampaignLobbyDisplayName(unicodeName, normalized));
    REQUIRE(normalized == "L\xC3\xA9" "a \xF0\x9F\x90\x89");
    REQUIRE(IsValidCampaignLobbyDisplayName(normalized));

    std::string boundedUnicode;
    for (std::size_t index = 0; index < 24; ++index)
        boundedUnicode += "\xC3\xA9";
    REQUIRE(NormalizeCampaignLobbyDisplayName(boundedUnicode, normalized));
    boundedUnicode += "\xC3\xA9";
    REQUIRE_FALSE(NormalizeCampaignLobbyDisplayName(
        boundedUnicode, normalized));
}
