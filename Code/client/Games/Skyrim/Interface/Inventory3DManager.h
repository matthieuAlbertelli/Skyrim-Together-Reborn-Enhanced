#pragma once

#include <cstdint>

struct InventoryEntry;

struct Inventory3DManagerDebugState
{
    float itemPosCopyX{};
    float itemPosCopyY{};
    float itemPosCopyZ{};
    float itemPosX{};
    float itemPosY{};
    float itemPosZ{};
    float itemScaleCopy{};
    float itemScale{};
    std::uint32_t lightScheme{};
    std::uintptr_t tempRef{};
    std::uint32_t loadedModelsCapacityFlags{};
    std::uint32_t loadedModelsCapacity{};
    std::uint32_t loadedModelsSize{};
    bool loadedModelsLocal{};
    std::uintptr_t loadedModelsData{};
    std::uintptr_t firstItemBase{};
    std::uintptr_t firstModelObject{};
    std::uintptr_t firstSceneObject{};
    float zoomProgress{};
    std::uintptr_t loadTask{};
    std::uint32_t stateFlags{};
    std::uint32_t tailFlags{};
};

// Minimal facade over Skyrim's native inventory model renderer.
struct Inventory3DManager
{
    static Inventory3DManager* GetSingleton() noexcept;

    void Begin3D(std::uint32_t aLightScheme = 1) noexcept;
    void End3D() noexcept;
    void UpdateItem3D(InventoryEntry* apEntry) noexcept;
    void Clear3D() noexcept;
    std::uint32_t Render() noexcept;

    [[nodiscard]] Inventory3DManagerDebugState CaptureDebugState() const noexcept;
};
