#include <CampaignCodec.h>

#include <bit>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <type_traits>

namespace STRE::Campaign::Codec
{
namespace
{
constexpr std::size_t kMaximumPayloadSize = 4 * 1024 * 1024;
constexpr std::size_t kMaximumStringSize = 4096;
constexpr std::uint32_t kMaximumSelections = 256;
constexpr std::uint32_t kMaximumInventoryEntries = 4096;
constexpr std::uint32_t kMaximumEffectsPerEntry = 128;
constexpr std::uint32_t kMaximumSpells = 1024;
constexpr std::uint32_t kMaximumSlots = 128;
constexpr std::uint32_t kMaximumAdapters = 256;
constexpr std::uint32_t kMaximumCharacterBuilds = 128;

constexpr char kCharacterBuildMagic[] = "STRECB01";
constexpr char kSnapshotMagic[] = "STRECS01";

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
        Unsigned value = static_cast<Unsigned>(aValue);
        if (m_output.size() + sizeof(Unsigned) > kMaximumPayloadSize)
            return false;
        for (std::size_t i = 0; i < sizeof(Unsigned); ++i)
        {
            m_output.push_back(
                static_cast<std::uint8_t>((value >> (i * 8)) & 0xFF));
        }
        return true;
    }

    bool Float(float aValue)
    {
        return Scalar(std::bit_cast<std::uint32_t>(aValue));
    }

    bool Bool(bool aValue) { return Scalar<std::uint8_t>(aValue ? 1 : 0); }

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

    bool String(std::string_view aValue)
    {
        return aValue.size() <= kMaximumStringSize &&
            Scalar<std::uint32_t>(static_cast<std::uint32_t>(aValue.size())) &&
            Raw(aValue.data(), aValue.size());
    }

    bool Blob(const Bytes& acValue)
    {
        return acValue.size() <= kMaximumPayloadSize &&
            Scalar<std::uint32_t>(
                static_cast<std::uint32_t>(acValue.size())) &&
            Raw(acValue.data(), acValue.size());
    }

    template <class Tag> bool Id(const DurableId<Tag>& acId)
    {
        return String(acId.Value);
    }

    bool Form(const FormId& acForm)
    {
        return Scalar(acForm.ModId) && Scalar(acForm.BaseId);
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
        for (std::size_t i = 0; i < sizeof(Unsigned); ++i)
        {
            value |= static_cast<Unsigned>(m_input[m_offset++]) << (i * 8);
        }
        aValue = static_cast<T>(value);
        return true;
    }

    bool Float(float& aValue)
    {
        std::uint32_t bits{};
        if (!Scalar(bits))
            return false;
        aValue = std::bit_cast<float>(bits);
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

    bool Raw(void* apOutput, std::size_t aSize)
    {
        if (Remaining() < aSize)
            return false;
        if (aSize != 0)
            std::memcpy(apOutput, m_input.data() + m_offset, aSize);
        m_offset += aSize;
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
        if (!Scalar(size) || size > kMaximumStringSize || Remaining() < size)
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

    template <class Tag> bool Id(DurableId<Tag>& aId)
    {
        return String(aId.Value);
    }

    bool Form(FormId& aForm)
    {
        return Scalar(aForm.ModId) && Scalar(aForm.BaseId);
    }

    [[nodiscard]] bool Done() const noexcept { return m_offset == m_input.size(); }

private:
    [[nodiscard]] std::size_t Remaining() const noexcept
    {
        return m_input.size() - m_offset;
    }

    const Bytes& m_input;
    std::size_t m_offset{};
};

bool WriteCharacterBuildBody(Writer& aWriter, const CharacterBuildState& acState)
{
    if (!aWriter.Id(acState.Slot) ||
        !aWriter.Id(acState.CharacterBinding) ||
        !aWriter.Scalar(acState.PersistenceCodecVersion) ||
        !aWriter.Scalar(acState.BuildVersion) ||
        !aWriter.Form(acState.RaceId) ||
        !aWriter.String(acState.ClassId) ||
        acState.Selections.size() > kMaximumSelections ||
        !aWriter.Scalar<std::uint32_t>(
            static_cast<std::uint32_t>(acState.Selections.size())))
    {
        return false;
    }

    for (const CharacterBuildSelection& selection : acState.Selections)
    {
        if (!aWriter.String(selection.GroupId) ||
            !aWriter.String(selection.OptionId))
        {
            return false;
        }
    }

    if (acState.CanonicalInventory.size() > kMaximumInventoryEntries ||
        !aWriter.Scalar<std::uint32_t>(
            static_cast<std::uint32_t>(acState.CanonicalInventory.size())))
    {
        return false;
    }
    for (const InventoryEntry& entry : acState.CanonicalInventory)
    {
        if (!aWriter.Form(entry.BaseId) ||
            !aWriter.Scalar(entry.Count) ||
            !aWriter.Float(entry.ExtraCharge) ||
            !aWriter.Form(entry.ExtraEnchantId) ||
            !aWriter.Scalar(entry.ExtraEnchantCharge) ||
            !aWriter.Bool(entry.EnchantmentIsWeapon) ||
            entry.EnchantmentEffects.size() > kMaximumEffectsPerEntry ||
            !aWriter.Scalar<std::uint32_t>(static_cast<std::uint32_t>(
                entry.EnchantmentEffects.size())))
        {
            return false;
        }
        for (const InventoryEffect& effect : entry.EnchantmentEffects)
        {
            if (!aWriter.Float(effect.Magnitude) ||
                !aWriter.Scalar(effect.Area) ||
                !aWriter.Scalar(effect.Duration) ||
                !aWriter.Float(effect.RawCost) ||
                !aWriter.Form(effect.EffectId))
            {
                return false;
            }
        }
        if (!aWriter.Float(entry.ExtraHealth) ||
            !aWriter.Form(entry.ExtraPoisonId) ||
            !aWriter.Scalar(entry.ExtraPoisonCount) ||
            !aWriter.Scalar(entry.ExtraSoulLevel) ||
            !aWriter.Form(entry.ExtraOwnerId) ||
            !aWriter.Bool(entry.ExtraEnchantRemoveUnequip) ||
            !aWriter.Bool(entry.ExtraWorn) ||
            !aWriter.Bool(entry.ExtraWornLeft) ||
            !aWriter.Bool(entry.IsQuestItem))
        {
            return false;
        }
    }

    if (!aWriter.Form(acState.LeftHandSpell) ||
        !aWriter.Form(acState.RightHandSpell) ||
        !aWriter.Form(acState.Shout) ||
        !aWriter.Scalar(acState.InventoryHash) ||
        acState.CanonicalSpells.size() > kMaximumSpells ||
        !aWriter.Scalar<std::uint32_t>(
            static_cast<std::uint32_t>(acState.CanonicalSpells.size())))
    {
        return false;
    }
    for (const FormId& spell : acState.CanonicalSpells)
    {
        if (!aWriter.Form(spell))
            return false;
    }
    return aWriter.Scalar(acState.SpellHash) &&
        aWriter.Bool(acState.Applied) &&
        aWriter.Scalar(acState.UpdatedRevision);
}

bool ReadCharacterBuildBody(Reader& aReader, CharacterBuildState& aState)
{
    std::uint32_t count{};
    if (!aReader.Id(aState.Slot) ||
        !aReader.Id(aState.CharacterBinding) ||
        !aReader.Scalar(aState.PersistenceCodecVersion) ||
        !aReader.Scalar(aState.BuildVersion) ||
        !aReader.Form(aState.RaceId) ||
        !aReader.String(aState.ClassId) ||
        !aReader.Scalar(count) || count > kMaximumSelections)
    {
        return false;
    }
    aState.Selections.resize(count);
    for (CharacterBuildSelection& selection : aState.Selections)
    {
        if (!aReader.String(selection.GroupId) ||
            !aReader.String(selection.OptionId))
        {
            return false;
        }
    }

    if (!aReader.Scalar(count) || count > kMaximumInventoryEntries)
        return false;
    aState.CanonicalInventory.resize(count);
    for (InventoryEntry& entry : aState.CanonicalInventory)
    {
        std::uint32_t effectCount{};
        if (!aReader.Form(entry.BaseId) ||
            !aReader.Scalar(entry.Count) ||
            !aReader.Float(entry.ExtraCharge) ||
            !aReader.Form(entry.ExtraEnchantId) ||
            !aReader.Scalar(entry.ExtraEnchantCharge) ||
            !aReader.Bool(entry.EnchantmentIsWeapon) ||
            !aReader.Scalar(effectCount) ||
            effectCount > kMaximumEffectsPerEntry)
        {
            return false;
        }
        entry.EnchantmentEffects.resize(effectCount);
        for (InventoryEffect& effect : entry.EnchantmentEffects)
        {
            if (!aReader.Float(effect.Magnitude) ||
                !aReader.Scalar(effect.Area) ||
                !aReader.Scalar(effect.Duration) ||
                !aReader.Float(effect.RawCost) ||
                !aReader.Form(effect.EffectId))
            {
                return false;
            }
        }
        if (!aReader.Float(entry.ExtraHealth) ||
            !aReader.Form(entry.ExtraPoisonId) ||
            !aReader.Scalar(entry.ExtraPoisonCount) ||
            !aReader.Scalar(entry.ExtraSoulLevel) ||
            !aReader.Form(entry.ExtraOwnerId) ||
            !aReader.Bool(entry.ExtraEnchantRemoveUnequip) ||
            !aReader.Bool(entry.ExtraWorn) ||
            !aReader.Bool(entry.ExtraWornLeft) ||
            !aReader.Bool(entry.IsQuestItem))
        {
            return false;
        }
    }

    if (!aReader.Form(aState.LeftHandSpell) ||
        !aReader.Form(aState.RightHandSpell) ||
        !aReader.Form(aState.Shout) ||
        !aReader.Scalar(aState.InventoryHash) ||
        !aReader.Scalar(count) || count > kMaximumSpells)
    {
        return false;
    }
    aState.CanonicalSpells.resize(count);
    for (FormId& spell : aState.CanonicalSpells)
    {
        if (!aReader.Form(spell))
            return false;
    }
    return aReader.Scalar(aState.SpellHash) &&
        aReader.Bool(aState.Applied) &&
        aReader.Scalar(aState.UpdatedRevision);
}

StoreResult CodecFailure(std::string aMessage)
{
    return {StoreError::IntegrityFailure, std::move(aMessage)};
}
}

StoreResult EncodeCharacterBuild(
    const CharacterBuildState& acState,
    Bytes& aPayload) noexcept
{
    try
    {
        Writer writer(aPayload);
        if (!writer.Raw(kCharacterBuildMagic, sizeof(kCharacterBuildMagic) - 1) ||
            !WriteCharacterBuildBody(writer, acState))
        {
            return CodecFailure("character-build payload exceeds persistence bounds");
        }
        return {};
    }
    catch (...)
    {
        return CodecFailure("character-build encoding failed");
    }
}

StoreValueResult<CharacterBuildState> DecodeCharacterBuild(
    const Bytes& acPayload) noexcept
{
    StoreValueResult<CharacterBuildState> result;
    try
    {
        Reader reader(acPayload);
        if (!reader.Magic(kCharacterBuildMagic, sizeof(kCharacterBuildMagic) - 1) ||
            !ReadCharacterBuildBody(reader, result.Value) || !reader.Done())
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "malformed persisted character-build payload";
        }
    }
    catch (...)
    {
        result.Error = StoreError::IntegrityFailure;
        result.Message = "character-build decoding failed";
    }
    return result;
}

StoreResult EncodeSnapshot(
    const CampaignProjection& acProjection,
    Bytes& aPayload) noexcept
{
    try
    {
        Writer writer(aPayload);
        const CampaignRecord& campaign = acProjection.Campaign;
        if (!writer.Raw(kSnapshotMagic, sizeof(kSnapshotMagic) - 1) ||
            !writer.Scalar(kCampaignSnapshotCodecVersion) ||
            !writer.Id(campaign.Id) ||
            !writer.Scalar(campaign.PersistenceSchemaVersion) ||
            !writer.Scalar(campaign.CurrentRevision) ||
            !writer.Bool(campaign.RosterSealed) ||
            !writer.Bool(campaign.LastCommittedCheckpoint.has_value()) ||
            (campaign.LastCommittedCheckpoint &&
             !writer.Id(*campaign.LastCommittedCheckpoint)) ||
            !writer.Scalar(campaign.CoreStateCodecVersion) ||
            !writer.Blob(campaign.CoreStatePayload) ||
            !writer.Scalar(campaign.CreatedAtUnixMs) ||
            !writer.Scalar(campaign.UpdatedAtUnixMs) ||
            acProjection.Slots.size() > kMaximumSlots ||
            !writer.Scalar<std::uint32_t>(
                static_cast<std::uint32_t>(acProjection.Slots.size())))
        {
            return CodecFailure("campaign snapshot exceeds persistence bounds");
        }
        for (const CampaignSlotRecord& slot : acProjection.Slots)
        {
            if (!writer.Id(slot.Slot) || !writer.Id(slot.Player) ||
                !writer.Id(slot.CharacterBinding))
            {
                return CodecFailure("campaign snapshot slot exceeds persistence bounds");
            }
        }

        if (acProjection.CharacterBuilds.size() > kMaximumCharacterBuilds ||
            !writer.Scalar<std::uint32_t>(static_cast<std::uint32_t>(
                acProjection.CharacterBuilds.size())))
        {
            return CodecFailure("too many character builds in campaign snapshot");
        }
        for (const CharacterBuildState& build : acProjection.CharacterBuilds)
        {
            Bytes encoded;
            const StoreResult encodedResult = EncodeCharacterBuild(build, encoded);
            if (!encodedResult || !writer.Blob(encoded))
                return CodecFailure("failed to encode character build in snapshot");
        }

        if (acProjection.AdapterStates.size() > kMaximumAdapters ||
            !writer.Scalar<std::uint32_t>(static_cast<std::uint32_t>(
                acProjection.AdapterStates.size())))
        {
            return CodecFailure("too many adapter states in campaign snapshot");
        }
        for (const AdapterState& state : acProjection.AdapterStates)
        {
            if (!writer.String(state.AdapterId) ||
                !writer.Scalar(state.AdapterVersion) ||
                !writer.Scalar(state.CodecVersion) ||
                !writer.Scalar(state.Audience) ||
                !writer.Bool(state.AudiencePlayer.has_value()) ||
                (state.AudiencePlayer && !writer.Id(*state.AudiencePlayer)) ||
                !writer.Blob(state.Payload) ||
                !writer.Scalar(state.UpdatedRevision))
            {
                return CodecFailure("adapter state exceeds snapshot bounds");
            }
        }
        return {};
    }
    catch (...)
    {
        return CodecFailure("campaign snapshot encoding failed");
    }
}

StoreValueResult<CampaignProjection> DecodeSnapshot(
    const Bytes& acPayload) noexcept
{
    StoreValueResult<CampaignProjection> result;
    try
    {
        Reader reader(acPayload);
        CampaignRecord& campaign = result.Value.Campaign;
        std::uint32_t codecVersion{};
        bool hasCheckpoint{};
        std::uint32_t count{};
        if (!reader.Magic(kSnapshotMagic, sizeof(kSnapshotMagic) - 1) ||
            !reader.Scalar(codecVersion) ||
            codecVersion != kCampaignSnapshotCodecVersion ||
            !reader.Id(campaign.Id) ||
            !reader.Scalar(campaign.PersistenceSchemaVersion) ||
            !reader.Scalar(campaign.CurrentRevision) ||
            !reader.Bool(campaign.RosterSealed) ||
            !reader.Bool(hasCheckpoint))
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "malformed or unsupported campaign snapshot header";
            return result;
        }
        if (hasCheckpoint)
        {
            CheckpointId checkpoint;
            if (!reader.Id(checkpoint))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "malformed campaign snapshot checkpoint identity";
                return result;
            }
            campaign.LastCommittedCheckpoint = std::move(checkpoint);
        }
        if (!reader.Scalar(campaign.CoreStateCodecVersion) ||
            !reader.Blob(campaign.CoreStatePayload) ||
            !reader.Scalar(campaign.CreatedAtUnixMs) ||
            !reader.Scalar(campaign.UpdatedAtUnixMs) ||
            !reader.Scalar(count) || count > kMaximumSlots)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "malformed campaign snapshot state";
            return result;
        }
        result.Value.Slots.resize(count);
        for (CampaignSlotRecord& slot : result.Value.Slots)
        {
            if (!reader.Id(slot.Slot) || !reader.Id(slot.Player) ||
                !reader.Id(slot.CharacterBinding))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "malformed campaign snapshot roster";
                return result;
            }
        }

        if (!reader.Scalar(count) || count > kMaximumCharacterBuilds)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "malformed campaign snapshot character-build count";
            return result;
        }
        result.Value.CharacterBuilds.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i)
        {
            Bytes encoded;
            if (!reader.Blob(encoded))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "malformed character-build payload in snapshot";
                return result;
            }
            auto decoded = DecodeCharacterBuild(encoded);
            if (!decoded)
            {
                result.Error = decoded.Error;
                result.Message = decoded.Message;
                return result;
            }
            result.Value.CharacterBuilds.push_back(std::move(decoded.Value));
        }

        if (!reader.Scalar(count) || count > kMaximumAdapters)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "malformed campaign snapshot adapter count";
            return result;
        }
        result.Value.AdapterStates.resize(count);
        for (AdapterState& state : result.Value.AdapterStates)
        {
            bool hasAudiencePlayer{};
            if (!reader.String(state.AdapterId) ||
                !reader.Scalar(state.AdapterVersion) ||
                !reader.Scalar(state.CodecVersion) ||
                !reader.Scalar(state.Audience) ||
                !reader.Bool(hasAudiencePlayer))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "malformed adapter state in snapshot";
                return result;
            }
            if (state.Audience != StateAudience::Public &&
                state.Audience != StateAudience::Private)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "invalid adapter audience in snapshot";
                return result;
            }
            if (hasAudiencePlayer)
            {
                PlayerId player;
                if (!reader.Id(player))
                {
                    result.Error = StoreError::IntegrityFailure;
                    result.Message = "malformed private adapter audience";
                    return result;
                }
                state.AudiencePlayer = std::move(player);
            }
            if (!reader.Blob(state.Payload) ||
                !reader.Scalar(state.UpdatedRevision))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "malformed adapter payload in snapshot";
                return result;
            }
        }

        if (!reader.Done())
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign snapshot contains trailing data";
        }
    }
    catch (...)
    {
        result.Error = StoreError::IntegrityFailure;
        result.Message = "campaign snapshot decoding failed";
    }
    return result;
}

std::string Checksum(const Bytes& acPayload) noexcept
{
    try
    {
        std::uint64_t hash = 14695981039346656037ull;
        for (const std::uint8_t byte : acPayload)
        {
            hash ^= byte;
            hash *= 1099511628211ull;
        }
        std::ostringstream stream;
        stream << std::hex << std::setfill('0') << std::setw(16) << hash;
        return stream.str();
    }
    catch (...)
    {
        return {};
    }
}

std::string MutationDigest(
    std::string_view acKind,
    StateVersion aExpectedRevision,
    std::uint32_t aCodecVersion,
    const Bytes& acPayload,
    std::string_view acEntityId) noexcept
{
    Bytes digestInput;
    try
    {
        digestInput.reserve(
            acKind.size() + acEntityId.size() + acPayload.size() + 32);
        digestInput.insert(digestInput.end(), acKind.begin(), acKind.end());
        digestInput.push_back(0);
        for (std::size_t i = 0; i < sizeof(aExpectedRevision); ++i)
            digestInput.push_back(static_cast<std::uint8_t>(
                (aExpectedRevision >> (i * 8)) & 0xFF));
        for (std::size_t i = 0; i < sizeof(aCodecVersion); ++i)
            digestInput.push_back(static_cast<std::uint8_t>(
                (aCodecVersion >> (i * 8)) & 0xFF));
        digestInput.insert(
            digestInput.end(), acEntityId.begin(), acEntityId.end());
        digestInput.push_back(0);
        digestInput.insert(
            digestInput.end(), acPayload.begin(), acPayload.end());
    }
    catch (...)
    {
        return {};
    }
    return Checksum(digestInput);
}
}
