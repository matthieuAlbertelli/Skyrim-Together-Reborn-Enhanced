#pragma once

#include <CampaignTypes.h>

#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace STRE::Campaign::Sqlite
{
inline constexpr std::size_t kMaximumIdLength = 128;
inline constexpr std::size_t kMaximumKindLength = 128;
inline constexpr std::size_t kMaximumAdapterIdLength = 256;
inline constexpr std::size_t kMaximumSaveIdentityLength = 512;
inline constexpr std::size_t kMaximumAlgorithmLength = 128;
inline constexpr std::size_t kMaximumPayloadSize = 4 * 1024 * 1024;
inline constexpr StateVersion kMaximumRevision =
    static_cast<StateVersion>(std::numeric_limits<std::int64_t>::max());

bool IsValidIdentifier(std::string_view acValue);

template <class Tag> bool IsValidId(const DurableId<Tag>& acId)
{
    return IsValidIdentifier(acId.Value);
}

bool IsValidPayload(const Bytes& acPayload);
std::string Hex64(std::uint64_t aValue);
bool ParseHex64(std::string_view acValue, std::uint64_t& aValue);

StoreResult ValidateOutbox(const std::vector<OutboxIntent>& acOutbox);
StoreResult ValidateSlots(const std::vector<CampaignSlotRecord>& acSlots);
StoreResult ValidateCharacterBuild(const CharacterBuildState& acBuild);
StoreResult ValidateAdapterState(const AdapterState& acState);
StoreResult ValidateCommonMutation(
    const CampaignId& acCampaign,
    StateVersion aExpectedRevision,
    const MutationId& acMutation,
    std::string_view acKind,
    std::uint32_t aCodecVersion,
    const Bytes& acPayload,
    const std::vector<OutboxIntent>& acOutbox);

void AppendDigestText(Bytes& aPayload, std::string_view acValue);

template <class T> void AppendDigestScalar(Bytes& aPayload, T aValue)
{
    using Unsigned = std::make_unsigned_t<T>;
    const Unsigned value = static_cast<Unsigned>(aValue);
    for (std::size_t index = 0; index < sizeof(Unsigned); ++index)
    {
        aPayload.push_back(static_cast<std::uint8_t>(
            (value >> (index * 8)) & 0xFF));
    }
}

void AppendDigestBlob(Bytes& aPayload, const Bytes& acValue);
void AppendDigestOutbox(
    Bytes& aPayload,
    const std::vector<OutboxIntent>& acOutbox);
}
