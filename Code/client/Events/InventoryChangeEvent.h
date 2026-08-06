#pragma once

#include <Structs/Inventory.h>
#include <ExtraData/ExtraDataList.h>

/**
 * @brief Dispatched when the contents of an object or actor inventory changes locally.
 *
 * The event has a Drop member variable, since dropped items need to be handled differently.
 */
struct InventoryChangeEvent
{
    InventoryChangeEvent(const uint32_t aFormId, Inventory::Entry arItem)
        : FormId(aFormId)
        , Item(std::move(arItem))
    {
    }

    InventoryChangeEvent(const uint32_t aFormId, Inventory::Entry arItem, bool aDrop)
        : FormId(aFormId)
        , Item(std::move(arItem))
        , Drop(aDrop)
    {
    }

    InventoryChangeEvent(const uint32_t aFormId, Inventory::Entry arItem, bool aDrop, bool aUpdateClients, uint32_t aDroppedFormId = 0)
        : FormId(aFormId)
        , Item(std::move(arItem))
        , Drop(aDrop)
        , UpdateClients(aUpdateClients)
        , DroppedFormId(aDroppedFormId)
    {
    }

    uint32_t FormId{};
    Inventory::Entry Item{};
    bool Drop = false;
    bool UpdateClients = true;
    /// Local temporary reference created by a drop, or consumed by a pickup.
    uint32_t DroppedFormId{};
};
