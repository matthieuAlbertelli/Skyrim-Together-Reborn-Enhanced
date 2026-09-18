#include <TiltedCore/Serialization.hpp>
#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <CharacterCreation/CreationSeating.h>
#include <CharacterCreation/StandingCreation.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <catch2/catch.hpp>
using namespace STRE::CharacterCreation;

TEST_CASE("Individual seating never waits for another build", "[creation-seating]")
{
    SeatProjection a, b;
    REQUIRE(a.Observe(1, 11, true, false, false, false, false) == SeatAction::Activate);
    a.Issued = true;
    REQUIRE(b.Observe(0, 0, false, false, false, false, false) == SeatAction::Pending);
    REQUIRE(a.Observe(1, 11, true, false, false, false, true) == SeatAction::AwaitEntry);
    REQUIRE(a.Observe(1, 11, true, true, true, false, true) == SeatAction::Complete);
    REQUIRE(b.Observe(2, 22, true, false, false, false, false) == SeatAction::Activate);
    REQUIRE(a.Observe(3, 33, true, false, false, false, false) == SeatAction::Activate);
    REQUIRE_FALSE(a.Issued);
}
TEST_CASE("Seating occupation is fail closed and correct occupant is idempotent", "[creation-seating]")
{
    SeatProjection p;
    REQUIRE(p.Observe(1, 11, true, false, false, false, true) == SeatAction::Conflict);
    REQUIRE(p.Observe(1, 11, true, false, false, true, false) == SeatAction::Conflict);
    REQUIRE(p.Observe(1, 11, true, true, true, false, true) == SeatAction::Complete);
    REQUIRE_FALSE(p.Issued);
    REQUIRE(p.Observe(1, 11, true, false, false, false, false) == SeatAction::Complete);
    SeatProjection entering;
    REQUIRE(entering.Observe(1, 11, true, true, false, false, true) == SeatAction::AwaitEntry);
}
TEST_CASE("Creation seat mapping shares the durable rank", "[creation-seating]")
{
    REQUIRE(CreationSeatLocalFormId(0) == 0xBF3DD);
    REQUIRE(CreationSeatLocalFormId(1) == 0xBF3DC);
    REQUIRE(CreationSeatLocalFormId(9) == 0xC5178);
    REQUIRE_FALSE(CreationSeatLocalFormId(10));
    const std::array<std::string_view, 2> roster{"b", "a"}, reversed{"a", "b"};
    REQUIRE(CreationSeatLocalFormId(*StandingCreationPositionIndex(roster, "a")) == CreationSeatLocalFormId(*StandingCreationPositionIndex(reversed, "a")));
    REQUIRE_FALSE(StandingCreationPositionIndex(roster, "missing"));
}
TEST_CASE("Build seating identity is bounded and survives wire roundtrip", "[creation-seating]")
{
    NotifyCharacterBuildState sent;
    sent.State = CharacterBuildNetworkState::Applied;
    sent.PlayerId = 4;
    sent.ServerId = 7;
    sent.Revision = 2;
    sent.SeatingCampaignId = "campaign";
    sent.SeatingPlayerId = "durable-player";
    TiltedPhoques::Buffer buffer(65536);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    sent.SerializeRaw(writer);
    TiltedPhoques::Buffer::Reader reader(&buffer);
    NotifyCharacterBuildState received;
    received.DeserializeRaw(reader);
    REQUIRE(received.SeatingCampaignId == sent.SeatingCampaignId);
    REQUIRE(received.SeatingPlayerId == sent.SeatingPlayerId);
    REQUIRE(received.PlayerId == 4);
}

TEST_CASE("Seating identity rejects oversized output", "[creation-seating]")
{
    NotifyCharacterBuildState sent;
    sent.SeatingCampaignId = "campaign";
    sent.SeatingPlayerId = String(129, 'x');
    TiltedPhoques::Buffer buffer(65536);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    sent.SerializeRaw(writer);
    TiltedPhoques::Buffer::Reader reader(&buffer);
    NotifyCharacterBuildState decoded;
    decoded.DeserializeRaw(reader);
    REQUIRE(decoded.SeatingPlayerId.empty());


}
