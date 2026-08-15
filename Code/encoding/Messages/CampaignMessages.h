#pragma once

#include "Message.h"

#include <Structs/Campaign.h>

struct CampaignCommandResponse final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kCampaignCommandResponse;
    CampaignCommandResponse() : ServerMessage(Opcode) {}

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
    bool WireValid{true};
};

struct NotifyCampaignSnapshot final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyCampaignSnapshot;
    NotifyCampaignSnapshot() : ServerMessage(Opcode) {}

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept { return Snapshot.IsValid(); }

    CampaignSnapshotData Snapshot;
};
