#include <Messages/CampaignMessages.h>

#include <TiltedCore/Serialization.hpp>

#include <algorithm>

using TiltedPhoques::Serialization;

void CampaignCommandResponse::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, static_cast<std::uint8_t>(Operation));
    Serialization::WriteVarInt(aWriter, static_cast<std::uint8_t>(Result));
    (void)WriteCampaignWireId(aWriter, MutationId);
    (void)WriteCampaignWireId(aWriter, CampaignId);
    Serialization::WriteVarInt(aWriter, StateVersion);
    (void)WriteCampaignWireId(aWriter, CampaignSlotId);
    (void)WriteCampaignWireId(aWriter, CharacterBindingId);
    (void)WriteCampaignWireId(aWriter, JoinCode);
}

void CampaignCommandResponse::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    Operation = static_cast<CampaignProtocolOperation>(static_cast<std::uint8_t>(Serialization::ReadVarInt(aReader)));
    Result = static_cast<CampaignProtocolResult>(static_cast<std::uint8_t>(Serialization::ReadVarInt(aReader)));
    WireValid = ReadCampaignWireId(aReader, MutationId);
    WireValid = ReadCampaignWireId(aReader, CampaignId) && WireValid;
    StateVersion = Serialization::ReadVarInt(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignSlotId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, CharacterBindingId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, JoinCode) && WireValid;
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
            static_cast<std::uint8_t>(CampaignProtocolOperation::JoinByCode) &&
        static_cast<std::uint8_t>(Result) <=
            static_cast<std::uint8_t>(
                CampaignProtocolResult::PartyAlignmentFailed) &&
        IsValidCampaignWireId(MutationId, true) &&
        IsValidCampaignWireId(CampaignId, !succeeded) &&
        (assignmentEmpty || assignmentValid) &&
        (JoinCode.empty() || IsValidCampaignJoinCode(JoinCode));
}

void NotifyCampaignSnapshot::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Snapshot.Serialize(aWriter);
}

void NotifyCampaignSnapshot::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    Snapshot.Deserialize(aReader);
}

void NotifyCampaignLobbyState::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, JoinCode);
    (void)WriteCampaignWireId(aWriter, CampaignId);
    Serialization::WriteVarInt(aWriter, StateVersion);
    Serialization::WriteVarInt(aWriter, Members.size());
    for (const CampaignLobbyMemberData& member : Members)
    {
        (void)WriteCampaignWireId(aWriter, member.Name);
        Serialization::WriteBool(aWriter, member.Present);
    }
    Serialization::WriteBool(aWriter, CanStart);
}

void NotifyCampaignLobbyState::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    WireValid = ReadCampaignWireId(aReader, JoinCode);
    WireValid = ReadCampaignWireId(aReader, CampaignId) && WireValid;
    StateVersion = Serialization::ReadVarInt(aReader);
    Members.clear();
    const std::uint64_t count = Serialization::ReadVarInt(aReader);
    if (count > kCampaignWireMaximumRosterSize)
    {
        WireValid = false;
        return;
    }
    Members.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t index = 0; index < count; ++index)
    {
        CampaignLobbyMemberData member;
        WireValid = ReadCampaignWireId(aReader, member.Name) && WireValid;
        member.Present = Serialization::ReadBool(aReader);
        Members.push_back(std::move(member));
    }
    CanStart = Serialization::ReadBool(aReader);
}

bool NotifyCampaignLobbyState::IsValid() const noexcept
{
    if (!WireValid || !IsValidCampaignJoinCode(JoinCode) ||
        !IsValidCampaignWireId(CampaignId) ||
        Members.empty() || Members.size() > kCampaignWireMaximumRosterSize)
    {
        return false;
    }
    return std::all_of(
        Members.begin(), Members.end(),
        [](const CampaignLobbyMemberData& acMember)
        {
            return IsValidCampaignLobbyDisplayName(acMember.Name);
        });
}

void NotifyCampaignHelgenState::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteBool(aWriter, InvestigationStartAuthorized);
    Serialization::WriteVarInt(aWriter, static_cast<std::uint8_t>(SpatialStatus));
    Serialization::WriteBool(aWriter, AllRequiredPlayersOutside);
}

void NotifyCampaignHelgenState::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    InvestigationStartAuthorized = Serialization::ReadBool(aReader);
    SpatialStatus = static_cast<CampaignHelgenSpatialStatus>(static_cast<std::uint8_t>(Serialization::ReadVarInt(aReader)));
    AllRequiredPlayersOutside = Serialization::ReadBool(aReader);
}

bool NotifyCampaignHelgenState::IsValid() const noexcept
{
    return static_cast<std::uint8_t>(SpatialStatus) <= static_cast<std::uint8_t>(CampaignHelgenSpatialStatus::UnknownPosition) &&
           (!AllRequiredPlayersOutside || SpatialStatus == CampaignHelgenSpatialStatus::Known);
}

void CampaignCheckpointSaveRequest::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, CampaignId);
    (void)WriteCampaignWireId(aWriter, CheckpointId);
    Serialization::WriteVarInt(aWriter, SourceRevision);
    (void)WriteCampaignWireId(aWriter, NativeSaveIdentity);
}

void CampaignCheckpointSaveRequest::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignId);
    WireValid = ReadCampaignWireId(aReader, CheckpointId) && WireValid;
    SourceRevision = Serialization::ReadVarInt(aReader);
    WireValid = ReadCampaignWireId(aReader, NativeSaveIdentity) && WireValid;
}

bool CampaignCheckpointSaveRequest::IsValid() const noexcept
{
    if (!WireValid || SourceRevision == 0 ||
        !IsValidCampaignWireId(CampaignId) ||
        !IsValidCampaignWireId(CheckpointId) ||
        !IsValidCampaignNativeSaveIdentity(NativeSaveIdentity))
    {
        return false;
    }
    TiltedPhoques::String expectedIdentity;
    return BuildCampaignNativeSaveIdentity(CheckpointId, expectedIdentity) &&
        expectedIdentity == NativeSaveIdentity;
}
