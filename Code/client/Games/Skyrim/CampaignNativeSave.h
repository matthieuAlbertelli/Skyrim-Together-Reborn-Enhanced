#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace CampaignNativeSaveDetail
{
enum class RequestSlotState
{
    Idle,
    Requested,
    Processing
};

class RequestSlot final
{
public:
    [[nodiscard]] bool TryRequest(std::string aIdentity) noexcept
    {
        if (m_state != RequestSlotState::Idle || aIdentity.empty())
            return false;

        m_identity = std::move(aIdentity);
        m_state = RequestSlotState::Requested;
        return true;
    }

    [[nodiscard]] const std::string* RequestedIdentity() const noexcept
    {
        return m_state == RequestSlotState::Requested ? &m_identity : nullptr;
    }

    [[nodiscard]] const std::string* BeginProcessing() noexcept
    {
        if (m_state != RequestSlotState::Requested)
            return nullptr;

        m_state = RequestSlotState::Processing;
        return &m_identity;
    }

    void FinishProcessing() noexcept
    {
        if (m_state != RequestSlotState::Processing)
            return;

        m_identity.clear();
        m_state = RequestSlotState::Idle;
    }

    [[nodiscard]] RequestSlotState GetState() const noexcept
    {
        return m_state;
    }

private:
    RequestSlotState m_state{RequestSlotState::Idle};
    std::string m_identity;
};
}

enum class CampaignNativeSaveRequestState
{
    InvalidIdentity,
    BoundaryUnavailable,
    RequestAlreadyActive,
    Accepted
};

struct CampaignNativeSaveRequestResult
{
    CampaignNativeSaveRequestState State{
        CampaignNativeSaveRequestState::InvalidIdentity};

    [[nodiscard]] bool WasAccepted() const noexcept
    {
        return State == CampaignNativeSaveRequestState::Accepted;
    }
};

class CampaignNativeSave final
{
public:
    // Called from STRE's game-update path. This only stores one request for the
    // engine save/load processing boundary and returns without calling Save.
    [[nodiscard]] static CampaignNativeSaveRequestResult RequestOnGameThread(
        std::string_view acNativeSaveIdentity) noexcept;
};
