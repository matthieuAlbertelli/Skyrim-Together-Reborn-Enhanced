#pragma once

#include <CampaignState.h>

namespace STRE::Campaign::RuntimeCodec
{
inline constexpr std::uint32_t kCampaignCoreCodecVersion = 1;
inline constexpr std::uint32_t kCampaignOutboxCodecVersion = 1;

StoreResult EncodeCoreState(
    const CampaignAggregate& acCampaign,
    Bytes& aPayload) noexcept;
StoreValueResult<CampaignAggregate> DecodeCoreState(
    const CampaignId& acCampaign,
    bool aRosterSealed,
    const std::vector<CampaignSlotRecord>& acRoster,
    StateVersion aPersistedVersion,
    const Bytes& acPayload) noexcept;

StoreResult EncodeSnapshotIntent(
    const CampaignAggregate& acCampaign,
    Bytes& aPayload) noexcept;
StoreValueResult<CampaignAggregate> DecodeSnapshotIntent(
    const Bytes& acPayload) noexcept;
}
