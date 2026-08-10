#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>

#include <optional>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <CharacterCreation/CharacterBuildCatalog.h>

#include <Messages/CharacterBuildAppliedRequest.h>
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
