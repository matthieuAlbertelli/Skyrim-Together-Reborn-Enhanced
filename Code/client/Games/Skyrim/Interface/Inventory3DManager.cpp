#include <TiltedOnlinePCH.h>

#include <cstddef>
#include <cmath>
#include <cstring>

#include <Interface/Inventory3DManager.h>
#include <Misc/InventoryEntry.h>

namespace
{
using TBegin3D = void(__fastcall)(Inventory3DManager*, std::uint32_t);
using TEnd3D = void(__fastcall)(Inventory3DManager*);
using TUpdateItem3D = void(__fastcall)(Inventory3DManager*, InventoryEntry*);
using TClear3D = void(__fastcall)(Inventory3DManager*);
using TRender = std::uint32_t(__fastcall)(Inventory3DManager*);

template <class T>
void WriteMember(
    Inventory3DManager* apManager,
    std::size_t aOffset,
    const T& aValue) noexcept
{
    auto* const pBase =
        reinterpret_cast<std::uint8_t*>(apManager);
    std::memcpy(pBase + aOffset, &aValue, sizeof(T));
}

template <class T>
T ReadMember(const Inventory3DManager* apManager, std::size_t aOffset) noexcept
{
    T value{};
    const auto* const pBase =
        reinterpret_cast<const std::uint8_t*>(apManager);
    std::memcpy(&value, pBase + aOffset, sizeof(T));
    return value;
}

template <class T>
T ReadAddress(std::uintptr_t aAddress) noexcept
{
    T value{};
    if (aAddress == 0)
        return value;

    std::memcpy(
        &value,
        reinterpret_cast<const void*>(aAddress),
        sizeof(T));
    return value;
}
}

Inventory3DManager* Inventory3DManager::GetSingleton() noexcept
{
    POINTER_SKYRIMSE(Inventory3DManager*, singleton, 403559);
    Inventory3DManager** const ppManager = singleton.Get();
    return ppManager ? *ppManager : nullptr;
}

void Inventory3DManager::Begin3D(std::uint32_t aLightScheme) noexcept
{
    POINTER_SKYRIMSE(TBegin3D, begin3D, 51754);
    if (auto* const pFunction = begin3D.Get())
        pFunction(this, aLightScheme);
}

void Inventory3DManager::End3D() noexcept
{
    POINTER_SKYRIMSE(TEnd3D, end3D, 51756);
    if (auto* const pFunction = end3D.Get())
        pFunction(this);
}

void Inventory3DManager::UpdateItem3D(InventoryEntry* apEntry) noexcept
{
    if (!apEntry || !apEntry->pObject)
        return;

    POINTER_SKYRIMSE(TUpdateItem3D, updateItem3D, 51757);
    if (auto* const pFunction = updateItem3D.Get())
        pFunction(this, apEntry);
}

void Inventory3DManager::Clear3D() noexcept
{
    POINTER_SKYRIMSE(TClear3D, clear3D, 51759);
    if (auto* const pFunction = clear3D.Get())
        pFunction(this);
}

std::uint32_t Inventory3DManager::Render() noexcept
{
    POINTER_SKYRIMSE(TRender, render, 51755);
    if (auto* const pFunction = render.Get())
        return pFunction(this);

    return 0;
}

void Inventory3DManager::SetPreviewTransform(
    float aX,
    float aY,
    float aZ,
    float aScale) noexcept
{
    // Skyrim AE 1.6.1170, post-1.6.629 layout. Both the copy and
    // current values are kept in sync because the renderer interpolates
    // between them during zoom and item changes.
    constexpr std::size_t kItemPosCopyOffset = 0x14;
    constexpr std::size_t kItemPosOffset = 0x20;
    constexpr std::size_t kItemScaleCopyOffset = 0x2C;
    constexpr std::size_t kItemScaleOffset = 0x30;

    WriteMember(this, kItemPosCopyOffset + 0x0, aX);
    WriteMember(this, kItemPosCopyOffset + 0x4, aY);
    WriteMember(this, kItemPosCopyOffset + 0x8, aZ);
    WriteMember(this, kItemPosOffset + 0x0, aX);
    WriteMember(this, kItemPosOffset + 0x4, aY);
    WriteMember(this, kItemPosOffset + 0x8, aZ);
    WriteMember(this, kItemScaleCopyOffset, aScale);
    WriteMember(this, kItemScaleOffset, aScale);
}

Inventory3DManagerDebugState
Inventory3DManager::CaptureDebugState() const noexcept
{
    static_assert(sizeof(std::uintptr_t) == 8);

    // Skyrim AE 1.6.1170 uses the post-1.6.629 Inventory3DManager layout.
    constexpr std::size_t kItemPosCopyOffset = 0x14;
    constexpr std::size_t kItemPosOffset = 0x20;
    constexpr std::size_t kItemScaleCopyOffset = 0x2C;
    constexpr std::size_t kItemScaleOffset = 0x30;
    constexpr std::size_t kLightSchemeOffset = 0x34;
    constexpr std::size_t kTempRefOffset = 0x38;
    constexpr std::size_t kLoadedModelsOffset = 0x60;
    constexpr std::size_t kLoadedModelsDataOffset = 0x68;
    constexpr std::size_t kLoadedModelsSizeOffset = 0x148;
    constexpr std::size_t kZoomProgressOffset = 0x154;
    constexpr std::size_t kLoadTaskOffset = 0x158;
    constexpr std::size_t kStateFlagsOffset = 0x160;
    constexpr std::size_t kTailFlagsOffset = 0x164;

    Inventory3DManagerDebugState state{};
    state.itemPosCopyX = ReadMember<float>(this, kItemPosCopyOffset + 0x0);
    state.itemPosCopyY = ReadMember<float>(this, kItemPosCopyOffset + 0x4);
    state.itemPosCopyZ = ReadMember<float>(this, kItemPosCopyOffset + 0x8);
    state.itemPosX = ReadMember<float>(this, kItemPosOffset + 0x0);
    state.itemPosY = ReadMember<float>(this, kItemPosOffset + 0x4);
    state.itemPosZ = ReadMember<float>(this, kItemPosOffset + 0x8);
    state.itemScaleCopy = ReadMember<float>(this, kItemScaleCopyOffset);
    state.itemScale = ReadMember<float>(this, kItemScaleOffset);
    state.lightScheme = ReadMember<std::uint32_t>(this, kLightSchemeOffset);
    state.tempRef = ReadMember<std::uintptr_t>(this, kTempRefOffset);

    state.loadedModelsCapacityFlags =
        ReadMember<std::uint32_t>(this, kLoadedModelsOffset);
    state.loadedModelsCapacity =
        state.loadedModelsCapacityFlags & 0x7FFFFFFFu;
    state.loadedModelsLocal =
        (state.loadedModelsCapacityFlags & 0x80000000u) != 0;
    state.loadedModelsSize =
        ReadMember<std::uint32_t>(this, kLoadedModelsSizeOffset);

    const auto* const pBase =
        reinterpret_cast<const std::uint8_t*>(this);
    state.loadedModelsData = state.loadedModelsLocal
        ? reinterpret_cast<std::uintptr_t>(pBase + kLoadedModelsDataOffset)
        : ReadMember<std::uintptr_t>(this, kLoadedModelsDataOffset);

    // The local buffer is part of Inventory3DManager itself, so reading its
    // first LoadedInventoryModel is safe when the array reports at least one.
    if (state.loadedModelsLocal && state.loadedModelsSize > 0)
    {
        state.firstItemBase =
            ReadMember<std::uintptr_t>(this, kLoadedModelsDataOffset + 0x00);
        state.firstModelObject =
            ReadMember<std::uintptr_t>(this, kLoadedModelsDataOffset + 0x08);
        state.firstSceneObject =
            ReadMember<std::uintptr_t>(this, kLoadedModelsDataOffset + 0x10);
    }

    state.zoomProgress = ReadMember<float>(this, kZoomProgressOffset);
    state.loadTask = ReadMember<std::uintptr_t>(this, kLoadTaskOffset);
    state.stateFlags = ReadMember<std::uint32_t>(this, kStateFlagsOffset);
    state.tailFlags = ReadMember<std::uint32_t>(this, kTailFlagsOffset);
    return state;
}

Inventory3DManagerPreviewModelBounds
Inventory3DManager::CaptureModelBounds(
    std::uintptr_t aExpectedModelObject) const noexcept
{
    // Skyrim AE 1.6.1170, post-1.6.629 layout. Resolve the active preview by
    // matching the selected TESBoundObject instead of assuming that the last
    // cached entry is active: Inventory3DManager reuses earlier cached models.
    constexpr std::size_t kLoadedModelsOffset = 0x60;
    constexpr std::size_t kLoadedModelsDataOffset = 0x68;
    constexpr std::size_t kLoadedModelsSizeOffset = 0x148;
    constexpr std::size_t kLoadedInventoryModelSize = 0x20;
    constexpr std::size_t kLoadedInventoryModelItemBaseOffset = 0x00;
    constexpr std::size_t kLoadedInventoryModelObjectOffset = 0x08;
    constexpr std::size_t kLoadedInventoryModelSceneOffset = 0x10;
    constexpr std::size_t kNiAVObjectWorldTransformOffset = 0x7C;
    constexpr std::size_t kNiTransformTranslateOffset = 0x24;
    constexpr std::size_t kNiTransformScaleOffset = 0x30;
    constexpr std::size_t kNiAVObjectWorldBoundOffset = 0xE4;

    Inventory3DManagerPreviewModelBounds bounds{};

    const std::uint32_t capacityFlags =
        ReadMember<std::uint32_t>(this, kLoadedModelsOffset);
    const bool usesLocalStorage =
        (capacityFlags & 0x80000000u) != 0;
    const std::uint32_t size =
        ReadMember<std::uint32_t>(this, kLoadedModelsSizeOffset);
    if (size == 0)
        return bounds;

    const auto* const pManagerBase =
        reinterpret_cast<const std::uint8_t*>(this);
    const std::uintptr_t data = usesLocalStorage
        ? reinterpret_cast<std::uintptr_t>(
              pManagerBase + kLoadedModelsDataOffset)
        : ReadMember<std::uintptr_t>(
              this, kLoadedModelsDataOffset);
    if (data == 0)
        return bounds;

    std::uintptr_t selectedModel = 0;
    for (std::uint32_t reverseIndex = 0; reverseIndex < size; ++reverseIndex)
    {
        const std::uint32_t index = size - 1 - reverseIndex;
        const std::uintptr_t candidate =
            data +
            static_cast<std::uintptr_t>(index) *
                kLoadedInventoryModelSize;
        const std::uintptr_t itemBase = ReadAddress<std::uintptr_t>(
            candidate + kLoadedInventoryModelItemBaseOffset);
        const std::uintptr_t modelObject = ReadAddress<std::uintptr_t>(
            candidate + kLoadedInventoryModelObjectOffset);

        if (aExpectedModelObject == 0 ||
            itemBase == aExpectedModelObject ||
            modelObject == aExpectedModelObject)
        {
            selectedModel = candidate;
            bounds.matchedExpectedModel =
                aExpectedModelObject != 0;
            bounds.itemBase = itemBase;
            bounds.modelObject = modelObject;
            break;
        }
    }

    if (selectedModel == 0)
        return bounds;

    bounds.sceneObject = ReadAddress<std::uintptr_t>(
        selectedModel + kLoadedInventoryModelSceneOffset);
    if (bounds.sceneObject == 0)
        return bounds;

    const std::uintptr_t worldTransform =
        bounds.sceneObject + kNiAVObjectWorldTransformOffset;
    bounds.rootWorldX = ReadAddress<float>(
        worldTransform + kNiTransformTranslateOffset + 0x0);
    bounds.rootWorldY = ReadAddress<float>(
        worldTransform + kNiTransformTranslateOffset + 0x4);
    bounds.rootWorldZ = ReadAddress<float>(
        worldTransform + kNiTransformTranslateOffset + 0x8);
    bounds.rootWorldScale = ReadAddress<float>(
        worldTransform + kNiTransformScaleOffset);

    const std::uintptr_t worldBound =
        bounds.sceneObject + kNiAVObjectWorldBoundOffset;
    bounds.centerX = ReadAddress<float>(worldBound + 0x0);
    bounds.centerY = ReadAddress<float>(worldBound + 0x4);
    bounds.centerZ = ReadAddress<float>(worldBound + 0x8);
    bounds.radius = ReadAddress<float>(worldBound + 0xC);

    bounds.valid =
        std::isfinite(bounds.centerX) &&
        std::isfinite(bounds.centerY) &&
        std::isfinite(bounds.centerZ) &&
        std::isfinite(bounds.radius) &&
        std::isfinite(bounds.rootWorldX) &&
        std::isfinite(bounds.rootWorldY) &&
        std::isfinite(bounds.rootWorldZ) &&
        std::isfinite(bounds.rootWorldScale) &&
        bounds.radius > 0.001F &&
        bounds.radius < 1000000.0F &&
        std::abs(bounds.rootWorldScale) > 0.0001F;
    return bounds;
}
