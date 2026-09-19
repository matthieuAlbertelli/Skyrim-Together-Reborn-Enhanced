#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>

#include <optional>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <CharacterCreation/CharacterBuildCatalog.h>

#include <Messages/CharacterBuildRequest.h>
#include <Messages/CharacterBuildAppliedRequest.h>
#include <Messages/CharacterBuildResponse.h>
#include <Messages/ClientMessageFactory.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <Messages/ServerMessageFactory.h>
#include <Structs/CharacterBuild.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace TiltedPhoques;

namespace
{
using SelectionMap = std::map<std::string, std::string>;

SelectionMap MakeMageSelections(
    const char* apDestruction,
    const char* apAlteration)
{
    return SelectionMap{
        {"mage.destruction", apDestruction},
        {"mage.alteration", apAlteration},
        {"mage.conjuration", "mage.conjuration.daedric"},
        {"mage.illusion", "mage.illusion.pacification"},
        {"mage.restoration", "mage.restoration.healing"},
        {"mage.enchanting", "mage.enchanting.weapons"},
    };
}

const STRE::CharacterCreation::ItemGrant* FindItemGrant(
    const std::vector<STRE::CharacterCreation::ItemGrant>& acGrants,
    const char* apPluginName,
    std::uint32_t aLocalFormId)
{
    const auto it = std::find_if(
        acGrants.begin(),
        acGrants.end(),
        [apPluginName, aLocalFormId](
            const STRE::CharacterCreation::ItemGrant& acGrant)
        {
            return std::string(acGrant.PluginName) == apPluginName &&
                acGrant.LocalFormId == aLocalFormId;
        });

    return it == acGrants.end() ? nullptr : &*it;
}

const STRE::CharacterCreation::EquipmentGrant* FindEquipmentGrant(
    const std::vector<STRE::CharacterCreation::EquipmentGrant>& acGrants,
    const char* apPluginName,
    std::uint32_t aLocalFormId)
{
    const auto it = std::find_if(
        acGrants.begin(),
        acGrants.end(),
        [apPluginName, aLocalFormId](
            const STRE::CharacterCreation::EquipmentGrant& acGrant)
        {
            return std::string(acGrant.PluginName) == apPluginName &&
                acGrant.LocalFormId == aLocalFormId;
        });

    return it == acGrants.end() ? nullptr : &*it;
}

using ItemKey = std::pair<std::string, std::uint32_t>;
using ItemPlan = std::map<ItemKey, std::int32_t>;

using EquipmentKey =
    std::pair<ItemKey, STRE::CharacterCreation::EquipmentSide>;
using EquipmentPlan = std::map<EquipmentKey, std::int32_t>;

ItemPlan ToItemPlan(
    const std::vector<STRE::CharacterCreation::ItemGrant>& acGrants)
{
    ItemPlan result;
    for (const auto& grant : acGrants)
        result[{grant.PluginName, grant.LocalFormId}] += grant.Count;
    return result;
}

EquipmentPlan ToEquipmentPlan(
    const std::vector<STRE::CharacterCreation::EquipmentGrant>& acGrants)
{
    EquipmentPlan result;
    for (const auto& grant : acGrants)
    {
        result[{{grant.PluginName, grant.LocalFormId}, grant.Side}] +=
            grant.Count;
    }
    return result;
}

template <class TMessage>
TMessage RoundTripClientMessage(const TMessage& acMessage)
{
    TiltedPhoques::Buffer buffer(4096);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    acMessage.Serialize(writer);

    TiltedPhoques::Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto pMessage = factory.Extract(reader);
    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == acMessage.GetOpcode());

    auto pTyped = TiltedPhoques::CastUnique<TMessage>(
        std::move(pMessage));
    REQUIRE(pTyped);
    return *pTyped;
}

template <class TMessage>
TMessage RoundTripServerMessage(const TMessage& acMessage)
{
    TiltedPhoques::Buffer buffer(4096);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    acMessage.Serialize(writer);

    TiltedPhoques::Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto pMessage = factory.Extract(reader);
    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == acMessage.GetOpcode());

    auto pTyped = TiltedPhoques::CastUnique<TMessage>(
        std::move(pMessage));
    REQUIRE(pTyped);
    return *pTyped;
}
}

TEST_CASE("Character build catalog validates the current class boundary", "[character-build.catalog]")
{
    REQUIRE(STRE::CharacterCreation::kCharacterBuildVersion == 5);
    REQUIRE(STRE::CharacterCreation::IsSupportedClassId("class.warrior"));
    REQUIRE(STRE::CharacterCreation::IsSupportedClassId("class.mage"));
    REQUIRE(STRE::CharacterCreation::IsSupportedClassId("class.thief"));
    REQUIRE_FALSE(STRE::CharacterCreation::IsSupportedClassId("class.archer"));
    REQUIRE_FALSE(STRE::CharacterCreation::IsSupportedClassId(""));
}

TEST_CASE("Character build catalog rejects incomplete or inactive selections", "[character-build.catalog]")
{
    SelectionMap incompleteWarrior{
        {"warrior.parade", "warrior.parade.hide_shield"},
    };
    REQUIRE_FALSE(
        STRE::CharacterCreation::ValidateSelections(
            "class.warrior",
            incompleteWarrior));

    SelectionMap steelWarrior{
        {"warrior.parade", "warrior.parade.hide_shield"},
        {"warrior.two_handed", "warrior.two_handed.greatsword"},
        {"warrior.archery", "warrior.archery.bow"},
        {"warrior.smithing", "warrior.smithing.light_leather"},
        {"warrior.one_handed_mode", "warrior.one_handed_mode.steel"},
        {"warrior.one_handed_steel", "warrior.one_handed_steel.dagger"},
    };
    REQUIRE(
        STRE::CharacterCreation::ValidateSelections(
            "class.warrior",
            steelWarrior));
    steelWarrior.emplace(
        "warrior.one_handed_iron_main",
        "warrior.one_handed_iron_main.sword");
    REQUIRE_FALSE(
        STRE::CharacterCreation::ValidateSelections(
            "class.warrior",
            steelWarrior));

    SelectionMap incompleteMage = MakeMageSelections(
        "mage.destruction.fire",
        "mage.alteration.protection");
    incompleteMage.erase("mage.enchanting");
    REQUIRE_FALSE(
        STRE::CharacterCreation::ValidateSelections(
            "class.mage",
            incompleteMage));

    SelectionMap invalidThief{
        {"thief.one_handed_mode", "thief.one_handed_mode.steel"},
        {"thief.one_handed_steel", "thief.one_handed_steel.dagger"},
        {"thief.lockpicking", "thief.lockpicking.toolkit"},
    };
    REQUIRE_FALSE(
        STRE::CharacterCreation::ValidateSelections(
            "class.thief",
            invalidThief));
}

TEST_CASE("Character build catalog exhaustively resolves Warrior paths", "[character-build.catalog]")
{
    struct ItemOption
    {
        const char* Id;
        ItemPlan Items;
    };

    const std::array paradeOptions{
        ItemOption{"warrior.parade.hide_shield", {{{"Skyrim.esm", 0x00013914}, 1}}},
        ItemOption{"warrior.parade.iron_shield", {{{"Skyrim.esm", 0x00012EB6}, 1}}},
        ItemOption{"warrior.parade.guard_pendant", {{{"STRE_AlternateStart.esp", 0x00003B41}, 1}}},
    };
    const std::array twoHandedOptions{
        ItemOption{"warrior.two_handed.greatsword", {{{"Skyrim.esm", 0x0001359D}, 1}}},
        ItemOption{"warrior.two_handed.battleaxe", {{{"Skyrim.esm", 0x00013980}, 1}}},
        ItemOption{"warrior.two_handed.warhammer", {{{"Skyrim.esm", 0x00013981}, 1}}},
    };
    const std::array archeryOptions{
        ItemOption{"warrior.archery.bow", {{{"Skyrim.esm", 0x00013985}, 1}, {{"Skyrim.esm", 0x0001397D}, 50}}},
        ItemOption{"warrior.archery.crossbow", {{{"Dawnguard.esm", 0x00000801}, 1}, {{"Dawnguard.esm", 0x00000BB3}, 50}}},
    };
    const std::array smithingOptions{
        ItemOption{"warrior.smithing.light_leather", {{{"Skyrim.esm", 0x000DB5D2}, 4}, {{"Skyrim.esm", 0x000800E4}, 8}}},
        ItemOption{"warrior.smithing.light_mixed", {{{"Skyrim.esm", 0x000DB5D2}, 3}, {{"Skyrim.esm", 0x000800E4}, 6}, {{"Skyrim.esm", 0x0005ACE4}, 2}}},
        ItemOption{"warrior.smithing.heavy_iron", {{{"Skyrim.esm", 0x0005ACE4}, 4}, {{"Skyrim.esm", 0x000800E4}, 4}}},
        ItemOption{"warrior.smithing.heavy_mixed", {{{"Skyrim.esm", 0x0005ACE4}, 2}, {{"Skyrim.esm", 0x0005AD93}, 2}, {{"Skyrim.esm", 0x000800E4}, 4}}},
        ItemOption{"warrior.smithing.maintenance", {{{"Skyrim.esm", 0x0005ACE4}, 2}, {{"Skyrim.esm", 0x0005ACE5}, 1}, {{"Skyrim.esm", 0x000DB5D2}, 2}, {{"Skyrim.esm", 0x000800E4}, 4}}},
    };
    const std::array steelWeapons{
        ItemOption{"warrior.one_handed_steel.dagger", {{{"Skyrim.esm", 0x00013986}, 1}}},
        ItemOption{"warrior.one_handed_steel.sword", {{{"Skyrim.esm", 0x00013989}, 1}}},
        ItemOption{"warrior.one_handed_steel.war_axe", {{{"Skyrim.esm", 0x00013983}, 1}}},
        ItemOption{"warrior.one_handed_steel.mace", {{{"Skyrim.esm", 0x00013988}, 1}}},
    };
    const std::array ironWeapons{
        ItemOption{"dagger", {{{"Skyrim.esm", 0x0001397E}, 1}}},
        ItemOption{"sword", {{{"Skyrim.esm", 0x00012EB7}, 1}}},
        ItemOption{"war_axe", {{{"Skyrim.esm", 0x00013790}, 1}}},
        ItemOption{"mace", {{{"Skyrim.esm", 0x00013982}, 1}}},
    };

    const ItemPlan baseItems{
        {{"Skyrim.esm", 0x00012E49}, 1},
        {{"Skyrim.esm", 0x00012E4B}, 1},
        {{"Skyrim.esm", 0x00012E46}, 1},
        {{"STRE_AlternateStart.esp", 0x00003B5D}, 1},
        {{"STRE_AlternateStart.esp", 0x00003B60}, 1},
    };
    const EquipmentPlan baseEquipment{
        {{{"Skyrim.esm", 0x00012E49}, STRE::CharacterCreation::EquipmentSide::Right}, 1},
        {{{"Skyrim.esm", 0x00012E4B}, STRE::CharacterCreation::EquipmentSide::Right}, 1},
        {{{"Skyrim.esm", 0x00012E46}, STRE::CharacterCreation::EquipmentSide::Right}, 1},
    };

    std::size_t validatedPaths = 0;
    const auto validatePath = [&](const SelectionMap& acSelections,
                                  ItemPlan aExpectedItems,
                                  EquipmentPlan aExpectedEquipment)
    {
        REQUIRE(
            STRE::CharacterCreation::ValidateSelections(
                "class.warrior",
                acSelections));
        REQUIRE(
            ToItemPlan(
                STRE::CharacterCreation::BuildItemGrants(
                    "class.warrior",
                    acSelections)) == aExpectedItems);
        REQUIRE(
            ToEquipmentPlan(
                STRE::CharacterCreation::BuildEquipmentGrants(
                    "class.warrior",
                    acSelections)) == aExpectedEquipment);
        REQUIRE(
            ToItemPlan(
                STRE::CharacterCreation::BuildItemGrants(
                    "class.warrior",
                    acSelections)) == aExpectedItems);
        ++validatedPaths;
    };

    for (const auto& parade : paradeOptions)
    {
        for (const auto& twoHanded : twoHandedOptions)
        {
            for (const auto& archery : archeryOptions)
            {
                for (const auto& smithing : smithingOptions)
                {
                    for (const auto& steel : steelWeapons)
                    {
                        SelectionMap selections{
                            {"warrior.parade", parade.Id},
                            {"warrior.two_handed", twoHanded.Id},
                            {"warrior.archery", archery.Id},
                            {"warrior.smithing", smithing.Id},
                            {"warrior.one_handed_mode", "warrior.one_handed_mode.steel"},
                            {"warrior.one_handed_steel", steel.Id},
                        };
                        ItemPlan expectedItems = baseItems;
                        for (const ItemPlan* plan : {&parade.Items, &twoHanded.Items, &archery.Items, &smithing.Items, &steel.Items})
                            for (const auto& [key, count] : *plan)
                                expectedItems[key] += count;

                        EquipmentPlan expectedEquipment = baseEquipment;
                        const auto paradeSide =
                            std::string(parade.Id) == "warrior.parade.guard_pendant"
                            ? STRE::CharacterCreation::EquipmentSide::Right
                            : STRE::CharacterCreation::EquipmentSide::Left;
                        for (const auto& [key, count] : parade.Items)
                            expectedEquipment[{key, paradeSide}] += count;
                        const ItemKey rangedAmmo =
                            std::string(archery.Id) == "warrior.archery.bow"
                            ? ItemKey{"Skyrim.esm", 0x0001397D}
                            : ItemKey{"Dawnguard.esm", 0x00000BB3};
                        expectedEquipment[{rangedAmmo, STRE::CharacterCreation::EquipmentSide::Right}] += 50;
                        for (const auto& [key, count] : steel.Items)
                            expectedEquipment[{key, STRE::CharacterCreation::EquipmentSide::Right}] += count;
                        validatePath(selections, expectedItems, expectedEquipment);
                    }

                    for (const auto& main : ironWeapons)
                    {
                        for (const auto& off : ironWeapons)
                        {
                            SelectionMap selections{
                                {"warrior.parade", parade.Id},
                                {"warrior.two_handed", twoHanded.Id},
                                {"warrior.archery", archery.Id},
                                {"warrior.smithing", smithing.Id},
                                {"warrior.one_handed_mode", "warrior.one_handed_mode.dual_iron"},
                                {"warrior.one_handed_iron_main", std::string("warrior.one_handed_iron_main.") + main.Id},
                                {"warrior.one_handed_iron_off", std::string("warrior.one_handed_iron_off.") + off.Id},
                            };
                            ItemPlan expectedItems = baseItems;
                            for (const ItemPlan* plan : {&parade.Items, &twoHanded.Items, &archery.Items, &smithing.Items, &main.Items, &off.Items})
                                for (const auto& [key, count] : *plan)
                                    expectedItems[key] += count;

                            EquipmentPlan expectedEquipment = baseEquipment;
                            if (std::string(parade.Id) == "warrior.parade.guard_pendant")
                            {
                                for (const auto& [key, count] : parade.Items)
                                    expectedEquipment[{key, STRE::CharacterCreation::EquipmentSide::Right}] += count;
                            }
                            const ItemKey rangedAmmo =
                                std::string(archery.Id) == "warrior.archery.bow"
                                ? ItemKey{"Skyrim.esm", 0x0001397D}
                                : ItemKey{"Dawnguard.esm", 0x00000BB3};
                            expectedEquipment[{rangedAmmo, STRE::CharacterCreation::EquipmentSide::Right}] += 50;
                            for (const auto& [key, count] : main.Items)
                                expectedEquipment[{key, STRE::CharacterCreation::EquipmentSide::Right}] += count;
                            for (const auto& [key, count] : off.Items)
                                expectedEquipment[{key, STRE::CharacterCreation::EquipmentSide::Left}] += count;
                            validatePath(selections, expectedItems, expectedEquipment);
                        }
                    }
                }
            }
        }
    }

    REQUIRE(validatedPaths == 1800);
}

TEST_CASE("Character build catalog exhaustively resolves Mage selections", "[character-build.catalog]")
{
    constexpr const char* kDestructionOptions[]{
        "mage.destruction.fire",
        "mage.destruction.frost",
        "mage.destruction.shock",
    };
    constexpr const char* kAlterationOptions[]{
        "mage.alteration.protection",
        "mage.alteration.exploration",
        "mage.alteration.matter",
    };
    constexpr const char* kConjurationOptions[]{
        "mage.conjuration.daedric",
        "mage.conjuration.necromancy",
        "mage.conjuration.bound",
    };
    constexpr const char* kIllusionOptions[]{
        "mage.illusion.pacification",
        "mage.illusion.discord",
        "mage.illusion.shadows",
    };
    constexpr const char* kRestorationOptions[]{
        "mage.restoration.healing",
        "mage.restoration.wards",
        "mage.restoration.sacred",
    };
    constexpr const char* kEnchantingOptions[]{
        "mage.enchanting.weapons",
        "mage.enchanting.body",
        "mage.enchanting.souls",
    };

    const ItemPlan expectedItems{
        {{"STRE_AlternateStart.esp", 0x00003B6E}, 1},
        {{"STRE_AlternateStart.esp", 0x00003B70}, 1},
    };
    const EquipmentPlan expectedEquipment{
        {{{"STRE_AlternateStart.esp", 0x00003B6E}, STRE::CharacterCreation::EquipmentSide::Right}, 1},
        {{{"STRE_AlternateStart.esp", 0x00003B70}, STRE::CharacterCreation::EquipmentSide::Right}, 1},
    };

    std::size_t validatedPaths = 0;
    for (const char* destruction : kDestructionOptions)
    for (const char* alteration : kAlterationOptions)
    for (const char* conjuration : kConjurationOptions)
    for (const char* illusion : kIllusionOptions)
    for (const char* restoration : kRestorationOptions)
    for (const char* enchanting : kEnchantingOptions)
    {
        const SelectionMap selections{
            {"mage.destruction", destruction},
            {"mage.alteration", alteration},
            {"mage.conjuration", conjuration},
            {"mage.illusion", illusion},
            {"mage.restoration", restoration},
            {"mage.enchanting", enchanting},
        };
        REQUIRE(
            STRE::CharacterCreation::ValidateSelections(
                "class.mage",
                selections));
        const auto spells =
            STRE::CharacterCreation::BuildSpellGrants(
                "class.mage",
                selections);
        REQUIRE(spells.size() == 7);
        REQUIRE(
            ToItemPlan(
                STRE::CharacterCreation::BuildItemGrants(
                    "class.mage",
                    selections)) == expectedItems);
        REQUIRE(
            ToEquipmentPlan(
                STRE::CharacterCreation::BuildEquipmentGrants(
                    "class.mage",
                    selections)) == expectedEquipment);
        ++validatedPaths;
    }
    REQUIRE(validatedPaths == 729);
}

TEST_CASE("Character build catalog exhaustively resolves Thief paths", "[character-build.catalog]")
{
    struct WeaponOption
    {
        const char* Id;
        ItemKey Item;
    };
    const std::array steelWeapons{
        WeaponOption{"dagger", {"Skyrim.esm", 0x00013986}},
        WeaponOption{"sword", {"Skyrim.esm", 0x00013989}},
        WeaponOption{"war_axe", {"Skyrim.esm", 0x00013983}},
        WeaponOption{"mace", {"Skyrim.esm", 0x00013988}},
    };
    const std::array ironWeapons{
        WeaponOption{"dagger", {"Skyrim.esm", 0x0001397E}},
        WeaponOption{"sword", {"Skyrim.esm", 0x00012EB7}},
        WeaponOption{"war_axe", {"Skyrim.esm", 0x00013790}},
        WeaponOption{"mace", {"Skyrim.esm", 0x00013982}},
    };
    const ItemPlan baseItems{
        {{"Skyrim.esm", 0x00013911}, 1},
        {{"Skyrim.esm", 0x00013910}, 1},
        {{"Skyrim.esm", 0x00013912}, 1},
        {{"Skyrim.esm", 0x0000000A}, 10},
        {{"STRE_AlternateStart.esp", 0x00003B57}, 1},
        {{"STRE_AlternateStart.esp", 0x00003B59}, 1},
        {{"STRE_AlternateStart.esp", 0x00003B4F}, 1},
        {{"STRE_AlternateStart.esp", 0x00003B51}, 1},
        {{"STRE_AlternateStart.esp", 0x00003B43}, 1},
        {{"STRE_AlternateStart.esp", 0x00003B45}, 1},
    };
    const EquipmentPlan baseEquipment{
        {{{"Skyrim.esm", 0x00013911}, STRE::CharacterCreation::EquipmentSide::Right}, 1},
        {{{"Skyrim.esm", 0x00013910}, STRE::CharacterCreation::EquipmentSide::Right}, 1},
        {{{"Skyrim.esm", 0x00013912}, STRE::CharacterCreation::EquipmentSide::Right}, 1},
    };

    std::size_t validatedPaths = 0;
    for (const auto& weapon : steelWeapons)
    {
        const SelectionMap selections{
            {"thief.one_handed_mode", "thief.one_handed_mode.steel"},
            {"thief.one_handed_steel", std::string("thief.one_handed_steel.") + weapon.Id},
        };
        ItemPlan expectedItems = baseItems;
        expectedItems[weapon.Item] += 1;
        EquipmentPlan expectedEquipment = baseEquipment;
        expectedEquipment[{weapon.Item, STRE::CharacterCreation::EquipmentSide::Right}] += 1;
        REQUIRE(STRE::CharacterCreation::ValidateSelections("class.thief", selections));
        REQUIRE(ToItemPlan(STRE::CharacterCreation::BuildItemGrants("class.thief", selections)) == expectedItems);
        REQUIRE(ToEquipmentPlan(STRE::CharacterCreation::BuildEquipmentGrants("class.thief", selections)) == expectedEquipment);
        ++validatedPaths;
    }

    for (const auto& main : ironWeapons)
    for (const auto& off : ironWeapons)
    {
        const SelectionMap selections{
            {"thief.one_handed_mode", "thief.one_handed_mode.dual_iron"},
            {"thief.one_handed_iron_main", std::string("thief.one_handed_iron_main.") + main.Id},
            {"thief.one_handed_iron_off", std::string("thief.one_handed_iron_off.") + off.Id},
        };
        ItemPlan expectedItems = baseItems;
        expectedItems[main.Item] += 1;
        expectedItems[off.Item] += 1;
        EquipmentPlan expectedEquipment = baseEquipment;
        expectedEquipment[{main.Item, STRE::CharacterCreation::EquipmentSide::Right}] += 1;
        expectedEquipment[{off.Item, STRE::CharacterCreation::EquipmentSide::Left}] += 1;
        REQUIRE(STRE::CharacterCreation::ValidateSelections("class.thief", selections));
        REQUIRE(ToItemPlan(STRE::CharacterCreation::BuildItemGrants("class.thief", selections)) == expectedItems);
        REQUIRE(ToEquipmentPlan(STRE::CharacterCreation::BuildEquipmentGrants("class.thief", selections)) == expectedEquipment);
        ++validatedPaths;
    }

    REQUIRE(validatedPaths == 20);
}

TEST_CASE("Character build catalog grants authored spells", "[character-build.catalog]")
{
    constexpr const char* kDestructionOptions[]{
        "mage.destruction.fire",
        "mage.destruction.frost",
        "mage.destruction.shock",
    };
    constexpr const char* kAlterationOptions[]{
        "mage.alteration.protection",
        "mage.alteration.exploration",
        "mage.alteration.matter",
    };

    for (const char* const pDestruction : kDestructionOptions)
    {
        for (const char* const pAlteration : kAlterationOptions)
        {
            INFO("destruction=" << pDestruction);
            INFO("alteration=" << pAlteration);

            const SelectionMap selections =
                MakeMageSelections(pDestruction, pAlteration);
            REQUIRE(
                STRE::CharacterCreation::ValidateSelections(
                    "class.mage",
                    selections));

            const auto grants =
                STRE::CharacterCreation::BuildSpellGrants(
                    "class.mage",
                    selections);
            REQUIRE(grants.size() == 7);

            std::set<std::pair<std::string, std::uint32_t>> unique;
            for (const auto& grant : grants)
            {
                REQUIRE(grant.PluginName != nullptr);
                REQUIRE(grant.LocalFormId != 0);
                unique.emplace(grant.PluginName, grant.LocalFormId);
            }
            REQUIRE(unique.size() == grants.size());

            std::set<std::pair<std::string, std::uint32_t>> expected;
            if (std::string(pDestruction) == "mage.destruction.fire")
            {
                expected.emplace("Skyrim.esm", 0x00012FCD);
                expected.emplace("STRE_AlternateStart.esp", 0x000040DA);
                expected.emplace("STRE_AlternateStart.esp", 0x000040DE);
            }
            else if (std::string(pDestruction) ==
                     "mage.destruction.frost")
            {
                expected.emplace("STRE_AlternateStart.esp", 0x000040E2);
                expected.emplace("STRE_AlternateStart.esp", 0x000040EA);
                expected.emplace("STRE_AlternateStart.esp", 0x000040E6);
            }
            else
            {
                expected.emplace("STRE_AlternateStart.esp", 0x000040EE);
                expected.emplace("STRE_AlternateStart.esp", 0x000040FA);
                expected.emplace("STRE_AlternateStart.esp", 0x000040F6);
            }

            if (std::string(pAlteration) ==
                "mage.alteration.protection")
            {
                expected.emplace("STRE_AlternateStart.esp", 0x000040FE);
                expected.emplace("STRE_AlternateStart.esp", 0x00004102);
                expected.emplace("STRE_AlternateStart.esp", 0x00006FCD);
                expected.emplace("STRE_AlternateStart.esp", 0x00006FD1);
            }
            else if (std::string(pAlteration) ==
                     "mage.alteration.exploration")
            {
                expected.emplace("STRE_AlternateStart.esp", 0x00006FD8);
                expected.emplace("STRE_AlternateStart.esp", 0x00006FDC);
                expected.emplace("STRE_AlternateStart.esp", 0x00006FE0);
                expected.emplace("STRE_AlternateStart.esp", 0x00006FE6);
            }
            else
            {
                expected.emplace("STRE_AlternateStart.esp", 0x00006FEA);
                expected.emplace("STRE_AlternateStart.esp", 0x00006FEE);
                expected.emplace("STRE_AlternateStart.esp", 0x00006FF2);
                expected.emplace("STRE_AlternateStart.esp", 0x00006FF8);
            }

            REQUIRE(unique == expected);
        }
    }
}

TEST_CASE("Character build catalog grants authored class attire", "[character-build.catalog]")
{
    const SelectionMap warriorSelections{
        {"warrior.parade", "warrior.parade.guard_pendant"},
        {"warrior.two_handed", "warrior.two_handed.greatsword"},
        {"warrior.archery", "warrior.archery.bow"},
        {"warrior.smithing", "warrior.smithing.light_leather"},
        {"warrior.one_handed_mode", "warrior.one_handed_mode.steel"},
        {"warrior.one_handed_steel", "warrior.one_handed_steel.sword"},
    };

    REQUIRE(
        STRE::CharacterCreation::ValidateSelections(
            "class.warrior",
            warriorSelections));

    const auto warriorItems =
        STRE::CharacterCreation::BuildItemGrants(
            "class.warrior",
            warriorSelections);
    REQUIRE(FindItemGrant(
        warriorItems, "STRE_AlternateStart.esp", 0x00003B41));
    REQUIRE(FindItemGrant(
        warriorItems, "STRE_AlternateStart.esp", 0x00003B5D));
    REQUIRE(FindItemGrant(
        warriorItems, "STRE_AlternateStart.esp", 0x00003B60));

    const auto warriorEquipment =
        STRE::CharacterCreation::BuildEquipmentGrants(
            "class.warrior",
            warriorSelections);
    REQUIRE(FindEquipmentGrant(
        warriorEquipment, "STRE_AlternateStart.esp", 0x00003B41));

    const SelectionMap mageSelections = MakeMageSelections(
        "mage.destruction.fire",
        "mage.alteration.protection");
    const auto mageItems =
        STRE::CharacterCreation::BuildItemGrants(
            "class.mage",
            mageSelections);
    REQUIRE(FindItemGrant(
        mageItems, "STRE_AlternateStart.esp", 0x00003B6E));
    REQUIRE(FindItemGrant(
        mageItems, "STRE_AlternateStart.esp", 0x00003B70));

    const auto mageEquipment =
        STRE::CharacterCreation::BuildEquipmentGrants(
            "class.mage",
            mageSelections);
    REQUIRE(FindEquipmentGrant(
        mageEquipment, "STRE_AlternateStart.esp", 0x00003B6E));
    REQUIRE(FindEquipmentGrant(
        mageEquipment, "STRE_AlternateStart.esp", 0x00003B70));
}

TEST_CASE("Character build catalog uses simplified thief utility rewards", "[character-build.catalog]")
{
    const SelectionMap selections{
        {"thief.one_handed_mode", "thief.one_handed_mode.steel"},
        {"thief.one_handed_steel", "thief.one_handed_steel.dagger"},
    };

    REQUIRE(
        STRE::CharacterCreation::ValidateSelections(
            "class.thief",
            selections));
    REQUIRE_FALSE(
        STRE::CharacterCreation::IsSupportedLoadoutOption(
            "class.thief",
            "thief.lockpicking",
            "thief.lockpicking.toolkit"));
    REQUIRE_FALSE(
        STRE::CharacterCreation::IsSupportedLoadoutOption(
            "class.thief",
            "thief.sneak",
            "thief.sneak.nature"));

    const auto grants =
        STRE::CharacterCreation::BuildItemGrants(
            "class.thief",
            selections);

    const auto* const pLockpicks =
        FindItemGrant(grants, "Skyrim.esm", 0x0000000A);
    REQUIRE(pLockpicks != nullptr);
    REQUIRE(pLockpicks->Count == 10);

    constexpr std::uint32_t kExpectedStreOutfitForms[]{
        0x00003B57,
        0x00003B59,
        0x00003B4F,
        0x00003B51,
        0x00003B43,
        0x00003B45,
    };
    for (const std::uint32_t formId : kExpectedStreOutfitForms)
    {
        const auto* const pGrant =
            FindItemGrant(
                grants,
                "STRE_AlternateStart.esp",
                formId);
        REQUIRE(pGrant != nullptr);
        REQUIRE(pGrant->Count == 1);
    }
}

TEST_CASE("Character spell hash is normalized", "[character-build.hash]")
{
    Vector<GameId> first{
        GameId{3, 0x000040DA},
        GameId{3, 0x00006FD1},
        GameId{0, 0x00012FCD},
        GameId{3, 0x000040DA},
    };
    Vector<GameId> reordered{
        GameId{0, 0x00012FCD},
        GameId{3, 0x000040DA},
        GameId{3, 0x00006FD1},
    };
    Vector<GameId> changed{
        GameId{0, 0x00012FCD},
        GameId{3, 0x000040DA},
        GameId{3, 0x00006FD8},
    };

    REQUIRE(
        ComputeCharacterBuildSpellHash(first) ==
        ComputeCharacterBuildSpellHash(reordered));
    REQUIRE(
        ComputeCharacterBuildSpellHash(first) !=
        ComputeCharacterBuildSpellHash(changed));
}

TEST_CASE("Character inventory hash is normalized", "[character-build.hash]")
{
    Inventory first;
    first.Entries.push_back(
        Inventory::Entry{GameId{3, 0x00003B41}, 1});
    first.Entries.push_back(
        Inventory::Entry{GameId{0, 0x0001397D}, 20});
    first.Entries.push_back(
        Inventory::Entry{GameId{0, 0x0001397D}, 30});

    Inventory reordered;
    reordered.Entries.push_back(
        Inventory::Entry{GameId{0, 0x0001397D}, 50});
    reordered.Entries.push_back(
        Inventory::Entry{GameId{7, 0x00000001}, 0});
    reordered.Entries.push_back(
        Inventory::Entry{GameId{3, 0x00003B41}, 1});
    reordered.Entries.push_back(
        Inventory::Entry{GameId{7, 0x00000002}, -1});

    Inventory changed = reordered;
    changed.Entries[0].Count = 49;

    REQUIRE(
        ComputeCharacterBuildInventoryHash(first) ==
        ComputeCharacterBuildInventoryHash(reordered));
    REQUIRE(
        ComputeCharacterBuildInventoryHash(first) !=
        ComputeCharacterBuildInventoryHash(changed));
}

TEST_CASE("Character build snapshot serializes canonical spells", "[character-build.encoding]")
{
    CharacterBuildSnapshotData sent;
    sent.BuildVersion = STRE::CharacterCreation::kCharacterBuildVersion;
    sent.RaceId = GameId{0, 0x00013746};
    sent.ClassId = "class.mage";
    sent.Selections.push_back(
        CharacterBuildSelectionData{
            "mage.destruction",
            "mage.destruction.fire"});
    sent.CanonicalSpells.push_back(GameId{0, 0x00012FCD});
    sent.CanonicalSpells.push_back(GameId{3, 0x000040DA});
    sent.SpellHash =
        ComputeCharacterBuildSpellHash(sent.CanonicalSpells);

    TiltedPhoques::Buffer buffer(4096);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    sent.Serialize(writer);

    CharacterBuildSnapshotData received;
    TiltedPhoques::Buffer::Reader reader(&buffer);
    received.Deserialize(reader);

    REQUIRE(received == sent);
}

TEST_CASE("Character build applied acknowledgement serializes spell hash", "[character-build.encoding]")
{
    CharacterBuildAppliedRequest sent;
    sent.Revision = 42;
    sent.InventoryHash = 0x1122334455667788ull;
    sent.SpellHash = 0x8877665544332211ull;

    TiltedPhoques::Buffer buffer(1024);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    sent.Serialize(writer);

    TiltedPhoques::Buffer::Reader reader(&buffer);
    std::uint64_t opcode{};
    reader.ReadBits(opcode, sizeof(ClientOpcode) * 8);
    REQUIRE(static_cast<ClientOpcode>(opcode) == sent.GetOpcode());

    CharacterBuildAppliedRequest received;
    received.DeserializeRaw(reader);
    REQUIRE(received.Revision == sent.Revision);
    REQUIRE(received.InventoryHash == sent.InventoryHash);
    REQUIRE(received.SpellHash == sent.SpellHash);
}

TEST_CASE("Character build messages round-trip through protocol factories", "[character-build.protocol]")
{
    CharacterBuildSnapshotData snapshot;
    snapshot.BuildVersion = STRE::CharacterCreation::kCharacterBuildVersion;
    snapshot.RaceId = GameId{0, 0x00013746};
    snapshot.ClassId = "class.mage";
    snapshot.Selections.push_back(
        CharacterBuildSelectionData{
            "mage.destruction",
            "mage.destruction.fire"});
    snapshot.Selections.push_back(
        CharacterBuildSelectionData{
            "mage.alteration",
            "mage.alteration.protection"});
    snapshot.CanonicalInventory.Entries.push_back(
        Inventory::Entry{GameId{3, 0x00003B6E}, 1});
    snapshot.CanonicalInventory.Entries.push_back(
        Inventory::Entry{GameId{3, 0x00003B70}, 1});
    snapshot.InventoryHash = ComputeCharacterBuildInventoryHash(
        snapshot.CanonicalInventory);
    snapshot.CanonicalSpells.push_back(GameId{0, 0x00012FCD});
    snapshot.CanonicalSpells.push_back(GameId{3, 0x000040DA});
    snapshot.CanonicalSpells.push_back(GameId{3, 0x000040DE});
    snapshot.CanonicalSpells.push_back(GameId{3, 0x000040FE});
    snapshot.CanonicalSpells.push_back(GameId{3, 0x00004102});
    snapshot.CanonicalSpells.push_back(GameId{3, 0x00006FCD});
    snapshot.CanonicalSpells.push_back(GameId{3, 0x00006FD1});
    snapshot.SpellHash = ComputeCharacterBuildSpellHash(
        snapshot.CanonicalSpells);

    CharacterBuildRequest request;
    request.BuildVersion = snapshot.BuildVersion;
    request.RaceId = snapshot.RaceId;
    request.ClassId = snapshot.ClassId;
    request.Selections = snapshot.Selections;
    REQUIRE(RoundTripClientMessage(request) == request);

    CharacterBuildResponse response;
    response.Result = CharacterBuildResult::Accepted;
    response.Revision = 42;
    response.ServerId = 7;
    response.Build = snapshot;
    const CharacterBuildResponse receivedResponse =
        RoundTripServerMessage(response);
    REQUIRE(receivedResponse.Result == response.Result);
    REQUIRE(receivedResponse.Revision == response.Revision);
    REQUIRE(receivedResponse.ServerId == response.ServerId);
    REQUIRE(receivedResponse.Build == response.Build);

    NotifyCharacterBuildState notification;
    notification.State = CharacterBuildNetworkState::Applied;
    notification.PlayerId = 11;
    notification.ServerId = 7;
    notification.Revision = 42;
    notification.Build = snapshot;
    const NotifyCharacterBuildState receivedNotification =
        RoundTripServerMessage(notification);
    REQUIRE(receivedNotification.State == notification.State);
    REQUIRE(receivedNotification.PlayerId == notification.PlayerId);
    REQUIRE(receivedNotification.ServerId == notification.ServerId);
    REQUIRE(receivedNotification.Revision == notification.Revision);
    REQUIRE(receivedNotification.Build == notification.Build);

    CharacterBuildAppliedRequest acknowledgement;
    acknowledgement.Revision = 42;
    acknowledgement.InventoryHash = snapshot.InventoryHash;
    acknowledgement.SpellHash = snapshot.SpellHash;
    const CharacterBuildAppliedRequest receivedAcknowledgement =
        RoundTripClientMessage(acknowledgement);
    REQUIRE(receivedAcknowledgement.Revision == acknowledgement.Revision);
    REQUIRE(
        receivedAcknowledgement.InventoryHash ==
        acknowledgement.InventoryHash);
    REQUIRE(
        receivedAcknowledgement.SpellHash ==
        acknowledgement.SpellHash);

    CharacterBuildResponse rejection;
    rejection.Result = CharacterBuildResult::RejectedSpellHash;
    const CharacterBuildResponse receivedRejection =
        RoundTripServerMessage(rejection);
    REQUIRE(receivedRejection.Result == rejection.Result);
}
