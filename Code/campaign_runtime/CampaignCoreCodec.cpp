#include <CampaignCoreCodec.h>

#include <algorithm>
#include <cstring>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace STRE::Campaign::RuntimeCodec
{
namespace
{
constexpr std::size_t kMaximumPayloadSize = 64 * 1024;
constexpr std::size_t kMaximumIdentitySize = 128;
constexpr char kCoreMagic[] = "STRECR01";
constexpr char kSnapshotIntentMagic[] = "STRECI01";

template <class T, bool = std::is_enum_v<T>> struct ScalarValue
{
    using Type = T;
};

template <class T> struct ScalarValue<T, true>
{
    using Type = std::underlying_type_t<T>;
};

class Writer
{
public:
    explicit Writer(Bytes& aOutput)
        : m_output(aOutput)
    {
        m_output.clear();
    }

    template <class T> bool Scalar(T aValue)
    {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        using Value = typename ScalarValue<T>::Type;
        using Unsigned = std::make_unsigned_t<Value>;
        if (m_output.size() + sizeof(Unsigned) > kMaximumPayloadSize)
            return false;
        const Unsigned value = static_cast<Unsigned>(aValue);
        for (std::size_t index = 0; index < sizeof(Unsigned); ++index)
        {
            m_output.push_back(static_cast<std::uint8_t>(
                (value >> (index * 8)) & 0xFF));
        }
        return true;
    }

    bool Bool(bool aValue)
    {
        return Scalar<std::uint8_t>(aValue ? 1 : 0);
    }

    bool Raw(const void* apData, std::size_t aSize)
    {
        if (aSize > kMaximumPayloadSize ||
            m_output.size() > kMaximumPayloadSize - aSize)
        {
            return false;
        }
        const auto* pBytes = static_cast<const std::uint8_t*>(apData);
        m_output.insert(m_output.end(), pBytes, pBytes + aSize);
        return true;
    }

    bool String(const std::string& acValue)
    {
        return acValue.size() <= kMaximumIdentitySize &&
            Scalar<std::uint32_t>(
                static_cast<std::uint32_t>(acValue.size())) &&
            Raw(acValue.data(), acValue.size());
    }

    bool Blob(const Bytes& acValue)
    {
        return acValue.size() <= kMaximumPayloadSize &&
            Scalar<std::uint32_t>(
                static_cast<std::uint32_t>(acValue.size())) &&
            Raw(acValue.data(), acValue.size());
    }

private:
    Bytes& m_output;
};

class Reader
{
public:
    explicit Reader(const Bytes& acInput)
        : m_input(acInput)
    {
    }

    template <class T> bool Scalar(T& aValue)
    {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        using Value = typename ScalarValue<T>::Type;
        using Unsigned = std::make_unsigned_t<Value>;
        if (Remaining() < sizeof(Unsigned))
            return false;
        Unsigned value{};
        for (std::size_t index = 0; index < sizeof(Unsigned); ++index)
        {
            value |= static_cast<Unsigned>(m_input[m_offset++]) <<
                (index * 8);
        }
        aValue = static_cast<T>(value);
        return true;
    }

    bool Bool(bool& aValue)
    {
        std::uint8_t value{};
        if (!Scalar(value) || value > 1)
            return false;
        aValue = value != 0;
        return true;
    }

    bool Magic(const char* apMagic, std::size_t aSize)
    {
        if (Remaining() < aSize ||
            std::memcmp(m_input.data() + m_offset, apMagic, aSize) != 0)
        {
            return false;
        }
        m_offset += aSize;
        return true;
    }

    bool String(std::string& aValue)
    {
        std::uint32_t size{};
        if (!Scalar(size) || size > kMaximumIdentitySize || Remaining() < size)
            return false;
        aValue.assign(
            reinterpret_cast<const char*>(m_input.data() + m_offset), size);
        m_offset += size;
        return true;
    }

    bool Blob(Bytes& aValue)
    {
        std::uint32_t size{};
        if (!Scalar(size) || size > kMaximumPayloadSize || Remaining() < size)
            return false;
        aValue.assign(
            m_input.begin() + static_cast<std::ptrdiff_t>(m_offset),
            m_input.begin() + static_cast<std::ptrdiff_t>(m_offset + size));
        m_offset += size;
        return true;
    }

    [[nodiscard]] bool Done() const noexcept
    {
        return m_offset == m_input.size();
    }

private:
    [[nodiscard]] std::size_t Remaining() const noexcept
    {
        return m_input.size() - m_offset;
    }

    const Bytes& m_input;
    std::size_t m_offset{};
};

StoreResult Failure(StoreError aError, std::string aMessage)
{
    return {aError, std::move(aMessage)};
}

bool IsValidPhase(CampaignPhase aPhase)
{
    return aPhase >= CampaignPhase::Lobby &&
        aPhase <= CampaignPhase::OpenWorld;
}

StoreResult ValidateAggregate(const CampaignAggregate& acCampaign)
{
    if (acCampaign.Id.Value.empty() || acCampaign.Version == 0 ||
        !IsValidPhase(acCampaign.Phase))
    {
        return Failure(
            StoreError::InvalidArgument,
            "campaign core state contains invalid identity, version, or phase");
    }
    CampaignDomainResult roster = CampaignStateMachine::ValidateRoster(
        acCampaign.Roster, !acCampaign.RosterSealed);
    if (!roster)
        return Failure(StoreError::InvalidArgument, roster.Message);
    if (!acCampaign.RosterSealed &&
        (acCampaign.Phase != CampaignPhase::Lobby || acCampaign.SessionManager))
    {
        return Failure(
            StoreError::InvalidArgument,
            "unsealed campaign core state must remain an unmanaged Lobby");
    }
    if (acCampaign.RosterSealed &&
        (acCampaign.Phase == CampaignPhase::Lobby ||
         !acCampaign.SessionManager))
    {
        return Failure(
            StoreError::InvalidArgument,
            "sealed campaign core state requires a manager and post-Lobby phase");
    }
    if (acCampaign.SessionManager &&
        std::none_of(
            acCampaign.Roster.begin(),
            acCampaign.Roster.end(),
            [&acCampaign](const CampaignSlotState& acSlot)
            {
                return acSlot.Player == *acCampaign.SessionManager;
            }))
    {
        return Failure(
            StoreError::InvalidArgument,
            "Session Manager is not a campaign roster member");
    }
    return {};
}

bool WriteCoreBody(Writer& aWriter, const CampaignAggregate& acCampaign)
{
    if (!aWriter.Scalar(acCampaign.Version) ||
        !aWriter.Scalar(acCampaign.Phase) ||
        !aWriter.Bool(acCampaign.SessionManager.has_value()) ||
        (acCampaign.SessionManager &&
         !aWriter.String(acCampaign.SessionManager->Value)) ||
        !aWriter.Scalar<std::uint32_t>(
            static_cast<std::uint32_t>(acCampaign.Roster.size())))
    {
        return false;
    }
    for (const CampaignSlotState& slot : acCampaign.Roster)
    {
        if (!aWriter.String(slot.Slot.Value) || !aWriter.Bool(slot.Ready))
            return false;
    }
    return true;
}
}

StoreResult EncodeCoreState(
    const CampaignAggregate& acCampaign,
    Bytes& aPayload) noexcept
{
    try
    {
        StoreResult validation = ValidateAggregate(acCampaign);
        if (!validation)
            return validation;
        Writer writer(aPayload);
        if (!writer.Raw(kCoreMagic, sizeof(kCoreMagic) - 1) ||
            !writer.Scalar(kCampaignCoreCodecVersion) ||
            !WriteCoreBody(writer, acCampaign))
        {
            return Failure(
                StoreError::InvalidArgument,
                "campaign core state exceeds codec bounds");
        }
        return {};
    }
    catch (...)
    {
        return Failure(
            StoreError::IntegrityFailure,
            "campaign core-state encoding failed safely");
    }
}

StoreValueResult<CampaignAggregate> DecodeCoreState(
    const CampaignId& acCampaign,
    bool aRosterSealed,
    const std::vector<CampaignSlotRecord>& acRoster,
    StateVersion aPersistedVersion,
    const Bytes& acPayload) noexcept
{
    StoreValueResult<CampaignAggregate> result;
    try
    {
        Reader reader(acPayload);
        std::uint32_t codecVersion{};
        bool hasManager{};
        std::uint32_t readinessCount{};
        CampaignAggregate& campaign = result.Value;
        campaign.Id = acCampaign;
        campaign.RosterSealed = aRosterSealed;
        if (!reader.Magic(kCoreMagic, sizeof(kCoreMagic) - 1) ||
            !reader.Scalar(codecVersion) ||
            codecVersion != kCampaignCoreCodecVersion ||
            !reader.Scalar(campaign.Version) ||
            campaign.Version != aPersistedVersion ||
            !reader.Scalar(campaign.Phase) ||
            !IsValidPhase(campaign.Phase) ||
            !reader.Bool(hasManager))
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "malformed or unsupported campaign core-state header";
            return result;
        }
        if (hasManager)
        {
            PlayerId manager;
            if (!reader.String(manager.Value))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "malformed campaign Session Manager identity";
                return result;
            }
            campaign.SessionManager = std::move(manager);
        }
        if (!reader.Scalar(readinessCount) ||
            readinessCount > kMaximumCampaignRosterSize ||
            readinessCount != acRoster.size())
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign readiness does not match the durable roster";
            return result;
        }

        std::unordered_map<std::string, bool> readiness;
        for (std::uint32_t index = 0; index < readinessCount; ++index)
        {
            std::string slot;
            bool ready{};
            if (!reader.String(slot) || !reader.Bool(ready) ||
                !readiness.emplace(std::move(slot), ready).second)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign readiness contains malformed or duplicate slots";
                return result;
            }
        }
        if (!reader.Done())
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign core state contains trailing data";
            return result;
        }

        campaign.Roster.reserve(acRoster.size());
        for (const CampaignSlotRecord& slot : acRoster)
        {
            const auto ready = readiness.find(slot.Slot.Value);
            if (ready == readiness.end())
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign readiness is missing an expected slot";
                return result;
            }
            campaign.Roster.push_back(
                {slot.Slot, slot.Player, slot.CharacterBinding, ready->second});
        }
        CampaignStateMachine::SortRoster(campaign.Roster);
        StoreResult validation = ValidateAggregate(campaign);
        if (!validation)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = validation.Message;
        }
    }
    catch (...)
    {
        result.Error = StoreError::IntegrityFailure;
        result.Message = "campaign core-state decoding failed safely";
    }
    return result;
}

StoreResult EncodeSnapshotIntent(
    const CampaignAggregate& acCampaign,
    Bytes& aPayload) noexcept
{
    try
    {
        StoreResult validation = ValidateAggregate(acCampaign);
        if (!validation)
            return validation;
        Writer writer(aPayload);
        if (!writer.Raw(
                kSnapshotIntentMagic,
                sizeof(kSnapshotIntentMagic) - 1) ||
            !writer.Scalar(kCampaignOutboxCodecVersion) ||
            !writer.String(acCampaign.Id.Value) ||
            !writer.Scalar(acCampaign.Version) ||
            !writer.Scalar(acCampaign.Phase) ||
            !writer.Bool(acCampaign.RosterSealed) ||
            !writer.Bool(acCampaign.SessionManager.has_value()) ||
            (acCampaign.SessionManager &&
             !writer.String(acCampaign.SessionManager->Value)) ||
            !writer.Scalar<std::uint32_t>(
                static_cast<std::uint32_t>(acCampaign.Roster.size())))
        {
            return Failure(
                StoreError::InvalidArgument,
                "campaign snapshot intent exceeds codec bounds");
        }
        for (const CampaignSlotState& slot : acCampaign.Roster)
        {
            if (!writer.String(slot.Slot.Value) ||
                !writer.String(slot.Player.Value) ||
                !writer.String(slot.CharacterBinding.Value) ||
                !writer.Bool(slot.Ready))
            {
                return Failure(
                    StoreError::InvalidArgument,
                    "campaign snapshot roster exceeds codec bounds");
            }
        }
        return {};
    }
    catch (...)
    {
        return Failure(
            StoreError::IntegrityFailure,
            "campaign snapshot-intent encoding failed safely");
    }
}

StoreValueResult<CampaignAggregate> DecodeSnapshotIntent(
    const Bytes& acPayload) noexcept
{
    StoreValueResult<CampaignAggregate> result;
    try
    {
        Reader reader(acPayload);
        std::uint32_t codecVersion{};
        bool hasManager{};
        std::uint32_t rosterCount{};
        CampaignAggregate& campaign = result.Value;
        if (!reader.Magic(
                kSnapshotIntentMagic,
                sizeof(kSnapshotIntentMagic) - 1) ||
            !reader.Scalar(codecVersion) ||
            codecVersion != kCampaignOutboxCodecVersion ||
            !reader.String(campaign.Id.Value) ||
            !reader.Scalar(campaign.Version) ||
            !reader.Scalar(campaign.Phase) ||
            !IsValidPhase(campaign.Phase) ||
            !reader.Bool(campaign.RosterSealed) ||
            !reader.Bool(hasManager))
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "malformed campaign snapshot-intent header";
            return result;
        }
        if (hasManager)
        {
            PlayerId manager;
            if (!reader.String(manager.Value))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "malformed snapshot Session Manager identity";
                return result;
            }
            campaign.SessionManager = std::move(manager);
        }
        if (!reader.Scalar(rosterCount) ||
            rosterCount > kMaximumCampaignRosterSize)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "malformed campaign snapshot roster count";
            return result;
        }
        campaign.Roster.resize(rosterCount);
        for (CampaignSlotState& slot : campaign.Roster)
        {
            if (!reader.String(slot.Slot.Value) ||
                !reader.String(slot.Player.Value) ||
                !reader.String(slot.CharacterBinding.Value) ||
                !reader.Bool(slot.Ready))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "malformed campaign snapshot roster";
                return result;
            }
        }
        if (!reader.Done())
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign snapshot intent contains trailing data";
            return result;
        }
        CampaignStateMachine::SortRoster(campaign.Roster);
        StoreResult validation = ValidateAggregate(campaign);
        if (!validation)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = validation.Message;
        }
    }
    catch (...)
    {
        result.Error = StoreError::IntegrityFailure;
        result.Message = "campaign snapshot-intent decoding failed safely";
    }
    return result;
}
}
