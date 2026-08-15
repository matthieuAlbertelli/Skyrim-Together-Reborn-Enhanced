#pragma once

#include <CampaignAdmissionService.h>
#include <Events/PacketEvent.h>

#include <string_view>

struct CampaignCreateRequest;
struct CampaignJoinRequest;
struct CampaignResumeRequest;
struct CampaignStartRequest;
struct CampaignSetReadyRequest;
struct CampaignLeaveRequest;
struct Player;
struct PlayerLeaveEvent;
struct World;

class CampaignProtocolService final
{
public:
    CampaignProtocolService(
        World& aWorld,
        entt::dispatcher& aDispatcher) noexcept;

    [[nodiscard]] STRE::Campaign::CampaignConnectionRegistration
    RegisterAuthenticatedPlayer(
        Player& aPlayer,
        const std::string& acDurablePlayerId) noexcept;
    void UnregisterAuthenticatedPlayer(Player& aPlayer) noexcept;

    [[nodiscard]] const STRE::Campaign::CampaignAdmissionRecord*
    GetAdmission(const Player& acPlayer) const noexcept;

private:
    using CommandResult = STRE::Campaign::CampaignProtocolCommandResult;

    [[nodiscard]] static STRE::Campaign::CampaignConnectionHandle
    ToHandle(const Player& acPlayer) noexcept;
    [[nodiscard]] bool SharesCampaignParty(
        const Player& acPlayer,
        const STRE::Campaign::CampaignId& acCampaign,
        bool aRequireCompleteRoster) noexcept;

    void SendResult(Player& aPlayer, const CommandResult& acResult) const noexcept;
    void BroadcastSnapshot(
        const CampaignSnapshotData& acSnapshot) const noexcept;
    void Finish(
        Player& aPlayer,
        CommandResult aResult,
        std::string_view acMutationId = {}) const noexcept;

    void OnCreate(
        const PacketEvent<CampaignCreateRequest>& acPacket) noexcept;
    void OnJoin(
        const PacketEvent<CampaignJoinRequest>& acPacket) noexcept;
    void OnResume(
        const PacketEvent<CampaignResumeRequest>& acPacket) noexcept;
    void OnStart(
        const PacketEvent<CampaignStartRequest>& acPacket) noexcept;
    void OnSetReady(
        const PacketEvent<CampaignSetReadyRequest>& acPacket) noexcept;
    void OnLeave(
        const PacketEvent<CampaignLeaveRequest>& acPacket) noexcept;
    void OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept;

    World& m_world;
    STRE::Campaign::CampaignAdmissionService m_admission;

    entt::scoped_connection m_createConnection;
    entt::scoped_connection m_joinConnection;
    entt::scoped_connection m_resumeConnection;
    entt::scoped_connection m_startConnection;
    entt::scoped_connection m_readyConnection;
    entt::scoped_connection m_leaveConnection;
    entt::scoped_connection m_playerLeaveConnection;
};
