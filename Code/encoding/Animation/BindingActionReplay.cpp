#include <Animation/BindingActionReplay.h>
#include <algorithm>

void BindingActionReplay::RemovePrefix(TiltedPhoques::List<ActionEvent>& aPending, uint32_t& aReplayCount, bool& aResetGraph) noexcept
{
    const auto count = m_injected;
    while (m_injected && !aPending.empty())
    {
        aPending.pop_front();
        --m_injected;
    }
    m_injected = 0;
    aReplayCount -= std::min(aReplayCount, count);
    if (!aReplayCount)
        aResetGraph = false;
}

void BindingActionReplay::ObserveBinding(AnimationBinding aBinding, TiltedPhoques::List<ActionEvent>& aPending,
                                        uint32_t& aReplayCount, bool& aResetGraph) noexcept
{
    if (m_binding.FormId && m_binding != aBinding)
        Invalidate(aPending, aReplayCount, aResetGraph); // Unannounced binding loss, never replay across it.
    m_binding = aBinding;
}

void BindingActionReplay::Consumed(const ActionEvent& aAction) noexcept
{
    if (m_injected)
        --m_injected;
    else if (m_enabled)
        m_cache.Append(aAction);
}

BindingActionReplay::Reconstruction BindingActionReplay::Rebind(const AnimationBindingChange& aChange,
    TiltedPhoques::List<ActionEvent>& aPending, uint32_t& aReplayCount, bool& aResetGraph) noexcept
{
    Reconstruction result;
    if (!m_enabled)
    {
        result.Reason = "context-invalidated";
        return result;
    }
    if (aChange == m_change || (m_change.Session == aChange.Session && aChange.Generation <= m_change.Generation))
    {
        result.Reason = "duplicate-or-stale-generation";
        return result;
    }
    if (!aChange.Session || !aChange.Generation || !aChange.Old.FormId || !aChange.Old.Token ||
        !aChange.New.FormId || !aChange.New.Token || aChange.Old == aChange.New ||
        (m_binding.FormId && m_binding != aChange.Old) ||
        (m_change.Session && m_change.Session != aChange.Session))
        return result;

    RemovePrefix(aPending, aReplayCount, aResetGraph);
    result.SourceCount = m_cache.GetActions().size();
    // A newer exit received but not yet consumed already invalidates all old
    // persistent state. Leave that exit and every other live pending action in
    // their original order; no transient resurrection before processing it.
    result.PendingExit = std::any_of(aPending.begin(), aPending.end(), ActionReplayCache::IsExitAction);
    if (result.PendingExit)
        m_cache.Clear();
    result.Chain = m_cache.FormRefinedReplayChain();
    aPending.insert(aPending.begin(), result.Chain.Actions.begin(), result.Chain.Actions.end());
    m_injected = static_cast<uint32_t>(result.Chain.Actions.size());
    aReplayCount += m_injected;
    aResetGraph = aResetGraph || result.Chain.ResetAnimationGraph;
    m_change = aChange;
    m_binding = aChange.New;
    m_waitLogged = false;
    result.Accepted = true;
    result.Reason = result.PendingExit ? "pending-exit-invalidated-history" : "consumed-history-before-pending";
    return result;
}

void BindingActionReplay::Invalidate(TiltedPhoques::List<ActionEvent>& aPending, uint32_t& aReplayCount, bool& aResetGraph) noexcept
{
    RemovePrefix(aPending, aReplayCount, aResetGraph);
    m_cache.Clear();
    m_binding = {};
    m_change = {};
    m_enabled = false;
}

bool BindingActionReplay::CancelForPendingExit(TiltedPhoques::List<ActionEvent>& aPending, uint32_t& aReplayCount, bool& aResetGraph) noexcept
{
    if (!m_injected)
        return false;
    auto live = aPending.begin();
    for (uint32_t i = 0; i < m_injected && live != aPending.end(); ++i)
        ++live;
    if (!std::any_of(live, aPending.end(), ActionReplayCache::IsExitAction))
        return false;
    RemovePrefix(aPending, aReplayCount, aResetGraph);
    m_cache.Clear();
    return true;
}
