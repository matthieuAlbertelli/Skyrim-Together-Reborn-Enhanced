#pragma once

struct TESForm;
struct MiddleProcess;
struct TESAmmo;

struct AIProcess
{
    bool SetCurrentAmmo(TESAmmo* apAmmo) noexcept;

    void KnockExplosion(Actor* apActor, const NiPoint3* aSourceLocation, float afMagnitude);

    void* unk0;
    MiddleProcess* middleProcess;
    void* unk8;
    void* packageLock;
    struct TESPackage* package;
    // Read-only seating diagnostics: CommonLibSSE-NG ActorPackage at 0x18.
    void* packageData;                 // 28
    uint32_t packageTargetHandle;      // 30
    int32_t packageProcedureIndex;     // 34
    float packageStartTime;            // 38
    uint32_t unk3C[2];
    uint32_t unk34[8];
    float unk54;
    uint32_t unk58[4];
    TESForm* equippedObject[2];

    uint8_t pad88[0x137 - 0x88];

    int8_t movementType;
};

struct HighProcessData
{
    uint8_t pad0[0x218];
    char* strVoiceSubtitle;
    GameArray<std::tuple<uint32_t, void*>> KnowledgeArray; // BSTuple, std::tuple is prolly wrong
};

static_assert(offsetof(AIProcess, movementType) == 0x137);
static_assert(offsetof(AIProcess, package) == 0x20);
static_assert(offsetof(AIProcess, packageData) == 0x28);
static_assert(offsetof(AIProcess, packageTargetHandle) == 0x30);
static_assert(offsetof(AIProcess, packageProcedureIndex) == 0x34);
static_assert(offsetof(AIProcess, packageStartTime) == 0x38);
