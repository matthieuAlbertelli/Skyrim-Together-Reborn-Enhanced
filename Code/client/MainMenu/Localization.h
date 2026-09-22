#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace STRE::MainMenu
{
inline constexpr std::string_view cHintCatalogFile = "localization.ini";

struct SkipHint
{
    std::string Key;
    std::string Action;

    [[nodiscard]] std::string Text() const;
};

// UTF-8 data only. Missing/invalid translations fall back to the en section;
// an unavailable key label hides the hint rather than advertising a wrong key.
[[nodiscard]] SkipHint ResolveSkipHint(std::string_view aCatalog, std::string_view aConfig, std::string_view aGameLanguage, std::uint32_t aScanCode);
} // namespace STRE::MainMenu
