#pragma once

#include "ExtraData.h"

struct TESForm;

struct ExtraOwnership : BSExtraData
{
    inline static constexpr auto eExtraData = ExtraDataType::Ownership;

    TESForm* pOwner{};
};

static_assert(sizeof(ExtraOwnership) == 0x18);
