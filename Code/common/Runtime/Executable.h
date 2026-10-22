#pragma once

#include <Runtime/Compatibility.h>
#include <span>

namespace STRE::Runtime
{
struct Executable
{
    Distribution Store{Distribution::Unknown};
    std::uint32_t ImageSize{};
};

// Read-only PE inspection. Steam admission requires its import and protected
// .bind entry point; a matching version string alone never admits GOG.
bool InspectExecutable(std::span<const std::uint8_t> aBytes, Executable& aResult);
} // namespace STRE::Runtime
