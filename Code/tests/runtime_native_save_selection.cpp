#include <catch2/catch.hpp>
#include <Runtime/NativeSaveSelection.h>

#include <limits>
#include <string_view>

using namespace STRE::Runtime;

TEST_CASE("Continue selects the remembered filename independently of the save list", "[runtime-port][native-contracts][save-selection]")
{
    const char* entries[] = {"ordinary-first", "campaign-last"};
    NativeSaveSelectionView manager{};
    manager.List = {entries, 2, 0, 2, 0};
    manager.pRememberedFileName = "campaign-committed";
    manager.RememberedNameLength = 18;
    REQUIRE(std::string_view(MostRecentSaveName(&manager)) == "campaign-committed");
    REQUIRE(std::string_view(entries[*NativeSaveListIndex(manager.List, 0)]) == "campaign-last");
    REQUIRE(std::string_view(entries[*NativeSaveListIndex(manager.List, 1)]) == "ordinary-first");

    // Native Continue does not require a populated Journal list.
    manager.List = {};
    REQUIRE(std::string_view(MostRecentSaveName(&manager)) == "campaign-committed");
}

TEST_CASE("An absent native remembered name cannot select an arbitrary list entry", "[runtime-port][native-contracts][save-selection]")
{
    const char* entries[] = {"campaign-first"};
    NativeSaveSelectionView manager{};
    manager.List = {entries, 1, 0, 1, 0};
    manager.pRememberedFileName = "stale";
    REQUIRE(MostRecentSaveName(&manager) == nullptr);
    manager.RememberedNameLength = 5;
    manager.pRememberedFileName = nullptr;
    REQUIRE(MostRecentSaveName(&manager) == nullptr);
    REQUIRE(MostRecentSaveName(nullptr) == nullptr);
}

TEST_CASE("Manual selection rejects corrupt arrays and out of range UI indices", "[runtime-port][native-contracts][save-selection]")
{
    const char* entries[] = {"first", "second", "third"};
    NativeSaveListView list{entries, 3, 0, 3, 0};
    for (std::uint32_t i = 0; i < 3; ++i)
        REQUIRE(NativeSaveListIndex(list, i) == 2 - i);
    REQUIRE_FALSE(NativeSaveListIndex(list, 3));
    REQUIRE_FALSE(NativeSaveListIndex(list, std::numeric_limits<std::uint32_t>::max()));
    list.Capacity = 2;
    REQUIRE_FALSE(NativeSaveListIndex(list, 0));
    list.Capacity = 10001;
    REQUIRE_FALSE(NativeSaveListIndex(list, 0));
    list.Capacity = 3;
    list.Size = 0;
    REQUIRE_FALSE(NativeSaveListIndex(list, 0));
    list.Size = 3;
    list.pData = nullptr;
    REQUIRE_FALSE(NativeSaveListIndex(list, 0));
}
