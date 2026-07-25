#include <TiltedOnlinePCH.h>

#include <Services/CharacterCreationService.h>

#include <Services/OverlayService.h>
#include <Services/PapyrusService.h>
#include <Services/TradeItemPreviewService.h>
#include <Services/UiSurfaceService.h>

#include <Events/UpdateEvent.h>

#include <Components/TESContainer.h>
#include <Games/Overrides.h>

#include <Forms/TESQuest.h>
#include <Forms/TESRace.h>
#include <Forms/MagicItem.h>
#include <Forms/SpellItem.h>
#include <Forms/TESActorBase.h>
#include <Forms/TESShout.h>
#include <TESObjectREFR.h>
#include <Structs/Inventory.h>
#include <Games/TES.h>
#include <Games/Skyrim/AI/Movement/PlayerControls.h>
#include <Games/Skyrim/Interface/UI.h>
#include <PlayerCharacter.h>
#include <World.h>

#include <OverlayApp.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace
{
constexpr char kAlternateStartQuestEditorId[] =
    "STRE_QUEST_AlternateStart";
constexpr std::uint16_t kSeatedAndLockedStage = 20;
constexpr double kRaceMenuOpenTimeoutSeconds = 5.0;
constexpr double kRecoveryPollSeconds = 1.0;
constexpr double kBuildSealSeconds = 1.6;
constexpr double kBuildApplicationSettleSeconds = 0.25;
constexpr std::uint8_t kMaxInventoryWipePasses = 8;
constexpr std::uint32_t kVanillaHealingSpell = 0x00012FCC;
constexpr std::uint32_t kVanillaFlamesSpell = 0x00012FCD;
constexpr char kDefaultClassId[] = "class.warrior";
constexpr char kWarriorClassId[] = "class.warrior";
constexpr char kMageClassId[] = "class.mage";
constexpr char kThiefClassId[] = "class.thief";

[[nodiscard]] MagicSystem::SpellType GetSpellType(
    const SpellItem* apSpell) noexcept
{
    if (!apSpell)
        return MagicSystem::SPELL_TYPE_COUNT;

    // SpellItem::Data starts with costOverride, flags and spellType at C0.
    // The local reverse-engineered header exposes these three fields as
    // unk6C[0..2]. Reading index 2 avoids depending on Editor IDs, which are
    // commonly unavailable for Skyrim.esm records at runtime.
    return static_cast<MagicSystem::SpellType>(apSpell->unk6C[2]);
}

[[nodiscard]] bool IsImportedPowerOrAbility(
    const SpellItem* apSpell) noexcept
{
    switch (GetSpellType(apSpell))
    {
    case MagicSystem::POWER:
    case MagicSystem::LESSER_POWER:
    case MagicSystem::ABILITY:
        return true;
    default:
        return false;
    }
}

struct LoadoutOptionRule
{
    const char* classId;
    const char* groupId;
    const char* optionId;
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

    LoadoutOptionRule{kThiefClassId, "thief.lockpicking", "thief.lockpicking.toolkit"},
    LoadoutOptionRule{kThiefClassId, "thief.lockpicking", "thief.lockpicking.picks"},
    LoadoutOptionRule{kThiefClassId, "thief.lockpicking", "thief.lockpicking.wax"},
    LoadoutOptionRule{kThiefClassId, "thief.sneak", "thief.sneak.nature"},
    LoadoutOptionRule{kThiefClassId, "thief.sneak", "thief.sneak.urban"},
    LoadoutOptionRule{kThiefClassId, "thief.sneak", "thief.sneak.dungeons"},
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
    const char* classId;
    const char* groupId;
    const char* conditionGroupId{};
    const char* conditionOptionId{};
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
    RequiredLoadoutGroup{kThiefClassId, "thief.lockpicking"},
    RequiredLoadoutGroup{kThiefClassId, "thief.sneak"},
    RequiredLoadoutGroup{kThiefClassId, "thief.one_handed_mode"},
    RequiredLoadoutGroup{kThiefClassId, "thief.one_handed_steel", "thief.one_handed_mode", "thief.one_handed_mode.steel"},
    RequiredLoadoutGroup{kThiefClassId, "thief.one_handed_iron_main", "thief.one_handed_mode", "thief.one_handed_mode.dual_iron"},
    RequiredLoadoutGroup{kThiefClassId, "thief.one_handed_iron_off", "thief.one_handed_mode", "thief.one_handed_mode.dual_iron"},
};

struct PreviewFormRule
{
    const char* key;
    const char* pluginName;
    std::uint32_t localFormId;
};

// Runtime preview mappings only. Stable business IDs remain independent from
// load-order-sensitive FormIDs. The runtime ID is resolved through the owning
// plugin before the item is handed to Skyrim's Inventory3DManager.
constexpr std::array kPreviewForms{
    PreviewFormRule{"preview.iron_armor", "Skyrim.esm", 0x00012E49},
    PreviewFormRule{"preview.iron_boots", "Skyrim.esm", 0x00012E4B},
    PreviewFormRule{"preview.iron_gauntlets", "Skyrim.esm", 0x00012E46},
    PreviewFormRule{"preview.hide_shield", "Skyrim.esm", 0x00013914},
    PreviewFormRule{"preview.iron_shield", "Skyrim.esm", 0x00012EB6},
    PreviewFormRule{"preview.iron_greatsword", "Skyrim.esm", 0x0001359D},
    PreviewFormRule{"preview.iron_battleaxe", "Skyrim.esm", 0x00013980},
    PreviewFormRule{"preview.iron_warhammer", "Skyrim.esm", 0x00013981},
    PreviewFormRule{"preview.hunting_bow", "Skyrim.esm", 0x00013985},
    PreviewFormRule{"preview.iron_arrow", "Skyrim.esm", 0x0001397D},
    PreviewFormRule{"preview.crossbow", "Dawnguard.esm", 0x00000801},
    PreviewFormRule{"preview.steel_bolt", "Dawnguard.esm", 0x00000BB3},
    PreviewFormRule{"preview.steel_dagger", "Skyrim.esm", 0x00013986},
    PreviewFormRule{"preview.steel_sword", "Skyrim.esm", 0x00013989},
    PreviewFormRule{"preview.steel_war_axe", "Skyrim.esm", 0x00013983},
    PreviewFormRule{"preview.steel_mace", "Skyrim.esm", 0x00013988},
    PreviewFormRule{"preview.iron_dagger", "Skyrim.esm", 0x0001397E},
    PreviewFormRule{"preview.iron_sword", "Skyrim.esm", 0x00012EB7},
    PreviewFormRule{"preview.iron_war_axe", "Skyrim.esm", 0x00013790},
    PreviewFormRule{"preview.iron_mace", "Skyrim.esm", 0x00013982},
    PreviewFormRule{"preview.lockpick", "Skyrim.esm", 0x0000000A},
    PreviewFormRule{"preview.leather_armor", "Skyrim.esm", 0x00013911},
    PreviewFormRule{"preview.hide_boots", "Skyrim.esm", 0x00013910},
    PreviewFormRule{"preview.hide_bracers", "Skyrim.esm", 0x00013912},
    PreviewFormRule{"preview.leather", "Skyrim.esm", 0x000DB5D2},
    PreviewFormRule{"preview.leather_strips", "Skyrim.esm", 0x000800E4},
    PreviewFormRule{"preview.iron_ingot", "Skyrim.esm", 0x0005ACE4},
    PreviewFormRule{"preview.steel_ingot", "Skyrim.esm", 0x0005ACE5},
    PreviewFormRule{"preview.corundum_ingot", "Skyrim.esm", 0x0005AD93},
};

struct ClassItemGrantRule
{
    const char* classId;
    const char* pluginName;
    std::uint32_t localFormId;
    std::int32_t count;
};

constexpr std::array kClassItemGrantRules{
    ClassItemGrantRule{kWarriorClassId, "Skyrim.esm", 0x00012E49, 1},
    ClassItemGrantRule{kWarriorClassId, "Skyrim.esm", 0x00012E4B, 1},
    ClassItemGrantRule{kWarriorClassId, "Skyrim.esm", 0x00012E46, 1},
    ClassItemGrantRule{kThiefClassId, "Skyrim.esm", 0x00013911, 1},
    ClassItemGrantRule{kThiefClassId, "Skyrim.esm", 0x00013910, 1},
    ClassItemGrantRule{kThiefClassId, "Skyrim.esm", 0x00013912, 1},
};

struct OptionItemGrantRule
{
    const char* optionId;
    const char* pluginName;
    std::uint32_t localFormId;
    std::int32_t count;
};

constexpr std::array kOptionItemGrantRules{
    OptionItemGrantRule{"warrior.parade.hide_shield", "Skyrim.esm", 0x00013914, 1},
    OptionItemGrantRule{"warrior.parade.iron_shield", "Skyrim.esm", 0x00012EB6, 1},
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

    OptionItemGrantRule{"thief.lockpicking.toolkit", "Skyrim.esm", 0x0000000A, 5},
    OptionItemGrantRule{"thief.lockpicking.picks", "Skyrim.esm", 0x0000000A, 20},
    OptionItemGrantRule{"thief.lockpicking.wax", "Skyrim.esm", 0x0000000A, 5},
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

struct PendingItemGrant
{
    const char* pluginName;
    std::uint32_t localFormId;
    std::int32_t count;
};


const char* PhaseName(CharacterCreationPhase aPhase) noexcept
{
    switch (aPhase)
    {
    case CharacterCreationPhase::WaitingForRaceMenuOpen:
        return "openingRaceMenu";
    case CharacterCreationPhase::WaitingForRaceMenuClose:
        return "editingRace";
    case CharacterCreationPhase::RaceReview:
        return "raceReview";
    case CharacterCreationPhase::ClassSelection:
        return "classSelection";
    case CharacterCreationPhase::LoadoutSelection:
        return "loadoutSelection";
    case CharacterCreationPhase::BuildSummary:
        return "buildSummary";
    case CharacterCreationPhase::BuildConfirmed:
        return "buildConfirmed";
    case CharacterCreationPhase::Error:
        return "error";
    case CharacterCreationPhase::Inactive:
    default:
        return "inactive";
    }
}

void AppendJsonString(
    std::string& aOutput,
    const std::string& acValue)
{
    aOutput.push_back('"');

    for (const unsigned char character : acValue)
    {
        switch (character)
        {
        case '"':
            aOutput += "\\\"";
            break;
        case '\\':
            aOutput += "\\\\";
            break;
        case '\n':
            aOutput += "\\n";
            break;
        case '\r':
            aOutput += "\\r";
            break;
        case '\t':
            aOutput += "\\t";
            break;
        default:
            if (character < 0x20)
                aOutput += fmt::format("\\u{:04x}", character);
            else
                aOutput.push_back(static_cast<char>(character));
            break;
        }
    }

    aOutput.push_back('"');
}
} // namespace

CharacterCreationService::CharacterCreationService(
    World& aWorld,
    UiSurfaceService& aUiSurfaceService,
    entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_uiSurfaceService(aUiSurfaceService)
    , m_updateConnection(
          aDispatcher.sink<UpdateEvent>().connect<
              &CharacterCreationService::OnUpdate>(this))
{
    if (auto* const pEvents = EventDispatcherManager::Get())
        pEvents->questStageEvent.RegisterSink(this);
}

CharacterCreationService::~CharacterCreationService() noexcept
{
    if (auto* const pEvents = EventDispatcherManager::Get())
        pEvents->questStageEvent.UnRegisterSink(this);
}

BSTEventResult CharacterCreationService::OnEvent(
    const TESQuestStageEvent* apEvent,
    const EventDispatcher<TESQuestStageEvent>*)
{
    if (!apEvent || apEvent->stageId != kSeatedAndLockedStage)
        return BSTEventResult::kOk;

    auto* const pQuest =
        Cast<TESQuest>(TESForm::GetById(apEvent->formId));
    if (!IsAlternateStartQuest(pQuest) || pQuest->IsStopped())
        return BSTEventResult::kOk;

    m_suppressStageRecovery = false;

    m_world.GetRunner().Queue(
        [this, formId = apEvent->formId]()
        {
            auto* const pQueuedQuest =
                Cast<TESQuest>(TESForm::GetById(formId));
            if (IsAlternateStartQuest(pQueuedQuest) &&
                !pQueuedQuest->IsStopped())
            {
                BeginFromStage20(pQueuedQuest);
            }
        });

    return BSTEventResult::kOk;
}

void CharacterCreationService::OnUpdate(
    const UpdateEvent& acEvent) noexcept
{
    m_recoveryAccumulator += acEvent.Delta;
    m_phaseElapsed += acEvent.Delta;

    if (m_phase == CharacterCreationPhase::Inactive &&
        !m_suppressStageRecovery &&
        m_recoveryAccumulator >= kRecoveryPollSeconds)
    {
        m_recoveryAccumulator = 0.0;

        TESQuest* const pQuest = FindAlternateStartQuest();
        if (pQuest &&
            !pQuest->IsStopped() &&
            pQuest->currentStage == kSeatedAndLockedStage)
        {
            spdlog::info(
                "[STRE][CharacterCreation] Recovering stage 20 flow");
            BeginFromStage20(pQuest);
        }
    }

    if (m_buildApplicationPending &&
        m_phase == CharacterCreationPhase::BuildSummary &&
        m_phaseElapsed >= kBuildApplicationSettleSeconds)
    {
        AdvanceBuildApplication();
    }

    switch (m_phase)
    {
    case CharacterCreationPhase::WaitingForRaceMenuOpen:
        if (IsRaceMenuOpen())
        {
            m_phase =
                CharacterCreationPhase::WaitingForRaceMenuClose;
            m_phaseElapsed = 0.0;
            spdlog::info(
                "[STRE][CharacterCreation] Race menu opened");
            PushState(true);
        }
        else if (m_phaseElapsed >= kRaceMenuOpenTimeoutSeconds)
        {
            Fail("Le menu de création du personnage ne s'est pas ouvert.");
        }
        break;

    case CharacterCreationPhase::WaitingForRaceMenuClose:
        if (!IsRaceMenuOpen())
        {
            spdlog::info(
                "[STRE][CharacterCreation] Race menu closed");
            ShowRaceReview();
        }
        break;

    case CharacterCreationPhase::BuildConfirmed:
        if (m_phaseElapsed >= kBuildSealSeconds)
            FinalizeCompletedBuild();
        break;

    default:
        break;
    }

    if (m_phase == CharacterCreationPhase::RaceReview ||
        m_phase == CharacterCreationPhase::ClassSelection ||
        m_phase == CharacterCreationPhase::LoadoutSelection ||
        m_phase == CharacterCreationPhase::BuildSummary ||
        m_phase == CharacterCreationPhase::BuildConfirmed ||
        m_phase == CharacterCreationPhase::Error)
    {
        PushState();
    }
}

void CharacterCreationService::BeginFromStage20(
    TESQuest* apQuest) noexcept
{
    if (!apQuest)
        return;

    if (m_phase != CharacterCreationPhase::Inactive &&
        m_phase != CharacterCreationPhase::Error)
    {
        return;
    }

    m_pQuest = apQuest;
    m_raceConfirmed = false;
    m_classConfirmed = false;
    m_selectedClassId.clear();
    ResetLoadoutState();
    m_error.clear();

    spdlog::info(
        "[STRE][CharacterCreation] Stage 20 detected, starting race flow");

    if (!LockCharacterCreationControls())
    {
        Fail("STRE n'a pas pu verrouiller le personnage.");
        return;
    }

    OpenRaceMenu();
}

void CharacterCreationService::OpenRaceMenu() noexcept
{
    ClearLoadoutPreview();
    m_error.clear();
    m_phaseElapsed = 0.0;
    m_phase = CharacterCreationPhase::WaitingForRaceMenuOpen;

    if (!m_controlsLocked && !LockCharacterCreationControls())
    {
        Fail("STRE n'a pas pu verrouiller le personnage.");
        return;
    }

    // CEF must release input while Skyrim's native RaceMenu owns it.
    m_uiSurfaceService.SetSurface(UiSurface::None);
    PushState(true);

    spdlog::info(
        "[STRE][CharacterCreation] Race menu requested");

    if (IsRaceMenuOpen())
    {
        m_phase = CharacterCreationPhase::WaitingForRaceMenuClose;
        m_phaseElapsed = 0.0;
        spdlog::info(
            "[STRE][CharacterCreation] Race menu was already open");
        PushState(true);
        return;
    }

    if (!InvokeShowRaceMenu())
        Fail("La fonction native Game.ShowRaceMenu est indisponible.");
}

void CharacterCreationService::ShowRaceReview() noexcept
{
    m_raceConfirmed = true;
    m_classConfirmed = false;
    ResetLoadoutState();

    if (m_selectedClassId.empty())
        m_selectedClassId = kDefaultClassId;

    spdlog::info(
        "[STRE][CharacterCreation] Race confirmed");
    spdlog::info(
        "[STRE][CharacterCreation] Skipping race review and opening class selection directly");

    ShowClassSelection();
}

void CharacterCreationService::ModifyRace() noexcept
{
    if (m_phase != CharacterCreationPhase::RaceReview &&
        m_phase != CharacterCreationPhase::ClassSelection &&
        m_phase != CharacterCreationPhase::LoadoutSelection &&
        m_phase != CharacterCreationPhase::BuildSummary &&
        m_phase != CharacterCreationPhase::BuildConfirmed)
    {
        return;
    }

    m_raceConfirmed = false;
    m_classConfirmed = false;
    ResetLoadoutState();
    OpenRaceMenu();
}

void CharacterCreationService::ConfirmRace() noexcept
{
    if (m_phase != CharacterCreationPhase::RaceReview)
        return;

    m_raceConfirmed = true;
    m_classConfirmed = false;
    ResetLoadoutState();

    if (m_selectedClassId.empty())
        m_selectedClassId = kDefaultClassId;

    spdlog::info(
        "[STRE][CharacterCreation] Race confirmed");

    ShowClassSelection();
}

void CharacterCreationService::ShowClassSelection() noexcept
{
    ClearLoadoutPreview();

    if (!m_raceConfirmed)
    {
        Fail("La race doit être confirmée avant le choix de la classe.");
        return;
    }

    if (m_selectedClassId.empty())
        m_selectedClassId = kDefaultClassId;

    m_classConfirmed = false;
    m_loadoutConfirmed = false;
    m_buildConfirmed = false;
    m_phase = CharacterCreationPhase::ClassSelection;
    m_phaseElapsed = 0.0;
    m_error.clear();

    m_uiSurfaceService.SetSurface(UiSurface::CharacterCreation);
    PushState(true);

    spdlog::info(
        "[STRE][CharacterCreation] Class menu requested selectedClassId={}",
        m_selectedClassId);
}

void CharacterCreationService::SelectClass(
    std::string aClassId) noexcept
{
    if (m_phase != CharacterCreationPhase::ClassSelection)
        return;

    if (!IsSupportedClassId(aClassId))
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Rejected unknown class id: {}",
            aClassId);
        return;
    }

    if (m_selectedClassId == aClassId)
        return;

    m_selectedClassId = std::move(aClassId);
    m_classConfirmed = false;
    ResetLoadoutState();

    spdlog::info(
        "[STRE][CharacterCreation] Class selected classId={}",
        m_selectedClassId);

    PushState(true);
}

void CharacterCreationService::ConfirmClass() noexcept
{
    if (m_phase != CharacterCreationPhase::ClassSelection)
        return;

    if (!m_raceConfirmed)
    {
        Fail("La race n'est pas confirmée.");
        return;
    }

    if (!IsSupportedClassId(m_selectedClassId))
    {
        Fail("La classe sélectionnée est invalide.");
        return;
    }

    m_classConfirmed = true;

    spdlog::info(
        "[STRE][CharacterCreation] Class confirmed classId={}",
        m_selectedClassId);

    ShowLoadoutSelection();
}

void CharacterCreationService::ShowLoadoutSelection() noexcept
{
    if (!m_raceConfirmed || !m_classConfirmed)
    {
        Fail("La race et la classe doivent être confirmées avant l'équipement.");
        return;
    }

    m_loadoutConfirmed = false;
    m_buildConfirmed = false;
    m_phase = CharacterCreationPhase::LoadoutSelection;
    m_phaseElapsed = 0.0;
    m_error.clear();

    m_uiSurfaceService.SetSurface(UiSurface::CharacterCreation);
    PushState(true);

    spdlog::info(
        "[STRE][CharacterCreation] Loadout selection requested classId={}",
        m_selectedClassId);
}

void CharacterCreationService::SelectLoadoutOption(
    std::string aSelection) noexcept
{
    if (m_phase != CharacterCreationPhase::LoadoutSelection)
        return;

    const std::size_t separator = aSelection.find('|');
    if (separator == std::string::npos ||
        separator == 0 ||
        separator + 1 >= aSelection.size())
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Rejected malformed loadout selection: {}",
            aSelection);
        return;
    }

    const std::string groupId = aSelection.substr(0, separator);
    const std::string optionId = aSelection.substr(separator + 1);

    if (!IsSupportedLoadoutOption(groupId, optionId))
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Rejected loadout option groupId={} optionId={} classId={}",
            groupId,
            optionId,
            m_selectedClassId);
        return;
    }

    if (!IsLoadoutGroupActive(groupId))
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Rejected inactive loadout group groupId={} optionId={} classId={}",
            groupId,
            optionId,
            m_selectedClassId);
        return;
    }

    const auto current = m_selectedLoadoutOptions.find(groupId);
    if (current != m_selectedLoadoutOptions.end() &&
        current->second == optionId)
    {
        return;
    }

    if (groupId == "warrior.one_handed_mode")
    {
        m_selectedLoadoutOptions.erase("warrior.one_handed_steel");
        m_selectedLoadoutOptions.erase("warrior.one_handed_iron_main");
        m_selectedLoadoutOptions.erase("warrior.one_handed_iron_off");
    }
    else if (groupId == "thief.one_handed_mode")
    {
        m_selectedLoadoutOptions.erase("thief.one_handed_steel");
        m_selectedLoadoutOptions.erase("thief.one_handed_iron_main");
        m_selectedLoadoutOptions.erase("thief.one_handed_iron_off");
    }

    m_selectedLoadoutOptions[groupId] = optionId;
    m_loadoutConfirmed = false;
    m_buildConfirmed = false;

    spdlog::info(
        "[STRE][CharacterCreation] Loadout option selected classId={} groupId={} optionId={}",
        m_selectedClassId,
        groupId,
        optionId);

    PushState(true);
}

void CharacterCreationService::ConfirmLoadout() noexcept
{
    if (m_phase != CharacterCreationPhase::LoadoutSelection)
        return;

    if (!HasCompleteLoadout())
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Build rejected reason=missingRequiredLoadout classId={}",
            m_selectedClassId);
        PushState(true);
        return;
    }

    m_loadoutConfirmed = true;
    m_buildConfirmed = false;

    spdlog::info(
        "[STRE][CharacterCreation] Loadout confirmed classId={} selectionCount={}",
        m_selectedClassId,
        m_selectedLoadoutOptions.size());

    ShowBuildSummary();
}

void CharacterCreationService::ShowBuildSummary() noexcept
{
    ClearLoadoutPreview();

    if (!m_raceConfirmed ||
        !m_classConfirmed ||
        !m_loadoutConfirmed ||
        !HasCompleteLoadout())
    {
        Fail("Le build est incomplet et ne peut pas être résumé.");
        return;
    }

    m_phase = CharacterCreationPhase::BuildSummary;
    m_phaseElapsed = 0.0;
    m_error.clear();

    m_uiSurfaceService.SetSurface(UiSurface::CharacterCreation);
    PushState(true);

    spdlog::info(
        "[STRE][CharacterCreation] Build summary requested classId={}",
        m_selectedClassId);
}

void CharacterCreationService::ConfirmBuild() noexcept
{
    if (m_phase != CharacterCreationPhase::BuildSummary)
        return;

    if (m_buildApplicationPending)
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Duplicate build submission ignored while application is pending");
        return;
    }

    spdlog::info(
        "[STRE][CharacterCreation] Build submitted classId={} selectionCount={} buildVersion=2",
        m_selectedClassId,
        m_selectedLoadoutOptions.size());

    if (!m_raceConfirmed ||
        !m_classConfirmed ||
        !m_loadoutConfirmed ||
        !HasCompleteLoadout())
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Build rejected reason=invalidState classId={}",
            m_selectedClassId);
        Fail("Le build n'est plus valide.");
        return;
    }

    ClearLoadoutPreview();
    ResetBuildApplicationState();
    m_buildApplicationPending = true;
    m_phaseElapsed = 0.0;

    spdlog::info(
        "[STRE][CharacterCreation] Build application queued path=stagedNativeInventoryWipe");

    PushState(true);
}

void CharacterCreationService::ShowBuildConfirmed() noexcept
{
    ClearLoadoutPreview();
    m_phase = CharacterCreationPhase::BuildConfirmed;
    m_phaseElapsed = 0.0;
    m_error.clear();

    m_uiSurfaceService.SetSurface(UiSurface::CharacterCreation);
    PushState(true);
}


void CharacterCreationService::FinalizeCompletedBuild() noexcept
{
    if (m_phase != CharacterCreationPhase::BuildConfirmed ||
        !m_buildConfirmed)
    {
        return;
    }

    if (!UnlockCharacterCreationControls())
    {
        Fail("Le paquetage a été appliqué, mais le personnage n'a pas pu être déverrouillé.");
        return;
    }

    if (m_pQuest)
    {
        m_pQuest->SetStopped();
        spdlog::info(
            "[STRE][CharacterCreation] Alternate-start quest stopped after successful build application");
    }

    m_uiSurfaceService.SetSurface(UiSurface::None);
    m_phase = CharacterCreationPhase::Inactive;
    m_phaseElapsed = 0.0;
    m_suppressStageRecovery = true;
    m_error.clear();
    PushState(true);

    spdlog::info(
        "[STRE][CharacterCreation] Character creation completed and controls unlocked classId={}",
        m_selectedClassId);
}

bool CharacterCreationService::ApplyBuild() noexcept
{
    PlayerCharacter* const pPlayer = PlayerCharacter::Get();
    if (!pPlayer)
    {
        spdlog::error(
            "[STRE][CharacterCreation] Build application failed: player unavailable");
        return false;
    }

    spdlog::info(
        "[STRE][CharacterCreation] Build application phase=removeStartingSpells");
    RemoveVanillaStartingSpells(pPlayer);

    spdlog::info(
        "[STRE][CharacterCreation] Build application phase=removeImportedMagicAndBlessings");
    RemoveImportedShoutsAndStandingStonePowers(pPlayer);

    spdlog::info(
        "[STRE][CharacterCreation] Build application phase=resolveItemGrants");

    std::vector<PendingItemGrant> grants;
    const auto appendGrant = [&grants](
                                 const char* apPluginName,
                                 std::uint32_t aLocalFormId,
                                 std::int32_t aCount)
    {
        if (!apPluginName || aLocalFormId == 0 || aCount <= 0)
            return;

        for (PendingItemGrant& grant : grants)
        {
            if (grant.localFormId == aLocalFormId &&
                std::strcmp(grant.pluginName, apPluginName) == 0)
            {
                grant.count += aCount;
                return;
            }
        }

        grants.push_back(
            PendingItemGrant{apPluginName, aLocalFormId, aCount});
    };

    for (const ClassItemGrantRule& grant : kClassItemGrantRules)
    {
        if (m_selectedClassId == grant.classId)
            appendGrant(grant.pluginName, grant.localFormId, grant.count);
    }

    for (const auto& [groupId, optionId] : m_selectedLoadoutOptions)
    {
        (void)groupId;
        for (const OptionItemGrantRule& grant : kOptionItemGrantRules)
        {
            if (optionId == grant.optionId)
                appendGrant(grant.pluginName, grant.localFormId, grant.count);
        }
    }

    std::size_t appliedGrantCount = 0;
    for (const PendingItemGrant& grant : grants)
    {
        const std::uint32_t formId =
            ResolvePluginFormId(grant.pluginName, grant.localFormId);
        TESBoundObject* const pObject =
            Cast<TESBoundObject>(TESForm::GetById(formId));
        if (!pObject)
        {
            spdlog::error(
                "[STRE][CharacterCreation] Build item resolution failed plugin={} localForm={:08X}",
                grant.pluginName,
                grant.localFormId);
            return false;
        }

        const std::int64_t currentCount =
            pPlayer->GetItemCountInInventory(pObject);
        const std::int64_t missingCount =
            static_cast<std::int64_t>(grant.count) - currentCount;
        if (missingCount > 0)
        {
            pPlayer->AddObjectToContainer(
                pObject,
                nullptr,
                static_cast<std::int32_t>(missingCount),
                nullptr);
        }

        const std::int64_t finalCount =
            pPlayer->GetItemCountInInventory(pObject);
        if (finalCount < grant.count)
        {
            spdlog::error(
                "[STRE][CharacterCreation] Build item grant failed form={:08X} expected={} actual={}",
                formId,
                grant.count,
                finalCount);
            return false;
        }

        ++appliedGrantCount;
        spdlog::info(
            "[STRE][CharacterCreation] Build item ensured form={:08X} count={}",
            formId,
            grant.count);
    }

    spdlog::info(
        "[STRE][CharacterCreation] Build application completed classId={} resolvedItemGrants={} unresolvedStreContent=deferred",
        m_selectedClassId,
        appliedGrantCount);
    return true;
}

void CharacterCreationService::AdvanceBuildApplication() noexcept
{
    PlayerCharacter* const pPlayer = PlayerCharacter::Get();
    if (!pPlayer)
    {
        ResetBuildApplicationState();
        Fail("STRE n'a pas pu accéder au personnage pendant le scellement.");
        return;
    }

    if (!m_inventoryWipeInitialized)
    {
        if (!CaptureInventoryWipePass(pPlayer))
        {
            ResetBuildApplicationState();
            Fail("STRE n'a pas pu préparer le vidage de l'inventaire.");
            return;
        }

        m_inventoryWipeInitialized = true;
    }

    while (m_inventoryWipeIndex < m_inventoryWipeFormIds.size())
    {
        const std::size_t itemIndex = m_inventoryWipeIndex;
        const std::uint32_t formId =
            m_inventoryWipeFormIds[m_inventoryWipeIndex++];

        TESBoundObject* const pObject =
            Cast<TESBoundObject>(TESForm::GetById(formId));
        if (!pObject)
        {
            spdlog::warn(
                "[STRE][CharacterCreation] Inventory wipe skipped vanished form pass={} index={}/{} form={:08X}",
                m_inventoryWipePass,
                itemIndex + 1,
                m_inventoryWipeFormIds.size(),
                formId);
            continue;
        }

        const std::int64_t currentCount =
            pPlayer->GetItemCountInInventory(pObject);
        if (currentCount <= 0)
            continue;

        const std::int32_t removalCount = static_cast<std::int32_t>(
            std::min<std::int64_t>(
                currentCount,
                std::numeric_limits<std::int32_t>::max()));

        // Remove one base form per update tick. Existing characters may contain
        // dozens of scripted, equipped and dynamically generated stacks. Skyrim's
        // bulk RemoveAllItems Papyrus path can recurse through all of them in one
        // call and crash. A staged native removal keeps the full anti-cheat wipe
        // while allowing the engine to settle between forms.
        spdlog::info(
            "[STRE][CharacterCreation] Inventory wipe removing pass={} index={}/{} form={:08X} count={} type={}",
            m_inventoryWipePass,
            itemIndex + 1,
            m_inventoryWipeFormIds.size(),
            formId,
            removalCount,
            static_cast<std::uint32_t>(pObject->formType));

        {
            ScopedInventoryOverride inventoryOverride;
            ScopedEquipOverride equipOverride;
            ScopedUnequipOverride unequipOverride;

            pPlayer->RemoveItem(
                pObject,
                removalCount,
                ITEM_REMOVE_REASON::kRemove,
                nullptr,
                nullptr);
        }

        spdlog::info(
            "[STRE][CharacterCreation] Inventory wipe removal issued pass={} form={:08X}",
            m_inventoryWipePass,
            formId);

        // Exactly one actual removal per update tick.
        return;
    }

    if (!CaptureInventoryWipePass(pPlayer))
    {
        ResetBuildApplicationState();
        Fail("STRE n'a pas pu vérifier le vidage de l'inventaire.");
        return;
    }

    if (!m_inventoryWipeFormIds.empty())
    {
        if (m_inventoryWipePass >= kMaxInventoryWipePasses)
        {
            spdlog::error(
                "[STRE][CharacterCreation] Inventory wipe exhausted pass limit remainingForms={}",
                m_inventoryWipeFormIds.size());
            ResetBuildApplicationState();
            Fail("L'inventaire n'a pas pu être entièrement vidé.");
        }
        return;
    }

    spdlog::info(
        "[STRE][CharacterCreation] Starting inventory sanitized path=stagedNativeInventoryWipe passes={}",
        m_inventoryWipePass);

    m_buildApplicationPending = false;
    m_inventoryWipeInitialized = false;
    m_inventoryWipeFormIds.clear();
    m_inventoryWipeIndex = 0;

    if (!ApplyBuild())
    {
        ResetBuildApplicationState();
        Fail("STRE n'a pas pu appliquer le paquetage de départ.");
        return;
    }

    m_buildConfirmed = true;

    spdlog::info(
        "[STRE][CharacterCreation] Build accepted classId={} buildVersion=2 application=complete",
        m_selectedClassId);

    ShowBuildConfirmed();
}

bool CharacterCreationService::CaptureInventoryWipePass(
    Actor* apPlayer) noexcept
{
    if (!apPlayer)
        return false;

    std::vector<std::uint32_t> formIds;
    const auto appendForm = [&formIds](TESForm* apForm)
    {
        TESBoundObject* const pObject = Cast<TESBoundObject>(apForm);
        if (pObject)
            formIds.push_back(pObject->formID);
    };

    if (TESContainer* const pContainer = apPlayer->GetContainer())
    {
        for (std::uint32_t i = 0; i < pContainer->count; ++i)
        {
            TESContainer::Entry* const pEntry = pContainer->entries[i];
            if (pEntry)
                appendForm(pEntry->form);
        }
    }

    if (ExtraContainerChanges::Data* const pChanges =
            apPlayer->GetContainerChanges();
        pChanges && pChanges->entries)
    {
        for (ExtraContainerChanges::Entry* const pEntry :
             *pChanges->entries)
        {
            if (pEntry)
                appendForm(pEntry->form);
        }
    }

    std::sort(formIds.begin(), formIds.end());
    formIds.erase(
        std::unique(formIds.begin(), formIds.end()),
        formIds.end());

    std::vector<std::uint32_t> remainingFormIds;
    remainingFormIds.reserve(formIds.size());

    for (const std::uint32_t formId : formIds)
    {
        TESBoundObject* const pObject =
            Cast<TESBoundObject>(TESForm::GetById(formId));
        if (pObject && apPlayer->GetItemCountInInventory(pObject) > 0)
            remainingFormIds.push_back(formId);
    }

    ++m_inventoryWipePass;
    m_inventoryWipeFormIds = std::move(remainingFormIds);
    m_inventoryWipeIndex = 0;

    spdlog::info(
        "[STRE][CharacterCreation] Inventory wipe pass captured pass={} remainingForms={} path=liveGameForms",
        m_inventoryWipePass,
        m_inventoryWipeFormIds.size());

    return true;
}

void CharacterCreationService::ResetBuildApplicationState() noexcept
{
    m_buildApplicationPending = false;
    m_inventoryWipeInitialized = false;
    m_inventoryWipePass = 0;
    m_inventoryWipeIndex = 0;
    m_inventoryWipeFormIds.clear();
}

void CharacterCreationService::RemoveVanillaStartingSpells(
    Actor* apPlayer) noexcept
{
    if (!apPlayer)
        return;

    for (const std::uint32_t formId :
         {kVanillaHealingSpell, kVanillaFlamesSpell})
    {
        MagicItem* const pSpell =
            Cast<MagicItem>(TESForm::GetById(formId));
        if (pSpell && apPlayer->RemoveSpell(pSpell))
        {
            spdlog::info(
                "[STRE][CharacterCreation] Removed vanilla starting spell form={:08X}",
                formId);
        }
    }
}

void CharacterCreationService::RemoveImportedShoutsAndStandingStonePowers(
    Actor* apPlayer) noexcept
{
    if (!apPlayer)
        return;

    std::vector<std::uint32_t> shoutFormIds;
    std::vector<std::uint32_t> learnedSpellFormIds;
    std::vector<std::uint32_t> importedPowerFormIds;
    std::vector<std::uint32_t> protectedRaceSpellFormIds;

    const auto appendUnique = [](std::vector<std::uint32_t>& arValues,
                                 std::uint32_t aFormId)
    {
        if (aFormId == 0 ||
            std::find(arValues.begin(), arValues.end(), aFormId) !=
                arValues.end())
        {
            return;
        }

        arValues.push_back(aFormId);
    };

    const auto containsFormId = [](
                                    const std::vector<std::uint32_t>& acValues,
                                    std::uint32_t aFormId)
    {
        return std::find(acValues.begin(), acValues.end(), aFormId) !=
            acValues.end();
    };

    // Protect spells inherited from the player base.
    TESActorBase* const pActorBase = Cast<TESActorBase>(apPlayer->baseForm);
    if (pActorBase && pActorBase->spellList.lists)
    {
        TESSpellList::Lists* const pLists = pActorBase->spellList.lists;

        if (pLists->spells)
        {
            for (std::uint32_t i = 0; i < pLists->spellCount; ++i)
            {
                SpellItem* const pSpell = pLists->spells[i];
                if (pSpell)
                {
                    appendUnique(
                        protectedRaceSpellFormIds,
                        pSpell->formID);
                }
            }
        }

        if (pLists->shouts)
        {
            for (std::uint32_t i = 0; i < pLists->shoultCount; ++i)
            {
                TESShout* const pShout = pLists->shouts[i];
                if (pShout)
                    appendUnique(shoutFormIds, pShout->formID);
            }
        }
    }

    // Race spell lists are exposed by SKSE. Protect them explicitly because
    // RaceMenu may materialize a racial power in addedSpells even though it is
    // conceptually inherited from the selected race.
    const void* const pGetRaceSpellCountAddress =
        m_world.ctx().at<PapyrusService>().Get("Race", "GetSpellCount");
    const void* const pGetNthRaceSpellAddress =
        m_world.ctx().at<PapyrusService>().Get("Race", "GetNthSpell");

    if (apPlayer->race &&
        pGetRaceSpellCountAddress &&
        pGetNthRaceSpellAddress)
    {
        PapyrusFunction<std::int32_t, TESRace> getRaceSpellCount(
            pGetRaceSpellCountAddress);
        PapyrusFunction<SpellItem*, TESRace, std::int32_t> getNthRaceSpell(
            pGetNthRaceSpellAddress);

        const std::int32_t raceSpellCount =
            std::max<std::int32_t>(
                0,
                getRaceSpellCount(apPlayer->race));

        for (std::int32_t i = 0; i < raceSpellCount; ++i)
        {
            SpellItem* const pSpell =
                getNthRaceSpell(apPlayer->race, i);
            if (pSpell)
            {
                appendUnique(
                    protectedRaceSpellFormIds,
                    pSpell->formID);
            }
        }
    }
    else
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Race spell protection fallback active raceAvailable={} getSpellCountAvailable={} getNthSpellAvailable={}",
            apPlayer->race != nullptr,
            pGetRaceSpellCountAddress != nullptr,
            pGetNthRaceSpellAddress != nullptr);
    }

    // Standing Stones add Power, Lesser Power and Ability records dynamically.
    // Classify them by native SpellType rather than Editor ID: Skyrim.esm
    // Editor IDs are often absent at runtime, which made the Doom* detector
    // miss the Serpent Stone entirely.
    auto& addedSpells = apPlayer->addedSpells;
    TESForm** pAddedForms = nullptr;
    if (addedSpells.capacity >= 0)
    {
        pAddedForms = reinterpret_cast<TESForm**>(addedSpells.data);
    }
    else
    {
        pAddedForms = reinterpret_cast<TESForm**>(&addedSpells.data);
    }

    if (pAddedForms)
    {
        for (std::uint32_t i = 0; i < addedSpells.size; ++i)
        {
            TESForm* const pForm = pAddedForms[i];
            if (!pForm)
                continue;

            if (Cast<TESShout>(pForm))
            {
                appendUnique(shoutFormIds, pForm->formID);
                continue;
            }

            SpellItem* const pSpell = Cast<SpellItem>(pForm);
            if (!pSpell)
                continue;

            if (containsFormId(
                    protectedRaceSpellFormIds,
                    pSpell->formID))
            {
                spdlog::info(
                    "[STRE][CharacterCreation] Protected racial spell retained form={:08X} spellType={} source=addedSpells",
                    pSpell->formID,
                    static_cast<std::int32_t>(
                        GetSpellType(pSpell)));
                continue;
            }

            const MagicSystem::SpellType spellType = GetSpellType(pSpell);
            if (spellType == MagicSystem::SPELL)
            {
                const std::size_t previousCount =
                    learnedSpellFormIds.size();
                appendUnique(learnedSpellFormIds, pSpell->formID);
                if (learnedSpellFormIds.size() != previousCount)
                {
                    spdlog::info(
                        "[STRE][CharacterCreation] Learned spell candidate form={:08X} source=addedSpells",
                        pSpell->formID);
                }
                continue;
            }

            if (!IsImportedPowerOrAbility(pSpell))
                continue;

            const std::size_t previousCount =
                importedPowerFormIds.size();
            appendUnique(importedPowerFormIds, pSpell->formID);
            if (importedPowerFormIds.size() != previousCount)
            {
                spdlog::info(
                    "[STRE][CharacterCreation] Imported power candidate form={:08X} spellType={} source=addedSpells",
                    pSpell->formID,
                    static_cast<std::int32_t>(spellType));
            }
        }
    }

    // Imported saves can expose dynamically learned powers through the live
    // spell chain even when addedSpells is incomplete. Apply the same
    // SpellType classification and racial allow-list here.
    std::size_t spellEntryGuard = 0;
    for (Actor::SpellItemEntry* pEntry = apPlayer->spellItemHead;
         pEntry && spellEntryGuard < 4096;
         pEntry = pEntry->pNext, ++spellEntryGuard)
    {
        SpellItem* const pSpell = pEntry->pItem;
        if (!pSpell)
            continue;

        if (containsFormId(
                protectedRaceSpellFormIds,
                pSpell->formID))
        {
            continue;
        }

        const MagicSystem::SpellType spellType = GetSpellType(pSpell);
        if (spellType == MagicSystem::SPELL)
        {
            const std::size_t previousCount =
                learnedSpellFormIds.size();
            appendUnique(learnedSpellFormIds, pSpell->formID);
            if (learnedSpellFormIds.size() != previousCount)
            {
                spdlog::info(
                    "[STRE][CharacterCreation] Learned spell candidate form={:08X} source=liveSpellChain",
                    pSpell->formID);
            }
            continue;
        }

        if (!IsImportedPowerOrAbility(pSpell))
            continue;

        const std::size_t previousCount =
            importedPowerFormIds.size();
        appendUnique(importedPowerFormIds, pSpell->formID);
        if (importedPowerFormIds.size() != previousCount)
        {
            spdlog::info(
                "[STRE][CharacterCreation] Imported power candidate form={:08X} spellType={} source=liveSpellChain",
                pSpell->formID,
                static_cast<std::int32_t>(spellType));
        }
    }

    const void* const pUnequipSpellAddress =
        m_world.ctx().at<PapyrusService>().Get("Actor", "UnequipSpell");

    std::size_t removedLearnedSpells = 0;
    for (const std::uint32_t formId : learnedSpellFormIds)
    {
        SpellItem* const pSpell =
            Cast<SpellItem>(TESForm::GetById(formId));
        if (!pSpell)
            continue;

        if (pUnequipSpellAddress)
        {
            PapyrusFunction<void, Actor, SpellItem*, std::int32_t>
                unequipSpell(pUnequipSpellAddress);
            unequipSpell(
                apPlayer,
                pSpell,
                static_cast<std::int32_t>(MagicSystem::LEFT_HAND));
            unequipSpell(
                apPlayer,
                pSpell,
                static_cast<std::int32_t>(MagicSystem::RIGHT_HAND));
            unequipSpell(
                apPlayer,
                pSpell,
                static_cast<std::int32_t>(MagicSystem::OTHER));
        }

        if (apPlayer->RemoveSpell(pSpell))
        {
            ++removedLearnedSpells;
            spdlog::info(
                "[STRE][CharacterCreation] Removed learned spell form={:08X}",
                formId);
        }
        else
        {
            spdlog::warn(
                "[STRE][CharacterCreation] Learned spell removal returned false form={:08X}",
                formId);
        }
    }

    std::size_t removedImportedPowers = 0;
    for (const std::uint32_t formId : importedPowerFormIds)
    {
        SpellItem* const pSpell =
            Cast<SpellItem>(TESForm::GetById(formId));
        if (!pSpell)
            continue;

        if (pUnequipSpellAddress)
        {
            PapyrusFunction<void, Actor, SpellItem*, std::int32_t>
                unequipSpell(pUnequipSpellAddress);
            unequipSpell(
                apPlayer,
                pSpell,
                static_cast<std::int32_t>(MagicSystem::OTHER));
        }

        if (apPlayer->RemoveSpell(pSpell))
        {
            ++removedImportedPowers;
            spdlog::info(
                "[STRE][CharacterCreation] Removed imported power form={:08X} spellType={}",
                formId,
                static_cast<std::int32_t>(GetSpellType(pSpell)));
        }
        else
        {
            spdlog::warn(
                "[STRE][CharacterCreation] Imported power removal returned false form={:08X} spellType={}",
                formId,
                static_cast<std::int32_t>(GetSpellType(pSpell)));
        }
    }

    std::size_t removedShouts = 0;
    const void* const pUnequipShoutAddress =
        m_world.ctx().at<PapyrusService>().Get("Actor", "UnequipShout");
    const void* const pRemoveShoutAddress =
        m_world.ctx().at<PapyrusService>().Get("Actor", "RemoveShout");

    if (!shoutFormIds.empty() && !pRemoveShoutAddress)
    {
        spdlog::error(
            "[STRE][CharacterCreation] Unable to remove imported shouts: Actor.RemoveShout is unavailable");
    }
    else if (pRemoveShoutAddress)
    {
        PapyrusFunction<bool, Actor, TESShout*> removeShout(
            pRemoveShoutAddress);

        for (const std::uint32_t formId : shoutFormIds)
        {
            TESShout* const pShout =
                Cast<TESShout>(TESForm::GetById(formId));
            if (!pShout)
                continue;

            if (pUnequipShoutAddress)
            {
                PapyrusFunction<void, Actor, TESShout*> unequipShout(
                    pUnequipShoutAddress);
                unequipShout(apPlayer, pShout);
            }

            if (removeShout(apPlayer, pShout))
            {
                ++removedShouts;
                spdlog::info(
                    "[STRE][CharacterCreation] Removed imported shout form={:08X}",
                    formId);
            }
        }
    }

    // Shrine blessings are active effects cast on the actor and generally are
    // not present in the known-spell arrays. An immediate dispel removes them
    // and all other temporary imported buffs. Known racial spells are not
    // deleted; their records were explicitly protected above.
    apPlayer->DispelAllSpells(true);
    spdlog::info(
        "[STRE][CharacterCreation] Active temporary magic dispelled path=DispelAllSpells immediate=true");

    spdlog::info(
        "[STRE][CharacterCreation] Imported magic sanitized shoutsFound={} shoutsRemoved={} learnedSpellsFound={} learnedSpellsRemoved={} importedPowersFound={} importedPowersRemoved={} protectedRaceSpells={} activeMagicDispelled=true",
        shoutFormIds.size(),
        removedShouts,
        learnedSpellFormIds.size(),
        removedLearnedSpells,
        importedPowerFormIds.size(),
        removedImportedPowers,
        protectedRaceSpellFormIds.size());
}

void CharacterCreationService::ReopenClassSelection() noexcept
{
    if (m_phase != CharacterCreationPhase::LoadoutSelection &&
        m_phase != CharacterCreationPhase::BuildSummary)
    {
        return;
    }

    ShowClassSelection();
}

void CharacterCreationService::ReopenLoadoutSelection() noexcept
{
    if (m_phase != CharacterCreationPhase::BuildSummary)
    {
        return;
    }

    if (!m_raceConfirmed || !m_classConfirmed)
        return;

    ShowLoadoutSelection();
}

void CharacterCreationService::PreviewLoadoutItem(
    std::string aPreviewKey) noexcept
{
    if (m_phase != CharacterCreationPhase::LoadoutSelection &&
        m_phase != CharacterCreationPhase::BuildSummary)
    {
        return;
    }

    const std::uint32_t formId = ResolvePreviewFormId(aPreviewKey);
    if (formId == 0)
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Unknown loadout preview key={}",
            aPreviewKey);
        ClearLoadoutPreview();
        return;
    }

    m_world.ctx()
        .at<TradeItemPreviewService>()
        .SelectGameForm(formId, formId);

    spdlog::info(
        "[STRE][CharacterCreation] Loadout preview selected key={} gameForm={:08X}",
        aPreviewKey,
        formId);
}

void CharacterCreationService::SetLoadoutPreviewRegion(
    std::string aRegion) noexcept
{
    float left{};
    float top{};
    float width{};
    float height{};

    if (std::sscanf(
            aRegion.c_str(),
            "%f,%f,%f,%f",
            &left,
            &top,
            &width,
            &height) != 4 ||
        !std::isfinite(left) ||
        !std::isfinite(top) ||
        !std::isfinite(width) ||
        !std::isfinite(height))
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Invalid loadout preview region: {}",
            aRegion);
        return;
    }

    if (m_phase != CharacterCreationPhase::LoadoutSelection &&
        m_phase != CharacterCreationPhase::BuildSummary)
    {
        left = 0.0F;
        top = 0.0F;
        width = 0.0F;
        height = 0.0F;
    }

    m_world.ctx()
        .at<TradeItemPreviewService>()
        .SetPreviewRegion(left, top, width, height);
}

void CharacterCreationService::ClearLoadoutPreview() noexcept
{
    m_world.ctx().at<TradeItemPreviewService>().Clear();
}

void CharacterCreationService::RetryRaceMenu() noexcept
{
    if (m_phase != CharacterCreationPhase::Error)
        return;

    OpenRaceMenu();
}

void CharacterCreationService::RecoverControls() noexcept
{
    spdlog::warn(
        "[STRE][CharacterCreation] Recovery path executed");

    ClearLoadoutPreview();

    if (!UnlockCharacterCreationControls())
    {
        Fail("Le déverrouillage de secours a échoué.");
        return;
    }

    m_uiSurfaceService.SetSurface(UiSurface::None);
    m_phase = CharacterCreationPhase::Inactive;
    m_phaseElapsed = 0.0;
    m_raceConfirmed = false;
    m_classConfirmed = false;
    m_selectedClassId.clear();
    ResetLoadoutState();
    m_error.clear();
    m_suppressStageRecovery = true;
    PushState(true);
}

void CharacterCreationService::Fail(
    std::string aMessage) noexcept
{
    ResetBuildApplicationState();
    ClearLoadoutPreview();
    m_phase = CharacterCreationPhase::Error;
    m_phaseElapsed = 0.0;
    m_error = std::move(aMessage);

    spdlog::error(
        "[STRE][CharacterCreation] {}",
        m_error);

    // Keep the player locked, but expose retry and emergency recovery.
    m_uiSurfaceService.SetSurface(UiSurface::CharacterCreation);
    PushState(true);
}

void CharacterCreationService::ResetLoadoutState() noexcept
{
    ResetBuildApplicationState();
    m_selectedLoadoutOptions.clear();
    m_loadoutConfirmed = false;
    m_buildConfirmed = false;
}

TESQuest* CharacterCreationService::FindAlternateStartQuest() const noexcept
{
    ModManager* const pModManager = ModManager::Get();
    if (!pModManager)
        return nullptr;

    for (TESQuest* const pQuest : pModManager->quests)
    {
        if (IsAlternateStartQuest(pQuest))
            return pQuest;
    }

    return nullptr;
}

bool CharacterCreationService::IsAlternateStartQuest(
    const TESQuest* apQuest) const noexcept
{
    if (!apQuest)
        return false;

    // BSString::AsAscii is not const in this reverse-engineered wrapper.
    const char* const pEditorId =
        const_cast<TESQuest*>(apQuest)->idName.AsAscii();

    return pEditorId &&
           std::strcmp(
               pEditorId,
               kAlternateStartQuestEditorId) == 0;
}

bool CharacterCreationService::IsRaceMenuOpen() const noexcept
{
    static BSFixedString s_raceMenuName{"RaceSex Menu"};

    UI* const pUi = UI::Get();
    return pUi && pUi->GetMenuOpen(s_raceMenuName);
}

bool CharacterCreationService::IsSupportedClassId(
    const std::string& acClassId) const noexcept
{
    return acClassId == kWarriorClassId ||
           acClassId == kMageClassId ||
           acClassId == kThiefClassId;
}

bool CharacterCreationService::IsSupportedLoadoutOption(
    const std::string& acGroupId,
    const std::string& acOptionId) const noexcept
{
    for (const LoadoutOptionRule& rule : kLoadoutOptionRules)
    {
        if (m_selectedClassId == rule.classId &&
            acGroupId == rule.groupId &&
            acOptionId == rule.optionId)
        {
            return true;
        }
    }

    return false;
}

bool CharacterCreationService::HasCompleteLoadout() const noexcept
{
    bool foundRequiredGroup = false;

    for (const RequiredLoadoutGroup& required : kRequiredLoadoutGroups)
    {
        if (m_selectedClassId != required.classId)
            continue;

        if (!IsLoadoutGroupActive(required.groupId))
            continue;

        foundRequiredGroup = true;
        const auto selection =
            m_selectedLoadoutOptions.find(required.groupId);
        if (selection == m_selectedLoadoutOptions.end() ||
            !IsSupportedLoadoutOption(
                selection->first,
                selection->second))
        {
            return false;
        }
    }

    return foundRequiredGroup;
}

bool CharacterCreationService::IsLoadoutGroupActive(
    const std::string& acGroupId) const noexcept
{
    for (const RequiredLoadoutGroup& required : kRequiredLoadoutGroups)
    {
        if (m_selectedClassId != required.classId ||
            acGroupId != required.groupId)
        {
            continue;
        }

        if (!required.conditionGroupId || !required.conditionOptionId)
            return true;

        const auto condition =
            m_selectedLoadoutOptions.find(required.conditionGroupId);
        return condition != m_selectedLoadoutOptions.end() &&
               condition->second == required.conditionOptionId;
    }

    return true;
}

std::uint32_t CharacterCreationService::ResolvePreviewFormId(
    const std::string& acPreviewKey) const noexcept
{
    for (const PreviewFormRule& preview : kPreviewForms)
    {
        if (acPreviewKey == preview.key)
        {
            return ResolvePluginFormId(
                preview.pluginName,
                preview.localFormId);
        }
    }

    return 0;
}

std::uint32_t CharacterCreationService::ResolvePluginFormId(
    const char* apPluginName,
    std::uint32_t aLocalFormId) const noexcept
{
    if (!apPluginName || aLocalFormId == 0)
        return 0;

    ModManager* const pModManager = ModManager::Get();
    if (!pModManager)
        return 0;

    Mod* const pMod = pModManager->GetByName(apPluginName);
    if (!pMod)
    {
        spdlog::warn(
            "[STRE][CharacterCreation] Plugin missing plugin={} localForm={:08X}",
            apPluginName,
            aLocalFormId);
        return 0;
    }

    return pMod->GetFormId(aLocalFormId);
}

bool CharacterCreationService::LockCharacterCreationControls() noexcept
{
    if (m_controlsLocked)
        return true;

    if (!SetPlayerActorLock(true))
        return false;

    m_controlsLocked = true;
    spdlog::info(
        "[STRE][CharacterCreation] Controls locked");
    return true;
}

bool CharacterCreationService::UnlockCharacterCreationControls() noexcept
{
    if (!m_controlsLocked)
        return true;

    if (!SetPlayerActorLock(false))
    {
        spdlog::error(
            "[STRE][CharacterCreation] Failed to unlock controls");
        return false;
    }

    m_controlsLocked = false;
    spdlog::info(
        "[STRE][CharacterCreation] Controls unlocked");
    return true;
}

bool CharacterCreationService::SetPlayerActorLock(
    bool aLocked) noexcept
{
    Actor* const pPlayer = PlayerCharacter::Get();
    PlayerControls* const pControls =
        PlayerControls::GetInstance();
    if (!pPlayer || !pControls)
        return false;

    const void* const pSetDontMove =
        m_world.ctx()
            .at<PapyrusService>()
            .Get("Actor", "SetDontMove");
    if (!pSetDontMove)
    {
        spdlog::error(
            "[STRE][CharacterCreation] Actor.SetDontMove is unavailable");
        return false;
    }

    PapyrusFunction<void, Actor, bool> setDontMove(
        pSetDontMove);

    const std::array<PlayerInputHandler*,
                     kLockedInputHandlerCount> handlers{
        pControls->pMovementHandler,
        pControls->pSprintHandler,
        pControls->pReadyWeaponHandler,
        pControls->pAutoMoveHandler,
        pControls->pToggleRunHandler,
        pControls->pActivateHandler,
        pControls->pJumpHandler,
        pControls->shoutHandler,
        pControls->attackBlockHandler,
        pControls->runHandler,
        pControls->sneakHandler};

    if (aLocked)
    {
        setDontMove(pPlayer, true);

        for (std::size_t i = 0; i < handlers.size(); ++i)
        {
            PlayerInputHandler* const pHandler = handlers[i];
            m_inputHandlerSnapshot[i] = {
                pHandler != nullptr,
                pHandler && pHandler->isEnabled};

            if (pHandler)
                pHandler->isEnabled = false;
        }

        m_inputSnapshotValid = true;
        return true;
    }

    setDontMove(pPlayer, false);

    if (m_inputSnapshotValid)
    {
        for (std::size_t i = 0; i < handlers.size(); ++i)
        {
            PlayerInputHandler* const pHandler = handlers[i];
            const InputHandlerSnapshot& snapshot =
                m_inputHandlerSnapshot[i];

            if (snapshot.present && pHandler)
                pHandler->isEnabled = snapshot.enabled;
        }
    }

    m_inputSnapshotValid = false;
    return true;
}

bool CharacterCreationService::InvokeShowRaceMenu() noexcept
{
    const void* const pShowRaceMenu =
        m_world.ctx()
            .at<PapyrusService>()
            .Get("Game", "ShowRaceMenu");

    if (pShowRaceMenu)
    {
        GlobalPapyrusFunction<void> showRaceMenu(
            pShowRaceMenu);
        showRaceMenu();
        return true;
    }

    // Some vanilla global Papyrus functions are registered before STRE's
    // NativeFunction hook starts observing registrations. In that case the
    // function is valid in Papyrus but absent from PapyrusService. Opening the
    // registered native menu through Skyrim's UI queue is the equivalent
    // engine-side fallback; the update loop still verifies that it actually
    // opened and reports a timeout if it did not.
    UI* const pUi = UI::Get();
    if (!pUi)
    {
        spdlog::error(
            "[STRE][CharacterCreation] Skyrim UI singleton unavailable");
        return false;
    }

    static BSFixedString s_raceMenuName{"RaceSex Menu"};
    if (!pUi->HasMenuRegistration(s_raceMenuName))
    {
        spdlog::error(
            "[STRE][CharacterCreation] RaceSex Menu is not registered");
        return false;
    }

    spdlog::warn(
        "[STRE][CharacterCreation] Game.ShowRaceMenu was not captured; "
        "using the native UI queue fallback");
    pUi->QueueMessage(s_raceMenuName, UIMessage::kShow);
    return true;
}

void CharacterCreationService::PushState(
    bool aForce) noexcept
{
    const auto pOverlay =
        m_uiSurfaceService.GetOverlayService().GetOverlayApp();
    if (!pOverlay)
        return;

    const std::string stateJson = BuildStateJson();
    if (!aForce && stateJson == m_lastStateJson)
        return;

    m_lastStateJson = stateJson;

    auto arguments = CefListValue::Create();
    arguments->SetString(0, stateJson);
    pOverlay->ExecuteAsync(
        "characterCreationState",
        arguments);
}

std::string CharacterCreationService::BuildStateJson() const
{
    std::string output;
    output.reserve(2048);

    output += "{\"visible\":";
    output +=
        m_phase == CharacterCreationPhase::RaceReview ||
                m_phase == CharacterCreationPhase::ClassSelection ||
                m_phase == CharacterCreationPhase::LoadoutSelection ||
                m_phase == CharacterCreationPhase::BuildSummary ||
                m_phase == CharacterCreationPhase::BuildConfirmed ||
                m_phase == CharacterCreationPhase::Error
            ? "true"
            : "false";
    output += ",\"phase\":";
    AppendJsonString(output, PhaseName(m_phase));
    output += ",\"controlsLocked\":";
    output += m_controlsLocked ? "true" : "false";
    output += ",\"raceConfirmed\":";
    output += m_raceConfirmed ? "true" : "false";

    std::uint32_t raceFormId = 0;
    std::string raceName;
    if (const PlayerCharacter* const pPlayer = PlayerCharacter::Get())
    {
        if (const TESRace* const pRace = pPlayer->race)
        {
            raceFormId = pRace->formID;
            const char* const pRaceName =
                const_cast<TESRace*>(pRace)->value.AsAscii();
            if (pRaceName)
                raceName = pRaceName;
        }
    }

    output += ",\"raceFormId\":";
    output += std::to_string(raceFormId);
    output += ",\"raceName\":";
    AppendJsonString(output, raceName);
    output += ",\"selectedClassId\":";
    AppendJsonString(output, m_selectedClassId);
    output += ",\"classConfirmed\":";
    output += m_classConfirmed ? "true" : "false";
    output += ",\"selectedLoadoutOptions\":{";

    bool firstSelection = true;
    for (const auto& [groupId, optionId] : m_selectedLoadoutOptions)
    {
        if (!firstSelection)
            output.push_back(',');
        firstSelection = false;

        AppendJsonString(output, groupId);
        output.push_back(':');
        AppendJsonString(output, optionId);
    }

    output += "},\"loadoutConfirmed\":";
    output += m_loadoutConfirmed ? "true" : "false";
    output += ",\"buildConfirmed\":";
    output += m_buildConfirmed ? "true" : "false";
    output += ",\"error\":";
    AppendJsonString(output, m_error);
    output += "}";

    return output;
}
