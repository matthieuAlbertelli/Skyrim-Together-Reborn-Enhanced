#pragma once

#include <string>
#include <string_view>

namespace STRE::MainMenu
{
inline constexpr std::string_view cHintCatalogFile = "localization.ini";

// UTF-8 data only. Missing/invalid translations fall back to the en section;
// keyboard artwork is resolved by Skyrim, not by the translation catalog.
[[nodiscard]] std::string ResolveSkipAction(std::string_view aCatalog, std::string_view aConfig, std::string_view aGameLanguage);
} // namespace STRE::MainMenu
