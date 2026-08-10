#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>

#include <catch2/catch.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <Messages/AuthenticationRequest.h>
#include <Messages/AuthenticationResponse.h>
#include <Messages/ClientMessageFactory.h>
#include <Messages/NotifyInventoryChanges.h>
#include <Messages/NotifyWorldEntityManipulation.h>
#include <Messages/RequestInventoryChanges.h>
#include <Messages/RequestWorldEntityManipulation.h>
#include <Messages/ServerMessageFactory.h>
#include <Structs/NativePlugins.h>

using namespace TiltedPhoques;

TEST_CASE("Native plugin list round-trips", "[stre.native-plugins]")
{
    NativePlugins sent;
    sent.PluginList.push_back({"BetterGrabbing.dll", "0.1.12.0"});
    sent.PluginList.push_back({"ExamplePlugin.dll", "2.0.0.0"});

    Buffer buffer(512);
    Buffer::Writer writer(&buffer);
    sent.Serialize(writer);

    NativePlugins received;
    Buffer::Reader reader(&buffer);
    received.Deserialize(reader);

    REQUIRE(received == sent);
}

TEST_CASE("Authentication request carries loaded native plugins", "[stre.native-plugins]")
{
    AuthenticationRequest request;
    request.Version = "stre-test";
    request.Username = "Dragonborn";
    request.UserNativePlugins.PluginList.push_back({"BetterGrabbing.dll", "0.1.12.0"});

    Buffer buffer(1024);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto message = factory.Extract(reader);

    REQUIRE(message);
    auto decoded = CastUnique<AuthenticationRequest>(std::move(message));
    REQUIRE(decoded->UserNativePlugins == request.UserNativePlugins);
}

TEST_CASE("Authentication response reports missing native plugins", "[stre.native-plugins]")
{
    AuthenticationResponse response;
    response.Type = AuthenticationResponse::ResponseType::kNativePluginsMissing;
    response.RequiredNativePlugins.PluginList.push_back({"BetterGrabbing.dll", {}});

    Buffer buffer(512);
    Buffer::Writer writer(&buffer);
    response.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto message = factory.Extract(reader);

    REQUIRE(message);
    auto decoded = CastUnique<AuthenticationResponse>(std::move(message));
    REQUIRE(decoded->Type == AuthenticationResponse::ResponseType::kNativePluginsMissing);
    REQUIRE(decoded->RequiredNativePlugins == response.RequiredNativePlugins);
}

TEST_CASE("World entity manipulation request round-trips", "[stre.world-entity]")
{
    RequestWorldEntityManipulation request;
    request.WorldEntityId = 0xFEDCBA9876543210ull;
    request.PlacedReferenceId = GameId{3, 0x123456};
    request.Action = WorldEntityManipulationAction::Update;
    request.Transform = {1.25f, -2.5f, 3.75f, 0.1f, 0.2f, 0.3f};

    Buffer buffer(512);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto message = factory.Extract(reader);

    REQUIRE(message);
    auto decoded = CastUnique<RequestWorldEntityManipulation>(std::move(message));
    REQUIRE(*decoded == request);
}

TEST_CASE("World entity manipulation notification round-trips", "[stre.world-entity]")
{
    NotifyWorldEntityManipulation notification;
    notification.WorldEntityId = 42;
    notification.PlacedReferenceId = GameId{5, 0x00ABCDEF};
    notification.Action = WorldEntityManipulationAction::Release;
    notification.AuthorityPlayerId = 7;
    notification.Transform = {-10.0f, 20.0f, 30.0f, -0.2f, 1.0f, 2.4f};

    Buffer buffer(512);
    Buffer::Writer writer(&buffer);
    notification.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto message = factory.Extract(reader);

    REQUIRE(message);
    auto decoded = CastUnique<NotifyWorldEntityManipulation>(std::move(message));
    REQUIRE(*decoded == notification);
}


TEST_CASE("Placed reference pickup request carries lazy adoption identity", "[stre.world-entity]")
{
    RequestInventoryChanges request;
    request.ServerId = 77;
    request.Item.BaseId = GameId{2, 0x1234};
    request.Item.Count = 1;
    request.Item.ExtraOwnerId = GameId{6, 0x00C0FFEE};
    request.UpdateClients = false;
    request.DroppedFormId = 0x01001234;
    request.PlacedReferenceId = GameId{9, 0x00FEDCBA};

    Buffer buffer(512);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto message = factory.Extract(reader);

    REQUIRE(message);
    auto decoded = CastUnique<RequestInventoryChanges>(std::move(message));
    REQUIRE(*decoded == request);
}

TEST_CASE("Inventory ownership provenance round-trips", "[stre.ownership]")
{
    Inventory::Entry sent;
    sent.BaseId = GameId{2, 0x123456};
    sent.Count = 1;
    sent.ExtraOwnerId = GameId{7, 0x00ABCDEF};

    Buffer buffer(256);
    Buffer::Writer writer(&buffer);
    sent.Serialize(writer);

    Inventory::Entry received;
    Buffer::Reader reader(&buffer);
    received.Deserialize(reader);

    REQUIRE(received == sent);
    REQUIRE(received.ContainsExtraData());
    REQUIRE(received.ExtraOwnerId == sent.ExtraOwnerId);
}

TEST_CASE("Placed reference lifecycle notification round-trips", "[stre.world-entity]")
{
    NotifyInventoryChanges notification;
    notification.ServerId = 88;
    notification.WorldEntityId = 1234;
    notification.PlacedReferenceId = GameId{4, 0x00A0B0C0};
    notification.LifecycleOnly = true;

    Buffer buffer(512);
    Buffer::Writer writer(&buffer);
    notification.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto message = factory.Extract(reader);

    REQUIRE(message);
    auto decoded = CastUnique<NotifyInventoryChanges>(std::move(message));
    REQUIRE(*decoded == notification);
}
