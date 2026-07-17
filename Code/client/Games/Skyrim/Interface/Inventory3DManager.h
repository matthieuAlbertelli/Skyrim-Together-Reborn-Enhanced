#pragma once

#include <cstdint>

struct InventoryEntry;

struct Inventory3DManagerPreviewModelBounds
{
    bool valid{};
    bool matchedExpectedModel{};
    std::uintptr_t itemBase{};
    std::uintptr_t modelObject{};
    std::uintptr_t sceneObject{};
    float centerX{};
    float centerY{};
    float centerZ{};
    float radius{};
    float rootWorldX{};
    float rootWorldY{};
    float rootWorldZ{};
    float rootWorldScale{};
};

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
    void SetPreviewTransform(
        float aX,
        float aY,
        float aZ,
        float aScale) noexcept;

    [[nodiscard]] Inventory3DManagerDebugState CaptureDebugState() const noexcept;
    [[nodiscard]] Inventory3DManagerPreviewModelBounds CaptureModelBounds(
        std::uintptr_t aExpectedModelObject) const noexcept;
};
