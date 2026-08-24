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

bool BuildCampaignNativeSaveIdentity(
    const TiltedPhoques::String& acCheckpointId,
    TiltedPhoques::String& aNativeSaveIdentity) noexcept
{
    aNativeSaveIdentity.clear();
    if (!IsValidCampaignWireId(acCheckpointId))
        return false;

    aNativeSaveIdentity.assign(
        kCampaignNativeSaveIdentityPrefix.data(),
        kCampaignNativeSaveIdentityPrefix.size());
    aNativeSaveIdentity.append(acCheckpointId);
    return true;
}

bool IsValidCampaignNativeSaveIdentity(
    const TiltedPhoques::String& acValue) noexcept
{
    if (acValue.size() <= kCampaignNativeSaveIdentityPrefix.size() ||
        !std::equal(
            kCampaignNativeSaveIdentityPrefix.begin(),
            kCampaignNativeSaveIdentityPrefix.end(),
            acValue.begin()))
    {
        return false;
    }

    TiltedPhoques::String checkpointId;
    checkpointId.assign(
        acValue.data() + kCampaignNativeSaveIdentityPrefix.size(),
        acValue.size() - kCampaignNativeSaveIdentityPrefix.size());
    return IsValidCampaignWireId(checkpointId);
}

bool NormalizeCampaignJoinCode(
    std::string_view acValue,
    TiltedPhoques::String& aNormalized) noexcept
{
    aNormalized.clear();
    if (acValue.size() != kCampaignJoinCodeLength)
        return false;

    for (char value : acValue)
    {
        if (value >= 'a' && value <= 'z')
            value = static_cast<char>(value - ('a' - 'A'));
        if (kCampaignJoinCodeAlphabet.find(value) == std::string_view::npos)
        {
            aNormalized.clear();
            return false;
        }
        aNormalized.push_back(value);
    }
    return true;
}

bool IsValidCampaignJoinCode(
    const TiltedPhoques::String& acValue) noexcept
{
    TiltedPhoques::String normalized;
    return NormalizeCampaignJoinCode(acValue.c_str(), normalized);
}

namespace
{
struct Utf8CodePoint
{
    std::size_t Begin{};
    std::size_t End{};
    std::uint32_t Value{};
};

bool IsContinuationByte(unsigned char aValue) noexcept
{
    return (aValue & 0xC0) == 0x80;
}

bool DecodeUtf8CodePoint(
    std::string_view acValue,
    std::size_t& aOffset,
    std::uint32_t& aCodePoint) noexcept
{
    const auto first = static_cast<unsigned char>(acValue[aOffset]);
    if (first <= 0x7F)
    {
        aCodePoint = first;
        ++aOffset;
        return true;
    }

    std::size_t length{};
    std::uint32_t value{};
    if (first >= 0xC2 && first <= 0xDF)
    {
        length = 2;
        value = first & 0x1F;
    }
    else if (first >= 0xE0 && first <= 0xEF)
    {
        length = 3;
        value = first & 0x0F;
    }
    else if (first >= 0xF0 && first <= 0xF4)
    {
        length = 4;
        value = first & 0x07;
    }
    else
    {
        return false;
    }

    if (aOffset + length > acValue.size())
        return false;
    for (std::size_t index = 1; index < length; ++index)
    {
        const auto next = static_cast<unsigned char>(acValue[aOffset + index]);
        if (!IsContinuationByte(next))
            return false;
        value = (value << 6) | (next & 0x3F);
    }

    if ((length == 3 && value < 0x800) ||
        (length == 4 && value < 0x10000) ||
        value > 0x10FFFF ||
        (value >= 0xD800 && value <= 0xDFFF))
    {
        return false;
    }

    aOffset += length;
    aCodePoint = value;
    return true;
}

bool IsUnicodeWhitespace(std::uint32_t aCodePoint) noexcept
{
    return aCodePoint == 0x20 || aCodePoint == 0xA0 ||
        aCodePoint == 0x1680 ||
        (aCodePoint >= 0x2000 && aCodePoint <= 0x200A) ||
        aCodePoint == 0x2028 || aCodePoint == 0x2029 ||
        aCodePoint == 0x202F || aCodePoint == 0x205F ||
        aCodePoint == 0x3000 || aCodePoint == 0xFEFF;
}

bool IsControlCodePoint(std::uint32_t aCodePoint) noexcept
{
    return aCodePoint <= 0x1F ||
        (aCodePoint >= 0x7F && aCodePoint <= 0x9F);
}
}

bool NormalizeCampaignLobbyDisplayName(
    std::string_view acValue,
    TiltedPhoques::String& aNormalized) noexcept
{
    aNormalized.clear();
    if (acValue.empty() ||
        acValue.size() > kCampaignLobbyMaximumDisplayNameBytes)
    {
        return false;
    }

    std::array<Utf8CodePoint, kCampaignLobbyMaximumDisplayNameBytes> points{};
    std::size_t pointCount{};
    std::size_t offset{};
    while (offset < acValue.size())
    {
        const std::size_t begin = offset;
        std::uint32_t codePoint{};
        if (!DecodeUtf8CodePoint(acValue, offset, codePoint) ||
            IsControlCodePoint(codePoint))
        {
            return false;
        }
        points[pointCount++] = {begin, offset, codePoint};
    }

    std::size_t first{};
    while (first < pointCount && IsUnicodeWhitespace(points[first].Value))
        ++first;
    std::size_t last = pointCount;
    while (last > first && IsUnicodeWhitespace(points[last - 1].Value))
        --last;
    if (first == last ||
        last - first > kCampaignLobbyMaximumDisplayNameLength)
    {
        return false;
    }

    aNormalized.assign(
        acValue.data() + points[first].Begin,
        points[last - 1].End - points[first].Begin);
    return true;
}

bool IsValidCampaignLobbyDisplayName(
    const TiltedPhoques::String& acValue) noexcept
{
    TiltedPhoques::String normalized;
    return NormalizeCampaignLobbyDisplayName(
        std::string_view(acValue.data(), acValue.size()), normalized);
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
