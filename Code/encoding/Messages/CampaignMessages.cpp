#include <Messages/CampaignMessages.h>

#include <TiltedCore/Serialization.hpp>

using TiltedPhoques::Serialization;

void CampaignCommandResponse::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter,
        static_cast<std::uint8_t>(Operation));
    Serialization::WriteVarInt(aWriter,
        static_cast<std::uint8_t>(Result));
    (void)WriteCampaignWireId(aWriter, MutationId);
    (void)WriteCampaignWireId(aWriter, CampaignId);
    Serialization::WriteVarInt(aWriter, StateVersion);
    (void)WriteCampaignWireId(aWriter, CampaignSlotId);
    (void)WriteCampaignWireId(aWriter, CharacterBindingId);
}

void CampaignCommandResponse::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    Operation = static_cast<CampaignProtocolOperation>(
        static_cast<std::uint8_t>(Serialization::ReadVarInt(aReader)));
    Result = static_cast<CampaignProtocolResult>(
        static_cast<std::uint8_t>(Serialization::ReadVarInt(aReader)));
    WireValid = ReadCampaignWireId(aReader, MutationId);
    WireValid = ReadCampaignWireId(aReader, CampaignId) && WireValid;
    StateVersion = Serialization::ReadVarInt(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignSlotId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, CharacterBindingId) && WireValid;
}

bool CampaignCommandResponse::IsValid() const noexcept
{
    const bool assignmentEmpty = CampaignSlotId.empty() &&
        CharacterBindingId.empty();
    const bool assignmentValid = IsValidCampaignWireId(CampaignSlotId) &&
        IsValidCampaignWireId(CharacterBindingId);
    const bool succeeded = Result == CampaignProtocolResult::Applied ||
        Result == CampaignProtocolResult::AcceptedNoOp ||
        Result == CampaignProtocolResult::IdempotentReplay;
    return WireValid && static_cast<std::uint8_t>(Operation) <=
            static_cast<std::uint8_t>(CampaignProtocolOperation::Leave) &&
        static_cast<std::uint8_t>(Result) <=
            static_cast<std::uint8_t>(CampaignProtocolResult::PersistenceFailure) &&
        IsValidCampaignWireId(MutationId, true) &&
        IsValidCampaignWireId(CampaignId, !succeeded) &&
        (assignmentEmpty || assignmentValid);
}

void NotifyCampaignSnapshot::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Snapshot.Serialize(aWriter);
}

void NotifyCampaignSnapshot::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    Snapshot.Deserialize(aReader);
}
