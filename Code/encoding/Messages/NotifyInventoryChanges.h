#pragma once

#include "Message.h"

#include <Structs/GameId.h>
#include <Structs/Inventory.h>

struct NotifyInventoryChanges final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyInventoryChanges;

    NotifyInventoryChanges()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const NotifyInventoryChanges& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() && ServerId == acRhs.ServerId && Item == acRhs.Item && Drop == acRhs.Drop &&
               WorldEntityId == acRhs.WorldEntityId && PlacedReferenceId == acRhs.PlacedReferenceId &&
               BindOnly == acRhs.BindOnly && OriginFormId == acRhs.OriginFormId && Snapshot == acRhs.Snapshot &&
               TransformUpdate == acRhs.TransformUpdate && LifecycleOnly == acRhs.LifecycleOnly && HasTransform == acRhs.HasTransform &&
               PositionX == acRhs.PositionX && PositionY == acRhs.PositionY && PositionZ == acRhs.PositionZ &&
               RotationX == acRhs.RotationX && RotationY == acRhs.RotationY && RotationZ == acRhs.RotationZ;
    }

    uint32_t ServerId{};
    Inventory::Entry Item{};
    bool Drop = false;
    uint64_t WorldEntityId{};
    GameId PlacedReferenceId{};
    bool BindOnly = false;
    uint32_t OriginFormId{};
    bool Snapshot = false;
    bool TransformUpdate = false;
    // Lifecycle-only notifies mutate/retire the physical WorldEntity binding but
    // deliberately do not mirror an inventory delta. This is used for placed
    // references whose vanilla activation synchronization already owns inventory.
    bool LifecycleOnly = false;
    bool HasTransform = false;
    float PositionX{};
    float PositionY{};
    float PositionZ{};
    float RotationX{};
    float RotationY{};
    float RotationZ{};
};
