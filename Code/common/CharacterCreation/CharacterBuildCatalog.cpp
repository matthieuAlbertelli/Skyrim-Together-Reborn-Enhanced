#include <CharacterCreation/CharacterBuildCatalog.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace STRE::CharacterCreation
{
namespace
{
inline constexpr char kWarriorClassId[] = "class.warrior";
inline constexpr char kMageClassId[] = "class.mage";
inline constexpr char kThiefClassId[] = "class.thief";

struct LoadoutOptionRule
{
    const char* ClassId;
    const char* GroupId;
    const char* OptionId;
};

constexpr std::array kLoadoutOptionRules{
    LoadoutOptionRule{kWarriorClassId, "warrior.parade", "warrior.parade.hide_shield"},
    LoadoutOptionRule{kWarriorClassId, "warrior.parade", "warrior.parade.iron_shield"},
    LoadoutOptionRule{kWarriorClassId, "warrior.parade", "warrior.parade.guard_pendant"},
    LoadoutOptionRule{kWarriorClassId, "warrior.two_handed", "warrior.two_handed.greatsword"},
    LoadoutOptionRule{kWarriorClassId, "warrior.two_handed", "warrior.two_handed.battleaxe"},
    LoadoutOptionRule{kWarriorClassId, "warrior.two_handed", "warrior.two_handed.warhammer"},
    LoadoutOptionRule{kWarriorClassId, "warrior.archery", "warrior.archery.bow"},
    LoadoutOptionRule{kWarriorClassId, "warrior.archery", "warrior.archery.crossbow"},
    LoadoutOptionRule{kWarriorClassId, "warrior.smithing", "warrior.smithing.light_leather"},
    LoadoutOptionRule{kWarriorClassId, "warrior.smithing", "warrior.smithing.light_mixed"},
    LoadoutOptionRule{kWarriorClassId, "warrior.smithing", "warrior.smithing.heavy_iron"},
    LoadoutOptionRule{kWarriorClassId, "warrior.smithing", "warrior.smithing.heavy_mixed"},
    LoadoutOptionRule{kWarriorClassId, "warrior.smithing", "warrior.smithing.maintenance"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_mode", "warrior.one_handed_mode.steel"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_mode", "warrior.one_handed_mode.dual_iron"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_steel", "warrior.one_handed_steel.dagger"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_steel", "warrior.one_handed_steel.sword"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_steel", "warrior.one_handed_steel.war_axe"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_steel", "warrior.one_handed_steel.mace"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_iron_main", "warrior.one_handed_iron_main.dagger"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_iron_main", "warrior.one_handed_iron_main.sword"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_iron_main", "warrior.one_handed_iron_main.war_axe"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_iron_main", "warrior.one_handed_iron_main.mace"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_iron_off", "warrior.one_handed_iron_off.dagger"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_iron_off", "warrior.one_handed_iron_off.sword"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_iron_off", "warrior.one_handed_iron_off.war_axe"},
    LoadoutOptionRule{kWarriorClassId, "warrior.one_handed_iron_off", "warrior.one_handed_iron_off.mace"},

    LoadoutOptionRule{kMageClassId, "mage.destruction", "mage.destruction.fire"},
    LoadoutOptionRule{kMageClassId, "mage.destruction", "mage.destruction.frost"},
    LoadoutOptionRule{kMageClassId, "mage.destruction", "mage.destruction.shock"},
    LoadoutOptionRule{kMageClassId, "mage.alteration", "mage.alteration.protection"},
    LoadoutOptionRule{kMageClassId, "mage.alteration", "mage.alteration.exploration"},
    LoadoutOptionRule{kMageClassId, "mage.alteration", "mage.alteration.matter"},
    LoadoutOptionRule{kMageClassId, "mage.conjuration", "mage.conjuration.daedric"},
    LoadoutOptionRule{kMageClassId, "mage.conjuration", "mage.conjuration.necromancy"},
    LoadoutOptionRule{kMageClassId, "mage.conjuration", "mage.conjuration.bound"},
    LoadoutOptionRule{kMageClassId, "mage.illusion", "mage.illusion.pacification"},
    LoadoutOptionRule{kMageClassId, "mage.illusion", "mage.illusion.discord"},
    LoadoutOptionRule{kMageClassId, "mage.illusion", "mage.illusion.shadows"},
    LoadoutOptionRule{kMageClassId, "mage.restoration", "mage.restoration.healing"},
    LoadoutOptionRule{kMageClassId, "mage.restoration", "mage.restoration.wards"},
    LoadoutOptionRule{kMageClassId, "mage.restoration", "mage.restoration.sacred"},
    LoadoutOptionRule{kMageClassId, "mage.enchanting", "mage.enchanting.weapons"},
    LoadoutOptionRule{kMageClassId, "mage.enchanting", "mage.enchanting.body"},
    LoadoutOptionRule{kMageClassId, "mage.enchanting", "mage.enchanting.souls"},

    LoadoutOptionRule{kThiefClassId, "thief.one_handed_mode", "thief.one_handed_mode.steel"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_mode", "thief.one_handed_mode.dual_iron"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_steel", "thief.one_handed_steel.dagger"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_steel", "thief.one_handed_steel.sword"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_steel", "thief.one_handed_steel.war_axe"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_steel", "thief.one_handed_steel.mace"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_iron_main", "thief.one_handed_iron_main.dagger"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_iron_main", "thief.one_handed_iron_main.sword"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_iron_main", "thief.one_handed_iron_main.war_axe"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_iron_main", "thief.one_handed_iron_main.mace"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_iron_off", "thief.one_handed_iron_off.dagger"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_iron_off", "thief.one_handed_iron_off.sword"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_iron_off", "thief.one_handed_iron_off.war_axe"},
    LoadoutOptionRule{kThiefClassId, "thief.one_handed_iron_off", "thief.one_handed_iron_off.mace"},
};

struct RequiredLoadoutGroup
{
    const char* ClassId;
    const char* GroupId;
    const char* ConditionGroupId{};
    const char* ConditionOptionId{};
};

constexpr std::array kRequiredLoadoutGroups{
    RequiredLoadoutGroup{kWarriorClassId, "warrior.parade"},
    RequiredLoadoutGroup{kWarriorClassId, "warrior.two_handed"},
    RequiredLoadoutGroup{kWarriorClassId, "warrior.archery"},
    RequiredLoadoutGroup{kWarriorClassId, "warrior.smithing"},
    RequiredLoadoutGroup{kWarriorClassId, "warrior.one_handed_mode"},
    RequiredLoadoutGroup{kWarriorClassId, "warrior.one_handed_steel", "warrior.one_handed_mode", "warrior.one_handed_mode.steel"},
    RequiredLoadoutGroup{kWarriorClassId, "warrior.one_handed_iron_main", "warrior.one_handed_mode", "warrior.one_handed_mode.dual_iron"},
    RequiredLoadoutGroup{kWarriorClassId, "warrior.one_handed_iron_off", "warrior.one_handed_mode", "warrior.one_handed_mode.dual_iron"},
    RequiredLoadoutGroup{kMageClassId, "mage.destruction"},
    RequiredLoadoutGroup{kMageClassId, "mage.alteration"},
    RequiredLoadoutGroup{kMageClassId, "mage.conjuration"},
    RequiredLoadoutGroup{kMageClassId, "mage.illusion"},
    RequiredLoadoutGroup{kMageClassId, "mage.restoration"},
    RequiredLoadoutGroup{kMageClassId, "mage.enchanting"},
    RequiredLoadoutGroup{kThiefClassId, "thief.one_handed_mode"},
    RequiredLoadoutGroup{kThiefClassId, "thief.one_handed_steel", "thief.one_handed_mode", "thief.one_handed_mode.steel"},
    RequiredLoadoutGroup{kThiefClassId, "thief.one_handed_iron_main", "thief.one_handed_mode", "thief.one_handed_mode.dual_iron"},
    RequiredLoadoutGroup{kThiefClassId, "thief.one_handed_iron_off", "thief.one_handed_mode", "thief.one_handed_mode.dual_iron"},
};

struct ClassItemGrantRule
{
    const char* ClassId;
    const char* PluginName;
    std::uint32_t LocalFormId;
    std::int32_t Count;
};

constexpr std::array kClassItemGrantRules{
    ClassItemGrantRule{kWarriorClassId, "Skyrim.esm", 0x00012E49, 1},
    ClassItemGrantRule{kWarriorClassId, "Skyrim.esm", 0x00012E4B, 1},
    ClassItemGrantRule{kWarriorClassId, "Skyrim.esm", 0x00012E46, 1},
    ClassItemGrantRule{kWarriorClassId, "STRE_AlternateStart.esp", 0x00003B5D, 1},
    ClassItemGrantRule{kWarriorClassId, "STRE_AlternateStart.esp", 0x00003B60, 1},

    ClassItemGrantRule{kMageClassId, "STRE_AlternateStart.esp", 0x00003B6E, 1},
    ClassItemGrantRule{kMageClassId, "STRE_AlternateStart.esp", 0x00003B70, 1},

    ClassItemGrantRule{kThiefClassId, "Skyrim.esm", 0x00013911, 1},
    ClassItemGrantRule{kThiefClassId, "Skyrim.esm", 0x00013910, 1},
    ClassItemGrantRule{kThiefClassId, "Skyrim.esm", 0x00013912, 1},
    ClassItemGrantRule{kThiefClassId, "Skyrim.esm", 0x0000000A, 10},
    ClassItemGrantRule{kThiefClassId, "STRE_AlternateStart.esp", 0x00003B57, 1},
    ClassItemGrantRule{kThiefClassId, "STRE_AlternateStart.esp", 0x00003B59, 1},
    ClassItemGrantRule{kThiefClassId, "STRE_AlternateStart.esp", 0x00003B4F, 1},
    ClassItemGrantRule{kThiefClassId, "STRE_AlternateStart.esp", 0x00003B51, 1},
    ClassItemGrantRule{kThiefClassId, "STRE_AlternateStart.esp", 0x00003B43, 1},
    ClassItemGrantRule{kThiefClassId, "STRE_AlternateStart.esp", 0x00003B45, 1},
};

struct OptionItemGrantRule
{
    const char* OptionId;
    const char* PluginName;
    std::uint32_t LocalFormId;
    std::int32_t Count;
};

constexpr std::array kOptionItemGrantRules{
    OptionItemGrantRule{"warrior.parade.hide_shield", "Skyrim.esm", 0x00013914, 1},
    OptionItemGrantRule{"warrior.parade.iron_shield", "Skyrim.esm", 0x00012EB6, 1},
    OptionItemGrantRule{"warrior.parade.guard_pendant", "STRE_AlternateStart.esp", 0x00003B41, 1},
    OptionItemGrantRule{"warrior.two_handed.greatsword", "Skyrim.esm", 0x0001359D, 1},
    OptionItemGrantRule{"warrior.two_handed.battleaxe", "Skyrim.esm", 0x00013980, 1},
    OptionItemGrantRule{"warrior.two_handed.warhammer", "Skyrim.esm", 0x00013981, 1},
    OptionItemGrantRule{"warrior.archery.bow", "Skyrim.esm", 0x00013985, 1},
    OptionItemGrantRule{"warrior.archery.bow", "Skyrim.esm", 0x0001397D, 50},
    OptionItemGrantRule{"warrior.archery.crossbow", "Dawnguard.esm", 0x00000801, 1},
    OptionItemGrantRule{"warrior.archery.crossbow", "Dawnguard.esm", 0x00000BB3, 50},
    OptionItemGrantRule{"warrior.smithing.light_leather", "Skyrim.esm", 0x000DB5D2, 4},
    OptionItemGrantRule{"warrior.smithing.light_leather", "Skyrim.esm", 0x000800E4, 8},
    OptionItemGrantRule{"warrior.smithing.light_mixed", "Skyrim.esm", 0x000DB5D2, 3},
    OptionItemGrantRule{"warrior.smithing.light_mixed", "Skyrim.esm", 0x000800E4, 6},
    OptionItemGrantRule{"warrior.smithing.light_mixed", "Skyrim.esm", 0x0005ACE4, 2},
    OptionItemGrantRule{"warrior.smithing.heavy_iron", "Skyrim.esm", 0x0005ACE4, 4},
    OptionItemGrantRule{"warrior.smithing.heavy_iron", "Skyrim.esm", 0x000800E4, 4},
    OptionItemGrantRule{"warrior.smithing.heavy_mixed", "Skyrim.esm", 0x0005ACE4, 2},
    OptionItemGrantRule{"warrior.smithing.heavy_mixed", "Skyrim.esm", 0x0005AD93, 2},
    OptionItemGrantRule{"warrior.smithing.heavy_mixed", "Skyrim.esm", 0x000800E4, 4},
    OptionItemGrantRule{"warrior.smithing.maintenance", "Skyrim.esm", 0x0005ACE4, 2},
    OptionItemGrantRule{"warrior.smithing.maintenance", "Skyrim.esm", 0x0005ACE5, 1},
    OptionItemGrantRule{"warrior.smithing.maintenance", "Skyrim.esm", 0x000DB5D2, 2},
    OptionItemGrantRule{"warrior.smithing.maintenance", "Skyrim.esm", 0x000800E4, 4},

    OptionItemGrantRule{"warrior.one_handed_steel.dagger", "Skyrim.esm", 0x00013986, 1},
    OptionItemGrantRule{"warrior.one_handed_steel.sword", "Skyrim.esm", 0x00013989, 1},
    OptionItemGrantRule{"warrior.one_handed_steel.war_axe", "Skyrim.esm", 0x00013983, 1},
    OptionItemGrantRule{"warrior.one_handed_steel.mace", "Skyrim.esm", 0x00013988, 1},
    OptionItemGrantRule{"warrior.one_handed_iron_main.dagger", "Skyrim.esm", 0x0001397E, 1},
    OptionItemGrantRule{"warrior.one_handed_iron_main.sword", "Skyrim.esm", 0x00012EB7, 1},
    OptionItemGrantRule{"warrior.one_handed_iron_main.war_axe", "Skyrim.esm", 0x00013790, 1},
    OptionItemGrantRule{"warrior.one_handed_iron_main.mace", "Skyrim.esm", 0x00013982, 1},
    OptionItemGrantRule{"warrior.one_handed_iron_off.dagger", "Skyrim.esm", 0x0001397E, 1},
    OptionItemGrantRule{"warrior.one_handed_iron_off.sword", "Skyrim.esm", 0x00012EB7, 1},
    OptionItemGrantRule{"warrior.one_handed_iron_off.war_axe", "Skyrim.esm", 0x00013790, 1},
    OptionItemGrantRule{"warrior.one_handed_iron_off.mace", "Skyrim.esm", 0x00013982, 1},

    OptionItemGrantRule{"thief.one_handed_steel.dagger", "Skyrim.esm", 0x00013986, 1},
    OptionItemGrantRule{"thief.one_handed_steel.sword", "Skyrim.esm", 0x00013989, 1},
    OptionItemGrantRule{"thief.one_handed_steel.war_axe", "Skyrim.esm", 0x00013983, 1},
    OptionItemGrantRule{"thief.one_handed_steel.mace", "Skyrim.esm", 0x00013988, 1},
    OptionItemGrantRule{"thief.one_handed_iron_main.dagger", "Skyrim.esm", 0x0001397E, 1},
    OptionItemGrantRule{"thief.one_handed_iron_main.sword", "Skyrim.esm", 0x00012EB7, 1},
    OptionItemGrantRule{"thief.one_handed_iron_main.war_axe", "Skyrim.esm", 0x00013790, 1},
    OptionItemGrantRule{"thief.one_handed_iron_main.mace", "Skyrim.esm", 0x00013982, 1},
    OptionItemGrantRule{"thief.one_handed_iron_off.dagger", "Skyrim.esm", 0x0001397E, 1},
    OptionItemGrantRule{"thief.one_handed_iron_off.sword", "Skyrim.esm", 0x00012EB7, 1},
    OptionItemGrantRule{"thief.one_handed_iron_off.war_axe", "Skyrim.esm", 0x00013790, 1},
    OptionItemGrantRule{"thief.one_handed_iron_off.mace", "Skyrim.esm", 0x00013982, 1},
};

struct OptionSpellGrantRule
{
    const char* OptionId;
    const char* PluginName;
    std::uint32_t LocalFormId;
};

constexpr std::array kOptionSpellGrantRules{
    OptionSpellGrantRule{"mage.destruction.fire", "Skyrim.esm", 0x00012FCD},
    OptionSpellGrantRule{"mage.destruction.fire", "STRE_AlternateStart.esp", 0x000040DA},
    OptionSpellGrantRule{"mage.destruction.fire", "STRE_AlternateStart.esp", 0x000040DE},
    OptionSpellGrantRule{"mage.destruction.frost", "STRE_AlternateStart.esp", 0x000040E2},
    OptionSpellGrantRule{"mage.destruction.frost", "STRE_AlternateStart.esp", 0x000040EA},
    OptionSpellGrantRule{"mage.destruction.frost", "STRE_AlternateStart.esp", 0x000040E6},
    OptionSpellGrantRule{"mage.destruction.shock", "STRE_AlternateStart.esp", 0x000040EE},
    OptionSpellGrantRule{"mage.destruction.shock", "STRE_AlternateStart.esp", 0x000040FA},
    OptionSpellGrantRule{"mage.destruction.shock", "STRE_AlternateStart.esp", 0x000040F6},

    OptionSpellGrantRule{"mage.alteration.protection", "STRE_AlternateStart.esp", 0x000040FE},
    OptionSpellGrantRule{"mage.alteration.protection", "STRE_AlternateStart.esp", 0x00004102},
    OptionSpellGrantRule{"mage.alteration.protection", "STRE_AlternateStart.esp", 0x00006FCD},
    OptionSpellGrantRule{"mage.alteration.protection", "STRE_AlternateStart.esp", 0x00006FD1},
    OptionSpellGrantRule{"mage.alteration.exploration", "STRE_AlternateStart.esp", 0x00006FD8},
    OptionSpellGrantRule{"mage.alteration.exploration", "STRE_AlternateStart.esp", 0x00006FDC},
    OptionSpellGrantRule{"mage.alteration.exploration", "STRE_AlternateStart.esp", 0x00006FE0},
    OptionSpellGrantRule{"mage.alteration.exploration", "STRE_AlternateStart.esp", 0x00006FE6},
    OptionSpellGrantRule{"mage.alteration.matter", "STRE_AlternateStart.esp", 0x00006FEA},
    OptionSpellGrantRule{"mage.alteration.matter", "STRE_AlternateStart.esp", 0x00006FEE},
    OptionSpellGrantRule{"mage.alteration.matter", "STRE_AlternateStart.esp", 0x00006FF2},
    OptionSpellGrantRule{"mage.alteration.matter", "STRE_AlternateStart.esp", 0x00006FF8},
};

struct OptionEquipmentRule
{
    const char* OptionId;
    const char* PluginName;
    std::uint32_t LocalFormId;
    std::int32_t Count;
    EquipmentSide Side;
};

constexpr std::array kOptionEquipmentRules{
    OptionEquipmentRule{"warrior.parade.hide_shield", "Skyrim.esm", 0x00013914, 1, EquipmentSide::Left},
    OptionEquipmentRule{"warrior.parade.iron_shield", "Skyrim.esm", 0x00012EB6, 1, EquipmentSide::Left},
    OptionEquipmentRule{"warrior.parade.guard_pendant", "STRE_AlternateStart.esp", 0x00003B41, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.archery.bow", "Skyrim.esm", 0x0001397D, 50, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.archery.crossbow", "Dawnguard.esm", 0x00000BB3, 50, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_steel.dagger", "Skyrim.esm", 0x00013986, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_steel.sword", "Skyrim.esm", 0x00013989, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_steel.war_axe", "Skyrim.esm", 0x00013983, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_steel.mace", "Skyrim.esm", 0x00013988, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_iron_main.dagger", "Skyrim.esm", 0x0001397E, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_iron_main.sword", "Skyrim.esm", 0x00012EB7, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_iron_main.war_axe", "Skyrim.esm", 0x00013790, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_iron_main.mace", "Skyrim.esm", 0x00013982, 1, EquipmentSide::Right},
    OptionEquipmentRule{"warrior.one_handed_iron_off.dagger", "Skyrim.esm", 0x0001397E, 1, EquipmentSide::Left},
    OptionEquipmentRule{"warrior.one_handed_iron_off.sword", "Skyrim.esm", 0x00012EB7, 1, EquipmentSide::Left},
    OptionEquipmentRule{"warrior.one_handed_iron_off.war_axe", "Skyrim.esm", 0x00013790, 1, EquipmentSide::Left},
    OptionEquipmentRule{"warrior.one_handed_iron_off.mace", "Skyrim.esm", 0x00013982, 1, EquipmentSide::Left},
    OptionEquipmentRule{"thief.one_handed_steel.dagger", "Skyrim.esm", 0x00013986, 1, EquipmentSide::Right},
    OptionEquipmentRule{"thief.one_handed_steel.sword", "Skyrim.esm", 0x00013989, 1, EquipmentSide::Right},
    OptionEquipmentRule{"thief.one_handed_steel.war_axe", "Skyrim.esm", 0x00013983, 1, EquipmentSide::Right},
    OptionEquipmentRule{"thief.one_handed_steel.mace", "Skyrim.esm", 0x00013988, 1, EquipmentSide::Right},
    OptionEquipmentRule{"thief.one_handed_iron_main.dagger", "Skyrim.esm", 0x0001397E, 1, EquipmentSide::Right},
    OptionEquipmentRule{"thief.one_handed_iron_main.sword", "Skyrim.esm", 0x00012EB7, 1, EquipmentSide::Right},
    OptionEquipmentRule{"thief.one_handed_iron_main.war_axe", "Skyrim.esm", 0x00013790, 1, EquipmentSide::Right},
    OptionEquipmentRule{"thief.one_handed_iron_main.mace", "Skyrim.esm", 0x00013982, 1, EquipmentSide::Right},
    OptionEquipmentRule{"thief.one_handed_iron_off.dagger", "Skyrim.esm", 0x0001397E, 1, EquipmentSide::Left},
    OptionEquipmentRule{"thief.one_handed_iron_off.sword", "Skyrim.esm", 0x00012EB7, 1, EquipmentSide::Left},
    OptionEquipmentRule{"thief.one_handed_iron_off.war_axe", "Skyrim.esm", 0x00013790, 1, EquipmentSide::Left},
    OptionEquipmentRule{"thief.one_handed_iron_off.mace", "Skyrim.esm", 0x00013982, 1, EquipmentSide::Left},
};

constexpr std::array kWarriorEquippedApparel{
    EquipmentGrant{"Skyrim.esm", 0x00012E49, 1, EquipmentSide::Right},
    EquipmentGrant{"Skyrim.esm", 0x00012E4B, 1, EquipmentSide::Right},
    EquipmentGrant{"Skyrim.esm", 0x00012E46, 1, EquipmentSide::Right},
};

constexpr std::array kThiefEquippedApparel{
    EquipmentGrant{"Skyrim.esm", 0x00013911, 1, EquipmentSide::Right},
    EquipmentGrant{"Skyrim.esm", 0x00013910, 1, EquipmentSide::Right},
    EquipmentGrant{"Skyrim.esm", 0x00013912, 1, EquipmentSide::Right},
};

constexpr std::array kMageEquippedApparel{
    EquipmentGrant{"STRE_AlternateStart.esp", 0x00003B6E, 1, EquipmentSide::Right},
    EquipmentGrant{"STRE_AlternateStart.esp", 0x00003B70, 1, EquipmentSide::Right},
};

constexpr std::array kFallbackEquippedClothes{
    EquipmentGrant{"Skyrim.esm", 0x000209A6, 1, EquipmentSide::Right},
    EquipmentGrant{"Skyrim.esm", 0x000209A5, 1, EquipmentSide::Right},
};

bool IsConditionSatisfied(
    const RequiredLoadoutGroup& acGroup,
    const std::map<std::string, std::string>& acSelections) noexcept
{
    if (!acGroup.ConditionGroupId || !acGroup.ConditionOptionId)
        return true;

    const auto condition = acSelections.find(acGroup.ConditionGroupId);
    return condition != acSelections.end() &&
        condition->second == acGroup.ConditionOptionId;
}
}

bool IsSupportedClassId(std::string_view aClassId) noexcept
{
    return aClassId == kWarriorClassId ||
        aClassId == kMageClassId ||
        aClassId == kThiefClassId;
}

bool IsSupportedLoadoutOption(
    std::string_view aClassId,
    std::string_view aGroupId,
    std::string_view aOptionId) noexcept
{
    return std::any_of(
        kLoadoutOptionRules.begin(),
        kLoadoutOptionRules.end(),
        [aClassId, aGroupId, aOptionId](const LoadoutOptionRule& acRule)
        {
            return aClassId == acRule.ClassId &&
                aGroupId == acRule.GroupId &&
                aOptionId == acRule.OptionId;
        });
}

bool IsLoadoutGroupActive(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections,
    std::string_view aGroupId) noexcept
{
    const auto group = std::find_if(
        kRequiredLoadoutGroups.begin(),
        kRequiredLoadoutGroups.end(),
        [aClassId, aGroupId](const RequiredLoadoutGroup& acRule)
        {
            return aClassId == acRule.ClassId &&
                aGroupId == acRule.GroupId;
        });

    if (group == kRequiredLoadoutGroups.end())
        return false;

    return IsConditionSatisfied(*group, acSelections);
}

bool HasCompleteLoadout(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections) noexcept
{
    if (!IsSupportedClassId(aClassId))
        return false;

    for (const RequiredLoadoutGroup& group : kRequiredLoadoutGroups)
    {
        if (aClassId != group.ClassId ||
            !IsConditionSatisfied(group, acSelections))
        {
            continue;
        }

        const auto selection = acSelections.find(group.GroupId);
        if (selection == acSelections.end() ||
            !IsSupportedLoadoutOption(
                aClassId,
                group.GroupId,
                selection->second))
        {
            return false;
        }
    }

    return true;
}

bool ValidateSelections(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections) noexcept
{
    if (acSelections.size() > kMaximumSelectionCount ||
        !HasCompleteLoadout(aClassId, acSelections))
    {
        return false;
    }

    for (const auto& [groupId, optionId] : acSelections)
    {
        if (!IsSupportedLoadoutOption(aClassId, groupId, optionId) ||
            !IsLoadoutGroupActive(aClassId, acSelections, groupId))
        {
            return false;
        }
    }

    return true;
}

std::vector<ItemGrant> BuildItemGrants(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections)
{
    std::vector<ItemGrant> grants;

    const auto appendGrant = [&grants](const ItemGrant& acGrant)
    {
        const auto existing = std::find_if(
            grants.begin(),
            grants.end(),
            [&acGrant](const ItemGrant& acExisting)
            {
                return acExisting.LocalFormId == acGrant.LocalFormId &&
                    std::strcmp(
                        acExisting.PluginName,
                        acGrant.PluginName) == 0;
            });

        if (existing != grants.end())
        {
            existing->Count += acGrant.Count;
            return;
        }

        grants.push_back(acGrant);
    };

    for (const ClassItemGrantRule& rule : kClassItemGrantRules)
    {
        if (aClassId == rule.ClassId)
        {
            appendGrant(
                ItemGrant{
                    rule.PluginName,
                    rule.LocalFormId,
                    rule.Count});
        }
    }

    for (const auto& [groupId, optionId] : acSelections)
    {
        (void)groupId;
        for (const OptionItemGrantRule& rule : kOptionItemGrantRules)
        {
            if (optionId == rule.OptionId)
            {
                appendGrant(
                    ItemGrant{
                        rule.PluginName,
                        rule.LocalFormId,
                        rule.Count});
            }
        }
    }

    // Every currently supported class has an explicit body set. Keep the
    // neutral fallback for future classes until their authored outfit exists.
    const bool receivesExplicitBodySet =
        aClassId == kWarriorClassId ||
        aClassId == kMageClassId ||
        aClassId == kThiefClassId;
    if (!receivesExplicitBodySet)
    {
        appendGrant(ItemGrant{"Skyrim.esm", 0x000209A6, 1});
        appendGrant(ItemGrant{"Skyrim.esm", 0x000209A5, 1});
    }

    return grants;
}

std::vector<SpellGrant> BuildSpellGrants(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections)
{
    std::vector<SpellGrant> grants;
    if (aClassId != kMageClassId)
        return grants;

    const auto appendGrant = [&grants](const SpellGrant& acGrant)
    {
        const auto existing = std::find_if(
            grants.begin(),
            grants.end(),
            [&acGrant](const SpellGrant& acExisting)
            {
                return acExisting.LocalFormId == acGrant.LocalFormId &&
                    std::strcmp(
                        acExisting.PluginName,
                        acGrant.PluginName) == 0;
            });

        if (existing == grants.end())
            grants.push_back(acGrant);
    };

    for (const auto& [groupId, optionId] : acSelections)
    {
        (void)groupId;
        for (const OptionSpellGrantRule& rule : kOptionSpellGrantRules)
        {
            if (optionId == rule.OptionId)
            {
                appendGrant(
                    SpellGrant{
                        rule.PluginName,
                        rule.LocalFormId});
            }
        }
    }

    return grants;
}

std::vector<EquipmentGrant> BuildEquipmentGrants(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections)
{
    std::vector<EquipmentGrant> equipment;

    const auto appendRange = [&equipment](const auto& acRange)
    {
        equipment.insert(
            equipment.end(),
            acRange.begin(),
            acRange.end());
    };

    if (aClassId == kWarriorClassId)
        appendRange(kWarriorEquippedApparel);
    else if (aClassId == kMageClassId)
        appendRange(kMageEquippedApparel);
    else if (aClassId == kThiefClassId)
        appendRange(kThiefEquippedApparel);
    else
        appendRange(kFallbackEquippedClothes);

    const auto mode = acSelections.find(
        aClassId == kWarriorClassId
            ? "warrior.one_handed_mode"
            : "thief.one_handed_mode");
    const bool dualIron =
        mode != acSelections.end() &&
        (mode->second == "warrior.one_handed_mode.dual_iron" ||
         mode->second == "thief.one_handed_mode.dual_iron");

    for (const auto& [groupId, optionId] : acSelections)
    {
        (void)groupId;

        for (const OptionEquipmentRule& rule : kOptionEquipmentRules)
        {
            if (optionId != rule.OptionId)
                continue;

            const bool isShield =
                optionId == "warrior.parade.hide_shield" ||
                optionId == "warrior.parade.iron_shield";
            if (isShield && dualIron)
                continue;

            equipment.push_back(
                EquipmentGrant{
                    rule.PluginName,
                    rule.LocalFormId,
                    rule.Count,
                    rule.Side});
        }
    }

    return equipment;
}
}
