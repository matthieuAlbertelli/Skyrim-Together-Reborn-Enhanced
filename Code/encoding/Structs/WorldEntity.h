#pragma once

#include <cstdint>
#include <TiltedCore/Buffer.hpp>

enum class WorldEntityManipulationAction : std::uint8_t
{
    Start = 0,
    Update,
    Release,
    Rejected,
};

struct WorldEntityTransform
{
    float PositionX{};
    float PositionY{};
    float PositionZ{};
    float RotationX{};
    float RotationY{};
    float RotationZ{};

    void Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept;

    bool operator==(const WorldEntityTransform& acRhs) const noexcept
    {
        return PositionX == acRhs.PositionX && PositionY == acRhs.PositionY && PositionZ == acRhs.PositionZ &&
               RotationX == acRhs.RotationX && RotationY == acRhs.RotationY && RotationZ == acRhs.RotationZ;
    }
};
