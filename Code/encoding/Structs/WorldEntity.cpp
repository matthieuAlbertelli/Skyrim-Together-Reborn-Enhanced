#include <Structs/WorldEntity.h>
#include <TiltedCore/Serialization.hpp>

using TiltedPhoques::Serialization;

void WorldEntityTransform::Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteFloat(aWriter, PositionX);
    Serialization::WriteFloat(aWriter, PositionY);
    Serialization::WriteFloat(aWriter, PositionZ);
    Serialization::WriteFloat(aWriter, RotationX);
    Serialization::WriteFloat(aWriter, RotationY);
    Serialization::WriteFloat(aWriter, RotationZ);
}

void WorldEntityTransform::Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    PositionX = Serialization::ReadFloat(aReader);
    PositionY = Serialization::ReadFloat(aReader);
    PositionZ = Serialization::ReadFloat(aReader);
    RotationX = Serialization::ReadFloat(aReader);
    RotationY = Serialization::ReadFloat(aReader);
    RotationZ = Serialization::ReadFloat(aReader);
}
