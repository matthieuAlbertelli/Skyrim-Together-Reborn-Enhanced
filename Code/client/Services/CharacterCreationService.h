#pragma once

#include <Events/EventDispatcher.h>
#include <Games/Events.h>
#include <Structs/CharacterBuild.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

struct Actor;
struct CharacterBuildResponse;
struct DisconnectedEvent;
struct NotifyCharacterBuildState;
struct PlayerCharacter;
struct TESQuest;
struct UiSurfaceService;
struct UpdateEvent;
struct World;

enum class CharacterCreationPhase : std::uint8_t
{
    Inactive,
    WaitingForRaceMenuOpen,
    WaitingForRaceMenuClose,
    RaceReview,
    ClassSelection,
    LoadoutSelection,
    BuildSummary,
    BuildConfirmed,
    Error
};

/**
 * @brief Owns the local STRE character-creation flow started by
 * STRE_QUEST_AlternateStart stage 20.
 *
 * The Creation Kit quest remains responsible for teleporting and seating the
 * player. This service owns the control lock, RaceMenu lifecycle, class and
 * loadout selection, build validation and CEF UI hand-off. It deliberately
 * does not depend on party or quest-sync state.
 */
class CharacterCreationService final
    : public BSTEventSink<TESQuestStageEvent>
    , public BSTEventSink<TESQuestStartStopEvent>
{
public:
    CharacterCreationService(
        World& aWorld,
        UiSurfaceService& aUiSurfaceService,
        entt::dispatcher& aDispatcher) noexcept;
    ~CharacterCreationService() noexcept override;

    TP_NOCOPYMOVE(CharacterCreationService);

    void ModifyRace() noexcept;
    void ConfirmRace() noexcept;
    void SelectClass(std::string aClassId) noexcept;
    void ConfirmClass() noexcept;
    void ReopenClassSelection() noexcept;

    void SelectLoadoutOption(std::string aSelection) noexcept;
    void ConfirmLoadout() noexcept;
    void ReopenLoadoutSelection() noexcept;
    void ConfirmBuild() noexcept;

    void PreviewLoadoutItem(std::string aPreviewKey) noexcept;
    void SetLoadoutPreviewRegion(std::string aRegion) noexcept;
    void ClearLoadoutPreview() noexcept;

    void RetryRaceMenu() noexcept;
    void RecoverControls() noexcept;

    [[nodiscard]] CharacterCreationPhase GetPhase() const noexcept
    {
        return m_phase;
    }

    [[nodiscard]] bool IsRaceConfirmed() const noexcept
    {
        return m_raceConfirmed;
    }

    [[nodiscard]] bool IsClassConfirmed() const noexcept
    {
        return m_classConfirmed;
    }

    [[nodiscard]] bool IsLoadoutConfirmed() const noexcept
    {
        return m_loadoutConfirmed;
    }

    [[nodiscard]] const std::string& GetSelectedClassId() const noexcept
    {
        return m_selectedClassId;
    }

private:
    BSTEventResult OnEvent(
        const TESQuestStartStopEvent* apEvent,
        const EventDispatcher<TESQuestStartStopEvent>* apSender) override;
    BSTEventResult OnEvent(
        const TESQuestStageEvent* apEvent,
        const EventDispatcher<TESQuestStageEvent>* apSender) override;

    void OnUpdate(const UpdateEvent& acEvent) noexcept;
    void OnCharacterBuildResponse(const CharacterBuildResponse& acMessage) noexcept;
    void OnNotifyCharacterBuildState(const NotifyCharacterBuildState& acMessage) noexcept;
    void OnDisconnected(const DisconnectedEvent& acEvent) noexcept;
    [[nodiscard]] bool ResetForFreshCharacterCreation() noexcept;
    void BeginFromStage20(TESQuest* apQuest) noexcept;
    void OpenRaceMenu() noexcept;
    void ShowRaceReview() noexcept;
    void ShowClassSelection() noexcept;
    void ShowLoadoutSelection() noexcept;
    void ShowBuildSummary() noexcept;
    void ShowBuildConfirmed() noexcept;
    void FinalizeCompletedBuild() noexcept;
    void Fail(std::string aMessage) noexcept;

    void ResetLoadoutState() noexcept;
    void AdvanceBuildApplication() noexcept;
    [[nodiscard]] bool CaptureInventoryWipePass(Actor* apPlayer) noexcept;
    void ResetBuildApplicationState() noexcept;
    [[nodiscard]] bool ApplyBuild() noexcept;
    [[nodiscard]] bool ApplyCanonicalServerInventory(Actor* apPlayer) noexcept;
    [[nodiscard]] bool ApplyCanonicalServerSpells(PlayerCharacter* apPlayer) noexcept;
    [[nodiscard]] bool EquipCanonicalInventory(Actor* apPlayer, const Inventory& acInventory) noexcept;
    [[nodiscard]] bool ApplyLocalBuildSpells(PlayerCharacter* apPlayer) noexcept;
    [[nodiscard]] bool EnsurePlayerSpell(
        PlayerCharacter* apPlayer,
        std::uint32_t aFormId) noexcept;
    [[nodiscard]] bool EquipLocalBuild(PlayerCharacter* apPlayer) noexcept;
    [[nodiscard]] bool ResetPlayerProgression(PlayerCharacter* apPlayer) noexcept;
    [[nodiscard]] bool SendAuthoritativeBuildRequest() noexcept;
    [[nodiscard]] bool SendBuildAppliedAcknowledgement(
        PlayerCharacter* apPlayer) noexcept;
    void ResetNetworkBuildState() noexcept;
    void ApplyRemoteCanonicalInventory(const NotifyCharacterBuildState& acMessage) noexcept;
    void RemoveVanillaStartingSpells(Actor* apPlayer) noexcept;
    void RemoveImportedShoutsAndStandingStonePowers(
        Actor* apPlayer) noexcept;

    [[nodiscard]] TESQuest* FindAlternateStartQuest() const noexcept;
    [[nodiscard]] bool IsAlternateStartQuest(
        const TESQuest* apQuest) const noexcept;
    [[nodiscard]] bool IsRaceMenuOpen() const noexcept;
    [[nodiscard]] bool IsSupportedClassId(
        const std::string& acClassId) const noexcept;
    [[nodiscard]] bool IsSupportedLoadoutOption(
        const std::string& acGroupId,
        const std::string& acOptionId) const noexcept;
    [[nodiscard]] bool HasCompleteLoadout() const noexcept;
    [[nodiscard]] bool IsLoadoutGroupActive(
        const std::string& acGroupId) const noexcept;
    [[nodiscard]] std::uint32_t ResolvePreviewFormId(
        const std::string& acPreviewKey) const noexcept;
    [[nodiscard]] std::uint32_t ResolvePluginFormId(
        const char* apPluginName,
        std::uint32_t aLocalFormId) const noexcept;

    [[nodiscard]] bool LockCharacterCreationControls() noexcept;
    [[nodiscard]] bool UnlockCharacterCreationControls() noexcept;
    [[nodiscard]] bool SetPlayerActorLock(bool aLocked) noexcept;

    struct InputHandlerSnapshot
    {
        bool present{};
        bool enabled{};
    };

    static constexpr std::size_t kLockedInputHandlerCount = 11;
    [[nodiscard]] bool InvokeShowRaceMenu() noexcept;

    void PushState(bool aForce = false) noexcept;
    [[nodiscard]] std::string BuildStateJson() const;

    World& m_world;
    UiSurfaceService& m_uiSurfaceService;

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_buildResponseConnection;
    entt::scoped_connection m_buildStateConnection;
    entt::scoped_connection m_disconnectedConnection;

    TESQuest* m_pQuest{};
    CharacterCreationPhase m_phase{CharacterCreationPhase::Inactive};
    bool m_controlsLocked{};
    bool m_raceConfirmed{};
    bool m_classConfirmed{};
    bool m_loadoutConfirmed{};
    bool m_buildConfirmed{};
    bool m_buildApplicationPending{};
    bool m_preGrantCharacterResetApplied{};
    bool m_inventoryWipeInitialized{};
    bool m_serverBuildRequestPending{};
    bool m_serverBuildAccepted{};
    bool m_waitingForServerFinalization{};
    bool m_suppressStageRecovery{};
    bool m_freshQuestStartPending{};
    bool m_inputSnapshotValid{};
    std::array<InputHandlerSnapshot, kLockedInputHandlerCount>
        m_inputHandlerSnapshot{};
    double m_phaseElapsed{};
    double m_recoveryAccumulator{};
    std::uint8_t m_inventoryWipePass{};
    std::size_t m_inventoryWipeIndex{};
    std::uint64_t m_serverBuildRevision{};
    std::uint32_t m_serverCharacterId{};
    Inventory m_serverCanonicalInventory{};
    Vector<GameId> m_serverCanonicalSpells{};
    std::uint64_t m_serverSpellHash{};
    std::vector<std::uint32_t> m_inventoryWipeFormIds;
    std::string m_selectedClassId;
    std::map<std::string, std::string> m_selectedLoadoutOptions;
    std::string m_error;
    std::string m_lastStateJson;
};
