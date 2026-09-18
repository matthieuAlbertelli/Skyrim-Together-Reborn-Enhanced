#pragma once

#include <Camera/TESCamera.h>
#include <cstdint>

// Read-only ABI view for Steam AE 1.6.1170 (non-VR).
// CommonLibSSE-NG include/RE/R/RaceSexCamera.h. Engine owns its lifetime;
// this wrapper is never constructed and its virtual methods are never called.
struct RaceSexCamera : TESCamera
{
    std::uint64_t unk38;
    std::uint64_t unk40;
    std::uint64_t unk48;
    std::uint64_t unk50;
};

static_assert(offsetof(RaceSexCamera, unk38) == 0x38);
static_assert(sizeof(RaceSexCamera) == 0x58);
