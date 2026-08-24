#include <Messages/CampaignRequests.h>

#include <TiltedCore/Serialization.hpp>

#include <Structs/NativeSaveBundle.h>

#include <array>
#include <limits>

using TiltedPhoques::Serialization;

namespace
{
void WriteMutationRequest(
    TiltedPhoques::Buffer::Writer& aWriter, const TiltedPhoques::String& acCampaignId, const TiltedPhoques::String& acMutationId, std::uint64_t aExpectedRevision) noexcept
{
    (void)WriteCampaignWireId(aWriter, acCampaignId);
    (void)WriteCampaignWireId(aWriter, acMutationId);
    Serialization::WriteVarInt(aWriter, aExpectedRevision);
}

void ReadMutationRequest(TiltedPhoques::Buffer::Reader& aReader, TiltedPhoques::String& aCampaignId, TiltedPhoques::String& aMutationId, std::uint64_t& aExpectedRevision) noexcept
{
    if (!ReadCampaignWireId(aReader, aCampaignId))
        aCampaignId.clear();
    if (!ReadCampaignWireId(aReader, aMutationId))
        aMutationId.clear();
    aExpectedRevision = Serialization::ReadVarInt(aReader);
}

bool IsValidMutationRequest(const TiltedPhoques::String& acCampaignId, const TiltedPhoques::String& acMutationId) noexcept
{
    return IsValidCampaignWireId(acCampaignId) && IsValidCampaignWireId(acMutationId);
}

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
    aValue.assign(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(size));
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
} // namespace

void CampaignCreateRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, MutationId);
    (void)WriteCampaignWireId(aWriter, DisplayName);
}

void CampaignCreateRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    if (!ReadCampaignWireId(aReader, MutationId))
        MutationId.clear();
    if (!ReadCampaignWireId(aReader, DisplayName))
        DisplayName.clear();
}

bool CampaignCreateRequest::IsValid() const noexcept
{
    return IsValidCampaignWireId(MutationId) &&
        IsValidCampaignLobbyDisplayName(DisplayName);
}

void CampaignJoinRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    WriteMutationRequest(aWriter, CampaignId, MutationId, ExpectedRevision);
}

void CampaignJoinRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    ReadMutationRequest(aReader, CampaignId, MutationId, ExpectedRevision);
}

bool CampaignJoinRequest::IsValid() const noexcept
{
    return IsValidMutationRequest(CampaignId, MutationId);
}

void CampaignResumeRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, CampaignId);
    (void)WriteCampaignWireId(aWriter, CharacterBindingId);
}

void CampaignResumeRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    if (!ReadCampaignWireId(aReader, CampaignId))
        CampaignId.clear();
    if (!ReadCampaignWireId(aReader, CharacterBindingId))
        CharacterBindingId.clear();
}

bool CampaignResumeRequest::IsValid() const noexcept
{
    return IsValidCampaignWireId(CampaignId) && IsValidCampaignWireId(CharacterBindingId);
}

void CampaignStartRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    WriteMutationRequest(aWriter, CampaignId, MutationId, ExpectedRevision);
}

void CampaignStartRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    ReadMutationRequest(aReader, CampaignId, MutationId, ExpectedRevision);
}

bool CampaignStartRequest::IsValid() const noexcept
{
    return IsValidMutationRequest(CampaignId, MutationId);
}

void CampaignSetReadyRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    WriteMutationRequest(aWriter, CampaignId, MutationId, ExpectedRevision);
    Serialization::WriteBool(aWriter, Ready);
}

void CampaignSetReadyRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    ReadMutationRequest(aReader, CampaignId, MutationId, ExpectedRevision);
    Ready = Serialization::ReadBool(aReader);
}

bool CampaignSetReadyRequest::IsValid() const noexcept
{
    return IsValidMutationRequest(CampaignId, MutationId);
}

void CampaignLeaveRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    WriteMutationRequest(aWriter, CampaignId, MutationId, ExpectedRevision);
}

void CampaignLeaveRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    ReadMutationRequest(aReader, CampaignId, MutationId, ExpectedRevision);
}

bool CampaignLeaveRequest::IsValid() const noexcept
{
    return IsValidMutationRequest(CampaignId, MutationId);
}

void CampaignJoinByCodeRequest::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, JoinCode);
    (void)WriteCampaignWireId(aWriter, MutationId);
    (void)WriteCampaignWireId(aWriter, DisplayName);
}

void CampaignJoinByCodeRequest::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    if (!ReadCampaignWireId(aReader, JoinCode))
        JoinCode.clear();
    if (!ReadCampaignWireId(aReader, MutationId))
        MutationId.clear();
    if (!ReadCampaignWireId(aReader, DisplayName))
        DisplayName.clear();
}

bool CampaignJoinByCodeRequest::IsValid() const noexcept
{
    return IsValidCampaignJoinCode(JoinCode) &&
        IsValidCampaignWireId(MutationId) &&
        IsValidCampaignLobbyDisplayName(DisplayName);
}

void CampaignHelgenInvestigationReadyRequest::SerializeRaw(TiltedPhoques::Buffer::Writer&) const noexcept
{
}

void CampaignHelgenInvestigationReadyRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
}

void CampaignCheckpointSaveResult::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, CampaignId);
    (void)WriteCampaignWireId(aWriter, CheckpointId);
    (void)WriteCampaignWireId(aWriter, NativeSaveIdentity);
    Serialization::WriteVarInt(aWriter, static_cast<std::uint8_t>(Result));
    (void)WriteCampaignWireId(aWriter, FingerprintAlgorithm);
    Serialization::WriteVarInt(aWriter, FingerprintVersion);
    (void)WriteBoundedBytes<STRE::Campaign::kNativeSaveSha256Size>(
        aWriter, Fingerprint);
    Serialization::WriteVarInt(aWriter, SaveMetadataCodecVersion);
    (void)WriteBoundedBytes<STRE::Campaign::kMaximumNativeSaveMetadataSize>(
        aWriter, SaveMetadata);
}

void CampaignCheckpointSaveResult::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    WireValid = ReadCampaignWireId(aReader, CampaignId);
    WireValid = ReadCampaignWireId(aReader, CheckpointId) && WireValid;
    WireValid = ReadCampaignWireId(aReader, NativeSaveIdentity) && WireValid;
    const std::uint64_t rawResult = Serialization::ReadVarInt(aReader);
    if (rawResult > static_cast<std::uint8_t>(
            CampaignCheckpointSaveResultCode::Failure))
    {
        WireValid = false;
    }
    Result = static_cast<CampaignCheckpointSaveResultCode>(
        static_cast<std::uint8_t>(rawResult));
    WireValid = ReadCampaignWireId(aReader, FingerprintAlgorithm) && WireValid;
    const std::uint64_t rawFingerprintVersion =
        Serialization::ReadVarInt(aReader);
    if (rawFingerprintVersion >
        std::numeric_limits<std::uint32_t>::max())
    {
        WireValid = false;
    }
    FingerprintVersion = static_cast<std::uint32_t>(rawFingerprintVersion);
    WireValid = ReadBoundedBytes<STRE::Campaign::kNativeSaveSha256Size>(
        aReader, Fingerprint) && WireValid;
    const std::uint64_t rawMetadataCodecVersion =
        Serialization::ReadVarInt(aReader);
    if (rawMetadataCodecVersion >
        std::numeric_limits<std::uint32_t>::max())
    {
        WireValid = false;
    }
    SaveMetadataCodecVersion =
        static_cast<std::uint32_t>(rawMetadataCodecVersion);
    WireValid = ReadBoundedBytes<STRE::Campaign::kMaximumNativeSaveMetadataSize>(
        aReader, SaveMetadata) && WireValid;
}

bool CampaignCheckpointSaveResult::IsValid() const noexcept
{
    if (!WireValid || !IsValidCampaignWireId(CampaignId) ||
        !IsValidCampaignWireId(CheckpointId) ||
        !IsValidCampaignNativeSaveIdentity(NativeSaveIdentity) ||
        static_cast<std::uint8_t>(Result) >
            static_cast<std::uint8_t>(CampaignCheckpointSaveResultCode::Failure))
    {
        return false;
    }
    TiltedPhoques::String expectedIdentity;
    if (!BuildCampaignNativeSaveIdentity(CheckpointId, expectedIdentity) ||
        expectedIdentity != NativeSaveIdentity)
    {
        return false;
    }
    if (Result == CampaignCheckpointSaveResultCode::Failure)
    {
        return FingerprintAlgorithm.empty() && FingerprintVersion == 0 &&
            Fingerprint.empty() && SaveMetadataCodecVersion == 0 &&
            SaveMetadata.empty();
    }
    return FingerprintAlgorithm ==
            STRE::Campaign::kNativeSaveFingerprintAlgorithm &&
        FingerprintVersion == STRE::Campaign::kNativeSaveFingerprintVersion &&
        Fingerprint.size() == STRE::Campaign::kNativeSaveSha256Size &&
        SaveMetadataCodecVersion ==
            STRE::Campaign::kNativeSaveMetadataCodecVersion &&
        !SaveMetadata.empty() &&
        SaveMetadata.size() <=
            STRE::Campaign::kMaximumNativeSaveMetadataSize;
}
