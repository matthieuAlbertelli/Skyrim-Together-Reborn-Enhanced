#include <Messages/NotifyCharacterBuildState.h>

#include <TiltedCore/Serialization.hpp>

void NotifyCharacterBuildState::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(
        aWriter,
        static_cast<std::uint8_t>(State));
    Serialization::WriteVarInt(aWriter, PlayerId);
    Serialization::WriteVarInt(aWriter, ServerId);
    Serialization::WriteVarInt(aWriter, Revision);
    Build.Serialize(aWriter);
    aWriter.WriteBits(1, 8);
    for (const auto* value : {&SeatingCampaignId, &SeatingPlayerId})
    {
        const auto size = value->size() <= MaxSeatingIdentityBytes ? value->size() : 0;
        aWriter.WriteBits(size, 8);
        if (size)
            aWriter.WriteBytes(reinterpret_cast<const uint8_t*>(value->data()), size);
    }
}

void NotifyCharacterBuildState::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    State = static_cast<CharacterBuildNetworkState>(
        static_cast<std::uint8_t>(
            Serialization::ReadVarInt(aReader)));
    PlayerId = static_cast<std::uint32_t>(
        Serialization::ReadVarInt(aReader));
    ServerId = static_cast<std::uint32_t>(
        Serialization::ReadVarInt(aReader));
    Revision = Serialization::ReadVarInt(aReader);
    Build.Deserialize(aReader);
    SeatingCampaignId.clear();
    SeatingPlayerId.clear();
    uint64_t version{}, size{};
    String campaign, player;
    if (!aReader.ReadBits(version, 8) || version != 1)
        return;
    for (auto* value : {&campaign, &player})
    {
        if (!aReader.ReadBits(size, 8) || size > MaxSeatingIdentityBytes)
            return;
        value->resize(size);
        if (size && !aReader.ReadBytes(reinterpret_cast<uint8_t*>(value->data()), size))
            return;
    }
    SeatingCampaignId = std::move(campaign);
    SeatingPlayerId = std::move(player);
}
