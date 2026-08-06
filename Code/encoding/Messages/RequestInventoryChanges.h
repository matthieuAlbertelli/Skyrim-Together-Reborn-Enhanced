#pragma once

#include "Message.h"
#include <Structs/Inventory.h>

struct RequestInventoryChanges final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kRequestInventoryChanges;

    RequestInventoryChanges()
        : ClientMessage(Opcode)
    {
    }

    virtual ~RequestInventoryChanges() = default;

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const RequestInventoryChanges& acRhs) const noexcept { return GetOpcode() == acRhs.GetOpcode() && ServerId == acRhs.ServerId && Item == acRhs.Item && Drop == acRhs.Drop && UpdateClients == acRhs.UpdateClients && DroppedFormId == acRhs.DroppedFormId && WorldEntityId == acRhs.WorldEntityId && TransformUpdate == acRhs.TransformUpdate && PositionX == acRhs.PositionX && PositionY == acRhs.PositionY && PositionZ == acRhs.PositionZ && RotationX == acRhs.RotationX && RotationY == acRhs.RotationY && RotationZ == acRhs.RotationZ; }

    uint32_t ServerId{};
    Inventory::Entry Item{};
    bool Drop = false;
    bool UpdateClients = true;
    uint32_t DroppedFormId{};
    uint64_t WorldEntityId{};
    bool TransformUpdate = false;
    float PositionX{};
    float PositionY{};
    float PositionZ{};
    float RotationX{};
    float RotationY{};
    float RotationZ{};
};
