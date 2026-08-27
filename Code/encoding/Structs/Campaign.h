#pragma once

#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Stl.hpp>

#include <cstddef>
#include <cstdint>
#include <string_view>

inline constexpr std::size_t kCampaignWireMaximumIdLength = 128;
inline constexpr std::size_t kCampaignWireMaximumRosterSize = 10;
inline constexpr std::size_t kCampaignJoinCodeLength = 4;
inline constexpr std::size_t kCampaignLobbyMaximumDisplayNameLength = 24;
inline constexpr std::size_t kCampaignLobbyMaximumDisplayNameBytes = 96;
inline constexpr std::uint8_t kCampaignWirePhaseCharacterCreation = 1;
inline constexpr std::uint8_t kCampaignWireRuntimeActive = 1;
inline constexpr std::uint8_t kCampaignWireRuntimeCheckpointing = 2;
inline constexpr std::uint8_t kCampaignWireRuntimeRecoveryLock = 3;
inline constexpr std::uint8_t kCampaignWireRuntimeRestoringCheckpoint = 4;
inline constexpr std::string_view kCampaignJoinCodeAlphabet =
    "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
inline constexpr std::string_view kCampaignNativeSaveIdentityPrefix = "stre-";

enum class CampaignProtocolOperation : std::uint8_t
{
    Create = 0,
    Join,
    Resume,
    Start,
    SetReady,
    Leave,
    JoinByCode
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
    ExistingMembershipRequiresResume,
    InvalidJoinCode,
    JoinCodeUnavailable,
    PartyAlignmentFailed
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
[[nodiscard]] bool BuildCampaignNativeSaveIdentity(
    const TiltedPhoques::String& acCheckpointId,
    TiltedPhoques::String& aNativeSaveIdentity) noexcept;
[[nodiscard]] bool IsValidCampaignNativeSaveIdentity(
    const TiltedPhoques::String& acValue) noexcept;
[[nodiscard]] bool NormalizeCampaignJoinCode(
    std::string_view acValue,
    TiltedPhoques::String& aNormalized) noexcept;
[[nodiscard]] bool IsValidCampaignJoinCode(
    const TiltedPhoques::String& acValue) noexcept;
[[nodiscard]] bool NormalizeCampaignLobbyDisplayName(
    std::string_view acValue,
    TiltedPhoques::String& aNormalized) noexcept;
[[nodiscard]] bool IsValidCampaignLobbyDisplayName(
    const TiltedPhoques::String& acValue) noexcept;
