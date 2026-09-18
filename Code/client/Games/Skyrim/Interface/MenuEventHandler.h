#pragma once

// Opaque ABI block; moved unchanged from Menus/SkillsMenu.h.
struct MenuEventHandler
{
    char pad0[0x10];
};

static_assert(sizeof(MenuEventHandler) == 0x10);
