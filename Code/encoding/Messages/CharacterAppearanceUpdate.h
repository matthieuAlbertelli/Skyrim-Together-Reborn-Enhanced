#pragma once

#include "Message.h"
#include <Structs/Tints.h>
#include <Structs/GameId.h>

struct CharacterAppearanceDescriptor
{
    GameId Race{}; // Mod-system identity, never a sender-local load-order FormID.
    uint8_t Sex{};
    float Weight{};
    bool IsValid() const noexcept;
    bool operator==(const CharacterAppearanceDescriptor&) const = default;
};

// Complete replacement snapshot. The appearance bytes use TESNPC's native format;
// FaceTints retains the same wire representation as assignment/spawn.
struct CharacterAppearanceUpdate
{
    static constexpr size_t MaxAppearanceBytes = 1 << 15;
    static constexpr size_t MaxTintCount = 255;
    static constexpr size_t MaxTextureNameBytes = 1023;
    static constexpr size_t MaxWireBytes = 1 << 16; // TransportService send buffer, including packet/opcode bytes.

    uint32_t ActorId{};
    uint64_t FinalBuildRevision{}; // Zero is legacy/unqualified, never a LAB final.
    uint32_t ChangeFlags{};
    String AppearanceBuffer;
    Tints FaceTints;
    CharacterAppearanceDescriptor Descriptor;

    bool IsValid() const noexcept;
    bool operator==(const CharacterAppearanceUpdate& acRhs) const noexcept;
    void SerializeAppearance(TiltedPhoques::Buffer::Writer& aWriter) const noexcept;
    void DeserializeAppearance(TiltedPhoques::Buffer::Reader& aReader) noexcept;

private:
    bool m_decodeValid{true};
};

struct RequestCharacterAppearanceUpdate final : ClientMessage, CharacterAppearanceUpdate
{
    static constexpr ClientOpcode Opcode = kRequestCharacterAppearanceUpdate;
    RequestCharacterAppearanceUpdate()
        : ClientMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override { SerializeAppearance(aWriter); }
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override { DeserializeAppearance(aReader); }
};

struct NotifyCharacterAppearanceUpdate final : ServerMessage, CharacterAppearanceUpdate
{
    static constexpr ServerOpcode Opcode = kNotifyCharacterAppearanceUpdate;
    NotifyCharacterAppearanceUpdate()
        : ServerMessage(Opcode)
    {
    }
    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override { SerializeAppearance(aWriter); }
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override { DeserializeAppearance(aReader); }
};
