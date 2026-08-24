#include <Structs/NativeSaveBundle.h>

#include <cryptopp/sha.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <type_traits>

namespace STRE::Campaign
{
namespace
{
constexpr char kMetadataMagic[] = "STRENSB1";
constexpr std::size_t kRequiredMemberCount = 2;
constexpr std::string_view kNativeSaveIdentityPrefix = "stre-";
constexpr std::size_t kMaximumCheckpointIdentityLength = 128;

template <class T, bool = std::is_enum_v<T>> struct ScalarValue
{
    using Type = T;
};

template <class T> struct ScalarValue<T, true>
{
    using Type = std::underlying_type_t<T>;
};

bool IsValidLogicalIdentity(std::string_view acValue) noexcept
{
    if (acValue.size() <= kNativeSaveIdentityPrefix.size() ||
        acValue.size() > kNativeSaveIdentityPrefix.size() +
            kMaximumCheckpointIdentityLength ||
        !std::equal(
            kNativeSaveIdentityPrefix.begin(),
            kNativeSaveIdentityPrefix.end(),
            acValue.begin()))
    {
        return false;
    }
    return std::all_of(
        acValue.begin() +
            static_cast<std::ptrdiff_t>(kNativeSaveIdentityPrefix.size()),
        acValue.end(),
        [](char aValue)
        {
            return (aValue >= 'a' && aValue <= 'z') ||
                (aValue >= 'A' && aValue <= 'Z') ||
                (aValue >= '0' && aValue <= '9') ||
                aValue == '-' || aValue == '_';
        });
}

bool IsKnownRole(NativeSaveMemberRole aRole) noexcept
{
    return aRole == NativeSaveMemberRole::Ess ||
        aRole == NativeSaveMemberRole::Skse;
}

class Writer final
{
public:
    explicit Writer(std::vector<std::uint8_t>& aOutput)
        : m_output(aOutput)
    {
        m_output.clear();
    }

    template <class T> bool Scalar(T aValue)
    {
        static_assert(std::is_integral_v<T> || std::is_enum_v<T>);
        using Value = typename ScalarValue<T>::Type;
        using Unsigned = std::make_unsigned_t<Value>;
        const Unsigned value = static_cast<Unsigned>(aValue);
        if (m_output.size() + sizeof(Unsigned) >
            kMaximumNativeSaveMetadataSize)
        {
            return false;
        }
        for (std::size_t index = 0; index < sizeof(Unsigned); ++index)
        {
            m_output.push_back(static_cast<std::uint8_t>(
                (value >> (index * 8)) & 0xFF));
        }
        return true;
    }

    bool Raw(const void* apData, std::size_t aSize)
    {
        if (aSize > kMaximumNativeSaveMetadataSize ||
            m_output.size() > kMaximumNativeSaveMetadataSize - aSize)
        {
            return false;
        }
        const auto* pBytes = static_cast<const std::uint8_t*>(apData);
        m_output.insert(m_output.end(), pBytes, pBytes + aSize);
        return true;
    }

    bool String(std::string_view acValue)
    {
        return acValue.size() <= std::numeric_limits<std::uint16_t>::max() &&
            Scalar<std::uint16_t>(
                static_cast<std::uint16_t>(acValue.size())) &&
            Raw(acValue.data(), acValue.size());
    }

private:
    std::vector<std::uint8_t>& m_output;
};

class Reader final
{
public:
    explicit Reader(std::span<const std::uint8_t> acInput)
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
        std::uint16_t size{};
        if (!Scalar(size) || Remaining() < size)
            return false;
        aValue.assign(
            reinterpret_cast<const char*>(m_input.data() + m_offset), size);
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

    std::span<const std::uint8_t> m_input;
    std::size_t m_offset{};
};

NativeSaveBundleError ValidateAndSortMembers(
    std::vector<NativeSaveBundleMember>& aMembers) noexcept
{
    if (aMembers.size() != kRequiredMemberCount)
        return NativeSaveBundleError::MissingRequiredMember;

    std::sort(
        aMembers.begin(), aMembers.end(),
        [](const NativeSaveBundleMember& acLeft,
           const NativeSaveBundleMember& acRight)
        {
            return static_cast<std::uint8_t>(acLeft.Role) <
                static_cast<std::uint8_t>(acRight.Role);
        });

    if (aMembers[0].Role == aMembers[1].Role)
        return NativeSaveBundleError::DuplicateMember;
    if (aMembers[0].Role != NativeSaveMemberRole::Ess ||
        aMembers[1].Role != NativeSaveMemberRole::Skse)
    {
        return NativeSaveBundleError::MissingRequiredMember;
    }
    for (const NativeSaveBundleMember& member : aMembers)
    {
        if (!IsKnownRole(member.Role) || member.Size == 0)
            return NativeSaveBundleError::InvalidMember;
    }
    return NativeSaveBundleError::None;
}

NativeSaveBundleError EncodeMetadata(
    const NativeSaveBundle& acBundle,
    std::vector<std::uint8_t>& aMetadata) noexcept
{
    try
    {
        Writer writer(aMetadata);
        if (!writer.Raw(kMetadataMagic, sizeof(kMetadataMagic) - 1) ||
            !writer.Scalar(kNativeSaveMetadataCodecVersion) ||
            !writer.String(acBundle.LogicalIdentity) ||
            !writer.Scalar<std::uint8_t>(
                static_cast<std::uint8_t>(acBundle.Members.size())))
        {
            return NativeSaveBundleError::MetadataTooLarge;
        }
        for (const NativeSaveBundleMember& member : acBundle.Members)
        {
            if (!writer.Scalar(member.Role) ||
                !writer.Scalar(member.Size) ||
                !writer.Raw(member.Sha256.data(), member.Sha256.size()))
            {
                return NativeSaveBundleError::MetadataTooLarge;
            }
        }
        return NativeSaveBundleError::None;
    }
    catch (...)
    {
        return NativeSaveBundleError::MetadataTooLarge;
    }
}
}

std::vector<NativeSaveMemberExpectation> BuildExpectedNativeSaveMembers(
    std::string_view acLogicalIdentity) noexcept
{
    try
    {
        if (!IsValidLogicalIdentity(acLogicalIdentity))
            return {};
        const std::string identity(acLogicalIdentity);
        return {
            {NativeSaveMemberRole::Ess, identity + ".ess"},
            {NativeSaveMemberRole::Skse, identity + ".skse"}};
    }
    catch (...)
    {
        return {};
    }
}

bool ComputeNativeSaveSha256(
    std::span<const std::uint8_t> acBytes,
    NativeSaveSha256& aDigest) noexcept
{
    try
    {
        CryptoPP::SHA256 hash;
        hash.CalculateDigest(
            aDigest.data(), acBytes.data(), acBytes.size());
        return true;
    }
    catch (...)
    {
        aDigest.fill(0);
        return false;
    }
}

std::string NativeSaveSha256ToHex(const NativeSaveSha256& acDigest)
{
    static constexpr std::array<char, 16> cHex{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    std::string result;
    result.reserve(acDigest.size() * 2);
    for (const std::uint8_t byte : acDigest)
    {
        result.push_back(cHex[byte >> 4]);
        result.push_back(cHex[byte & 0x0F]);
    }
    return result;
}

NativeSaveBundleResult BuildNativeSaveBundleArtifact(
    std::string aLogicalIdentity,
    std::vector<NativeSaveBundleMember> aMembers) noexcept
{
    NativeSaveBundleResult result;
    try
    {
        if (!IsValidLogicalIdentity(aLogicalIdentity))
        {
            result.Error = NativeSaveBundleError::InvalidIdentity;
            return result;
        }
        result.Error = ValidateAndSortMembers(aMembers);
        if (result.Error != NativeSaveBundleError::None)
            return result;

        result.Value.Bundle.LogicalIdentity = std::move(aLogicalIdentity);
        result.Value.Bundle.Members = std::move(aMembers);
        result.Error = EncodeMetadata(
            result.Value.Bundle, result.Value.Metadata);
        if (result.Error != NativeSaveBundleError::None)
            return result;
        if (!ComputeNativeSaveSha256(
                result.Value.Metadata, result.Value.Fingerprint))
        {
            result.Error = NativeSaveBundleError::HashFailure;
        }
    }
    catch (...)
    {
        result.Error = NativeSaveBundleError::HashFailure;
    }
    return result;
}

NativeSaveBundleDecodeResult DecodeNativeSaveMetadata(
    std::span<const std::uint8_t> acMetadata) noexcept
{
    NativeSaveBundleDecodeResult result;
    try
    {
        if (acMetadata.size() > kMaximumNativeSaveMetadataSize)
        {
            result.Error = NativeSaveBundleError::MetadataTooLarge;
            return result;
        }

        Reader reader(acMetadata);
        std::uint16_t codecVersion{};
        std::uint8_t memberCount{};
        if (!reader.Magic(kMetadataMagic, sizeof(kMetadataMagic) - 1) ||
            !reader.Scalar(codecVersion))
        {
            result.Error = NativeSaveBundleError::MalformedMetadata;
            return result;
        }
        if (codecVersion != kNativeSaveMetadataCodecVersion)
        {
            result.Error = NativeSaveBundleError::UnsupportedMetadataVersion;
            return result;
        }
        if (!reader.String(result.Value.LogicalIdentity) ||
            !IsValidLogicalIdentity(result.Value.LogicalIdentity) ||
            !reader.Scalar(memberCount) ||
            memberCount != kRequiredMemberCount)
        {
            result.Error = NativeSaveBundleError::MalformedMetadata;
            return result;
        }

        result.Value.Members.resize(memberCount);
        for (NativeSaveBundleMember& member : result.Value.Members)
        {
            if (!reader.Scalar(member.Role) ||
                !reader.Scalar(member.Size) ||
                !reader.Raw(member.Sha256.data(), member.Sha256.size()))
            {
                result.Error = NativeSaveBundleError::MalformedMetadata;
                return result;
            }
        }
        if (!reader.Done())
        {
            result.Error = NativeSaveBundleError::MalformedMetadata;
            return result;
        }
        result.Error = ValidateAndSortMembers(result.Value.Members);
        if (result.Error == NativeSaveBundleError::MissingRequiredMember ||
            result.Error == NativeSaveBundleError::InvalidMember)
        {
            result.Error = NativeSaveBundleError::MalformedMetadata;
        }
    }
    catch (...)
    {
        result.Error = NativeSaveBundleError::MalformedMetadata;
    }
    return result;
}

NativeSaveBundleArtifactParseResult ParseNativeSaveBundleArtifact(
    std::string_view acExpectedLogicalIdentity,
    std::span<const std::uint8_t> acFingerprint,
    std::span<const std::uint8_t> acMetadata) noexcept
{
    NativeSaveBundleArtifactParseResult result;
    try
    {
        if (acFingerprint.size() != kNativeSaveSha256Size)
        {
            result.Error = NativeSaveBundleError::MalformedMetadata;
            return result;
        }
        NativeSaveBundleDecodeResult decoded =
            DecodeNativeSaveMetadata(acMetadata);
        if (!decoded)
        {
            result.Error = decoded.Error;
            return result;
        }
        if (decoded.Value.LogicalIdentity != acExpectedLogicalIdentity)
        {
            result.Error = NativeSaveBundleError::InvalidIdentity;
            return result;
        }

        NativeSaveSha256 computed{};
        if (!ComputeNativeSaveSha256(acMetadata, computed))
        {
            result.Error = NativeSaveBundleError::HashFailure;
            return result;
        }
        if (!std::equal(computed.begin(), computed.end(), acFingerprint.begin()))
        {
            result.Error = NativeSaveBundleError::MalformedMetadata;
            return result;
        }

        result.Value.Bundle = std::move(decoded.Value);
        result.Value.Metadata.assign(acMetadata.begin(), acMetadata.end());
        result.Value.Fingerprint = computed;
    }
    catch (...)
    {
        result.Error = NativeSaveBundleError::MalformedMetadata;
    }
    return result;
}
}
