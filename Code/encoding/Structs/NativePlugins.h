#pragma once

#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Stl.hpp>

struct NativePlugins
{
    struct Entry
    {
        TiltedPhoques::String Filename;
        TiltedPhoques::String Version;

        bool operator==(const Entry& acRhs) const noexcept
        {
            return Filename == acRhs.Filename && Version == acRhs.Version;
        }
    };

    TiltedPhoques::Vector<Entry> PluginList{};

    bool operator==(const NativePlugins& acRhs) const noexcept { return PluginList == acRhs.PluginList; }
    bool operator!=(const NativePlugins& acRhs) const noexcept { return !(*this == acRhs); }

    void Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept;
};
