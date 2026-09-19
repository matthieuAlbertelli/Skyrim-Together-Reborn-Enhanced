#pragma once

#include <Animation/ActionReplayCache.h>
#include <utility>

// Value identities only: a token is compared, never dereferenced or retained as
// an engine pointer. The owning ECS component supplies entity/version lifetime.
struct AnimationBinding
{
    uint32_t FormId{};
    uintptr_t Token{};
    bool operator==(const AnimationBinding&) const = default;
};

struct AnimationBindingChange
{
    uint64_t Session{}, Generation{};
    uint32_t ServerId{}, EntityVersioned{};
    AnimationBinding Old, New;
    bool operator==(const AnimationBindingChange&) const = default;
};

// Observer history stops at the consumed frontier. Pending actions are never
// cached a second time, and injected history is not recorded again on replay.
// Uses exactly the server's bounded cache/refinement rules.
class BindingActionReplay
{
public:
    struct Reconstruction
    {
        bool Accepted{};
        const char* Reason{"invalid-binding-change"};
        bool PendingExit{};
        size_t SourceCount{};
        ActionReplayChain Chain;
    };

    void ObserveBinding(AnimationBinding aBinding, TiltedPhoques::List<ActionEvent>& aPending,
                        uint32_t& aReplayCount, bool& aResetGraph) noexcept;
    void Consumed(const ActionEvent& aAction) noexcept;
    Reconstruction Rebind(const AnimationBindingChange& aChange, TiltedPhoques::List<ActionEvent>& aPending,
                          uint32_t& aReplayCount, bool& aResetGraph) noexcept;
    bool CancelForPendingExit(TiltedPhoques::List<ActionEvent>& aPending, uint32_t& aReplayCount, bool& aResetGraph) noexcept;
    // Recovery/disconnect invalidate this component until Setup creates a fresh
    // one. Remove only our synthetic prefix, preserving the normal STR queue.
    void Invalidate(TiltedPhoques::List<ActionEvent>& aPending, uint32_t& aReplayCount, bool& aResetGraph) noexcept;

    const ActionReplayCache& Cache() const noexcept { return m_cache; }
    const AnimationBindingChange& Change() const noexcept { return m_change; }
    uint32_t Remaining() const noexcept { return m_injected; }
    bool Enabled() const noexcept { return m_enabled; }
    bool LogWaitOnce() noexcept { return !std::exchange(m_waitLogged, true); }

private:
    void RemovePrefix(TiltedPhoques::List<ActionEvent>& aPending, uint32_t& aReplayCount, bool& aResetGraph) noexcept;
    ActionReplayCache m_cache;
    AnimationBinding m_binding;
    AnimationBindingChange m_change;
    uint32_t m_injected{};
    bool m_enabled{true}, m_waitLogged{};
};
