#include <Structs/Campaign.h>

#include <TiltedCore/Serialization.hpp>

#include <algorithm>
#include <array>

using TiltedPhoques::Serialization;

bool IsValidCampaignWireId(
    const TiltedPhoques::String& acValue,
    bool aAllowEmpty) noexcept
{
    if (acValue.empty())
        return aAllowEmpty;
    if (acValue.size() > kCampaignWireMaximumIdLength)
        return false;
    return std::all_of(
        acValue.begin(), acValue.end(), [](char aValue)
        {
            return (aValue >= 'a' && aValue <= 'z') ||
                (aValue >= 'A' && aValue <= 'Z') ||
                (aValue >= '0' && aValue <= '9') ||
                aValue == '-' || aValue == '_';
        });
}

bool WriteCampaignWireId(
    TiltedPhoques::Buffer::Writer& aWriter,
    const TiltedPhoques::String& acValue) noexcept
{
    if (acValue.size() > kCampaignWireMaximumIdLength)
    {
        Serialization::WriteVarInt(aWriter, 0);
        return false;
    }
    Serialization::WriteVarInt(aWriter, acValue.size());
    return acValue.empty() || aWriter.WriteBytes(
        reinterpret_cast<const std::uint8_t*>(acValue.data()),
        acValue.size());
}

bool ReadCampaignWireId(
    TiltedPhoques::Buffer::Reader& aReader,
    TiltedPhoques::String& aValue) noexcept
{
    aValue.clear();
    const std::uint64_t length = Serialization::ReadVarInt(aReader);
    if (length > kCampaignWireMaximumIdLength)
        return false;
    std::array<std::uint8_t, kCampaignWireMaximumIdLength> buffer{};
    if (length > 0 && !aReader.ReadBytes(
            buffer.data(), static_cast<std::size_t>(length)))
    {
        return false;
    }
    aValue.assign(
        reinterpret_cast<const char*>(buffer.data()),
        static_cast<std::size_t>(length));
    return true;
}

void CampaignPublicSlotData::Serialize(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, SlotId);
    (void)WriteCampaignWireId(aWriter, PlayerId);
    Serialization::WriteBool(aWriter, Ready);
    Serialization::WriteBool(aWriter, Present);
}

void CampaignPublicSlotData::Deserialize(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    const bool slotValid = ReadCampaignWireId(aReader, SlotId);
    const bool playerValid = ReadCampaignWireId(aReader, PlayerId);
    Ready = Serialization::ReadBool(aReader);
    Present = Serialization::ReadBool(aReader);
    if (!slotValid || !playerValid)
        SlotId.clear();
}

bool CampaignPublicSlotData::IsValid() const noexcept
{
    return IsValidCampaignWireId(SlotId) &&
        IsValidCampaignWireId(PlayerId);
}

void CampaignSnapshotData::Serialize(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    (void)WriteCampaignWireId(aWriter, CampaignId);
    Serialization::WriteVarInt(aWriter, StateVersion);
    Serialization::WriteVarInt(aWriter, Phase);
    Serialization::WriteVarInt(aWriter, RuntimeState);
    Serialization::WriteBool(aWriter, RosterSealed);
    (void)WriteCampaignWireId(aWriter, SessionManagerPlayerId);
    Serialization::WriteVarInt(aWriter, Roster.size());
    for (const CampaignPublicSlotData& slot : Roster)
        slot.Serialize(aWriter);
}

void CampaignSnapshotData::Deserialize(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    WireValid = true;
    const bool campaignValid = ReadCampaignWireId(aReader, CampaignId);
    StateVersion = Serialization::ReadVarInt(aReader);
    Phase = static_cast<std::uint8_t>(Serialization::ReadVarInt(aReader));
    RuntimeState = static_cast<std::uint8_t>(
        Serialization::ReadVarInt(aReader));
    RosterSealed = Serialization::ReadBool(aReader);
    const bool managerValid = ReadCampaignWireId(
        aReader, SessionManagerPlayerId);
    Roster.clear();
    const std::uint64_t count = Serialization::ReadVarInt(aReader);
    if (count > kCampaignWireMaximumRosterSize)
    {
        WireValid = false;
        return;
    }
    Roster.reserve(static_cast<std::size_t>(count));
    for (std::uint64_t index = 0; index < count; ++index)
    {
        CampaignPublicSlotData slot;
        slot.Deserialize(aReader);
        Roster.push_back(std::move(slot));
    }
    WireValid = campaignValid && managerValid && IsValid();
}

bool CampaignSnapshotData::IsValid() const noexcept
{
    if (!WireValid || !IsValidCampaignWireId(CampaignId) ||
        !IsValidCampaignWireId(SessionManagerPlayerId, !RosterSealed) ||
        Roster.size() > kCampaignWireMaximumRosterSize || Phase > 8 ||
        RuntimeState > 4)
    {
        return false;
    }
    return std::all_of(
        Roster.begin(), Roster.end(),
        [](const CampaignPublicSlotData& acSlot)
        {
            return acSlot.IsValid();
        });
}
