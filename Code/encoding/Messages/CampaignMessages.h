#pragma once

#include "Message.h"

#include <Structs/Campaign.h>

struct CampaignCommandResponse final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kCampaignCommandResponse;
    CampaignCommandResponse()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;

    CampaignProtocolOperation Operation{CampaignProtocolOperation::Create};
    CampaignProtocolResult Result{CampaignProtocolResult::InvalidRequest};
    TiltedPhoques::String MutationId;
    TiltedPhoques::String CampaignId;
    std::uint64_t StateVersion{};
    TiltedPhoques::String CampaignSlotId;
    TiltedPhoques::String CharacterBindingId;
    TiltedPhoques::String JoinCode;
    bool WireValid{true};
};

struct NotifyCampaignSnapshot final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyCampaignSnapshot;
    NotifyCampaignSnapshot()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept { return Snapshot.IsValid(); }

    CampaignSnapshotData Snapshot;
};

struct CampaignLobbyMemberData
{
    TiltedPhoques::String Name;
    bool Present{};
};

struct NotifyCampaignLobbyState final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyCampaignLobbyState;
    NotifyCampaignLobbyState() : ServerMessage(Opcode) {}

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;

    TiltedPhoques::String JoinCode;
    TiltedPhoques::String CampaignId;
    std::uint64_t StateVersion{};
    TiltedPhoques::Vector<CampaignLobbyMemberData> Members;
    bool CanStart{};
    bool WireValid{true};
};

enum class CampaignHelgenSpatialStatus : std::uint8_t
{
    Known = 0,
    GateClosed,
    EmptyFootprint,
    IncompleteRoster,
    UnknownPosition
};

struct NotifyCampaignHelgenState final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyCampaignHelgenState;
    NotifyCampaignHelgenState()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;

    bool InvestigationStartAuthorized{};
    CampaignHelgenSpatialStatus SpatialStatus{CampaignHelgenSpatialStatus::GateClosed};
    bool AllRequiredPlayersOutside{};
};
