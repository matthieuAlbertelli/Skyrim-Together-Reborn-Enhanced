#pragma once
#include <cstdint>

struct TESForm;
struct TESObjectREFR;
struct World;

bool IsNativeLifetimeProbeEnabled() noexcept;
void SetNativeLifetimeProbeEnabled(bool aEnabled) noexcept;
#if (!IS_MASTER)
// One-shot request; selection/validation occurs on the next game service update.
void RequestObserveCurrentPrivateRemote() noexcept;
#endif

// Arms one naturally occurring private-player creation. Never owns a native ref.
class NativeLifetimeCreationScope
{
public:
    NativeLifetimeCreationScope(uint32_t aEntity, uint32_t aServerId) noexcept;
    ~NativeLifetimeCreationScope() noexcept;
    NativeLifetimeCreationScope(const NativeLifetimeCreationScope&) = delete;
    NativeLifetimeCreationScope& operator=(const NativeLifetimeCreationScope&) = delete;

private:
    uint64_t m_previous{};
};

void NativeLifetimeStage(const char* apPhase, const TESForm* apLiveForm = nullptr, bool aBase = false, uint32_t aSpawnHandle = 0) noexcept;
uint64_t NativeLifetimeDeleteEnter(const TESObjectREFR* apLiveReference) noexcept;
void NativeLifetimeDeleteReturn(uint64_t aSession) noexcept;
void NativeLifetimeDiscovery(uint32_t aFormId, bool aAdded) noexcept;
void NativeLifetimeServerRemoval(uint32_t aServerId) noexcept;
void NativeLifetimeDisconnect() noexcept;
void TickNativeLifetimeProbe(World& aWorld, uint64_t aServiceTick) noexcept;
