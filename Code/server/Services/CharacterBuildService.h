#pragma once

#include <Events/PacketEvent.h>
#include <Structs/CharacterBuild.h>

#include <cstdint>

struct CharacterBuildAppliedRequest;
struct CharacterBuildRequest;
struct Player;
struct World;

class CharacterBuildService
{
public:
    CharacterBuildService(
        World& aWorld,
        entt::dispatcher& aDispatcher) noexcept;

    void OnCharacterBuildRequest(
        const PacketEvent<CharacterBuildRequest>& acPacket) noexcept;
    void OnCharacterBuildAppliedRequest(
        const PacketEvent<CharacterBuildAppliedRequest>& acPacket) noexcept;

private:
    void SendRejected(
        Player& aPlayer,
        CharacterBuildResult aResult) const noexcept;

    World& m_world;
    std::uint64_t m_nextRevision{1};

    entt::scoped_connection m_buildRequestConnection;
    entt::scoped_connection m_buildAppliedConnection;
};
