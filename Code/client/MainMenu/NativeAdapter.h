#pragma once

#include <Runtime/NativeContracts.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>

namespace STRE::MainMenu
{
// Exact Steam 1.7.104 disk/source evidence: runtime17 presentation audit.
// Independent of campaign admission; never authorizes another native boundary.
constexpr bool SupportsPresentation(std::string_view aVersion) noexcept
{
    return aVersion == "1.7.104.0" &&
           Runtime::NativeContracts[static_cast<std::size_t>(Runtime::NativeContract::MenuPresentation)].Qualified();
}

// MainMenu::AdvanceMovie + 2A and ProcessMessage + 2DD both call the
// no-argument music trigger (52185). Neither caller consumes its return value.
inline constexpr std::array<std::array<std::uint8_t, 5>, 2> cMusicCalls{{
    {0xE8, 0x01, 0x68, 0x00, 0x00}, {0xE8, 0x9E, 0x47, 0x00, 0x00}}};

inline bool PrepareMusicCall(std::size_t aSite, std::span<const std::uint8_t> aBytes,
                             std::uintptr_t aAddress, std::uintptr_t aOriginal,
                             std::uintptr_t aHook, std::int32_t& aDisplacement) noexcept
{
    if (aSite >= cMusicCalls.size() || aBytes.size() != 5 || !aOriginal || !aHook ||
        std::memcmp(aBytes.data(), cMusicCalls[aSite].data(), 5) != 0)
        return false;
    std::int32_t original{};
    std::memcpy(&original, aBytes.data() + 1, sizeof(original));
    if (aAddress > static_cast<std::uintptr_t>(std::numeric_limits<std::int64_t>::max()) - 5 ||
        aHook > static_cast<std::uintptr_t>(std::numeric_limits<std::int64_t>::max()))
        return false;
    const auto after = static_cast<std::int64_t>(aAddress + 5);
    if (aAddress + 5 + static_cast<std::uintptr_t>(original) != aOriginal)
        return false;
    const auto distance = static_cast<std::int64_t>(aHook) - after;
    if (distance < std::numeric_limits<std::int32_t>::min() || distance > std::numeric_limits<std::int32_t>::max())
        return false;
    aDisplacement = static_cast<std::int32_t>(distance);
    return true;
}

// Startup-only transaction. Every address/byte/original is preflighted by the
// adapter before this step. Commit cannot fail/throw and runs only after every
// write permission is held. Release in reverse order, including shared pages.
template <class Acquire, class Restore, class Commit>
bool WithWritablePatches(std::size_t aCount, Acquire aAcquire, Restore aRestore, Commit aCommit) noexcept
{
    std::size_t writable{};
    while (writable < aCount && aAcquire(writable))
        ++writable;
    const bool ready = aCount != 0 && writable == aCount;
    if (ready)
        aCommit();
    while (writable)
        aRestore(--writable);
    return ready;
}
} // namespace STRE::MainMenu
