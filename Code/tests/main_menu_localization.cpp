#include "../client/MainMenu/Localization.h"

#include <catch2/catch.hpp>

using namespace STRE::MainMenu;

namespace
{
std::string SkipAction(std::string_view aCatalog, std::string_view aConfig, std::string_view aGameLanguage)
{
    return ResolveText(aCatalog, aConfig, aGameLanguage).SkipAction;
}
const std::string cCatalog = "[en]\nSkyrimLanguage=ENGLISH\nSkipAction=Skip\n"
                             "[fr]\nSkyrimLanguage=FRENCH\nSkipAction=Passer\n";
} // namespace

TEST_CASE("Intro action follows Skyrim language or an explicit locale", "[main-menu][main-menu-localization]")
{
    REQUIRE(SkipAction(cCatalog, "", "French") == "Passer");
    REQUIRE(SkipAction(cCatalog, "", "ENGLISH") == "Skip");
    REQUIRE(SkipAction(cCatalog, "[Presentation]\nLanguage=en", "FRENCH") == "Skip");
    REQUIRE(SkipAction(cCatalog, "[Presentation]\nLanguage=fr", "ENGLISH") == "Passer");
    REQUIRE(SkipAction(cCatalog, "", "") == "Skip");
    REQUIRE(SkipAction(cCatalog, "[Presentation]\nLanguage=unknown", "FRENCH") == "Skip");
}

TEST_CASE("Intro catalog translates actions independently of native key artwork", "[main-menu][main-menu-localization]")
{
    const auto extended = cCatalog + "[de]\nSkyrimLanguage=GERMAN\nSkipAction=Weiter\n";
    REQUIRE(SkipAction(extended, "", "german") == "Weiter");
    REQUIRE(SkipAction(extended, "[Presentation]\nLanguage=de", "") == "Weiter");
    // Old catalogs remain usable; stale key labels cannot override Skyrim art.
    REQUIRE(SkipAction(cCatalog + "Key.1=WRONG\nKey.57=WRONG\n", "", "FRENCH") == "Passer");
    REQUIRE(SkipAction("[en]\nKey.1=ESC", "", "").empty());
    // The renderer must use plain text, including characters meaningful in HTML.
    REQUIRE(SkipAction("[en]\nSkipAction=<A & B>", "", "") == "<A & B>");
    REQUIRE(
        SkipAction(
            "[fr]\nSkipAction=\xC3\x89"
            "tape",
            "[Presentation]\nLanguage=fr", "") == "\xC3\x89"
                                                  "tape");
}

TEST_CASE("Intro action malformed or unavailable resources cannot affect playback", "[main-menu][main-menu-localization]")
{
    REQUIRE(SkipAction("", "", "FRENCH").empty());
    REQUIRE(SkipAction("not an ini", "", "").empty());
    REQUIRE(SkipAction(std::string(16385, 'x'), "", "").empty());
    REQUIRE(SkipAction(cCatalog + std::string(1, '\0'), "", "").empty());
    REQUIRE(SkipAction("\xEF\xBB\xBF" + cCatalog, "", "FRENCH") == "Passer");
    REQUIRE(SkipAction(cCatalog, std::string(4097, 'x'), "FRENCH") == "Passer");
    REQUIRE(SkipAction(cCatalog + "[fr]\nSkipAction=<<<END\ninvalid\nmultiline\nEND\n", "", "FRENCH") == "Skip");
    REQUIRE(SkipAction(cCatalog + "[fr]\nSkipAction=" + std::string(129, 'x'), "", "FRENCH") == "Skip");
    REQUIRE(SkipAction(cCatalog + "[fr]\nSkipAction=bad\x7f", "", "FRENCH") == "Skip");
}

TEST_CASE("Main Menu subtitle uses the shared locale and per-key fallback without coupling the hint", "[main-menu][main-menu-localization]")
{
    const std::string french = "La Compagnie de l\xE2\x80\x99"
                               "Enfant de Dragon";
    const std::string english = "Fellowship of the Dragonborn";
    const auto catalog = cCatalog + "[en]\nMainMenuSubtitle=" + english + "\n[fr]\nMainMenuSubtitle=" + french + "\n";
    REQUIRE(ResolveText(catalog, "", "FRENCH").MainMenuSubtitle == french);
    REQUIRE(ResolveText(catalog, "", "ENGLISH").MainMenuSubtitle == english);
    REQUIRE(ResolveText(catalog, "[Presentation]\nLanguage=en", "FRENCH").MainMenuSubtitle == english);
    REQUIRE(ResolveText(catalog, "[Presentation]\nLanguage=fr", "ENGLISH").MainMenuSubtitle == french);
    REQUIRE(ResolveText(catalog, "", "UNKNOWN").MainMenuSubtitle == english);
    const auto partial = catalog + "[de]\nSkyrimLanguage=GERMAN\nSkipAction=Weiter\n";
    REQUIRE(ResolveText(partial, "", "GERMAN").SkipAction == "Weiter");
    REQUIRE(ResolveText(partial, "", "GERMAN").MainMenuSubtitle == english);
    REQUIRE(ResolveText(partial + "MainMenuSubtitle=Drachen\n", "", "GERMAN").MainMenuSubtitle == "Drachen");
    REQUIRE(ResolveText(cCatalog, "", "FRENCH").MainMenuSubtitle.empty());
    REQUIRE(ResolveText(cCatalog, "", "FRENCH").SkipAction == "Passer");
    REQUIRE(ResolveText(catalog + "[fr]\nMainMenuSubtitle=" + std::string(129, 'x'), "", "FRENCH").MainMenuSubtitle == english);
    REQUIRE(ResolveText("[en]\nMainMenuSubtitle=<A & B>", "", "").MainMenuSubtitle == "<A & B>");
    REQUIRE(ResolveText("", "", "").MainMenuSubtitle.empty());
    for (const auto* invalid : {"\x80", "\xc0\xaf", "\xe2\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x28\xa1"})
        REQUIRE(ResolveText(catalog + "[fr]\nMainMenuSubtitle=" + invalid, "", "FRENCH").MainMenuSubtitle == english);
}
