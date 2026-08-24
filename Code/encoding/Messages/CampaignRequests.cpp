#include <Messages/CampaignRequests.h>

#include <TiltedCore/Serialization.hpp>

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
