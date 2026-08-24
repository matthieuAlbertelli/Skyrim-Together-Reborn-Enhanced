#pragma once

#include <CampaignAdmissionService.h>
#include <CampaignLobbyDirectory.h>
#include <Events/PacketEvent.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

struct CampaignCreateRequest;
struct CampaignJoinRequest;
struct CampaignResumeRequest;
struct CampaignStartRequest;
struct CampaignSetReadyRequest;
struct CampaignLeaveRequest;
struct CampaignJoinByCodeRequest;
struct CampaignHelgenInvestigationReadyRequest;
struct CampaignCheckpointSaveResult;
struct Player;
struct PlayerLeaveEvent;
struct World;

class CampaignProtocolService final
{
public:
    CampaignProtocolService(World& aWorld, entt::dispatcher& aDispatcher) noexcept;

    [[nodiscard]] STRE::Campaign::CampaignConnectionRegistration RegisterAuthenticatedPlayer(Player& aPlayer, const std::string& acDurablePlayerId) noexcept;
    void UnregisterAuthenticatedPlayer(Player& aPlayer) noexcept;

    [[nodiscard]] const STRE::Campaign::CampaignAdmissionRecord* GetAdmission(const Player& acPlayer) const noexcept;
    void OnPlayerLocationChanged(const Player& acPlayer) noexcept;
    [[nodiscard]] bool BeginCheckpointDevelopment(
        const std::string& acCampaignId) noexcept;
    [[nodiscard]] bool ResendCheckpointDevelopment(
        const std::string& acCampaignId) noexcept;

private:
    using CommandResult = STRE::Campaign::CampaignProtocolCommandResult;

    [[nodiscard]] static STRE::Campaign::CampaignConnectionHandle ToHandle(const Player& acPlayer) noexcept;
    [[nodiscard]] bool SharesCampaignParty(const Player& acPlayer, const STRE::Campaign::CampaignId& acCampaign, bool aRequireCompleteRoster) noexcept;

    void SendResult(Player& aPlayer, const CommandResult& acResult) const noexcept;
    void BroadcastSnapshot(
        const CampaignSnapshotData& acSnapshot) const noexcept;
    void BroadcastLobbyState(
        const STRE::Campaign::CampaignId& acCampaign) noexcept;
    void Finish(
        Player& aPlayer,
        CommandResult aResult,
        std::string_view acMutationId = {}) noexcept;

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
    void OnJoinByCode(
        const PacketEvent<CampaignJoinByCodeRequest>& acPacket) noexcept;
    void OnHelgenInvestigationReady(const PacketEvent<CampaignHelgenInvestigationReadyRequest>& acPacket) noexcept;
    void OnCheckpointSaveResult(
        const PacketEvent<CampaignCheckpointSaveResult>& acPacket) noexcept;
    void OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept;

    [[nodiscard]] bool SendCheckpointRequest(
        const STRE::Campaign::CampaignCheckpointActivity& acActivity) noexcept;

    void BroadcastHelgenState(const STRE::Campaign::CampaignId& acCampaign, Player* apOnlyPlayer = nullptr) noexcept;

    World& m_world;
    STRE::Campaign::CampaignAdmissionService m_admission;
    STRE::Campaign::CampaignLobbyDirectory m_lobbies;

    struct PendingResumeAlignment
    {
        std::uint32_t PartyId{};
        bool Added{};
        std::string DisplayName;
    };
    std::unordered_map<
        STRE::Campaign::CampaignConnectionHandle,
        PendingResumeAlignment> m_pendingResumeAlignments;
    std::unordered_map<std::string, std::unordered_set<STRE::Campaign::CampaignConnectionHandle>> m_helgenReadyConnections;
    std::unordered_set<std::string> m_helgenStartedCampaigns;

    entt::scoped_connection m_createConnection;
    entt::scoped_connection m_joinConnection;
    entt::scoped_connection m_resumeConnection;
    entt::scoped_connection m_startConnection;
    entt::scoped_connection m_readyConnection;
    entt::scoped_connection m_leaveConnection;
    entt::scoped_connection m_joinByCodeConnection;
    entt::scoped_connection m_helgenReadyConnection;
    entt::scoped_connection m_checkpointResultConnection;
    entt::scoped_connection m_playerLeaveConnection;
};
