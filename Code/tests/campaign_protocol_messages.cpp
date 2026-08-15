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
static_assert(kNotifyCharacterBuildState == 63);
static_assert(kNotifyWorldEntityManipulation == 64);
static_assert(kCampaignCommandResponse == 65);
static_assert(kNotifyCampaignSnapshot == 66);

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
}

TEST_CASE("Authentication carries durable STRE PlayerId metadata", "[campaign.protocol]")
{
    AuthenticationRequest request;
    request.StrePlayerId =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    const auto decoded = RoundTripClient(request);
    REQUIRE(decoded->StrePlayerId == request.StrePlayerId);
}

TEST_CASE("Every campaign client command round-trips through the factory", "[campaign.protocol]")
{
    CampaignCreateRequest create;
    create.MutationId = "mutation-create";
    REQUIRE(RoundTripClient(create)->MutationId == create.MutationId);

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
    existingMember.Result =
        CampaignProtocolResult::ExistingMembershipRequiresResume;
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
    notification.Snapshot.Roster.push_back(
        {"slot-01", "player-1", true, true});
    notification.Snapshot.Roster.push_back(
        {"slot-02", "player-2", false, true});
    const auto decodedSnapshot = RoundTripServer(notification);
    REQUIRE(decodedSnapshot->Snapshot == notification.Snapshot);
    REQUIRE(decodedSnapshot->IsValid());
}

TEST_CASE("Malformed and truncated campaign packets fail validation safely", "[campaign.protocol][robustness]")
{
    Buffer oversizedIdentifier(256);
    Buffer::Writer oversizedIdentifierWriter(&oversizedIdentifier);
    Serialization::WriteVarInt(
        oversizedIdentifierWriter, kCampaignWireMaximumIdLength + 1);
    Buffer::Reader oversizedIdentifierReader(&oversizedIdentifier);
    String decodedIdentifier = "must-be-cleared";
    REQUIRE_FALSE(ReadCampaignWireId(
        oversizedIdentifierReader, decodedIdentifier));
    REQUIRE(decodedIdentifier.empty());

    CampaignJoinRequest oversizedJoin;
    oversizedJoin.CampaignId = String(
        kCampaignWireMaximumIdLength + 1, 'x');
    oversizedJoin.MutationId = "mutation-join";
    REQUIRE_FALSE(RoundTripClient(oversizedJoin)->IsValid());

    Buffer truncatedClient(8);
    Buffer::Writer clientWriter(&truncatedClient);
    clientWriter.WriteBits(kCampaignSetReadyRequest, 8);
    Buffer::Reader clientReader(&truncatedClient);
    auto clientMessage = ClientMessageFactory{}.Extract(clientReader);
    REQUIRE(clientMessage);
    const auto ready = CastUnique<CampaignSetReadyRequest>(
        std::move(clientMessage));
    REQUIRE_FALSE(ready->IsValid());

    Buffer truncatedServer(8);
    Buffer::Writer serverWriter(&truncatedServer);
    serverWriter.WriteBits(kNotifyCampaignSnapshot, 8);
    Buffer::Reader serverReader(&truncatedServer);
    auto serverMessage = ServerMessageFactory{}.Extract(serverReader);
    REQUIRE(serverMessage);
    const auto snapshot = CastUnique<NotifyCampaignSnapshot>(
        std::move(serverMessage));
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
        oversized.Snapshot.Roster.push_back(
            {"slot-x", "player-x", false, false});
    }
    REQUIRE_FALSE(RoundTripServer(oversized)->IsValid());
}
