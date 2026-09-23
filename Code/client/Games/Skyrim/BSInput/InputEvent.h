#pragma once

#include <Misc/BSFixedString.h>

// Public CommonLibSSE-NG input ABI, shared by existing menu-control tracing
// and Main Menu presentation. Runtime hooks remain in their owning adapters.
struct InputEvent
{
    virtual ~InputEvent();
    [[nodiscard]] virtual bool HasIDCode() const;
    [[nodiscard]] virtual const BSFixedString& QUserEvent() const;

    std::uint32_t Device;
    std::uint32_t EventType;
    InputEvent* pNext;
};
static_assert(sizeof(InputEvent) == 0x18);

struct ButtonInputEvent : InputEvent
{
    BSFixedString UserEvent;
    std::uint32_t IdCode;
    std::uint32_t Pad24;
    float Value;
    float HeldDownSeconds;
};
static_assert(sizeof(ButtonInputEvent) == 0x30);
