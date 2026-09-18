#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>
#include <optional>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <catch2/catch.hpp>

#include <Messages/CharacterAppearanceUpdate.h>
#include <Messages/ClientMessageFactory.h>
#include <Messages/ServerMessageFactory.h>
#include <StringCache.h>
#include <server/Services/CharacterAppearanceUpdate.h>
#include <CharacterCreation/FinalRespawn.h>
#include <CharacterCreation/RemoteMaterializationLifecycle.h>
#include <limits>

using namespace TiltedPhoques;

namespace
{
RequestCharacterAppearanceUpdate MakeAppearance()
{
    RequestCharacterAppearanceUpdate request;
    request.ActorId = 0xFFFF1234;
    request.FinalBuildRevision = 0x123456789ABC;
    request.Descriptor = {GameId{0, 0x13746}, 0, 50.f};
    request.ChangeFlags = 0x02000A48;
    request.AppearanceBuffer.assign("npc\0bytes", 9);
    Tints::Entry tint{};
    tint.Name = String("textures/actors/skin.dds");
    tint.Alpha = 0.75f;
    tint.Color = 0x12345678;
    tint.Type = 3;
    request.FaceTints.Entries.push_back(tint);
    tint.Name = String("textures/actors/paint.dds");
    tint.Alpha = 0.25f;
    tint.Color = 0xABCDEF01;
    tint.Type = 7;
    request.FaceTints.Entries.push_back(tint);
    return request;
}
} // namespace

TEST_CASE("Nord Orc Khajiit and Argonian finals share eligibility and the identity transaction", "[final-respawn]")
{
    using namespace STRE::CharacterCreation;
    const auto race = GENERATE(0x13746u, 0x13747u, 0x13745u, 0x13740u);
    const auto sex = GENERATE(0u, 1u);
    auto request = MakeAppearance();
    request.Descriptor.Race = GameId{0, race};
    request.Descriptor.Sex = sex;
    REQUIRE(request.IsValid());
    REQUIRE(MatchesAppliedCharacterBuild(request, true, request.FinalBuildRevision, request.Descriptor.Race, std::nullopt));
    REQUIRE(FinalRespawnEligible(true, true, true, request.FinalBuildRevision, request.FinalBuildRevision));
    // Same production policy/model for every row; native creation is covered by
    // source contracts and still needs the corresponding in-game acceptance.
    RemoteMaterializationLifecycle cycle;
    const MaterializationKey key{7, 0x100003, request.ActorId, 91};
    const MaterializationForms old{0xFF000011, 0xFF000012}, candidate{0xFF000013, 0xFF000014};
    REQUIRE(cycle.Reserve(key, old));
    REQUIRE(cycle.RecordCandidate(key, candidate));
    REQUIRE(cycle.Observe(key, candidate.Actor, true) == DiscoveryRoute::CandidateStaged);
    REQUIRE(cycle.MarkReady(key));
    REQUIRE(cycle.Commit(key));
    REQUIRE(cycle.Key() == key);
    REQUIRE(cycle.BoundActor() == candidate.Actor);
}

TEST_CASE("Every sealed appearance kind uses the same final respawn eligibility", "[final-respawn]")
{
    auto request = MakeAppearance();
    // No-change, cosmetics only, same/same, same/sex, race/same, race/sex.
    for (unsigned scenario = 0; scenario != 6; ++scenario)
    {
        DYNAMIC_SECTION("appearance scenario " << scenario)
        {
            request.Descriptor.Race = GameId{0, scenario >= 4 ? 0x13741u : 0x13746u};
            request.Descriptor.Sex = scenario == 3 || scenario == 5;
            if (scenario == 1)
                request.FaceTints.Entries.front().Color ^= 1;
            REQUIRE(MatchesAppliedCharacterBuild(request, true, request.FinalBuildRevision, request.Descriptor.Race, std::nullopt));
            REQUIRE(STRE::CharacterCreation::FinalRespawnEligible(true, true, true, request.FinalBuildRevision, request.FinalBuildRevision));
        }
    }
}

TEST_CASE("Final build rejects intermediate stale and contradictory final snapshots", "[final-respawn]")
{
    auto request = MakeAppearance();
    const auto revision = request.FinalBuildRevision;
    const auto race = request.Descriptor.Race;
    REQUIRE_FALSE(MatchesAppliedCharacterBuild(request, false, revision, race, std::nullopt));
    REQUIRE_FALSE(MatchesAppliedCharacterBuild(request, true, revision + 1, race, std::nullopt));
    REQUIRE_FALSE(MatchesAppliedCharacterBuild(request, true, revision, GameId{0, 0x13741}, std::nullopt));
    const std::optional<CharacterAppearanceUpdate> canonical{request};
    REQUIRE(MatchesAppliedCharacterBuild(request, true, revision, race, canonical)); // Idempotent delivery.
    request.FaceTints.Entries.front().Color ^= 1;
    REQUIRE_FALSE(MatchesAppliedCharacterBuild(request, true, revision, race, canonical));
    request.FinalBuildRevision = 0;
    REQUIRE_FALSE(MatchesAppliedCharacterBuild(request, true, 0, race, std::nullopt));
}

TEST_CASE("Appearance snapshots survive both factories and cached tint names", "[appearance]")
{
    StringCache::Get().Clear();
    auto request = MakeAppearance();
    SECTION("uncached names")
    {
    }
    SECTION("cached names")
    {
        for (const auto& tint : request.FaceTints.Entries)
            StringCache::Get().Add(tint.Name);
    }
    SECTION("maximum supported snapshot")
    {
        request.AppearanceBuffer.assign(CharacterAppearanceUpdate::MaxAppearanceBytes, 'n');
        auto tint = request.FaceTints.Entries.front();
        request.FaceTints.Entries.assign(CharacterAppearanceUpdate::MaxTintCount, tint);
    }
    SECTION("maximum texture name")
    {
        request.FaceTints.Entries.front().Name = String(CharacterAppearanceUpdate::MaxTextureNameBytes, 't');
    }
    SECTION("empty tint replacement")
    {
        request.FaceTints.Entries.clear();
    }

    Buffer buffer(400000);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);
    Buffer::Reader reader(&buffer);
    auto message = ClientMessageFactory{}.Extract(reader);
    REQUIRE(message);
    REQUIRE(message->GetOpcode() == RequestCharacterAppearanceUpdate::Opcode);
    const auto& decoded = static_cast<const RequestCharacterAppearanceUpdate&>(*message);
    REQUIRE(decoded.IsValid());
    REQUIRE(decoded == request);

    NotifyCharacterAppearanceUpdate notify;
    static_cast<CharacterAppearanceUpdate&>(notify) = decoded;
    writer.Reset();
    notify.Serialize(writer);
    reader.Reset();
    auto notification = ServerMessageFactory{}.Extract(reader);
    REQUIRE(notification);
    REQUIRE(notification->GetOpcode() == NotifyCharacterAppearanceUpdate::Opcode);
    const auto& received = static_cast<const NotifyCharacterAppearanceUpdate&>(*notification);
    REQUIRE(received.IsValid());
    REQUIRE(received == request);
    StringCache::Get().Clear();
}

TEST_CASE("Appearance tint bytes retain the assignment and spawn format", "[appearance]")
{
    StringCache::Get().Clear();
    auto request = MakeAppearance();
    Buffer buffer(1024);
    Buffer::Writer writer(&buffer);
    request.SerializeRaw(writer);
    Buffer::Reader reader(&buffer);
    REQUIRE(Serialization::ReadVarInt(reader) == request.ActorId);
    uint64_t flags{};
    REQUIRE(reader.ReadBits(flags, 32));
    REQUIRE(flags == request.ChangeFlags);
    REQUIRE(Serialization::ReadString(reader) == request.AppearanceBuffer);
    Tints tints;
    tints.Deserialize(reader);
    REQUIRE(tints == request.FaceTints);
    StringCache::Get().Clear();
}

TEST_CASE("Appearance rejects truncated packets without retaining partial state", "[appearance]")
{
    StringCache::Get().Clear();
    auto request = MakeAppearance();
    Buffer buffer(1024);
    Buffer::Writer writer(&buffer);
    request.SerializeRaw(writer);
    const auto bytes = (writer.GetBitPosition() + 7) / 8;
    for (size_t length = 0; length < bytes; ++length)
    {
        INFO("truncated bytes=" << length);
        Buffer truncated(buffer.GetData(), length);
        Buffer::Reader reader(&truncated);
        auto decoded = MakeAppearance();
        decoded.DeserializeRaw(reader);
        REQUIRE_FALSE(decoded.IsValid());
        REQUIRE(decoded.AppearanceBuffer.empty());
        REQUIRE(decoded.FaceTints.Entries.empty());
    }
    StringCache::Get().Clear();
}

TEST_CASE("Appearance refuses invalid local payloads instead of truncating", "[appearance]")
{
    auto request = MakeAppearance();
    SECTION("empty native buffer")
    {
        request.AppearanceBuffer.clear();
    }
    SECTION("oversized buffer")
    {
        request.AppearanceBuffer.resize(CharacterAppearanceUpdate::MaxAppearanceBytes + 1);
    }
    SECTION("oversized collection")
    {
        request.FaceTints.Entries.resize(256);
    }
    SECTION("oversized name")
    {
        request.FaceTints.Entries.front().Name.resize(1024);
    }
    SECTION("embedded null name")
    {
        request.FaceTints.Entries.front().Name = String("bad\0name", 8);
    }
    SECTION("aggregate exceeds client send buffer")
    {
        auto tint = request.FaceTints.Entries.front();
        tint.Name = String(CharacterAppearanceUpdate::MaxTextureNameBytes, 't');
        request.FaceTints.Entries.assign(CharacterAppearanceUpdate::MaxTintCount, tint);
    }
    SECTION("NaN alpha")
    {
        request.FaceTints.Entries.front().Alpha = std::numeric_limits<float>::quiet_NaN();
    }
    SECTION("alpha outside range")
    {
        request.FaceTints.Entries.front().Alpha = 2.f;
    }
    REQUIRE_FALSE(request.IsValid());
    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    request.SerializeRaw(writer);
    Buffer::Reader reader(&buffer);
    RequestCharacterAppearanceUpdate decoded;
    decoded.DeserializeRaw(reader);
    REQUIRE_FALSE(decoded.IsValid());
}

TEST_CASE("Appearance refuses overlong varints and unknown cached texture IDs", "[appearance]")
{
    StringCache::Get().Clear();
    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    SECTION("unterminated actor ID")
    {
        for (int i = 0; i < 10; ++i)
        {
            writer.WriteBits(0, 7);
            writer.WriteBits(1, 1);
        }
    }
    SECTION("unknown texture ID")
    {
        Serialization::WriteVarInt(writer, 2);
        writer.WriteBits(0x02000A48, 32);
        Serialization::WriteString(writer, String("npc"));
        writer.WriteBits(1, 8);
        Serialization::WriteVarInt(writer, 3);
        writer.WriteBits(0, 32);
        writer.WriteBits(1, 1);
        Serialization::WriteVarInt(writer, 999);
        writer.WriteBits(0, 32);
    }
    Buffer::Reader reader(&buffer);
    RequestCharacterAppearanceUpdate decoded;
    decoded.DeserializeRaw(reader);
    REQUIRE_FALSE(decoded.IsValid());
}

TEST_CASE("Appearance ownership policy replaces canonical state before broadcast", "[appearance]")
{
    // Exercise the exact production policy with a component-shaped fixture;
    // entity lookup and spatial routing remain server integration concerns.
    struct Character
    {
        String SaveBuffer{"old"};
        uint32_t ChangeFlags{1};
        Tints FaceTints;
        int InventorySentinel{42};
    } character;
    int owner{}, other{};
    auto request = MakeAppearance();
    int broadcasts{};
    auto broadcast = [&](const NotifyCharacterAppearanceUpdate& acNotify)
    {
        ++broadcasts;
        REQUIRE(acNotify == request);
        REQUIRE(character.SaveBuffer == acNotify.AppearanceBuffer);
        REQUIRE(character.ChangeFlags == acNotify.ChangeFlags);
        REQUIRE(character.FaceTints == acNotify.FaceTints);
    };
    REQUIRE_FALSE(ApplyCharacterAppearanceUpdate(request, &owner, &other, character, broadcast));
    REQUIRE(character.SaveBuffer == "old");
    REQUIRE(character.ChangeFlags == 1);
    REQUIRE(character.FaceTints.Entries.empty());
    REQUIRE(broadcasts == 0);

    REQUIRE(ApplyCharacterAppearanceUpdate(request, &owner, &owner, character, broadcast));
    REQUIRE(broadcasts == 1);
    request.FaceTints.Entries.front().Color ^= 0xFF;
    REQUIRE(ApplyCharacterAppearanceUpdate(request, &owner, &owner, character, broadcast));
    REQUIRE(broadcasts == 2);
    // Repeated snapshots retain identical state and can still relay a forced final.
    REQUIRE(ApplyCharacterAppearanceUpdate(request, &owner, &owner, character, broadcast));
    REQUIRE(broadcasts == 3);
    // Empty tints clear the previous canonical tints, rather than retaining them.
    request.FaceTints.Entries.clear();
    REQUIRE(ApplyCharacterAppearanceUpdate(request, &owner, &owner, character, broadcast));
    REQUIRE(character.FaceTints.Entries.empty());
    REQUIRE(character.InventorySentinel == 42);
    request.AppearanceBuffer.clear();
    REQUIRE_FALSE(ApplyCharacterAppearanceUpdate(request, &owner, &owner, character, broadcast));
    REQUIRE(broadcasts == 4);
    REQUIRE(character.SaveBuffer == String("npc\0bytes", 9));
}
