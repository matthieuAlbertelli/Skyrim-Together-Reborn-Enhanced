#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace STRE::Campaign
{
enum class CampaignSaveOrigin : std::uint8_t
{
    Manual,
    Quick,
    Auto,
    Unknown
};

enum class CampaignSaveRuntimeState : std::uint8_t
{
    Unavailable,
    WaitingForRoster,
    Active,
    Checkpointing,
    RecoveryLock,
    RestoringCheckpoint
};

enum class CampaignSaveDecision : std::uint8_t
{
    AllowVanilla,
    AllowInternalCheckpoint,
    RequestCollectiveCheckpoint,
    CoalesceWithCheckpoint,
    BlockAutosave,
    BlockUnavailable,
    BlockUnknown
};

struct CampaignSavePolicyContext
{
    bool InCampaign{};
    bool RuntimeFenced{};
    bool InternalCheckpoint{};
    CampaignSaveRuntimeState RuntimeState{
        CampaignSaveRuntimeState::Unavailable};
};

[[nodiscard]] CampaignSaveOrigin ClassifyCampaignNativeSaveName(
    std::string_view acNativeSaveName) noexcept;

[[nodiscard]] CampaignSaveDecision EvaluateCampaignSavePolicy(
    CampaignSaveOrigin aOrigin,
    const CampaignSavePolicyContext& acContext) noexcept;

// The native Quick path creates one request synchronously from the actionable
// ProcessButton call, then executes it later through the save/load process
// queue. Only that exact live request pointer may carry Quick provenance.
class ScopedCampaignQuickSaveAction final
{
public:
    explicit ScopedCampaignQuickSaveAction(bool aActionable) noexcept;
    ~ScopedCampaignQuickSaveAction() noexcept;

    ScopedCampaignQuickSaveAction(
        const ScopedCampaignQuickSaveAction&) = delete;
    ScopedCampaignQuickSaveAction& operator=(
        const ScopedCampaignQuickSaveAction&) = delete;

private:
    bool m_active{};
};

class ScopedCampaignQuickSaveProcessBoundary final
{
public:
    ScopedCampaignQuickSaveProcessBoundary() noexcept = default;
    ~ScopedCampaignQuickSaveProcessBoundary() noexcept;

    ScopedCampaignQuickSaveProcessBoundary(
        const ScopedCampaignQuickSaveProcessBoundary&) = delete;
    ScopedCampaignQuickSaveProcessBoundary& operator=(
        const ScopedCampaignQuickSaveProcessBoundary&) = delete;
};

class ScopedCampaignManualNewSlotSave final
{
public:
    ScopedCampaignManualNewSlotSave() noexcept;
    ~ScopedCampaignManualNewSlotSave() noexcept;

    ScopedCampaignManualNewSlotSave(
        const ScopedCampaignManualNewSlotSave&) = delete;
    ScopedCampaignManualNewSlotSave& operator=(
        const ScopedCampaignManualNewSlotSave&) = delete;
};

class CampaignSaveProvenance final
{
public:
    static constexpr std::uint32_t QuickRequestOperationCode = 0xF0000200;

    static void ObserveQuickRequestPush(
        const void* apRequest,
        std::uint32_t aOperationCode,
        bool aSucceeded) noexcept;
    static void ObserveQuickRequestPop(
        const void* apRequest,
        std::uint32_t aOperationCode) noexcept;

    // Returns explicit provenance once. Filename classification remains the
    // fail-closed fallback when no proven transport is active.
    [[nodiscard]] static std::optional<CampaignSaveOrigin> Consume() noexcept;

    // Drops a popped request that was consumed/coalesced/failed without ever
    // reaching Save_Impl. Successfully requeued requests remain correlated.
    static void FinishQuickProcessBoundary() noexcept;
};

// The managed #55 call crosses the same Save_Impl hook as vanilla. This scoped
// provenance is the only bypass; a caller cannot obtain it by choosing a
// filename in the stre- namespace.
class ScopedCampaignCheckpointNativeSave final
{
public:
    ScopedCampaignCheckpointNativeSave() noexcept;
    ~ScopedCampaignCheckpointNativeSave() noexcept;

    ScopedCampaignCheckpointNativeSave(
        const ScopedCampaignCheckpointNativeSave&) = delete;
    ScopedCampaignCheckpointNativeSave& operator=(
        const ScopedCampaignCheckpointNativeSave&) = delete;

    [[nodiscard]] static bool IsActive() noexcept;
};
}
