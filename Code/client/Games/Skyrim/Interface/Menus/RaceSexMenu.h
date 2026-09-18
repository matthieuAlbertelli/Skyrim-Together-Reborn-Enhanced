#pragma once

#include <Camera/RaceSexCamera.h>
#include <Interface/IMenu.h>
#include <Interface/MenuEventHandler.h>
#include <RTTI.h>
#include <cstddef>
#include <type_traits>

// RaceSexMenu remains an opaque RTTI identity. This is only the prefix needed
// to read its embedded camera, not a constructible or complete menu class.
// CommonLibSSE-NG include/RE/R/RaceSexMenu.h: non-VR runtime starts at 0x40,
// seven BSTArray blocks precede the camera at runtime + 0xA8. Target: 1.6.1170.
struct RaceSexMenuLayout
{
    std::byte menu[sizeof(IMenu)];
    std::byte eventHandler[sizeof(MenuEventHandler)];
    GameArray<void*> headParts[7];
    alignas(RaceSexCamera) std::byte camera[sizeof(RaceSexCamera)];
};

static_assert(sizeof(IMenu) == 0x30);
static_assert(sizeof(GameArray<void*>) == 0x18);
static_assert(std::is_standard_layout_v<RaceSexMenuLayout>);
static_assert(offsetof(RaceSexMenuLayout, eventHandler) == 0x30);
static_assert(offsetof(RaceSexMenuLayout, headParts) == 0x40);
static_assert(offsetof(RaceSexMenuLayout, camera) == 0xE8);
static_assert(sizeof(RaceSexMenuLayout) == 0x140);

[[nodiscard]] inline const RaceSexCamera* GetRaceSexMenuCamera(IMenu* apMenu)
{
    if (!apMenu)
        return nullptr;

    const auto* const pMenu = Cast<RaceSexMenu>(apMenu);
    if (!pMenu)
        return nullptr;

    const auto* const pLayout = reinterpret_cast<const RaceSexMenuLayout*>(pMenu);
    // Confirm the embedded object's runtime type as well as the owning menu.
    return Cast<const RaceSexCamera>(reinterpret_cast<const TESCamera*>(pLayout->camera));
}
