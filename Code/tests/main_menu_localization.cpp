#include "../client/MainMenu/Localization.h"

#include <catch2/catch.hpp>

using namespace STRE::MainMenu;

namespace
{
const std::string cCatalog = "[en]\nSkyrimLanguage=ENGLISH\nSkipAction=Skip\n"
                             "[fr]\nSkyrimLanguage=FRENCH\nSkipAction=Passer\n";
}

TEST_CASE("Intro action follows Skyrim language or an explicit locale", "[main-menu][main-menu-localization]")
{
    REQUIRE(ResolveSkipAction(cCatalog, "", "French") == "Passer");
    REQUIRE(ResolveSkipAction(cCatalog, "", "ENGLISH") == "Skip");
    REQUIRE(ResolveSkipAction(cCatalog, "[Presentation]\nLanguage=en", "FRENCH") == "Skip");
    REQUIRE(ResolveSkipAction(cCatalog, "[Presentation]\nLanguage=fr", "ENGLISH") == "Passer");
    REQUIRE(ResolveSkipAction(cCatalog, "", "") == "Skip");
    REQUIRE(ResolveSkipAction(cCatalog, "[Presentation]\nLanguage=unknown", "FRENCH") == "Skip");
}

TEST_CASE("Intro catalog translates actions independently of native key artwork", "[main-menu][main-menu-localization]")
{
    const auto extended = cCatalog + "[de]\nSkyrimLanguage=GERMAN\nSkipAction=Weiter\n";
    REQUIRE(ResolveSkipAction(extended, "", "german") == "Weiter");
    REQUIRE(ResolveSkipAction(extended, "[Presentation]\nLanguage=de", "") == "Weiter");
    // Old catalogs remain usable; stale key labels cannot override Skyrim art.
    REQUIRE(ResolveSkipAction(cCatalog + "Key.1=WRONG\nKey.57=WRONG\n", "", "FRENCH") == "Passer");
    REQUIRE(ResolveSkipAction("[en]\nKey.1=ESC", "", "").empty());
    // The renderer must use plain text, including characters meaningful in HTML.
    REQUIRE(ResolveSkipAction("[en]\nSkipAction=<A & B>", "", "") == "<A & B>");
    REQUIRE(
        ResolveSkipAction(
            "[fr]\nSkipAction=\xC3\x89"
            "tape",
            "[Presentation]\nLanguage=fr", "") == "\xC3\x89"
                                                  "tape");
}

TEST_CASE("Intro action malformed or unavailable resources cannot affect playback", "[main-menu][main-menu-localization]")
{
    REQUIRE(ResolveSkipAction("", "", "FRENCH").empty());
    REQUIRE(ResolveSkipAction("not an ini", "", "").empty());
    REQUIRE(ResolveSkipAction(std::string(16385, 'x'), "", "").empty());
    REQUIRE(ResolveSkipAction(cCatalog + std::string(1, '\0'), "", "").empty());
    REQUIRE(ResolveSkipAction("\xEF\xBB\xBF" + cCatalog, "", "FRENCH") == "Passer");
    REQUIRE(ResolveSkipAction(cCatalog, std::string(4097, 'x'), "FRENCH") == "Passer");
    REQUIRE(ResolveSkipAction(cCatalog + "[fr]\nSkipAction=<<<END\ninvalid\nmultiline\nEND\n", "", "FRENCH") == "Skip");
    REQUIRE(ResolveSkipAction(cCatalog + "[fr]\nSkipAction=" + std::string(129, 'x'), "", "FRENCH") == "Skip");
    REQUIRE(ResolveSkipAction(cCatalog + "[fr]\nSkipAction=bad\x7f", "", "FRENCH") == "Skip");
}
