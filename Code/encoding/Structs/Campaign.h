#pragma once

#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Stl.hpp>

#include <cstddef>
#include <cstdint>

inline constexpr std::size_t kCampaignWireMaximumIdLength = 128;
inline constexpr std::size_t kCampaignWireMaximumRosterSize = 10;

enum class CampaignProtocolOperation : std::uint8_t
{
    Create = 0,
    Join,
    Resume,
    Start,
    SetReady,
    Leave
};

enum class CampaignProtocolResult : std::uint8_t
{
    Applied = 0,
    AcceptedNoOp,
    IdempotentReplay,
    InvalidRequest,
    Unauthorized,
    SessionMismatch,
    CampaignNotFound,
    NotAdmitted,
    RosterSealed,
    RosterNotSealed,
    IdentityMismatch,
    BindingMismatch,
    DuplicateIdentity,
    StaleRevision,
    IdempotencyConflict,
    RosterIncomplete,
    PersistenceFailure,
    ExistingMembershipRequiresResume
};

struct CampaignPublicSlotData
{
    TiltedPhoques::String SlotId;
    TiltedPhoques::String PlayerId;
    bool Ready{};
    bool Present{};

    void Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept;
    [[nodiscard]] bool IsValid() const noexcept;

    bool operator==(const CampaignPublicSlotData&) const noexcept = default;
};

struct CampaignSnapshotData
{
    TiltedPhoques::String CampaignId;
    std::uint64_t StateVersion{};
    std::uint8_t Phase{};
    std::uint8_t RuntimeState{};
    bool RosterSealed{};
    TiltedPhoques::String SessionManagerPlayerId;
    TiltedPhoques::Vector<CampaignPublicSlotData> Roster;
    bool WireValid{true};

    void Serialize(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void Deserialize(TiltedPhoques::Buffer::Reader& aReader) noexcept;
    [[nodiscard]] bool IsValid() const noexcept;

    bool operator==(const CampaignSnapshotData& acRhs) const noexcept
    {
        return CampaignId == acRhs.CampaignId &&
            StateVersion == acRhs.StateVersion && Phase == acRhs.Phase &&
            RuntimeState == acRhs.RuntimeState &&
            RosterSealed == acRhs.RosterSealed &&
            SessionManagerPlayerId == acRhs.SessionManagerPlayerId &&
            Roster == acRhs.Roster;
    }
};

[[nodiscard]] bool IsValidCampaignWireId(
    const TiltedPhoques::String& acValue,
    bool aAllowEmpty = false) noexcept;
[[nodiscard]] bool WriteCampaignWireId(
    TiltedPhoques::Buffer::Writer& aWriter,
    const TiltedPhoques::String& acValue) noexcept;
[[nodiscard]] bool ReadCampaignWireId(
    TiltedPhoques::Buffer::Reader& aReader,
    TiltedPhoques::String& aValue) noexcept;
