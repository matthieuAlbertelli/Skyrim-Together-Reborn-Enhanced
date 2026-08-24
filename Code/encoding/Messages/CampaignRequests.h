#pragma once

#include "Message.h"

#include <Structs/Campaign.h>

#include <cstdint>

enum class CampaignCheckpointSaveResultCode : std::uint8_t
{
    Success = 0,
    Failure = 1
};

struct CampaignCreateRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignCreateRequest;
    CampaignCreateRequest()
        : ClientMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;
    TiltedPhoques::String MutationId;
    TiltedPhoques::String DisplayName;
};

struct CampaignJoinRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignJoinRequest;
    CampaignJoinRequest()
        : ClientMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;
    TiltedPhoques::String CampaignId;
    TiltedPhoques::String MutationId;
    std::uint64_t ExpectedRevision{};
};

struct CampaignResumeRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignResumeRequest;
    CampaignResumeRequest()
        : ClientMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;
    TiltedPhoques::String CampaignId;
    TiltedPhoques::String CharacterBindingId;
};

struct CampaignStartRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignStartRequest;
    CampaignStartRequest()
        : ClientMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;
    TiltedPhoques::String CampaignId;
    TiltedPhoques::String MutationId;
    std::uint64_t ExpectedRevision{};
};

struct CampaignSetReadyRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignSetReadyRequest;
    CampaignSetReadyRequest()
        : ClientMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;
    TiltedPhoques::String CampaignId;
    TiltedPhoques::String MutationId;
    std::uint64_t ExpectedRevision{};
    bool Ready{};
};

struct CampaignLeaveRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignLeaveRequest;
    CampaignLeaveRequest()
        : ClientMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;
    TiltedPhoques::String CampaignId;
    TiltedPhoques::String MutationId;
    std::uint64_t ExpectedRevision{};
};

struct CampaignJoinByCodeRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignJoinByCodeRequest;
    CampaignJoinByCodeRequest() : ClientMessage(Opcode) {}
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;
    TiltedPhoques::String JoinCode;
    TiltedPhoques::String MutationId;
    TiltedPhoques::String DisplayName;
};

struct CampaignHelgenInvestigationReadyRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignHelgenInvestigationReadyRequest;
    CampaignHelgenInvestigationReadyRequest()
        : ClientMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
};

struct CampaignCheckpointSaveResult final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCampaignCheckpointSaveResult;
    CampaignCheckpointSaveResult()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;
    [[nodiscard]] bool IsValid() const noexcept;

    TiltedPhoques::String CampaignId;
    TiltedPhoques::String CheckpointId;
    TiltedPhoques::String NativeSaveIdentity;
    CampaignCheckpointSaveResultCode Result{
        CampaignCheckpointSaveResultCode::Failure};
    TiltedPhoques::String FingerprintAlgorithm;
    std::uint32_t FingerprintVersion{};
    TiltedPhoques::Vector<std::uint8_t> Fingerprint;
    std::uint32_t SaveMetadataCodecVersion{};
    TiltedPhoques::Vector<std::uint8_t> SaveMetadata;
    bool WireValid{true};
};
