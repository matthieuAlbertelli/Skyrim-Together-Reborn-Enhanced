#include <sqlite/SqliteValidation.h>

#include <codec/CampaignCodec.h>
#include <sqlite/SqlitePrimitives.h>

#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace STRE::Campaign::Sqlite
{
bool IsValidIdentifier(std::string_view acValue)
{
    if (acValue.empty() || acValue.size() > kMaximumIdLength)
        return false;
    return std::all_of(
        acValue.begin(), acValue.end(), [](unsigned char aCharacter)
        {
            return aCharacter >= 0x21 && aCharacter <= 0x7E;
        });
}

bool IsValidPayload(const Bytes& acPayload)
{
    return acPayload.size() <= kMaximumPayloadSize;
}

std::string Hex64(std::uint64_t aValue)
{
    std::ostringstream stream;
    stream << std::hex;
    stream.width(16);
    stream.fill('0');
    stream << aValue;
    return stream.str();
}

bool ParseHex64(std::string_view acValue, std::uint64_t& aValue)
{
    if (acValue.size() != 16)
        return false;
    std::uint64_t value{};
    for (char character : acValue)
    {
        value <<= 4;
        if (character >= '0' && character <= '9')
            value |= static_cast<std::uint64_t>(character - '0');
        else if (character >= 'a' && character <= 'f')
            value |= static_cast<std::uint64_t>(character - 'a' + 10);
        else if (character >= 'A' && character <= 'F')
            value |= static_cast<std::uint64_t>(character - 'A' + 10);
        else
            return false;
    }
    aValue = value;
    return true;
}

StoreResult ValidateOutbox(const std::vector<OutboxIntent>& acOutbox)
{
    if (acOutbox.size() > 256)
        return Failure(StoreError::InvalidArgument, "too many outbox intents");
    for (const OutboxIntent& intent : acOutbox)
    {
        if (intent.CodecVersion == 0 || !IsValidPayload(intent.Payload))
        {
            return Failure(
                StoreError::InvalidArgument,
                "outbox intent has invalid codec version or payload size");
        }
    }
    return {};
}

StoreResult ValidateSlots(const std::vector<CampaignSlotRecord>& acSlots)
{
    if (acSlots.empty() || acSlots.size() > 128)
        return Failure(StoreError::InvalidArgument, "campaign roster size is invalid");
    std::unordered_set<std::string> slots;
    std::unordered_set<std::string> players;
    std::unordered_set<std::string> bindings;
    for (const CampaignSlotRecord& slot : acSlots)
    {
        if (!IsValidId(slot.Slot) || !IsValidId(slot.Player) ||
            !IsValidId(slot.CharacterBinding))
        {
            return Failure(StoreError::InvalidArgument, "campaign roster contains an invalid identity");
        }
        if (!slots.insert(slot.Slot.Value).second ||
            !players.insert(slot.Player.Value).second ||
            !bindings.insert(slot.CharacterBinding.Value).second)
        {
            return Failure(StoreError::InvalidArgument, "campaign roster identities must be unique");
        }
    }
    return {};
}

StoreResult ValidateCharacterBuild(const CharacterBuildState& acBuild)
{
    if (!IsValidId(acBuild.Slot) || !IsValidId(acBuild.CharacterBinding) ||
        acBuild.PersistenceCodecVersion == 0 || acBuild.BuildVersion == 0 ||
        acBuild.ClassId.empty() || acBuild.ClassId.size() > 256 ||
        acBuild.Selections.size() > 256 ||
        acBuild.CanonicalInventory.size() > 4096 ||
        acBuild.CanonicalSpells.size() > 1024)
    {
        return Failure(StoreError::InvalidArgument, "character-build state is outside persistence bounds");
    }
    Bytes encoded;
    StoreResult encodedResult = Codec::EncodeCharacterBuild(acBuild, encoded);
    if (!encodedResult)
        return encodedResult;
    return {};
}

StoreResult ValidateAdapterState(const AdapterState& acState)
{
    if (acState.AdapterId.empty() ||
        acState.AdapterId.size() > kMaximumAdapterIdLength ||
        acState.AdapterVersion > kMaximumRevision || acState.CodecVersion == 0 ||
        !IsValidPayload(acState.Payload))
    {
        return Failure(StoreError::InvalidArgument, "adapter state is outside persistence bounds");
    }
    if (acState.Audience == StateAudience::Public && acState.AudiencePlayer)
        return Failure(StoreError::InvalidArgument, "public adapter state cannot name a private audience");
    if (acState.Audience == StateAudience::Private &&
        (!acState.AudiencePlayer || !IsValidId(*acState.AudiencePlayer)))
    {
        return Failure(StoreError::InvalidArgument, "private adapter state requires a valid player audience");
    }
    return {};
}

StoreResult ValidateCommonMutation(
    const CampaignId& acCampaign,
    StateVersion aExpectedRevision,
    const MutationId& acMutation,
    std::string_view acKind,
    std::uint32_t aCodecVersion,
    const Bytes& acPayload,
    const std::vector<OutboxIntent>& acOutbox)
{
    if (!IsValidId(acCampaign) || !IsValidId(acMutation) ||
        aExpectedRevision > kMaximumRevision || acKind.empty() ||
        acKind.size() > kMaximumKindLength || aCodecVersion == 0 ||
        !IsValidPayload(acPayload))
    {
        return Failure(StoreError::InvalidArgument, "mutation metadata is outside persistence bounds");
    }
    return ValidateOutbox(acOutbox);
}

void AppendDigestText(Bytes& aPayload, std::string_view acValue)
{
    aPayload.insert(aPayload.end(), acValue.begin(), acValue.end());
    aPayload.push_back(0);
}

void AppendDigestBlob(Bytes& aPayload, const Bytes& acValue)
{
    AppendDigestScalar<std::uint64_t>(aPayload, acValue.size());
    aPayload.insert(aPayload.end(), acValue.begin(), acValue.end());
}

void AppendDigestOutbox(
    Bytes& aPayload,
    const std::vector<OutboxIntent>& acOutbox)
{
    AppendDigestScalar<std::uint64_t>(aPayload, acOutbox.size());
    for (const OutboxIntent& intent : acOutbox)
    {
        AppendDigestScalar(aPayload, intent.CodecVersion);
        AppendDigestBlob(aPayload, intent.Payload);
    }
}
}
