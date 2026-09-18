#include <Messages/CharacterAppearanceUpdate.h>
#include <StringCache.h>

#include <bit>
#include <cmath>

namespace
{
// TiltedCore varints use seven data bits followed by one continuation bit.
// Check every read, overflow and termination before trusting a length or ID.
bool ReadUint32(TiltedPhoques::Buffer::Reader& aReader, uint32_t& aValue) noexcept
{
    aValue = 0;
    for (unsigned shift = 0; shift <= 28; shift += 7)
    {
        uint64_t data{}, more{};
        if (!aReader.ReadBits(data, 7) || !aReader.ReadBits(more, 1) || (shift == 28 && (data > 15 || more)))
            return false;
        aValue |= static_cast<uint32_t>(data) << shift;
        if (!more)
            return true;
    }
    return false;
}

bool ReadString(TiltedPhoques::Buffer::Reader& aReader, String& aValue, size_t aMaximum) noexcept
{
    uint32_t size{};
    if (!ReadUint32(aReader, size) || size > aMaximum)
        return false;
    aValue.resize(size);
    return size == 0 || aReader.ReadBytes(reinterpret_cast<uint8_t*>(aValue.data()), size);
}

bool ReadTints(TiltedPhoques::Buffer::Reader& aReader, Tints& aTints) noexcept
{
    uint64_t count{};
    if (!aReader.ReadBits(count, 8))
        return false;
    for (uint64_t i = 0; i < count; ++i)
    {
        Tints::Entry entry{};
        uint64_t color{}, cached{}, alpha{};
        if (!ReadUint32(aReader, entry.Type) || !aReader.ReadBits(color, 32) || !aReader.ReadBits(cached, 1))
            return false;
        entry.Color = static_cast<uint32_t>(color);
        if (cached)
        {
            uint32_t id{};
            if (!ReadUint32(aReader, id))
                return false;
            const auto name = StringCache::Get()[id];
            if (!name || name->size() > CharacterAppearanceUpdate::MaxTextureNameBytes)
                return false;
            entry.Name = *name;
        }
        else if (!ReadString(aReader, entry.Name, CharacterAppearanceUpdate::MaxTextureNameBytes))
            return false;
        if (!aReader.ReadBits(alpha, 32))
            return false;
        entry.Alpha = std::bit_cast<float>(static_cast<uint32_t>(alpha));
        aTints.Entries.push_back(std::move(entry));
    }
    return true;
}
} // namespace

bool CharacterAppearanceDescriptor::IsValid() const noexcept
{
    return Race.BaseId != 0 && Race.ModId != UINT32_MAX && Sex <= 1 && std::isfinite(Weight) && Weight >= 0.f && Weight <= 100.f;
}

bool CharacterAppearanceUpdate::IsValid() const noexcept
{
    if (!m_decodeValid || !Descriptor.IsValid() || AppearanceBuffer.empty() || AppearanceBuffer.size() > MaxAppearanceBytes || FaceTints.Entries.size() > MaxTintCount)
        return false;
    // Bound independently of StringCache: include packet/opcode bytes, maximum
    // varints and byte-rounded tint fields, even when names later leave cache.
    size_t wireBytes = 39 + AppearanceBuffer.size();
    for (const auto& entry : FaceTints.Entries)
    {
        if (entry.Name.size() > MaxTextureNameBytes || entry.Name.find('\0') != String::npos || !std::isfinite(entry.Alpha) || entry.Alpha < 0.f || entry.Alpha > 1.f)
            return false;
        wireBytes += 19 + entry.Name.size();
    }
    return wireBytes <= MaxWireBytes;
}

bool CharacterAppearanceUpdate::operator==(const CharacterAppearanceUpdate& acRhs) const noexcept
{
    return ActorId == acRhs.ActorId && FinalBuildRevision == acRhs.FinalBuildRevision && ChangeFlags == acRhs.ChangeFlags && AppearanceBuffer == acRhs.AppearanceBuffer && FaceTints == acRhs.FaceTints &&
           Descriptor == acRhs.Descriptor && m_decodeValid == acRhs.m_decodeValid;
}

void CharacterAppearanceUpdate::SerializeAppearance(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, ActorId);
    aWriter.WriteBits(ChangeFlags, 32);
    // Invalid local payloads encode an explicitly rejected length, never a
    // truncated tint count or string that could become a different snapshot.
    if (!IsValid())
    {
        Serialization::WriteVarInt(aWriter, MaxAppearanceBytes + 1);
        return;
    }
    Serialization::WriteString(aWriter, AppearanceBuffer);
    FaceTints.Serialize(aWriter);
    aWriter.WriteBits(2, 8); // Final Character Build revision; matching client/server builds required.
    Descriptor.Race.Serialize(aWriter);
    aWriter.WriteBits(Descriptor.Sex, 8);
    aWriter.WriteBits(std::bit_cast<uint32_t>(Descriptor.Weight), 32);
    aWriter.WriteBits(FinalBuildRevision, 64);
}

void CharacterAppearanceUpdate::DeserializeAppearance(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    CharacterAppearanceUpdate decoded;
    uint64_t flags{}, schema{}, sex{}, weight{};
    const bool valid = ReadUint32(aReader, decoded.ActorId) && aReader.ReadBits(flags, 32) && ReadString(aReader, decoded.AppearanceBuffer, MaxAppearanceBytes) &&
                       ReadTints(aReader, decoded.FaceTints) && aReader.ReadBits(schema, 8) && schema == 2 && ReadUint32(aReader, decoded.Descriptor.Race.BaseId) &&
                       ReadUint32(aReader, decoded.Descriptor.Race.ModId) && aReader.ReadBits(sex, 8) && aReader.ReadBits(weight, 32) &&
                       aReader.ReadBits(decoded.FinalBuildRevision, 64);
    decoded.Descriptor.Sex = static_cast<uint8_t>(sex);
    decoded.Descriptor.Weight = std::bit_cast<float>(static_cast<uint32_t>(weight));
    decoded.ChangeFlags = static_cast<uint32_t>(flags);
    decoded.m_decodeValid = valid;
    if (!decoded.IsValid())
    {
        *this = CharacterAppearanceUpdate{};
        m_decodeValid = false;
        return;
    }
    *this = std::move(decoded);
    for (const auto& entry : FaceTints.Entries)
        StringCache::Get().AddWanted(entry.Name);
}
