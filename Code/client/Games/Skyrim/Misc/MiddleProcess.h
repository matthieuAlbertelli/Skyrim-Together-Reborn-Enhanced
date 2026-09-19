#pragma once

struct ActiveEffect;
struct BGSLoadFormBuffer;

struct InventoryEntry;
struct TESPackage;
struct TESIdleForm;

struct MiddleProcess
{
    // void SaveActiveEffects()
    void LoadActiveEffects(BGSLoadFormBuffer* apLoadGameBuffer);

    // 0xB0 - pitch
    // Read-only seating diagnostics. CommonLibSSE-NG MiddleHighProcessData
    // runOncePackage (ActorPackage at 0x58); all existing offsets stay fixed.
    uint8_t pad0[0x60];
    TESPackage* runOncePackage;
    void* runOncePackageData;
    uint32_t runOnceTargetHandle;
    int32_t runOnceProcedureIndex;
    float runOnceStartTime;
    uint8_t pad7C[0xB8 - 0x7C];
    float direction; // B8
    uint8_t padBC[0xD4 - 0xBC];
    NiPoint3 furniturePathPoint;
    uint8_t padE0[0x1A0 - 0xE0];
    GameList<ActiveEffect>* ActiveEffects;
    uint8_t pad1A8[0x208 - 0x1A8];
    // CommonLibSSE-NG MiddleHighProcessData::occupiedFurniture (SE/AE).
    BSPointerHandle<TESObjectREFR> occupiedFurniture;
    uint8_t pad20C[0x218 - 0x20C];
    BSPointerHandle<TESObjectREFR> commandingActor;
    uint8_t pad21C[0x220 - 0x21C];
    InventoryEntry* leftEquippedObject;
    TESIdleForm* furnitureIdle;
    uint8_t pad230[0x260 - 0x230];
    InventoryEntry* rightEquippedObject;
    InventoryEntry* ammoEquippedObject; // could be more than just ammo
    // 0xB8 - direction
    //
    // 0x326 - bool lookat
};

static_assert(offsetof(MiddleProcess, direction) == 0xB8);
static_assert(offsetof(MiddleProcess, leftEquippedObject) == 0x220);
static_assert(offsetof(MiddleProcess, rightEquippedObject) == 0x260);

static_assert(offsetof(MiddleProcess, occupiedFurniture) == 0x208);
static_assert(offsetof(MiddleProcess, runOncePackage) == 0x60);
static_assert(offsetof(MiddleProcess, runOnceTargetHandle) == 0x70);
static_assert(offsetof(MiddleProcess, runOnceProcedureIndex) == 0x74);
static_assert(offsetof(MiddleProcess, runOnceStartTime) == 0x78);
static_assert(offsetof(MiddleProcess, furniturePathPoint) == 0xD4);
static_assert(offsetof(MiddleProcess, furnitureIdle) == 0x228);
