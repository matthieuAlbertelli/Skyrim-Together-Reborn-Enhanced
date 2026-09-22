#include "Localization.h"

// Reuse the native console's bundled SimpleIni parser. UTF-8 char strings need
// no wide-character conversion library (including on Linux policy tests).
#define SI_NO_CONVERSION
#include "../../base/simpleini/SimpleIni.h"

#include <algorithm>

namespace STRE::MainMenu
{
namespace
{
bool EqualLanguage(std::string_view aLeft, std::string_view aRight)
{
    const auto lower = [](unsigned char aChar)
    {
        return aChar >= 'A' && aChar <= 'Z' ? aChar + ('a' - 'A') : aChar;
    };
    return aLeft.size() == aRight.size() && std::equal(aLeft.begin(), aLeft.end(), aRight.begin(), [&](char a, char b) { return lower(a) == lower(b); });
}

std::string Label(const CSimpleIniCaseA& aCatalog, const char* apSection, const char* apKey)
{
    const char* value = aCatalog.GetValue(apSection, apKey, "");
    const std::string_view text(value);
    // A hint is a short single line. Control characters/oversized labels are
    // ignored; the parser's whole input is also bounded before allocation.
    if (text.empty() || text.size() > 128 || text.starts_with("<<<") || std::any_of(text.begin(), text.end(), [](unsigned char c) { return c < 0x20 || c == 0x7f; }))
        return {};
    return std::string(text);
}
} // namespace

std::string ResolveSkipAction(std::string_view aCatalog, std::string_view aConfig, std::string_view aGameLanguage)
{
    if (aCatalog.empty() || aCatalog.size() > 16384 || aCatalog.find('\0') != std::string_view::npos)
        return {};
    CSimpleIniCaseA catalog(true);
    if (catalog.LoadData(aCatalog.data(), aCatalog.size()) < 0)
        return {};

    CSimpleIniCaseA config(true);
    if (aConfig.size() <= 4096 && aConfig.find('\0') == std::string_view::npos)
        config.LoadData(aConfig.data(), aConfig.size());
    const char* requested = config.GetValue("Presentation", "Language", "auto");
    std::string section = "en";
    if (EqualLanguage(requested, "auto"))
    {
        CSimpleIniCaseA::TNamesDepend sections;
        catalog.GetAllSections(sections);
        for (const auto& entry : sections)
        {
            const char* gameLanguage = catalog.GetValue(entry.pItem, "SkyrimLanguage", "");
            if (!aGameLanguage.empty() && EqualLanguage(gameLanguage, aGameLanguage))
            {
                section = entry.pItem;
                break;
            }
        }
    }
    else
        section = requested;

    auto action = Label(catalog, section.c_str(), "SkipAction");
    if (action.empty())
        action = Label(catalog, "en", "SkipAction");
    return action;
}
} // namespace STRE::MainMenu
