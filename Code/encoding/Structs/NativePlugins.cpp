#include <Structs/NativePlugins.h>
#include <TiltedCore/Serialization.hpp>

#include <algorithm>

using TiltedPhoques::Serialization;

void NativePlugins::Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    const std::uint16_t count = static_cast<std::uint16_t>(std::min(PluginList.size(), size_t(512)));
    aWriter.WriteBits(count, 10);

    for (std::uint16_t i = 0; i < count; ++i)
    {
        Serialization::WriteString(aWriter, PluginList[i].Filename);
        Serialization::WriteString(aWriter, PluginList[i].Version);
    }
}

void NativePlugins::Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    std::uint64_t data = 0;
    aReader.ReadBits(data, 10);

    const size_t count = static_cast<size_t>(data & 0x3FF);
    PluginList.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        PluginList[i].Filename = Serialization::ReadString(aReader);
        PluginList[i].Version = Serialization::ReadString(aReader);
    }
}
