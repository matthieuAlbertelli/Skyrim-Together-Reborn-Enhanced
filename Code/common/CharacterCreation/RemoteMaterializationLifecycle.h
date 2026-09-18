#pragma once

#include <cstdint>

namespace STRE::CharacterCreation
{
// Pure contract model, shared by TPTests and the non-MASTER Slice 3 adapter.
// No ECS, engine or transport effects in this model.
// Events must carry a key captured when observed, not reconstructed from the
// current FormID binding at delivery. The LAB observes synchronously at source
// against immutable transaction records; it does not enqueue LAB callbacks.
struct MaterializationKey
{
    uint64_t Session{};
    uint64_t Entity{}; // Full entity identity, including its version.
    uint32_t ServerId{};
    uint64_t Generation{};
    bool operator==(const MaterializationKey&) const = default;
};

struct MaterializationForms
{
    uint32_t Actor{};
    uint32_t Base{};
    bool operator==(const MaterializationForms&) const = default;
    bool Valid() const noexcept { return Actor && Base && Actor != Base; }
};

enum class MaterializationState
{
    Idle, CandidateReserved, CandidateCreated, CandidateDiscovered,
    CandidateReady, Committed, Aborted, Invalidated
};

enum class DiscoveryRoute
{
    Unrelated, Stale, LiveBinding, CandidateStaged, Quarantined, Duplicate,
    OldLostBeforeCommit, OldRetirementObserved, CandidateLostBeforeCommit,
    CandidateRetirementObserved, LiveBindingLost
};

struct MaterializationRetirement
{
    bool Requested{};
    bool Dispatched{}; // Pure intent consumed, not an actual native Delete.
    bool Observed{}; // Discovery absence only; native/base completion is UNKNOWN.
};

class RemoteMaterializationLifecycle
{
public:
    bool Reserve(MaterializationKey aKey, MaterializationForms aOld) noexcept
    {
        if (m_state != MaterializationState::Idle || !aKey.Session || !aKey.Generation || !aOld.Valid())
            return false;
        m_key = aKey;
        m_old = aOld;
        m_boundActor = aOld.Actor;
        m_state = MaterializationState::CandidateReserved;
        return true;
    }

    bool RecordCandidate(MaterializationKey aKey, MaterializationForms aCandidate) noexcept
    {
        if (!Matches(aKey) || !aCandidate.Valid() || aCandidate.Actor == m_old.Actor || aCandidate.Base == m_old.Base ||
            aCandidate.Actor == m_old.Base || aCandidate.Base == m_old.Actor)
            return false;
        if (m_candidate.Valid())
            return m_candidate == aCandidate; // Duplicate record never creates another candidate.
        if (m_state != MaterializationState::CandidateReserved && m_state != MaterializationState::Aborted &&
            m_state != MaterializationState::Invalidated)
            return false;
        m_candidate = aCandidate;
        if (m_state == MaterializationState::CandidateReserved)
            m_state = MaterializationState::CandidateCreated;
        else
            m_candidateRetirement.Requested = true; // Creation returned after cancellation: cleanup only.
        return true; // Tracked, not necessarily eligible for publication.
    }

    bool MarkReady(MaterializationKey aKey) noexcept
    {
        if (!Matches(aKey) || m_state != MaterializationState::CandidateDiscovered)
            return false;
        // External evidence obligation, not a native readiness check.
        m_state = MaterializationState::CandidateReady;
        return true;
    }

    bool Commit(MaterializationKey aKey) noexcept
    {
        if (!Matches(aKey) || m_state != MaterializationState::CandidateReady)
            return false;
        m_boundActor = m_candidate.Actor;
        m_committed = true;
        m_state = MaterializationState::Committed;
        m_oldRetirement.Requested = true;
        return true;
    }

    bool Abort(MaterializationKey aKey) noexcept
    {
        if (!Matches(aKey) || m_committed || m_state == MaterializationState::Invalidated)
            return false;
        m_state = MaterializationState::Aborted;
        m_candidateRetirement.Requested = m_candidate.Valid();
        return true;
    }

    bool Invalidate(MaterializationKey aKey) noexcept
    {
        if (!Matches(aKey))
            return false;
        m_state = MaterializationState::Invalidated; // Authoritative removal or session end wins.
        m_boundActor = 0;
        m_oldRetirement.Requested = true;
        m_candidateRetirement.Requested = m_candidate.Valid();
        return true;
    }

    DiscoveryRoute Observe(MaterializationKey aKey, uint32_t aFormId, bool aAdded) noexcept
    {
        if (!Matches(aKey))
            return DiscoveryRoute::Stale;
        if (aFormId != m_old.Actor && (!m_candidate.Valid() || aFormId != m_candidate.Actor))
            return DiscoveryRoute::Unrelated;
        if (aFormId == m_old.Actor)
        {
            if (aAdded)
                return m_committed || m_state == MaterializationState::Invalidated || m_oldRetirement.Observed
                           ? DiscoveryRoute::Quarantined : DiscoveryRoute::LiveBinding;
            if (m_oldRetirement.Observed)
                return DiscoveryRoute::Duplicate;
            m_oldRetirement.Observed = true;
            if (!m_committed && m_state != MaterializationState::Invalidated)
            {
                m_boundActor = 0;
                Abort(aKey);
                return DiscoveryRoute::OldLostBeforeCommit;
            }
            return DiscoveryRoute::OldRetirementObserved;
        }
        if (aAdded)
        {
            if (m_state == MaterializationState::Aborted || m_state == MaterializationState::Invalidated)
                return DiscoveryRoute::Quarantined;
            if (m_committed)
                return DiscoveryRoute::LiveBinding;
            if (m_state == MaterializationState::CandidateCreated)
                m_state = MaterializationState::CandidateDiscovered;
            return DiscoveryRoute::CandidateStaged;
        }
        if (m_candidateRetirement.Observed)
            return DiscoveryRoute::Duplicate;
        m_candidateRetirement.Observed = true;
        if (m_state == MaterializationState::Aborted || m_state == MaterializationState::Invalidated)
            return DiscoveryRoute::CandidateRetirementObserved;
        if (m_committed)
        {
            m_boundActor = 0;
            m_state = MaterializationState::Aborted;
            return DiscoveryRoute::LiveBindingLost; // Genuine current-actor unload is not hidden.
        }
        Abort(aKey);
        return DiscoveryRoute::CandidateLostBeforeCommit;
    }

    // Bit 1 = old, bit 2 = candidate; once per materialization, even after abort
    // followed by server removal. No deletion is performed or completion claimed.
    uint8_t TakeRetirementIntents(MaterializationKey aKey) noexcept
    {
        if (!Matches(aKey))
            return 0;
        uint8_t result = 0;
        if (m_oldRetirement.Requested && !m_oldRetirement.Dispatched)
        {
            m_oldRetirement.Dispatched = true;
            result |= 1;
        }
        if (m_candidateRetirement.Requested && !m_candidateRetirement.Dispatched)
        {
            m_candidateRetirement.Dispatched = true;
            result |= 2;
        }
        return result;
    }

    MaterializationKey Key() const noexcept { return m_key; }
    MaterializationState State() const noexcept { return m_state; }
    uint32_t BoundActor() const noexcept { return m_boundActor; }
    bool HasIdentity() const noexcept { return m_state != MaterializationState::Idle && m_state != MaterializationState::Invalidated; }
    MaterializationRetirement OldRetirement() const noexcept { return m_oldRetirement; }
    MaterializationRetirement CandidateRetirement() const noexcept { return m_candidateRetirement; }

private:
    bool Matches(MaterializationKey aKey) const noexcept { return m_state != MaterializationState::Idle && aKey == m_key; }
    MaterializationKey m_key{};
    MaterializationForms m_old{}, m_candidate{};
    MaterializationState m_state{MaterializationState::Idle};
    MaterializationRetirement m_oldRetirement{}, m_candidateRetirement{};
    uint32_t m_boundActor{};
    bool m_committed{};
};
} // namespace STRE::CharacterCreation
