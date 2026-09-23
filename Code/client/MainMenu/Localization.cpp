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
bool ValidUtf8(std::string_view aText)
{
    for (std::size_t i = 0; i < aText.size();)
    {
        const auto first = static_cast<unsigned char>(aText[i++]);
        if (first < 0x80)
            continue;
        const unsigned count = first >= 0xc2 && first <= 0xdf ? 1 : first >= 0xe0 && first <= 0xef ? 2 : first >= 0xf0 && first <= 0xf4 ? 3 : 0;
        if (!count || i + count > aText.size())
            return false;
        unsigned point = first & (0x7f >> (count + 1));
        for (unsigned j = 0; j < count; ++j)
        {
            const auto next = static_cast<unsigned char>(aText[i++]);
            if ((next & 0xc0) != 0x80)
                return false;
            point = (point << 6) | (next & 0x3f);
        }
        if ((count == 2 && point < 0x800) || (count == 3 && point < 0x10000) || (point >= 0xd800 && point <= 0xdfff) || point > 0x10ffff)
            return false;
    }
    return true;
}

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
    // Presentation labels are short single lines. Control characters/oversized labels are
    // ignored; the parser's whole input is also bounded before allocation.
    if (text.empty() || text.size() > 128 || text.starts_with("<<<") || !ValidUtf8(text) ||
        std::any_of(text.begin(), text.end(), [](unsigned char c) { return c < 0x20 || c == 0x7f; }))
        return {};
    return std::string(text);
}
} // namespace

LocalizedText ResolveText(std::string_view aCatalog, std::string_view aConfig, std::string_view aGameLanguage)
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

    const auto resolve = [&](const char* apKey)
    {
        auto label = Label(catalog, section.c_str(), apKey);
        return label.empty() ? Label(catalog, "en", apKey) : label;
    };
    return {resolve("SkipAction"), resolve("MainMenuSubtitle")};
}
} // namespace STRE::MainMenu
