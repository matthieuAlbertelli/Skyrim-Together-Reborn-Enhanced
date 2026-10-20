#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

namespace STRE::Runtime
{
// Exact flat AE 1.6.1170 / 1.7.104 prefix, not an owning game object.
// Binary evidence: docs/audit/2026-10-20-runtime-1.7.104-lot3.md, C06.
struct NativeSaveListView
{
    void* pData;
    std::uint32_t Capacity;
    std::uint32_t Pad0C;
    std::uint32_t Size;
    std::uint32_t Pad14;
};
static_assert(sizeof(NativeSaveListView) == 0x18);

struct NativeSaveSelectionView
{
    std::byte Pad000[0x100];
    NativeSaveListView List;
    std::byte Pad118[0x128];
    const char* pRememberedFileName;
    std::uint16_t RememberedNameLength;
};
static_assert(offsetof(NativeSaveSelectionView, List) == 0x100);
static_assert(offsetof(NativeSaveSelectionView, pRememberedFileName) == 0x240);
static_assert(offsetof(NativeSaveSelectionView, RememberedNameLength) == 0x248);

// ID35766 checks the BSString length, then reads its pointer. List order is
// unrelated to this selection. The caller must protect the memory read and
// copy/validate the borrowed string before forwarding to the engine.
inline const char* MostRecentSaveName(const NativeSaveSelectionView* apManager) noexcept
{
    return apManager && apManager->RememberedNameLength != 0
        ? apManager->pRememberedFileName : nullptr;
}

// The UI reverses the native array. Bounds are STRE's conservative admission
// checks; they do not claim a chronological ordering of the native list.
inline std::optional<std::uint32_t> NativeSaveListIndex(
    const NativeSaveListView& aList, std::uint32_t aSelection) noexcept
{
    if (!aList.pData || aList.Size == 0 || aList.Size > aList.Capacity ||
        aList.Capacity > 10000 || aSelection >= aList.Size)
        return std::nullopt;
    return aList.Size - aSelection - 1;
}
} // namespace STRE::Runtime
