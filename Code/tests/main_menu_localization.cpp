#include "../client/MainMenu/Localization.h"

#include <catch2/catch.hpp>

using namespace STRE::MainMenu;

namespace
{
const std::string cCatalog = "[en]\nSkyrimLanguage=ENGLISH\nSkipAction=Skip\nKey.1=ESC\nKey.57=SPACE\n"
                             "[fr]\nSkyrimLanguage=FRENCH\nSkipAction=Passer\nKey.1=\xC3\x89"
                             "CHAP\nKey.57=ESPACE\n";
}

TEST_CASE("Intro hint follows Skyrim language or an explicit locale", "[main-menu][main-menu-localization]")
{
    const auto french = ResolveSkipHint(cCatalog, "", "French", 1);
    REQUIRE(
        french.Key == "\xC3\x89"
                      "CHAP");
    REQUIRE(french.Action == "Passer");
    REQUIRE(
        french.Text() == "[\xC3\x89"
                         "CHAP] Passer");
    REQUIRE(ResolveSkipHint(cCatalog, "", "ENGLISH", 1).Text() == "[ESC] Skip");
    REQUIRE(ResolveSkipHint(cCatalog, "[Presentation]\nLanguage=en", "FRENCH", 1).Text() == "[ESC] Skip");
    REQUIRE(ResolveSkipHint(cCatalog, "[Presentation]\nLanguage=fr", "ENGLISH", 1).Text() == french.Text());
    REQUIRE(ResolveSkipHint(cCatalog, "", "", 1).Text() == "[ESC] Skip");
    REQUIRE(ResolveSkipHint(cCatalog, "[Presentation]\nLanguage=unknown", "FRENCH", 1).Text() == "[ESC] Skip");
}

TEST_CASE("Intro translations and rebound key names are catalog data", "[main-menu][main-menu-localization]")
{
    const auto extended = cCatalog + "[de]\nSkyrimLanguage=GERMAN\nSkipAction=Weiter\nKey.1=ESC\n";
    REQUIRE(ResolveSkipHint(extended, "", "german", 1).Text() == "[ESC] Weiter");
    REQUIRE(ResolveSkipHint(extended, "[Presentation]\nLanguage=de", "", 1).Text() == "[ESC] Weiter");
    REQUIRE(ResolveSkipHint(cCatalog, "", "FRENCH", 57).Text() == "[ESPACE] Passer");
    REQUIRE(ResolveSkipHint(cCatalog, "", "FRENCH", 28).Text().empty());
    REQUIRE(ResolveSkipHint("[en]\nKey.1=ESC", "", "", 1).Text().empty());
}

TEST_CASE("Intro hint malformed or unavailable resources cannot affect playback", "[main-menu][main-menu-localization]")
{
    REQUIRE(ResolveSkipHint("", "", "FRENCH", 1).Text().empty());
    REQUIRE(ResolveSkipHint("not an ini", "", "", 1).Text().empty());
    REQUIRE(ResolveSkipHint(std::string(16385, 'x'), "", "", 1).Text().empty());
    REQUIRE(ResolveSkipHint(cCatalog + std::string(1, '\0'), "", "", 1).Text().empty());
    REQUIRE(
        ResolveSkipHint("\xEF\xBB\xBF" + cCatalog, "", "FRENCH", 1).Text() == "[\xC3\x89"
                                                                              "CHAP] Passer");
    REQUIRE(ResolveSkipHint(cCatalog, std::string(4097, 'x'), "FRENCH", 1).Action == "Passer");
    const auto incomplete = cCatalog + "[fr]\nSkipAction=<<<END\ninvalid\nmultiline\nEND\nKey.1=" + std::string(129, 'x') + "\n";
    REQUIRE(ResolveSkipHint(incomplete, "", "FRENCH", 1).Text() == "[ESC] Skip");
}
