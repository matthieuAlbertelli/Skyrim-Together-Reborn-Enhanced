#pragma once

#include <Games/Primitives.h>

struct NiNode;
struct NiObject;
struct NiCamera;
struct TESCameraState;

struct TESCamera
{
    virtual ~TESCamera(){};

    virtual void SetNode(NiNode* node){};
    virtual void Update(){};

    NiCamera* GetNiCamera();

    float rotZ;
    float rotX;
    NiPoint3 pos;
    float zoom;
    NiNode* cameraNode;
    TESCameraState* state;
    bool unk;
};

static_assert(offsetof(TESCamera, rotZ) == 0x08);
static_assert(offsetof(TESCamera, rotX) == 0x0C);
static_assert(offsetof(TESCamera, pos) == 0x10);
static_assert(offsetof(TESCamera, zoom) == 0x1C);
static_assert(offsetof(TESCamera, cameraNode) == 0x20);
static_assert(offsetof(TESCamera, state) == 0x28);
static_assert(offsetof(TESCamera, unk) == 0x30);
static_assert(sizeof(TESCamera) == 0x38);
