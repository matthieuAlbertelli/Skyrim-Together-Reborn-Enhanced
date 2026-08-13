#pragma once

#include <CampaignStore.h>

namespace STRE::Campaign::Codec
{
StoreResult EncodeCharacterBuild(
    const CharacterBuildState& acState,
    Bytes& aPayload) noexcept;
StoreValueResult<CharacterBuildState> DecodeCharacterBuild(
    const Bytes& acPayload) noexcept;

StoreResult EncodeSnapshot(
    const CampaignProjection& acProjection,
    Bytes& aPayload) noexcept;
StoreValueResult<CampaignProjection> DecodeSnapshot(
    const Bytes& acPayload) noexcept;

std::string Checksum(const Bytes& acPayload) noexcept;
std::string MutationDigest(
    std::string_view acKind,
    StateVersion aExpectedRevision,
    std::uint32_t aCodecVersion,
    const Bytes& acPayload,
    std::string_view acEntityId = {}) noexcept;
}
