#include <NativeSaveBundle.h>

#include <catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace STRE::Campaign;

NativeSaveSha256 Hash(std::string_view acValue)
{
    NativeSaveSha256 digest{};
    const auto* pBytes = reinterpret_cast<const std::uint8_t*>(acValue.data());
    REQUIRE(ComputeNativeSaveSha256(
        std::span<const std::uint8_t>(pBytes, acValue.size()), digest));
    return digest;
}

std::vector<NativeSaveBundleMember> CompleteMembers()
{
    return {
        {NativeSaveMemberRole::Ess, 4096, Hash("ess-bytes")},
        {NativeSaveMemberRole::Skse, 128, Hash("skse-bytes")}};
}
}

TEST_CASE(
    "Native save bundle member names are strict and path independent",
    "[campaign.checkpoint][native-save][bundle]")
{
    const auto members = BuildExpectedNativeSaveMembers("stre-checkpoint_A-42");
    REQUIRE(members.size() == 2);
    REQUIRE(members[0].Role == NativeSaveMemberRole::Ess);
    REQUIRE(members[0].FileName == "stre-checkpoint_A-42.ess");
    REQUIRE(members[1].Role == NativeSaveMemberRole::Skse);
    REQUIRE(members[1].FileName == "stre-checkpoint_A-42.skse");
    REQUIRE(BuildExpectedNativeSaveMembers("../escape").empty());
}

TEST_CASE(
    "Native save SHA-256 fingerprints actual bytes",
    "[campaign.checkpoint][native-save][bundle]")
{
    const NativeSaveSha256 first = Hash("same bytes");
    const NativeSaveSha256 second = Hash("same bytes");
    const NativeSaveSha256 changed = Hash("changed bytes");
    REQUIRE(first == second);
    REQUIRE(first != changed);
    REQUIRE(
        NativeSaveSha256ToHex(Hash("abc")) ==
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_CASE(
    "Native save bundle metadata is canonical and round trips",
    "[campaign.checkpoint][native-save][bundle]")
{
    auto forward = CompleteMembers();
    auto reverse = forward;
    std::reverse(reverse.begin(), reverse.end());

    const NativeSaveBundleResult first = BuildNativeSaveBundleArtifact(
        "stre-checkpoint_A-42", forward);
    const NativeSaveBundleResult second = BuildNativeSaveBundleArtifact(
        "stre-checkpoint_A-42", reverse);
    REQUIRE(first);
    REQUIRE(second);
    REQUIRE(first.Value.Metadata == second.Value.Metadata);
    REQUIRE(first.Value.Fingerprint == second.Value.Fingerprint);
    REQUIRE(first.Value.Metadata.size() <= kMaximumNativeSaveMetadataSize);

    const NativeSaveBundleDecodeResult decoded = DecodeNativeSaveMetadata(
        first.Value.Metadata);
    REQUIRE(decoded);
    REQUIRE(decoded.Value == first.Value.Bundle);
    REQUIRE(decoded.Value.Members[0].Role == NativeSaveMemberRole::Ess);
    REQUIRE(decoded.Value.Members[1].Role == NativeSaveMemberRole::Skse);
}

TEST_CASE(
    "Native save bundle construction rejects incomplete or ambiguous input",
    "[campaign.checkpoint][native-save][bundle][security]")
{
    auto missing = CompleteMembers();
    missing.pop_back();
    REQUIRE(
        BuildNativeSaveBundleArtifact("stre-checkpoint_A-42", missing).Error ==
        NativeSaveBundleError::MissingRequiredMember);

    auto duplicate = CompleteMembers();
    duplicate[1].Role = NativeSaveMemberRole::Ess;
    REQUIRE(
        BuildNativeSaveBundleArtifact("stre-checkpoint_A-42", duplicate).Error ==
        NativeSaveBundleError::DuplicateMember);

    auto empty = CompleteMembers();
    empty[0].Size = 0;
    REQUIRE(
        BuildNativeSaveBundleArtifact("stre-checkpoint_A-42", empty).Error ==
        NativeSaveBundleError::InvalidMember);

    REQUIRE(
        BuildNativeSaveBundleArtifact("stre-../escape", CompleteMembers()).Error ==
        NativeSaveBundleError::InvalidIdentity);
}

TEST_CASE(
    "Native save metadata rejects malformed oversized and unsupported payloads",
    "[campaign.checkpoint][native-save][bundle][security]")
{
    const NativeSaveBundleResult valid = BuildNativeSaveBundleArtifact(
        "stre-checkpoint_A-42", CompleteMembers());
    REQUIRE(valid);

    std::vector<std::uint8_t> malformed = valid.Value.Metadata;
    malformed.pop_back();
    REQUIRE(
        DecodeNativeSaveMetadata(malformed).Error ==
        NativeSaveBundleError::MalformedMetadata);

    std::vector<std::uint8_t> unsupported = valid.Value.Metadata;
    unsupported[8] = 2;
    REQUIRE(
        DecodeNativeSaveMetadata(unsupported).Error ==
        NativeSaveBundleError::UnsupportedMetadataVersion);

    std::vector<std::uint8_t> oversized(
        kMaximumNativeSaveMetadataSize + 1, 0);
    REQUIRE(
        DecodeNativeSaveMetadata(oversized).Error ==
        NativeSaveBundleError::MetadataTooLarge);

    std::vector<std::uint8_t> trailing = valid.Value.Metadata;
    trailing.push_back(0);
    REQUIRE(
        DecodeNativeSaveMetadata(trailing).Error ==
        NativeSaveBundleError::MalformedMetadata);
}
