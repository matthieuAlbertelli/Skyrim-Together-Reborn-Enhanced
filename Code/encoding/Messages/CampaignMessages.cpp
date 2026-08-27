#include <Messages/CampaignMessages.h>

#include <TiltedCore/Serialization.hpp>

#include <Structs/NativeSaveBundle.h>

#include <algorithm>
#include <array>
#include <limits>

using TiltedPhoques::Serialization;

namespace
{
template <std::size_t Maximum>
bool ReadBoundedBytes(
    TiltedPhoques::Buffer::Reader& aReader,
    TiltedPhoques::Vector<std::uint8_t>& aValue) noexcept
{
    aValue.clear();
    const std::uint64_t size = Serialization::ReadVarInt(aReader);
    if (size > Maximum)
        return false;
    std::array<std::uint8_t, Maximum> buffer{};
    if (size != 0 &&
        !aReader.ReadBytes(buffer.data(), static_cast<std::size_t>(size)))
    {
        return false;
    }
    aValue.assign(
        buffer.begin(),
        buffer.begin() + static_cast<std::ptrdiff_t>(size));
    return true;
}

template <std::size_t Maximum>
bool WriteBoundedBytes(
    TiltedPhoques::Buffer::Writer& aWriter,
    const TiltedPhoques::Vector<std::uint8_t>& acValue) noexcept
{
    if (acValue.size() > Maximum)
    {
        Serialization::WriteVarInt(aWriter, Maximum + 1);
        return false;
    }
    Serialization::WriteVarInt(aWriter, acValue.size());
    return acValue.empty() ||
        aWriter.WriteBytes(acValue.data(), acValue.size());
}

bool IsExactRecoveryArtifact(
    const TiltedPhoques::String& acIdentity,
    const TiltedPhoques::String& acAlgorithm,
    std::uint32_t aFingerprintVersion,
    const TiltedPhoques::Vector<std::uint8_t>& acFingerprint,
    std::uint32_t aMetadataCodecVersion,
    const TiltedPhoques::Vector<std::uint8_t>& acMetadata) noexcept
{
    if (acAlgorithm != STRE::Campaign::kNativeSaveFingerprintAlgorithm ||
        aFingerprintVersion !=
            STRE::Campaign::kNativeSaveFingerprintVersion ||
        acFingerprint.size() != STRE::Campaign::kNativeSaveSha256Size ||
        aMetadataCodecVersion !=
            STRE::Campaign::kNativeSaveMetadataCodecVersion ||
        acMetadata.empty() ||
        acMetadata.size() > STRE::Campaign::kMaximumNativeSaveMetadataSize)
    {
        return false;
    }
    return STRE::Campaign::ParseNativeSaveBundleArtifact(
        acIdentity.c_str(),
        std::span<const std::uint8_t>(
            acFingerprint.data(), acFingerprint.size()),
        std::span<const std::uint8_t>(
            acMetadata.data(), acMetadata.size()))
        .Succeeded();
}
}

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

void NotifyCampaignCheckpointState::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, CampaignId);
    (void)WriteCampaignWireId(aWriter, CheckpointId);
    Serialization::WriteVarInt(
        aWriter, static_cast<std::uint8_t>(State));
}

void NotifyCampaignCheckpointState::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignId);
    WireValid = ReadCampaignWireId(aReader, CheckpointId) && WireValid;
    const std::uint64_t rawState = Serialization::ReadVarInt(aReader);
    WireValid = rawState <= static_cast<std::uint8_t>(
        CampaignCheckpointPublicState::Failed) && WireValid;
    State = static_cast<CampaignCheckpointPublicState>(
        static_cast<std::uint8_t>(rawState));
}

bool NotifyCampaignCheckpointState::IsValid() const noexcept
{
    const bool failed = State == CampaignCheckpointPublicState::Failed;
    return WireValid && IsValidCampaignWireId(CampaignId) &&
        IsValidCampaignWireId(CheckpointId, failed) &&
        static_cast<std::uint8_t>(State) <=
            static_cast<std::uint8_t>(
                CampaignCheckpointPublicState::Failed);
}

void CampaignRecoveryLoadRequest::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, CampaignId);
    (void)WriteCampaignWireId(aWriter, RestoreAttemptId);
    (void)WriteCampaignWireId(aWriter, CheckpointId);
    Serialization::WriteVarInt(aWriter, SourceRevision);
    (void)WriteCampaignWireId(aWriter, CampaignSlotId);
    (void)WriteCampaignWireId(aWriter, CharacterBindingId);
    (void)WriteCampaignWireId(aWriter, NativeSaveIdentity);
    (void)WriteCampaignWireId(aWriter, FingerprintAlgorithm);
    Serialization::WriteVarInt(aWriter, FingerprintVersion);
    (void)WriteBoundedBytes<STRE::Campaign::kNativeSaveSha256Size>(
        aWriter, Fingerprint);
    Serialization::WriteVarInt(aWriter, SaveMetadataCodecVersion);
    (void)WriteBoundedBytes<STRE::Campaign::kMaximumNativeSaveMetadataSize>(
        aWriter, SaveMetadata);
}

void CampaignRecoveryLoadRequest::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignId);
    WireValid = ReadCampaignWireId(aReader, RestoreAttemptId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, CheckpointId) && WireValid;
    SourceRevision = Serialization::ReadVarInt(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignSlotId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, CharacterBindingId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, NativeSaveIdentity) && WireValid;
    WireValid = ReadCampaignWireId(aReader, FingerprintAlgorithm) && WireValid;
    const std::uint64_t rawFingerprintVersion =
        Serialization::ReadVarInt(aReader);
    if (rawFingerprintVersion > std::numeric_limits<std::uint32_t>::max())
        WireValid = false;
    FingerprintVersion = static_cast<std::uint32_t>(rawFingerprintVersion);
    WireValid = ReadBoundedBytes<STRE::Campaign::kNativeSaveSha256Size>(
        aReader, Fingerprint) && WireValid;
    const std::uint64_t rawMetadataCodecVersion =
        Serialization::ReadVarInt(aReader);
    if (rawMetadataCodecVersion > std::numeric_limits<std::uint32_t>::max())
        WireValid = false;
    SaveMetadataCodecVersion =
        static_cast<std::uint32_t>(rawMetadataCodecVersion);
    WireValid =
        ReadBoundedBytes<STRE::Campaign::kMaximumNativeSaveMetadataSize>(
            aReader, SaveMetadata) && WireValid;
}

bool CampaignRecoveryLoadRequest::IsValid() const noexcept
{
    if (!WireValid || SourceRevision == 0 ||
        !IsValidCampaignWireId(CampaignId) ||
        !IsValidCampaignWireId(RestoreAttemptId) ||
        !IsValidCampaignWireId(CheckpointId) ||
        !IsValidCampaignWireId(CampaignSlotId) ||
        !IsValidCampaignWireId(CharacterBindingId) ||
        !IsValidCampaignNativeSaveIdentity(NativeSaveIdentity))
    {
        return false;
    }
    TiltedPhoques::String expectedIdentity;
    return BuildCampaignNativeSaveIdentity(CheckpointId, expectedIdentity) &&
        expectedIdentity == NativeSaveIdentity &&
        IsExactRecoveryArtifact(
            NativeSaveIdentity,
            FingerprintAlgorithm,
            FingerprintVersion,
            Fingerprint,
            SaveMetadataCodecVersion,
            SaveMetadata);
}

void CampaignRecoverySnapshot::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, CampaignId);
    (void)WriteCampaignWireId(aWriter, RestoreAttemptId);
    (void)WriteCampaignWireId(aWriter, CheckpointId);
    Serialization::WriteVarInt(aWriter, RestoreRevision);
    Snapshot.Serialize(aWriter);
}

void CampaignRecoverySnapshot::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignId);
    WireValid = ReadCampaignWireId(aReader, RestoreAttemptId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, CheckpointId) && WireValid;
    RestoreRevision = Serialization::ReadVarInt(aReader);
    Snapshot.Deserialize(aReader);
}

bool CampaignRecoverySnapshot::IsValid() const noexcept
{
    return WireValid && RestoreRevision != 0 &&
        IsValidCampaignWireId(CampaignId) &&
        IsValidCampaignWireId(RestoreAttemptId) &&
        IsValidCampaignWireId(CheckpointId) && Snapshot.IsValid() &&
        Snapshot.CampaignId == CampaignId &&
        Snapshot.StateVersion == RestoreRevision &&
        Snapshot.RuntimeState ==
            kCampaignWireRuntimeRestoringCheckpoint;
}

void CampaignRecoveryComplete::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, CampaignId);
    (void)WriteCampaignWireId(aWriter, RestoreAttemptId);
    (void)WriteCampaignWireId(aWriter, CheckpointId);
    Serialization::WriteVarInt(aWriter, RestoreRevision);
}

void CampaignRecoveryComplete::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignId);
    WireValid = ReadCampaignWireId(aReader, RestoreAttemptId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, CheckpointId) && WireValid;
    RestoreRevision = Serialization::ReadVarInt(aReader);
}

bool CampaignRecoveryComplete::IsValid() const noexcept
{
    return WireValid && RestoreRevision != 0 &&
        IsValidCampaignWireId(CampaignId) &&
        IsValidCampaignWireId(RestoreAttemptId) &&
        IsValidCampaignWireId(CheckpointId);
}
