#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace STRE::Campaign
{
inline constexpr std::uint16_t kNativeSaveMetadataCodecVersion = 1;
inline constexpr std::uint32_t kNativeSaveFingerprintVersion = 1;
inline constexpr std::string_view kNativeSaveFingerprintAlgorithm = "SHA-256";
inline constexpr std::size_t kNativeSaveSha256Size = 32;
inline constexpr std::size_t kMaximumNativeSaveMetadataSize = 256;

using NativeSaveSha256 = std::array<std::uint8_t, kNativeSaveSha256Size>;

enum class NativeSaveMemberRole : std::uint8_t
{
    Ess = 1,
    Skse = 2
};

struct NativeSaveMemberExpectation
{
    NativeSaveMemberRole Role{};
    std::string FileName;

    bool operator==(const NativeSaveMemberExpectation&) const noexcept = default;
};

struct NativeSaveBundleMember
{
    NativeSaveMemberRole Role{};
    std::uint64_t Size{};
    NativeSaveSha256 Sha256{};

    bool operator==(const NativeSaveBundleMember&) const noexcept = default;
};

struct NativeSaveBundle
{
    std::string LogicalIdentity;
    std::vector<NativeSaveBundleMember> Members;

    bool operator==(const NativeSaveBundle&) const noexcept = default;
};

struct NativeSaveBundleArtifact
{
    NativeSaveBundle Bundle;
    std::vector<std::uint8_t> Metadata;
    NativeSaveSha256 Fingerprint{};

    bool operator==(const NativeSaveBundleArtifact&) const noexcept = default;
};

enum class NativeSaveBundleError
{
    None,
    InvalidIdentity,
    MissingRequiredMember,
    DuplicateMember,
    InvalidMember,
    MetadataTooLarge,
    MalformedMetadata,
    UnsupportedMetadataVersion,
    HashFailure
};

struct NativeSaveBundleResult
{
    NativeSaveBundleError Error{NativeSaveBundleError::None};
    NativeSaveBundleArtifact Value;

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == NativeSaveBundleError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

struct NativeSaveBundleDecodeResult
{
    NativeSaveBundleError Error{NativeSaveBundleError::None};
    NativeSaveBundle Value;

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == NativeSaveBundleError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

struct NativeSaveBundleArtifactParseResult
{
    NativeSaveBundleError Error{NativeSaveBundleError::None};
    NativeSaveBundleArtifact Value;

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == NativeSaveBundleError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

[[nodiscard]] std::vector<NativeSaveMemberExpectation>
BuildExpectedNativeSaveMembers(std::string_view acLogicalIdentity) noexcept;

[[nodiscard]] bool ComputeNativeSaveSha256(
    std::span<const std::uint8_t> acBytes,
    NativeSaveSha256& aDigest) noexcept;

[[nodiscard]] std::string NativeSaveSha256ToHex(
    const NativeSaveSha256& acDigest);

[[nodiscard]] NativeSaveBundleResult BuildNativeSaveBundleArtifact(
    std::string aLogicalIdentity,
    std::vector<NativeSaveBundleMember> aMembers) noexcept;

[[nodiscard]] NativeSaveBundleDecodeResult DecodeNativeSaveMetadata(
    std::span<const std::uint8_t> acMetadata) noexcept;

[[nodiscard]] NativeSaveBundleArtifactParseResult ParseNativeSaveBundleArtifact(
    std::string_view acExpectedLogicalIdentity,
    std::span<const std::uint8_t> acFingerprint,
    std::span<const std::uint8_t> acMetadata) noexcept;
}
