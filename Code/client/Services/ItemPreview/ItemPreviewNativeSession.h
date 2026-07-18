#pragma once

#include <Interface/Inventory3DManager.h>
#include <Misc/InventoryEntry.h>
#include <Services/ItemPreview/ItemPreviewFitSolver.h>

#include <cstdint>

class ItemPreviewNativeSession
{
public:
    [[nodiscard]] bool Begin(std::uint32_t aLightScheme = 1) noexcept;
    void End() noexcept;

    [[nodiscard]] bool IsOpen() const noexcept
    {
        return m_open;
    }

    [[nodiscard]] std::uint32_t GetLightScheme() const noexcept
    {
        return m_lightScheme;
    }

    [[nodiscard]] void* GetManagerAddress() const noexcept;
    [[nodiscard]] Inventory3DManagerDebugState CaptureDebugState() const noexcept;
    [[nodiscard]] Inventory3DManagerPreviewModelBounds CaptureModelBounds(
        std::uintptr_t aExpectedModel) const noexcept;

    [[nodiscard]] bool Clear() noexcept;
    [[nodiscard]] bool Load(
        InventoryEntry& aEntry,
        const ItemPreviewTransform* apTransform = nullptr) noexcept;
    [[nodiscard]] bool RestartAndLoad(
        InventoryEntry& aEntry,
        const ItemPreviewTransform& aTransform,
        std::uint32_t aLightScheme = 0) noexcept;

private:
    [[nodiscard]] static Inventory3DManager* GetManager() noexcept;

    bool m_open{};
    std::uint32_t m_lightScheme{1};
};
